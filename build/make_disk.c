/*
 * build/make_disk.c: Build disk image for filesystem & OS
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "C/stdint.h"
#include "fs/fs.h"

// TODO: Add inodes & root directory dir_entrys for bootSect and 2ndstage?
//   currently those are only in the boot block, but are not in the regular filesystem
//   anywhere

typedef struct {
    char name[60];
    uint32_t size;
    FILE *fp;
} file_name_size_pointer;

const char IMAGE_NAME[] = "../bin/OS.bin";

FILE *IMAGE_PTR = 0;
uint32_t file_blocks = 0;
const uint8_t null_block[FS_BLOCK_SIZE] = {0};
superblock_t superblock = {0};
uint32_t disk_size = 512*2880;  // Default size is 1.44MB

file_name_size_pointer files[] = {
    {"../bin/bootSect.bin", 0, NULL},
    {"../bin/2ndstage.bin", 0, NULL},
    {"../bin/3rdstage.bin", 0, NULL},
    {"../bin/testfont.bin", 0, NULL},
    {"../bin/termu16n.bin", 0, NULL},
    {"../bin/termu18n.bin", 0, NULL},
    {"../bin/calculator.bin", 0, NULL},
    {"../bin/editor.bin", 0, NULL},
    {"../bin/malloctst.bin", 0, NULL},
    {"../bin/kernel.bin", 0, NULL},
};
const uint32_t num_files = sizeof files / sizeof files[0];
    
uint32_t num_padding_bytes(const uint32_t bytes, const uint32_t boundary) {
    if (bytes % boundary == 0) return 0;

    return (bytes - (bytes % boundary) + boundary) - bytes;
}

bool write_boot_block() {
    boot_block_t boot_block = {0}; 
    //   boot sector
    assert(fread(boot_block.sector[0], FS_SECTOR_SIZE, 1, files[0].fp) == 1);

    //   2nd stage bootloader
    for (uint8_t i = 1; i < bytes_to_sectors(files[1].size); i++) {
        if (i > 7) {
            fprintf(stderr, "%s is too large, maximum size for boot block is 7 sectors!", files[1].name);
            return false;
        }
        assert(fread(boot_block.sector[i], FS_SECTOR_SIZE, 1, files[1].fp) == 1);
    }

    //   write block to disk image
    for (uint8_t i = 0; i < FS_BLOCK_SIZE / FS_SECTOR_SIZE; i++)
        assert(fwrite(boot_block.sector[i], FS_SECTOR_SIZE, 1, IMAGE_PTR) == 1);

    return true;
}

bool write_superblock() {
    superblock.num_inodes               = num_files;        // -2 for bootSect & 2ndstage, +2 = invalid inode 0 and root_dir
    superblock.first_inode_bitmap_block = 2;                // block 0 = boot block, block 1 = superblock
    superblock.num_inode_bitmap_blocks  = superblock.num_inodes / (FS_BLOCK_SIZE * 8) + ((superblock.num_inodes % (FS_BLOCK_SIZE * 8) > 0) ? 1: 0);
    superblock.first_data_bitmap_block  = superblock.first_inode_bitmap_block + superblock.num_inode_bitmap_blocks;
    superblock.num_inode_blocks         = bytes_to_blocks(superblock.num_inodes * sizeof(inode_t));   

    const uint32_t data_blocks = bytes_to_blocks(disk_size);

    // TODO: Also subtract blocks taken by data_bitmap for num_data_bits? or does not matter?
    uint32_t num_data_bits = (data_blocks - superblock.first_data_bitmap_block - superblock.num_inode_blocks); 

    superblock.num_data_bitmap_blocks     = num_data_bits / (FS_BLOCK_SIZE * 8) + ((num_data_bits % (FS_BLOCK_SIZE * 8) > 0) ? 1 : 0);
    superblock.first_inode_block          = superblock.first_data_bitmap_block + superblock.num_data_bitmap_blocks;
    superblock.first_data_block           = superblock.first_inode_block + superblock.num_inode_blocks;
    superblock.num_data_blocks            = (file_blocks-2) + 1;        // +1 block for root directory entries, -1 for bootsect & 2ndstage file blocks
    superblock.max_file_size_bytes        = 0xFFFFFFFF;                 // 32 bit size max = 0xFFFFFFFF
    superblock.block_size_bytes           = FS_BLOCK_SIZE;              // Should be FS_BLOCK_SIZE
    superblock.inode_size_bytes           = sizeof(inode_t);
    superblock.root_inode_pointer         = 0;                          // Filled at runtime from kernel, or 3rdstage bootloader
    superblock.inodes_per_block           = FS_BLOCK_SIZE / sizeof(inode_t);
    superblock.direct_extents_per_inode   = 4;                          // Probably will be 4
    superblock.extents_per_indirect_block = FS_BLOCK_SIZE / sizeof(extent_t); 
    superblock.first_free_inode_bit       = superblock.num_inodes;      // First 0 bit in inode bitmap
    superblock.first_free_data_bit        = (file_blocks-2) + 1;        // First 0 bit in data bitmap; // +1 block for root directory entries, -1 for bootsect & 2ndstage file blocks
    superblock.device_number              = 0x1;
    superblock.first_unreserved_inode     = 3;                          // inode 0 = invalid, inode 1 = root dir, inode 2 = bootloader/3rdstage

    assert(fwrite(&superblock, sizeof(superblock_t), 1, IMAGE_PTR) == 1);

    //   Pad out to end of block
    assert(fwrite(&null_block, num_padding_bytes(sizeof(superblock_t), FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);

    return true;
}

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

bool write_inode_blocks() {
    inode_t inode = {0};

    // Inode 0: Invalid inode
    assert(fwrite(&inode, sizeof inode, 1, IMAGE_PTR) == 1);

    // Inode 1: Root directory
    inode = (inode_t){
        .id = 1,
        .type = FILETYPE_DIR,
        .size_bytes = sizeof(dir_entry_t) * num_files,
        .last_modified_timestamp = (fs_datetime_t){
            .second = 0,
            .minute = 37,
            .hour = 13,
            .day = 20,
            .month = 4,
            .year = 2022,
        },
        .extent[0] = (extent_t){
            .first_block = superblock.first_data_block, // Root dir will be first file data on disk
        },
    };

    inode.size_sectors = bytes_to_sectors(inode.size_bytes);    
    inode.extent[0].length_blocks = bytes_to_blocks(inode.size_bytes);  

    assert(fwrite(&inode, sizeof inode, 1, IMAGE_PTR) == 1);

    // Inode 2+ rest of the files starting with 3rd stage bootloader
    uint32_t inode_id = 2;
    uint32_t first_block = superblock.first_data_block + inode.extent[0].length_blocks;

    for (uint32_t i = 2; i < num_files; i++) {
        memset(&inode, 0, sizeof inode);    // Clear out inode first
        inode.id   = inode_id++;
        inode.type = FILETYPE_FILE;
        inode.size_bytes = files[i].size;
        inode.size_sectors = bytes_to_sectors(inode.size_bytes);
        inode.last_modified_timestamp = (fs_datetime_t){
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

        assert(fwrite(&inode, sizeof inode, 1, IMAGE_PTR) == 1);

        first_block += inode.extent[0].length_blocks;   // Next file will start at this disk block
    }

    assert(fwrite(null_block, num_padding_bytes((num_files) * sizeof(inode_t), FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);

    return true;
}

bool write_data_blocks() {
    uint8_t sector[FS_SECTOR_SIZE] = {0};

    // Root directory dir_entrys will be first data at disk blocks
    dir_entry_t dir_entry = {
        .id = 1,
        .name = ".",  
    };

    assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);
    strcpy(dir_entry.name, "..");
    assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);

    for (uint32_t i = 2; i < num_files; i++) {
        dir_entry.id = i;
        strcpy(dir_entry.name, &files[i].name[7]);  // Copy file name, skipping over "../bin/" prefix
        assert(fwrite(&dir_entry, sizeof dir_entry, 1, IMAGE_PTR) == 1);
    }

    // Pad out to end of current block, files + 2 due to . + .. entries
    assert(fwrite(null_block, num_padding_bytes((num_files) * sizeof(dir_entry_t), FS_BLOCK_SIZE), 1, IMAGE_PTR) == 1);

    // Write actual file data
    uint32_t bytes_written = 0;

    for (uint32_t i = 2; i < num_files; i++) {
        uint32_t num_blocks = bytes_to_blocks(files[i].size);

        printf("File: %s, blocks: %u\n", files[i].name, num_blocks);

        bytes_written = 0;

        // Write file data in sectors
        for (uint32_t j = 0; j < num_blocks * (FS_BLOCK_SIZE / FS_SECTOR_SIZE); j++) {
            uint32_t num_bytes = fread(sector, 1, sizeof sector, files[i].fp);
            fwrite(sector, 1, num_bytes, IMAGE_PTR);
            bytes_written += num_bytes;

            if (num_bytes < sizeof sector) break;   // Reached EOF
        }

        memset(sector, 0, sizeof sector);   // Clear sector

        // Pad out to end of block
        fwrite(null_block, num_padding_bytes(bytes_written, FS_BLOCK_SIZE), 1, IMAGE_PTR);
    }

    // Pad out to full disk size in blocks
    // 6 types of blocks written so far = boot block, superblock, inode_bitmap_blocks, data_bitmap_blocks, inode blocks, data_blocks
    const uint32_t blocks_written = 1 + 1 + superblock.num_inode_bitmap_blocks + superblock.num_data_bitmap_blocks +
        superblock.num_inode_blocks + superblock.num_data_blocks;

    const uint32_t disk_blocks = bytes_to_blocks(disk_size);
    for (uint32_t i = 0; i < disk_blocks - blocks_written; i++)
        assert(fwrite(null_block, FS_BLOCK_SIZE, 1, IMAGE_PTR) == 1);

    return true;
}

// TODO: Pass in array/list of files, instead of hardcoding
// TODO: Pass in user input disk size? Or default value from makefile, don't hardcode 1.44MB
int main(int argc, char *argv[]) {
    // Get pointer to output image
    IMAGE_PTR = fopen(IMAGE_NAME, "wb");

    // Fill out files[] info
    for (uint32_t i = 0; i < num_files; i++) {
        files[i].fp = fopen(files[i].name, "rb");   // Get file pointer
        fseek(files[i].fp, 0, SEEK_END);            // Get file size
        files[i].size = ftell(files[i].fp);

        file_blocks += bytes_to_blocks(files[i].size);

        rewind(files[i].fp);
    }

    printf("\nCreating disk image: %s\n", &IMAGE_NAME[7]);
    printf("Block size: %d, Total disk_blocks: %d\n", FS_BLOCK_SIZE, bytes_to_blocks(disk_size));
    printf("File blocks (sans boot block): %d\n", file_blocks - 2);

    // Boot block
    if (!write_boot_block()) return EXIT_FAILURE;

    // Super block
    if (!write_superblock()) return EXIT_FAILURE;

    // inode bitmap blocks
    if (!write_inode_bitmap_blocks()) return EXIT_FAILURE;

    // data bitmap blocks
    if (!write_data_bitmap_blocks()) return EXIT_FAILURE;

    // inode blocks
    if (!write_inode_blocks()) return EXIT_FAILURE;

    // data blocks
    if (!write_data_blocks()) return EXIT_FAILURE;
    
    // File cleanup
    for (uint32_t i = 0; i < num_files; i++) 
        fclose(files[i].fp);

    fclose(IMAGE_PTR);
}
