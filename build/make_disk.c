/*
 * build/make_disk.c: Build disk image for filesystem & OS
 */
#define _DEFAULT_SOURCE // DT_DIR & DT_REG definitions for dirent->d_type
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>

#include "C/stdint.h"
#include "fs/fs.h"

const char IMAGE_NAME[] = "../bin/OS.bin";

FILE *IMAGE_PTR = 0;
uint32_t file_blocks = 0, num_files = 0;
const uint8_t null_block[FS_BLOCK_SIZE] = {0};
uint8_t sector_buf[FS_SECTOR_SIZE] = {0};
superblock_t superblock = {0};
uint32_t disk_size = 512*2880;  // Default size is 1.44MB

// Default starting inode will be after root directory (id=1) & bootloader (id=2)
uint32_t next_inode_id = 3;     
uint32_t first_block = 0;   // First data block to start writing new file data at

const uint32_t ROOT_INODE_ID = 1;
const uint32_t BOOTLOADER_INODE_ID = 2;

// =============================================================
// Return number of bytes to align to next boundary byte value
// =============================================================
uint32_t num_padding_bytes(const uint32_t bytes, const uint32_t boundary) {
    if (bytes % boundary == 0) return 0;
    return (bytes - (bytes % boundary) + boundary) - bytes;
}

// ============================
// Write Boot block
// ============================
bool write_boot_block(void) {
    boot_block_t boot_block = {0}; 

    FILE *fp = fopen("../bin/fs_root/sys/bin/bootSect.bin", "rb");
    if (!fp) {
        fprintf(stderr, "Error: Boot Sector binary not found or could not be opened!\n");
        return false;
    }

    // Boot sector
    assert(fread(boot_block.sector[0], FS_SECTOR_SIZE, 1, fp) == 1);

    fclose(fp);

    // 2nd stage bootloader
    fp = fopen("../bin/fs_root/sys/bin/2ndstage.bin", "rb");
    if (!fp) {
        fprintf(stderr, "Error: 2nd Stage binary not found or could not be opened!\n");
        return false;
    }

    for (uint8_t i = 1; ; i++) {
        if (i > 7) {
            fprintf(stderr, "2nd stage binary is too large, maximum size is 7 sectors!\n");
            return false;
        }

        // Read next sector until end of file
        if (fread(boot_block.sector[i], 1, FS_SECTOR_SIZE, fp) < FS_SECTOR_SIZE)
            break;
    }

    fclose(fp);

    // Write boot block to disk image
    assert(fwrite(&boot_block, FS_BLOCK_SIZE, 1, IMAGE_PTR) == 1);
    return true;
}

