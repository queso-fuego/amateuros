/*
 * fs/fs.h: Filesystem definitions & functions
 */
#pragma once

#include "C/stdint.h"
#include "C/stdbool.h"

#define FILETYPE_FILE 0
#define FILETYPE_DIR  1

#define FS_BLOCK_SIZE 4096    // 4KB disk 'blocks'
#define FS_SECTOR_SIZE 512    // 512 byte sectors/LBA

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} __attribute__ ((packed)) fs_datetime_t;

typedef struct {
    uint8_t sectors[8][FS_SECTOR_SIZE];
} boot_block_t;

typedef struct {
    uint16_t number_of_inodes;
    uint16_t number_of_inode_bitmap_blocks;
    uint16_t number_of_data_bitmap_blocks;
    uint16_t first_inode_bitmap_block;
    uint16_t first_data_bitmap_block;
    uint32_t first_inode_block;
    uint32_t first_data_block;
    uint16_t block_size_bytes;
    uint32_t max_file_size_bytes;
    uint8_t  inode_size_bytes;
    uint16_t number_of_data_blocks;
    uint32_t root_inode_pointer;
    uint8_t  inodes_per_block;
    uint8_t  direct_extents_per_inode;
    uint16_t number_of_extents_per_indirect_block;
    uint32_t first_free_bit_in_inode_bitmap;
    uint32_t first_free_bit_in_data_bitmap;
    uint16_t device_number;
    uint16_t first_non_reserved_inode;

    uint8_t padding[17];    // Pad out to 64 bytes; reserved for future use
} __attribute__ ((packed)) superblock_t;    // Size should be 64 bytes

typedef struct {
    uint32_t first_block;
    uint32_t length_blocks;
} extent_t;

typedef struct {
    uint32_t   id;
    uint8_t    type;
    uint32_t   size_bytes;
    uint32_t   size_sectors;
    fs_datetime_t last_changed_timestamp;
    extent_t   extent[4];
    uint32_t   single_indirect_pointer;
    uint32_t   double_indirect_pointer;
    uint32_t   unused;  // Triple indirect pointer?
} __attribute__ ((packed)) inode_t; // Size should be 64 bytes

typedef struct {
    uint32_t id;        // Same id as file inode id
    char  name[60];     // Max name length = 60; More space, simple implementation
} __attribute__ ((packed)) dir_entry_t; // Size should be 64 bytes; 512 byte sector = 8 entries

typedef struct {
    dir_entry_t *dir_entry;
    uint32_t disk_sector;
    uint8_t *sector_in_memory;
} dir_entry_sector_t;

typedef struct {
    uint32_t id;
    uint8_t status_flags;
    uint32_t ref_count;
    uint8_t *address;
    uint32_t offset;
} open_file_table_entry_t;

// Convert bytes to number of blocks
uint32_t bytes_to_blocks(const uint32_t size_bytes) {
    if (size_bytes == 0)
        return 0;
    else if (size_bytes <= FS_BLOCK_SIZE) 
        return 1;
    else
        return (size_bytes / FS_BLOCK_SIZE) + ((size_bytes % FS_BLOCK_SIZE > 0) ? 1 : 0);
}

// Convert bytes to number of sectors
uint32_t bytes_to_sectors(const uint32_t size_bytes) {
    if (size_bytes == 0) 
        return 0;
    else if (size_bytes <= FS_SECTOR_SIZE) 
        return 1;
    else
        return (size_bytes / FS_SECTOR_SIZE) + ((size_bytes % FS_SECTOR_SIZE > 0) ? 1 : 0);
}
