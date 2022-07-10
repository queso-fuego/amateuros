/*
 * make_disk.c: Create disk image with homemade filesystem for OS
 *
 * -----------------------------
 * Minimum disk image size for filesystem, if all blocks used:
 * -----------------------------
 * 1 Boot block
 * 1 Superblock
 * 1 Inode bitmap block: 4096 block size * 8 bits per byte = 32768 inode bits
 * 1 Data bitmap block:  4096 block size * 8 bits per byte = 32768 data bits
 * 512 Inode blocks: 32768 inodes * 64 bytes per inode = 2097152 / 4096 block size = 512
 * 32768 Data blocks: 32768 data bits = 32768 data blocks
 * -----------------------------
 * 33284 overall blocks minimum
 * 33284 * 4096 block size = 136,331,264 bytes or ~130MB
 */

// TODO: Ensure this builds a working image file for any amount of blocks
//   e.g. if using -O2 and the files are a lot larger than -Os
// Issue may be in bootsect, 2ndstage, 3rdstage, or kernel and NOT in this
//  file, but should make sure.

// NOTE: Unlike the regular OS files, this program _does_ use C stdlib from host OS & C Compiler
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "C/stdint.h"
#include "C/stdbool.h"
#include "C/math.h"
#include "fs/fs.h"

// Number of bytes to pad to next boundary from current size location
int num_padding_bytes(const int current_size, const int boundary) {
    if (current_size % boundary == 0) return 0;

    return (current_size - (current_size % boundary) + boundary) - current_size;
}