// ============================
// Write Superblock
// ============================
bool write_superblock(void) {
    superblock.num_inodes               = num_files+2;      // +2 = invalid inode 0 and root_dir
    superblock.first_inode_bitmap_block = 2;                // block 0 = boot block, block 1 = superblock
    superblock.num_inode_bitmap_blocks  = superblock.num_inodes / (FS_BLOCK_SIZE * 8) + ((superblock.num_inodes % (FS_BLOCK_SIZE * 8) > 0) ? 1: 0);
    superblock.first_data_bitmap_block  = superblock.first_inode_bitmap_block + superblock.num_inode_bitmap_blocks;
    superblock.num_inode_blocks         = bytes_to_blocks(superblock.num_inodes * sizeof(inode_t));   

    const uint32_t data_blocks = bytes_to_blocks(disk_size);

    // Total disk blocks - (boot block + superblock + inode bitmap blocks + data bit map blocks + inode blocks)
    uint32_t num_data_bits = (data_blocks - superblock.first_data_bitmap_block - superblock.num_inode_blocks - 1); 

    superblock.num_data_bitmap_blocks     = num_data_bits / (FS_BLOCK_SIZE * 8) + ((num_data_bits % (FS_BLOCK_SIZE * 8) > 0) ? 1 : 0);
    superblock.first_inode_block          = superblock.first_data_bitmap_block + superblock.num_data_bitmap_blocks;
    superblock.first_data_block           = superblock.first_inode_block + superblock.num_inode_blocks;

    superblock.num_data_blocks            = superblock.first_data_block + file_blocks;    // Only need to add file blocks here

    superblock.max_file_size_bytes        = 0xFFFFFFFF;                 // 32 bit size max = 0xFFFFFFFF
    superblock.block_size_bytes           = FS_BLOCK_SIZE;              // Should be FS_BLOCK_SIZE
    superblock.inode_size_bytes           = sizeof(inode_t);
    superblock.root_inode_pointer         = 0;                          // Filled at runtime from kernel, or 3rdstage bootloader
    superblock.inodes_per_block           = FS_BLOCK_SIZE / sizeof(inode_t);
    superblock.direct_extents_per_inode   = 4;                          // Probably will be 4
    superblock.extents_per_indirect_block = FS_BLOCK_SIZE / sizeof(extent_t); 
    superblock.first_free_inode_bit       = superblock.num_inodes;      // First 0 bit in inode bitmap

    // 0-based index of first available disk block in data bitmap
    superblock.first_free_data_bit        = superblock.first_data_block + file_blocks;    // Only need to add file blocks here

    superblock.device_number              = 0x1;
    superblock.first_unreserved_inode     = 3;                          // inode 0 = invalid, inode 1 = root dir, inode 2 = bootloader/3rdstage

    assert(fwrite(&superblock, sizeof(superblock_t), 1, IMAGE_PTR) == 1);

    // Pad out to end of block
    assert(fwrite(&null_block, num_padding_bytes(sizeof(superblock_t), FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);
    return true;
}

// ============================================================
// Write Inode bitmap blocks
// ============================================================
bool write_inode_bitmap_blocks(void) {
    uint64_t bit_chunk = 0;
    uint32_t total_blocks = superblock.num_inodes / 32;
    uint32_t bytes_written = 0;

    // Set number of bits = number of inodes
    while (total_blocks > 0) {
        bit_chunk = 0xFFFFFFFF;  // Set 32 bits at a time
        total_blocks--;
        assert(fwrite(&bit_chunk, 4, 1, IMAGE_PTR) == 1);
        bytes_written += 4;
    }

    // Check special case for bits % 32 == 0, or multiple of 32;
    if ((superblock.num_inodes % 32) > 0) {
        // Set last partial amount of < 32 bits
        bit_chunk = (2 << ((superblock.num_inodes % 32) - 1)) - 1;
        assert(fwrite(&bit_chunk, 4, 1, IMAGE_PTR) == 1);
        bytes_written += 4;
    }

    // Pad out to end of block
    assert(fwrite(null_block, num_padding_bytes(bytes_written, FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);
    return true;
}

// ============================================================
// Write Data bitmap blocks
// ============================================================
bool write_data_bitmap_blocks(void) {
    uint64_t bit_chunk = 0;
    uint32_t total_blocks = superblock.num_data_blocks / 32;
    uint32_t bytes_written = 0;

    // Set number of bits = number of data blocks
    while (total_blocks > 0) {
        bit_chunk = 0xFFFFFFFF;  // Set 32 bits at a time
        total_blocks--;
        assert(fwrite(&bit_chunk, 4, 1, IMAGE_PTR) == 1);
        bytes_written += 4;
    }

    // Check special case for bits % 32 == 0, or multiple of 32;
    if ((superblock.num_data_blocks % 32) > 0) {
        // Set last partial amount of < 32 bits
        bit_chunk = (2 << ((superblock.num_data_blocks % 32) - 1)) - 1;
        assert(fwrite(&bit_chunk, 4, 1, IMAGE_PTR) == 1);
        bytes_written += 4;
    }

    // Pad out to end of block
    assert(fwrite(null_block, num_padding_bytes(bytes_written, FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);
    return true;
}

// ============================================================
// Get initial values for total # of files in filesystem, and 
//   total size of those files in blocks
// ============================================================
bool get_file_info(char *dir_path) {
    DIR *dirp = opendir(dir_path);
    if (!dirp) {
        fprintf(stderr, "Error: Could not open directory %s\n", dir_path);
        return EXIT_FAILURE;
    }

    uint32_t dir_entries = 2;   // Default/emptry dir is only "." and ".." entries

    for (struct dirent *dirent = readdir(dirp); dirent; dirent = readdir(dirp)) {
        if (!strncmp(dirent->d_name, ".", 2) ||
            !strncmp(dirent->d_name, "..", 3)) {
            continue;
        }

        // Set new qualified name for stat() call
        char temp_name[512] = {0};
        snprintf(temp_name, sizeof temp_name, "%s/%s", dir_path, dirent->d_name);

        if (dirent->d_type == DT_DIR) {
            printf("Found directory %s\n", temp_name);
        } else if (dirent->d_type == DT_REG) {
            printf("Found file %s\n", temp_name);
        }

        num_files++;    // Found new file/directory
        dir_entries++;  // Add to local count for this directory

        // Get size from stat struct in blocks, add to total
        struct stat file_stat;
        if (stat(temp_name, &file_stat) < 0) {
            fprintf(stderr, "Error: Could not get stat() for %s\n", temp_name);
            return false;
        }

        if (dirent->d_type == DT_REG) {
            // Regular file
            file_blocks += bytes_to_blocks(file_stat.st_size);

        } else if (dirent->d_type == DT_DIR) {
            // If found directory, read through its files as well
            if (!get_file_info(temp_name)) return false;
        }
    }

    closedir(dirp);

    // Get size for this directory in blocks
    file_blocks += bytes_to_blocks(dir_entries * sizeof(dir_entry_t));
    return true;
}

// ============================================================
// Add data (inode and data blocks) for a directory and all files in the directory
// ============================================================
bool write_file_data(char *dir_path, uint32_t curr_inode_id, uint32_t parent_inode_id) {
    DIR *dirp = opendir(dir_path);
    if (!dirp) {
        fprintf(stderr, "Error: Could not open directory %s\n", dir_path);
        return EXIT_FAILURE;
    }

    // Read through dir once to begin with, to get total # of dir_entries;
    //    need this value to determine total size in blocks for inode extent length_blocks
    uint32_t dir_entries = 0; 
    for (struct dirent *dirent = readdir(dirp); dirent; dirent = readdir(dirp)) 
        dir_entries++;

    uint32_t dir_size = dir_entries * sizeof(dir_entry_t);

    // Add dir inode
    inode_t dir_inode = {0};
    dir_inode.id = curr_inode_id;  
    dir_inode.type = FILETYPE_DIR;
    dir_inode.size_bytes = dir_size;
    dir_inode.size_sectors = bytes_to_sectors(dir_size);
    dir_inode.last_modified_timestamp = (fs_datetime_t){
        .second = 0,
        .minute = 37,
        .hour = 13,
        .day = 20,
        .month = 4,
        .year = 2023,
    };
    dir_inode.extent[0] = (extent_t){
        .first_block = first_block,
        .length_blocks = bytes_to_blocks(dir_size),
    };

    // Set next position to start writing a new file at
    first_block += dir_inode.extent[0].length_blocks;

    // Write inode to disk image
    fseek(IMAGE_PTR, 
          (superblock.first_inode_block + (dir_inode.id / INODES_PER_BLOCK)) * FS_BLOCK_SIZE, 
          SEEK_SET);

    fseek(IMAGE_PTR, (dir_inode.id % INODES_PER_BLOCK) * sizeof(inode_t), SEEK_CUR);

    assert(fwrite(&dir_inode, sizeof dir_inode, 1, IMAGE_PTR) == 1);

    // Add dir_entry's for "." and ".."
    fseek(IMAGE_PTR, dir_inode.extent[0].first_block * FS_BLOCK_SIZE, SEEK_SET);

    dir_entry_t dir_entry = {0};
    dir_entry.id = dir_inode.id;    // "." = current (this) directory
    strcpy(dir_entry.name, ".");
    assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

    dir_entry.id = parent_inode_id; // ".." = parent directory
    strcpy(dir_entry.name, "..");
    assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

    // Rewind to read through directory again, and add file data for all files in directory
    rewinddir(dirp);   
    uint32_t curr_dir_pos = 0;  // Save variable outside local directory loop
    for (struct dirent *dirent = readdir(dirp); dirent; dirent = readdir(dirp)) {
        if (!strncmp(dirent->d_name, ".", 2) ||
            !strncmp(dirent->d_name, "..", 3)) {
            continue;
        }

        // Set new qualified name for stat() call
        char temp_name[512] = {0};
        snprintf(temp_name, sizeof temp_name, "%s/%s", dir_path, dirent->d_name);

        // Add dir_entry for this file
        if (!strncmp(dirent->d_name, "3rdstage.bin", 13)) {
            // If file is 3rdstage.bin, use specific inode 2, so that boot sector
            //   continues to work and booting can be simpler.
            dir_entry.id = BOOTLOADER_INODE_ID;
        } else {
            dir_entry.id = next_inode_id++;
        }

        strcpy(dir_entry.name, dirent->d_name);
        assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

        // Save current position in dir for next dir_entry
        curr_dir_pos = ftell(IMAGE_PTR);

        // Get size from stat struct in blocks
        struct stat file_stat;
        if (stat(temp_name, &file_stat) < 0) {
            fprintf(stderr, "Error: Could not get stat() for %s\n", temp_name);
            return false;
        }

        if (dirent->d_type == DT_REG) {
            // Regular file
            // Add file inode 
            inode_t file_inode = {0};
            file_inode.id   = dir_entry.id; 
            file_inode.type = FILETYPE_FILE;
            file_inode.size_bytes = file_stat.st_size;
            file_inode.size_sectors = bytes_to_sectors(file_stat.st_size);
            file_inode.last_modified_timestamp = (fs_datetime_t){
                .second = 0,
                .minute = 37,
                .hour   = 13,
                .day    = 20,
                .month  = 4,
                .year   = 2025,
            };
            file_inode.extent[0] = (extent_t){
                .first_block = first_block,
                .length_blocks = bytes_to_blocks(file_stat.st_size),
            };

            // Set next position to start writing a new file at
            first_block += file_inode.extent[0].length_blocks;

            // Add file inode to inode blocks
            fseek(IMAGE_PTR, 
                  (superblock.first_inode_block + (file_inode.id / INODES_PER_BLOCK)) * FS_BLOCK_SIZE, 
                  SEEK_SET);

            fseek(IMAGE_PTR, (file_inode.id % INODES_PER_BLOCK) * sizeof(inode_t), SEEK_CUR);

            assert(fwrite(&file_inode, sizeof file_inode, 1, IMAGE_PTR) == 1);

            // Add file data 
            fseek(IMAGE_PTR, file_inode.extent[0].first_block * FS_BLOCK_SIZE, SEEK_SET);

            FILE *fp = fopen(temp_name, "rb");
            if (!fp) {
                fprintf(stderr, "Error: Could not open file %s\n", temp_name);
                return false;
            }

            uint32_t total_bytes_written = 0;
            for (uint32_t i = 0; i < file_inode.size_sectors; i++) {
                uint32_t bytes_read = fread(sector_buf, 1, FS_SECTOR_SIZE, fp);
                assert(fwrite(sector_buf, 1, bytes_read, IMAGE_PTR) == bytes_read);
                total_bytes_written += bytes_read;
            }
            fclose(fp);  

            // Pad out to end of data block for this file
            assert(fwrite(null_block, 1, num_padding_bytes(total_bytes_written, FS_BLOCK_SIZE), IMAGE_PTR) ==
                   num_padding_bytes(total_bytes_written, FS_BLOCK_SIZE));

            printf("Wrote file %s, %llu (%u blocks)\n", 
                   temp_name, (unsigned long long)file_stat.st_size, bytes_to_blocks(file_stat.st_size));

        } else if (dirent->d_type == DT_DIR) {
            // If found directory, read through its files as well
            if (!write_file_data(temp_name, dir_entry.id, dir_inode.id)) return false;
        }

        // Restore current position in dir for next dir_entry
        fseek(IMAGE_PTR, curr_dir_pos, SEEK_SET);
    }

    closedir(dirp);

    // Pad out to end of data block for this directory
    assert(fwrite(null_block, 1, num_padding_bytes(dir_size, FS_BLOCK_SIZE), IMAGE_PTR) ==
           num_padding_bytes(dir_size, FS_BLOCK_SIZE));

    printf("Wrote dir %s, %u (%u dir entries + 2, %u blocks)\n", 
           dir_path, dir_size, (dir_size / (uint32_t)sizeof(dir_entry_t))-2, bytes_to_blocks(dir_size));
    return true;
}

// ============================================
// Write inode and data blocks to disk image
// ============================================
bool write_inode_and_data_blocks(void) {
    // 1st available position on disk to start writing new file data
    first_block = superblock.first_data_block;  

    // Add all files/directories under root as new initial filesystem
    if (!write_file_data("../bin/fs_root", ROOT_INODE_ID, ROOT_INODE_ID)) return false;

    // Pad out disk image to final size
    uint32_t disk_blocks = bytes_to_blocks(disk_size);

    fseek(IMAGE_PTR, 0, SEEK_END);
    uint32_t diff = disk_blocks - bytes_to_blocks(ftell(IMAGE_PTR));

    for (uint32_t i = 0; i < diff; i++)
      assert(fwrite(null_block, FS_BLOCK_SIZE, 1, IMAGE_PTR) == 1);
    return true;
}

// ============================================
// M A I N
// ============================================
// TODO: Pass in user input disk size? Or default value from makefile, don't hardcode 1.44MB
int main(void) {
    // Get pointer to output image
    IMAGE_PTR = fopen(IMAGE_NAME, "wb");

    // Get total number of files (variable num_files) and 
    //     total size of those files (variable file_blocks), to build superblock and rest
    //     of disk image
    if (!get_file_info("../bin/fs_root")) return EXIT_FAILURE;

    printf("\nCreating disk image: %s\n", &IMAGE_NAME[7]);
    printf("Block size: %d, Total disk_blocks: %d\n", FS_BLOCK_SIZE, bytes_to_blocks(disk_size));
    printf("Total # of Files: %u, File blocks: %d\n", 
           num_files, file_blocks);

    // Boot block
    if (!write_boot_block()) return EXIT_FAILURE;

    // Super block
    if (!write_superblock()) return EXIT_FAILURE;

    // Inode bitmap blocks
    if (!write_inode_bitmap_blocks()) return EXIT_FAILURE;

    // Data bitmap blocks
    if (!write_data_bitmap_blocks()) return EXIT_FAILURE;

    // Inode and Data blocks for files
    if (!write_inode_and_data_blocks()) return EXIT_FAILURE;

    // Finished writing disk image
    fclose(IMAGE_PTR);
}
