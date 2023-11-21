/*
 * build/make_disk.c: Build disk image for filesystem & OS
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>

#include "C/stdint.h"
#include "fs/fs.h"

#define FILESYSTEM_ROOT_PATH "../bin/fs_root"
#define BOOTLOADER_NAME      "3rdstage.bin"
#define ROOT_INODE_ID       1
#define BOOTLOADER_INODE_ID 2

const char IMAGE_NAME[] = "../bin/OS.bin";

FILE *IMAGE_PTR = 0;
uint32_t file_blocks = 0;
const uint8_t null_block[FS_BLOCK_SIZE] = {0};
uint8_t temp_sector[FS_SECTOR_SIZE] = {0};
superblock_t superblock = {0};
uint32_t disk_size = 512*2880;  // Default size is 1.44MB
uint32_t first_block = 0;
uint32_t next_inode_id = 3; // 0 = invalid inode, 1 = root directory, 2 = 3rd stage bootloader

uint32_t num_files = 0;
    
// ==============================================================
// Return number of bytes to pad out to next boundary alignment
// ==============================================================
uint32_t num_padding_bytes(const uint32_t bytes, const uint32_t boundary) {
    if (bytes % boundary == 0) return 0;

    return (bytes - (bytes % boundary) + boundary) - bytes;
}

// =============================================================================
// Write boot block, first 8 sectors for boot sector and 2nd stage boot loader
// =============================================================================
bool write_boot_block(void) {
    boot_block_t boot_block = {0}; 

    // Boot sector
    FILE *fp = fopen("../bin/fs_root/system/bin/bootSect.bin", "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open boot sector.\n");
        return false;
    }

    assert(fread(boot_block.sector[0], FS_SECTOR_SIZE, 1, fp) == 1);
    fclose(fp);

    // 2nd stage bootloader
    fp = fopen("../bin/fs_root/system/bin/2ndstage.bin", "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open 2nd stage bootloader.\n");
        return false;
    }

    for (uint8_t i = 1; ; i++) {
        if (i > 7) {
            fprintf(stderr, "2ndstage.bin is too large, maximum size for boot block is 7 sectors!\n");
            return false;
        }

        if (fread(boot_block.sector[i], FS_SECTOR_SIZE, 1, fp) < 1) break;
    }
    fclose(fp);

    // Write boot block to disk image
    assert(fwrite(&boot_block, sizeof boot_block, 1, IMAGE_PTR) == 1);

    return true;
}

// ===================
// Write superblock
// ===================
bool write_superblock(void) {
    superblock.num_inodes               = num_files+2;      // +2 = invalid inode 0 and root_dir
    superblock.first_inode_bitmap_block = 2;                // block 0 = boot block, block 1 = superblock
    superblock.num_inode_bitmap_blocks  = superblock.num_inodes / (FS_BLOCK_SIZE * 8) + ((superblock.num_inodes % (FS_BLOCK_SIZE * 8) > 0) ? 1: 0);
    superblock.first_data_bitmap_block  = superblock.first_inode_bitmap_block + superblock.num_inode_bitmap_blocks;
    superblock.num_inode_blocks         = bytes_to_blocks(superblock.num_inodes * sizeof(inode_t));   

    const uint32_t data_blocks = bytes_to_blocks(disk_size);

    // Total disk blocks - (boot block + superblock + inode bitmap blocks + data bit map blocks + inode blocks)
    // TODO: This is probably 1+ too many due to the blocks for the data bitmap itself, need to fix somehow?
    uint32_t num_data_bits = (data_blocks - 1 - 1 - superblock.num_inode_bitmap_blocks - superblock.num_inode_blocks); 

    superblock.num_data_bitmap_blocks     = num_data_bits / (FS_BLOCK_SIZE * 8) + ((num_data_bits % (FS_BLOCK_SIZE * 8) > 0) ? 1 : 0);
    superblock.first_inode_block          = superblock.first_data_bitmap_block + superblock.num_data_bitmap_blocks;
    superblock.first_data_block           = superblock.first_inode_block + superblock.num_inode_blocks;

    superblock.num_data_blocks            = file_blocks + superblock.first_data_block;    // Also include blocks before data blocks start

    superblock.max_file_size_bytes        = 0xFFFFFFFF;                 // 32 bit size max = 0xFFFFFFFF
    superblock.block_size_bytes           = FS_BLOCK_SIZE;              // Should be FS_BLOCK_SIZE
    superblock.inode_size_bytes           = sizeof(inode_t);
    superblock.root_inode_pointer         = 0;                          // Filled at runtime from kernel, or 3rdstage bootloader
    superblock.inodes_per_block           = FS_BLOCK_SIZE / sizeof(inode_t);
    superblock.direct_extents_per_inode   = 4;                          // Probably will be 4
    superblock.extents_per_indirect_block = FS_BLOCK_SIZE / sizeof(extent_t); 
    superblock.first_free_inode_bit       = superblock.num_inodes;      // First 0 bit in inode bitmap

    superblock.first_free_data_bit        = file_blocks + superblock.first_data_block;    // 1st 0 bit in data bitmap

    superblock.device_number              = 0x1;
    superblock.first_unreserved_inode     = 3;                          // inode 0 = invalid, inode 1 = root dir, inode 2 = bootloader/3rdstage

    assert(fwrite(&superblock, sizeof(superblock_t), 1, IMAGE_PTR) == 1);

    //   Pad out to end of block
    assert(fwrite(&null_block, num_padding_bytes(sizeof(superblock_t), FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);

    return true;
}

// ====================
// Write inode bitmap
// ====================
bool write_inode_bitmap_blocks() {
    uint8_t sector[FS_SECTOR_SIZE];

    // Set number of bits = number of inodes
    *(uint32_t *)sector |= (2 << (superblock.num_inodes - 1)) - 1;

    // Write to disk
    assert(fwrite(sector, FS_SECTOR_SIZE, 1, IMAGE_PTR) == 1);

    // Pad out to end of block
    assert(fwrite(null_block, num_padding_bytes(FS_SECTOR_SIZE, FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);

    return true;
}

// ===================
// Write data bitmap
// ===================
bool write_data_bitmap_blocks() {
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

    // Set last partial amount of < 32 bits
    bit_chunk = (2 << ((superblock.num_data_blocks % 32) - 1)) - 1;
    assert(fwrite(&bit_chunk, 4, 1, IMAGE_PTR) == 1);
    bytes_written += 4;

    // Pad out to end of block
    assert(fwrite(null_block, num_padding_bytes(bytes_written, FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);

    return true;
}

// ================================================================================
// Determine total number of files and total size of all folders & files to add,
//   to fill out num_files and file_blocks which are used to build the superblock,
//   reading everything under ../bin/fs_root
// ================================================================================
bool get_total_files_and_blocks(char *dir_path) {
    DIR *dirp = opendir(dir_path);
    if (!dirp) {
        fprintf(stderr, "Could not open dir for '%s'\n", dir_path);
        return false;
    }

    struct stat file_stat = {0};
    if (stat(dir_path, &file_stat) < 0) {
        fprintf(stderr, "Could not get stat for '%s'\n", dir_path);
        return false;
    }

    uint32_t dir_files = 0;
    for (struct dirent *dirent = readdir(dirp); dirent != NULL; dirent = readdir(dirp)) 
        dir_files++;

    num_files++; // Found directory file

    // Add blocks for directory
    file_blocks += bytes_to_blocks(dir_files * sizeof(dir_entry_t));

    printf("Found directory '%s'\n", dir_path);

    rewinddir(dirp);   
    for (struct dirent *dirent = readdir(dirp); dirent != NULL; dirent = readdir(dirp)) {
        if (!strncmp(dirent->d_name, ".", 2) || !strncmp(dirent->d_name, "..", 3)) 
            continue;

        char temp_name[256] = {0};
        snprintf(temp_name, sizeof temp_name, "%s/%s", dir_path, dirent->d_name);

        if (stat(temp_name, &file_stat) < 0) {
            fprintf(stderr, "Could not open get stat for '%s'\n", dir_path);
            return false;
        }

        if (dirent->d_type == DT_DIR) {
            // Get files and blocks under this nested directory, recursively
            if (!get_total_files_and_blocks(temp_name)) return false;

        } else if (dirent->d_type == DT_REG) {
            num_files++; // Found regular file

            // Add blocks for file
            file_blocks += bytes_to_blocks(file_stat.st_size);

            printf("Found file '%s'\n", temp_name);
        }
    }

    // Cleanup
    closedir(dirp);

    return true;
}

// ==========================================================
// Add a directory's files' inodes & data under a directory
// ==========================================================
bool add_file_data(uint32_t current_inode_id, uint32_t parent_inode_id, char *dir_path) {
    // Get total # of files under this directory
    DIR *dirp = opendir(dir_path);
    if (!dirp) {
        fprintf(stderr, "Could not open dir for '%s'\n", dir_path);
        return false;
    }

    uint32_t dir_files = 0;
    for (struct dirent *dirent = readdir(dirp); dirent != NULL; dirent = readdir(dirp)) 
        dir_files++;

    // Write inode for this directory
    fseek(IMAGE_PTR, 
          (superblock.first_inode_block * FS_BLOCK_SIZE) + (current_inode_id * sizeof(inode_t)),
          SEEK_SET);

    inode_t inode = {0};
    inode.id = current_inode_id;
    inode.type = FILETYPE_DIR;
    inode.size_bytes = dir_files * sizeof(dir_entry_t);
    inode.size_sectors = bytes_to_sectors(inode.size_bytes);
    inode.last_modified_timestamp = (fs_datetime_t){
        .second = 0,
        .minute = 37,
        .hour = 13,
        .day = 20,
        .month = 4,
        .year = 2023,
    };

    inode.extent[0].first_block = first_block;
    inode.extent[0].length_blocks = bytes_to_blocks(inode.size_bytes);

    // Next file's data will start here in the data blocks
    first_block += inode.extent[0].length_blocks;   

    // Write inode for this directory
    assert(fwrite(&inode, sizeof inode, 1, IMAGE_PTR) == 1);

    // Go to data blocks for this directory
    fseek(IMAGE_PTR, inode.extent[0].first_block * FS_BLOCK_SIZE, SEEK_SET);

    uint32_t dir_bytes_written = 0;

    // Write . and .. dir entries first 
    dir_entry_t dir_entry = {0};
    dir_entry.id = current_inode_id;
    strcpy(dir_entry.name, ".");
    assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

    dir_bytes_written += sizeof dir_entry;

    dir_entry.id = parent_inode_id;
    strcpy(dir_entry.name, "..");
    assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

    dir_bytes_written += sizeof dir_entry;

    // Read through directory again, writing dir entries for files found in data blocks for
    //   this directory, and write the files' inodes and data blocks
    uint32_t save_pos = 0;

    rewinddir(dirp);
    for (struct dirent *dirent = readdir(dirp); dirent != NULL; dirent = readdir(dirp)) {
        if (!strncmp(dirent->d_name, ".", 2) || !strncmp(dirent->d_name, "..", 3))
            continue;

        // Write new dir entry for this file
        if (!strncmp(dirent->d_name, BOOTLOADER_NAME, strlen(BOOTLOADER_NAME)))
            dir_entry.id = BOOTLOADER_INODE_ID;
        else
            dir_entry.id = next_inode_id++;

        strcpy(dir_entry.name, dirent->d_name);

        assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

        dir_bytes_written += sizeof dir_entry;

        // Save current position in data blocks for directory to write dir entries to
        save_pos = ftell(IMAGE_PTR);

        char temp_name[256] = {0};
        snprintf(temp_name, sizeof temp_name, "%s/%s", dir_path, dirent->d_name);

        struct stat file_stat = {0};
        if (stat(temp_name, &file_stat) < 0) {
            fprintf(stderr, "Could not get stat for '%s'\n", temp_name);
            return false;
        }

        if (dirent->d_type == DT_REG) {
            // Add inode for file in inode blocks
            inode.id = dir_entry.id;
            inode.type = FILETYPE_FILE;
            inode.size_bytes = file_stat.st_size;
            inode.size_sectors = bytes_to_sectors(inode.size_bytes);
            inode.last_modified_timestamp = (fs_datetime_t){
                .second = 0,
                .minute = 37,
                .hour = 13,
                .day = 20,
                .month = 4,
                .year = 2023,
            };
            inode.extent[0].first_block = first_block;
            inode.extent[0].length_blocks = bytes_to_blocks(inode.size_bytes);

            // Next file's data will start here in the data blocks
            first_block += inode.extent[0].length_blocks;   

            fseek(IMAGE_PTR, 
                  (superblock.first_inode_block * FS_BLOCK_SIZE) + (inode.id * sizeof(inode_t)),
                  SEEK_SET);

            assert(fwrite(&inode, sizeof inode, 1, IMAGE_PTR) == 1);

            // Add data for file in data blocks
            fseek(IMAGE_PTR, inode.extent[0].first_block * FS_BLOCK_SIZE, SEEK_SET);

            FILE *fp = fopen(temp_name, "rb");
            uint32_t bytes_written = 0;
            for (uint32_t i = 0; i < inode.size_sectors; i++) {
                uint32_t bytes_read = fread(temp_sector, 1, FS_SECTOR_SIZE, fp);
                fwrite(temp_sector, 1, bytes_read, IMAGE_PTR);
                bytes_written += bytes_read;

                if (bytes_read < FS_SECTOR_SIZE) break; // Reached EOF
            }

            // Pad out to end of this block for file data
            assert(fwrite(null_block, 1, num_padding_bytes(bytes_written, FS_BLOCK_SIZE), IMAGE_PTR) ==
                   num_padding_bytes(bytes_written, FS_BLOCK_SIZE));

            printf("Wrote file '%s', size: %u\n", temp_name, bytes_written);

        } else if (dirent->d_type == DT_DIR) {
            // Add data for directory, recursively
            if (!add_file_data(dir_entry.id, current_inode_id, temp_name)) return false;
        }

        // Restore position in data blocks for directory to write dir entries to
        fseek(IMAGE_PTR, save_pos, SEEK_SET);
    }

    printf("Wrote directory '%s', size: %u (%llu files)\n", 
           dir_path, dir_bytes_written, (uint64_t)(dir_bytes_written / sizeof(dir_entry_t)));

    // Pad out directory's file data (dir entries) to end of block
    assert(fwrite(null_block, 1, num_padding_bytes(dir_bytes_written, FS_BLOCK_SIZE), IMAGE_PTR) ==
           num_padding_bytes(dir_bytes_written, FS_BLOCK_SIZE));

    // File cleanup
    closedir(dirp);

    return true;
}

// ================================================================================
// Write inode blocks and data blocks including dir entries and file data, for
//   filesystem laid out under ../bin/fs_root
// ================================================================================
bool write_inode_and_data_blocks(void) {
     // Write initial invalid inode 0
     fseek(IMAGE_PTR, superblock.first_inode_block * FS_BLOCK_SIZE, SEEK_SET);

     inode_t inode = {0};
     assert(fwrite(&inode, sizeof(inode_t), 1, IMAGE_PTR) == 1);
    
     // Add inode blocks and data blocks for all directories and files under the initial root directory
     first_block = superblock.first_data_block;
     if (!add_file_data(ROOT_INODE_ID, ROOT_INODE_ID, FILESYSTEM_ROOT_PATH)) return false;

    // Pad out disk image to full disk size in blocks
    // 6 types of blocks written so far = boot block, superblock, inode_bitmap_blocks, data_bitmap_blocks, inode blocks, data_blocks
    const uint32_t blocks_written = 1 + 1 + superblock.num_inode_bitmap_blocks + 
        superblock.num_data_bitmap_blocks + superblock.num_inode_blocks + superblock.num_data_blocks;

    // Write at end of current file data to pad out size
    fseek(IMAGE_PTR, 0, SEEK_END);

    const uint32_t disk_blocks = bytes_to_blocks(disk_size);
    for (uint32_t i = 0; i < (disk_blocks - blocks_written) + superblock.first_data_block; i++)
        assert(fwrite(null_block, FS_BLOCK_SIZE, 1, IMAGE_PTR) == 1);

    return true;
}

// ==================
// M A I N
// ==================
// TODO: Pass in user input disk size? Or default value from makefile, don't hardcode 1.44MB
int main(int argc, char *argv[]) {
    // Get pointer to output image
    IMAGE_PTR = fopen(IMAGE_NAME, "wb");

    // Determine total number of files and total size of all folders & files to add,
    //   to fill out num_files and file_blocks which are used to build the superblock
    if (!get_total_files_and_blocks("../bin/fs_root")) EXIT_FAILURE;

    printf("\nCreating disk image: %s\n", &IMAGE_NAME[7]);
    printf("Block size: %d, Total disk_blocks: %d\n", FS_BLOCK_SIZE, bytes_to_blocks(disk_size));
    printf("Total # of files: %d, File blocks: %d\n", num_files, file_blocks);

    // Boot block
    if (!write_boot_block()) return EXIT_FAILURE;

    // Super block
    if (!write_superblock()) return EXIT_FAILURE;

    // inode bitmap blocks
    if (!write_inode_bitmap_blocks()) return EXIT_FAILURE;

    // data bitmap blocks
    if (!write_data_bitmap_blocks()) return EXIT_FAILURE;

    // Write inode and data blocks
    if (!write_inode_and_data_blocks()) return EXIT_FAILURE;

    // File cleanup
    fclose(IMAGE_PTR);
}