int main(int argc, char *argv[]) {
    const char BOOT_SECTOR_NAME[] = "../bin/bootSect.bin";
    const char SECOND_STAGE_NAME[] = "../bin/2ndstage.bin";
    const char OS_BIN[] = "../bin/OS.bin";

    typedef struct {
        char name[32];
        long size;
    } file_name_size;

    file_name_size files[] = {
        {"../bin/3rdstage.bin", 0},
        {"../bin/calculator.bin", 0},
        {"../bin/editor.bin", 0},
        {"../bin/kernel.bin", 0},
        {"../bin/malloctst.bin", 0},
        {"../bin/termu16n.bin", 0},
        {"../bin/termu18n.bin", 0},
        {"../bin/testfont.bin", 0},
    };
    const int num_files = sizeof files / sizeof files[0];

    int file_blocks = 0;
    uint8_t sector[FS_SECTOR_SIZE] = {0};
    uint8_t null_block[FS_BLOCK_SIZE] = {0};
    long file_size = 0;
    FILE *fp = 0;
    FILE *disk_image = fopen(OS_BIN, "wb");
    inode_t inode = {0};
    uint32_t inode_id = 0;
    int num_sectors = 0;
    int padding_bytes = 0;
    long bytes_written = 0;
    int disk_size = 0;
    uint16_t num_data_bits = 0;
    int disk_blocks = 0;

    // Set disk size from optional input argument
    if (argc < 2) {
        disk_size = 512*2880;   // Default size is 1.44MB 
    } else {
        disk_size = 1024 * 1024 * atoi(argv[1]);    // Size in MB
    }

    printf("\nCreating disk image %s:\n", &OS_BIN[7]);

    // Get file sizes
    for (int i = 0; i < num_files; i++) {
        FILE *tmp = fopen(files[i].name, "rb");
        if (!tmp) {
            fprintf(stderr, "ERROR: Could not open '%s' for reading!\n", files[i].name);
            return EXIT_FAILURE;
        }

        fseek(tmp, 0, SEEK_END);
        files[i].size = ftell(tmp);
        file_blocks += bytes_to_blocks(files[i].size);
        fclose(tmp);
    }

    disk_blocks = disk_size / FS_BLOCK_SIZE;

    printf("Disk size: %0.2fMB, Block size: 4096\n"
           "Disk blocks: %d\nTotal file blocks: %d\n", 
           disk_size / (1024.0*1024.0), disk_blocks, file_blocks);

    // Write boot block ------------------------
    fp = fopen(BOOT_SECTOR_NAME, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: '%s' does not exist!\n", BOOT_SECTOR_NAME);
        return EXIT_FAILURE;
    }

    boot_block_t boot_block = {0};

    // Read boot sector into 1st sector of boot block
    assert(fread(boot_block.sectors[0], 1, FS_SECTOR_SIZE, fp) == FS_SECTOR_SIZE);

    fclose(fp);

    // Read 2nd stage into next sectors of boot block
    fp = fopen(SECOND_STAGE_NAME, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: '%s' does not exist!\n", SECOND_STAGE_NAME);
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    for (long i = 0; i < file_size / 512; i++) {
        if (i >= 7) {
            fprintf(stderr, "ERROR: '%s' is too large to fit in the boot block, limit is 7 sectors!\n", SECOND_STAGE_NAME);
            return EXIT_FAILURE;
        }
        assert(fread(boot_block.sectors[1 + i], 1, FS_SECTOR_SIZE, fp) == FS_SECTOR_SIZE);
    }

    fclose(fp);

    for (int i = 0; i < 8; i++)
        assert(fwrite(boot_block.sectors[i], 1, FS_SECTOR_SIZE , disk_image) == FS_SECTOR_SIZE);

    // Write file system info block ("Superblock")
    superblock_t superblock = {0};
    superblock.number_of_inodes              = num_files + 2;   // Files + invalid inode 0 + root directory
    superblock.first_inode_bitmap_block      = 2;               // 0-based; block 0 = boot block, block 1 = superblock
    superblock.number_of_inode_bitmap_blocks = num_files / (FS_BLOCK_SIZE * 8) + ((num_files % (FS_BLOCK_SIZE * 8) > 0) ? 1 : 0);
    superblock.first_data_bitmap_block       = superblock.first_inode_bitmap_block + superblock.number_of_inode_bitmap_blocks; 

    num_data_bits = (disk_blocks - superblock.first_data_bitmap_block);

    superblock.number_of_data_bitmap_blocks         = num_data_bits / (FS_BLOCK_SIZE * 8) + ((num_data_bits % (FS_BLOCK_SIZE * 8) > 0) ? 1 : 0);
    superblock.first_inode_block                    = superblock.first_data_bitmap_block + superblock.number_of_data_bitmap_blocks; 
    superblock.first_data_block                     = superblock.first_inode_block + bytes_to_blocks(superblock.number_of_inodes * sizeof(inode_t));
    superblock.block_size_bytes                     = FS_BLOCK_SIZE;
    superblock.max_file_size_bytes                  = 0xFFFFFFFF;
    superblock.inode_size_bytes                     = sizeof(inode_t);
    superblock.number_of_data_blocks                = file_blocks;
    superblock.root_inode_pointer                   = 0;                // Pointer to RAM address, not LBA; Kernel sets this at runtime
    superblock.inodes_per_block                     = FS_BLOCK_SIZE / sizeof(inode_t);
    superblock.direct_extents_per_inode             = 4;
    superblock.number_of_extents_per_indirect_block = FS_BLOCK_SIZE / sizeof(extent_t);
    superblock.first_free_bit_in_inode_bitmap       = superblock.number_of_inodes;  // 0-based
    superblock.first_free_bit_in_data_bitmap        = file_blocks + 1;  // All files + root dir data 
    superblock.device_number                        = 0x01;
    superblock.first_non_reserved_inode             = 3;                // 0 = invalid inode, 1 = root inode, 2 = bootloader inode

    assert(fwrite(&superblock, 1, sizeof superblock, disk_image) == sizeof superblock);

    // Pad out to end of superblock disk block
    fwrite(null_block, num_padding_bytes(sizeof superblock, FS_BLOCK_SIZE), 1, disk_image);

    // Write Inode bitmap blocks
    sector[0] = 0xFF;   // Bits 0-7 in use; bit 0 = invalid inode
    sector[1] = 0x07;   // Bits 8-10 in use
    assert(fwrite(sector, 1, FS_SECTOR_SIZE, disk_image) == FS_SECTOR_SIZE); 
    sector[0] = 0;
    sector[1] = 0;

    // Pad out to end of block
    fwrite(null_block, num_padding_bytes(FS_SECTOR_SIZE, FS_BLOCK_SIZE), 1, disk_image);

    // Write Data bitmap blocks
    *(unsigned long *)sector |= uipow(2, superblock.number_of_data_blocks) - 1;    // Set bits 0 to (num_data_blocks - 1)
    assert(fwrite(sector, 1, FS_SECTOR_SIZE, disk_image) == FS_SECTOR_SIZE); 
    *(unsigned long *)sector = 0;

    // Pad out to end of data bitmap blocks
    fwrite(null_block, num_padding_bytes(FS_SECTOR_SIZE, FS_BLOCK_SIZE), 1, disk_image);

    // Write Inode blocks
    // Inode 0: invalid inode
    memset(&inode, 0, sizeof inode);
    assert(fwrite(&inode, 1, sizeof inode, disk_image) == sizeof inode);

    // Inode 1: Root directory
    inode_id = 1;
    inode.id = inode_id;
    inode.type = FILETYPE_DIR; 
    inode.size_bytes = sizeof(dir_entry_t)*10;
    inode.size_sectors = bytes_to_sectors(inode.size_bytes);
    inode.last_changed_timestamp = (fs_datetime_t){
        .second = 0,
        .minute = 37,
        .hour = 13,
        .day = 20,
        .month = 4,
        .year = 2022,
    };
    inode.extent[0] = (extent_t){
        .first_block = superblock.first_data_block,     // Will be first data block on disk
        .length_blocks = bytes_to_blocks(inode.size_bytes),
    };

    assert(fwrite(&inode, 1, sizeof inode, disk_image) == sizeof inode);

    // Inode 2: Bootloader & then rest of files
    uint32_t first_block = superblock.first_data_block + 1;
    inode_id = 2;   // Bootloader is at inode 2, the rest come after

    for (int i = 0; i < num_files; i++) {
        inode.id = inode_id++;
        inode.type = FILETYPE_FILE;
        inode.size_bytes = files[i].size;
        inode.size_sectors = bytes_to_sectors(inode.size_bytes);
        inode.last_changed_timestamp = (fs_datetime_t){
            .second = 0,
            .minute = 37,
            .hour = 13,
            .day = 20,
            .month = 4,
            .year = 2022,
        };
        inode.extent[0] = (extent_t){
            .first_block = first_block,
            .length_blocks = bytes_to_blocks(inode.size_bytes),
        };

        assert(fwrite(&inode, 1, sizeof inode, disk_image) == sizeof inode);    // Write inode to image

        first_block += inode.extent[0].length_blocks; // Next starting disk block for next inode/file
    }

    // Pad out to end of block
    fwrite(null_block, num_padding_bytes((num_files + 2) * sizeof(inode_t), FS_BLOCK_SIZE), 1, disk_image);

    // Write Data blocks
    // Root directory data, directory entries
    // . and .. both point to root, from root
    dir_entry_t dir_entry = {
        .id = 1,
        .name = ".",
    };
    assert(fwrite(&dir_entry, 1, sizeof dir_entry, disk_image) == sizeof dir_entry);
    strcpy(dir_entry.name, "..");
    assert(fwrite(&dir_entry, 1, sizeof dir_entry, disk_image) == sizeof dir_entry);

    inode_id = 2; 
    for (int i = 0; i < num_files; i++) {
        dir_entry.id = inode_id;
        memset(dir_entry.name, 0, sizeof dir_entry.name);   // Reset name
        strcpy(dir_entry.name, &files[i].name[7]);          // Write file name, skipping "../bin/" text at start of name
        assert(fwrite(&dir_entry, 1, sizeof dir_entry, disk_image) == sizeof dir_entry);
        inode_id++;
    }

    // Pad out to end of block
    fwrite(null_block, num_padding_bytes((num_files + 2) * sizeof(dir_entry_t), FS_BLOCK_SIZE), 1, disk_image);

    // Non-boot-block file data
    for (int i = 0; i < num_files; i++) {
        FILE *tmp = fopen(files[i].name, "rb");
        int num_blocks = bytes_to_blocks(files[i].size);

        printf("File: %s, blocks: %d\n", files[i].name, num_blocks);

        bytes_written = 0;

        // Write file data in sectors
        for (int j = 0; j < num_blocks * (FS_BLOCK_SIZE / FS_SECTOR_SIZE); j++) {
            long num_bytes = fread(sector, 1, sizeof sector, tmp);
            fwrite(sector, 1, num_bytes, disk_image);
            bytes_written += num_bytes;

            if (num_bytes < sizeof sector) break;   // Reached EOF
        }

        memset(sector, 0, sizeof sector);

        // Pad out to end of block
        fwrite(null_block, 1, num_padding_bytes(bytes_written, FS_BLOCK_SIZE), disk_image);

        // File cleanup
        fclose(tmp);
    }

    // Pad out image file to requested size e.g. 1.44MB
    // 1.44MB = 512*2880 bytes
    // Write full blocks
    const int blocks_written = 6 + file_blocks;    // boot block, superblock, inode bitmap, data bitmap, inodes, root dir data
    for (int i = 0; i < disk_blocks - blocks_written; i++)
        fwrite(null_block, FS_BLOCK_SIZE, 1, disk_image);
}
