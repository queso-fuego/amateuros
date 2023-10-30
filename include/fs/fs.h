/*
 * fs/fs.h: file system definitions, maybe helper functions
 */
#pragma once

#include "C/stdint.h"

// File system sizes/bounds/limits
#define FS_BLOCK_SIZE 4096  // 1 disk block = 4KB, same as page size
#define FS_SECTOR_SIZE 512  // 1 disk sector/LBA = 512 bytes
                            
#define SUPERBLOCK_DISK_SECTOR 8    // Sectors 0-7 are for boot block

const uint8_t SECTORS_PER_BLOCK = FS_BLOCK_SIZE / FS_SECTOR_SIZE; 
const uint32_t BITS_PER_BLOCK  = 8 * FS_BLOCK_SIZE; // 1 byte = 8 bits

// File types
enum {
    FILETYPE_FILE = 0,
    FILETYPE_DIR  = 1
};

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;

    uint8_t padding;    // Unused
} __attribute__ ((packed)) fs_datetime_t;

typedef struct {
    uint8_t sector[FS_BLOCK_SIZE / FS_SECTOR_SIZE][FS_SECTOR_SIZE];
} __attribute__ ((packed)) boot_block_t;

typedef struct {
    uint32_t num_inodes;                    // Total # of inodes
    uint16_t first_inode_bitmap_block;
    uint16_t first_data_bitmap_block;
    uint16_t num_inode_bitmap_blocks;
    uint16_t num_data_bitmap_blocks;
    uint32_t first_inode_block;
    uint32_t first_data_block;
    uint16_t num_inode_blocks;
    uint16_t num_data_blocks;
    uint32_t max_file_size_bytes;           // 32 bit size max = 0xFFFFFFFF
    uint16_t block_size_bytes;              // Should be FS_BLOCK_SIZE
    uint8_t inode_size_bytes;
    uint32_t root_inode_pointer;            // RAM address or disk LBA?
    uint8_t inodes_per_block;
    uint8_t direct_extents_per_inode;       // Probably will be 4
    uint16_t extents_per_indirect_block;    // FS_BLOCK_SIZE / sizeof(extent_t)
    uint32_t first_free_inode_bit;          // First 0 bit in inode bitmap
    uint32_t first_free_data_bit;           // First 0 bit in inode bitmap
    uint16_t device_number;
    uint8_t first_unreserved_inode;

    uint8_t padding[14];                    // Unused
} __attribute__ ((packed)) superblock_t;    // sizeof(superblock_t) = 64

typedef struct {
    uint32_t first_block;
    uint32_t length_blocks;
} __attribute__ ((packed)) extent_t;

typedef struct {
    uint32_t id;                            // Unique # per file
    uint8_t type;                           // File type
    uint32_t size_bytes;
    uint32_t size_sectors;
    fs_datetime_t last_modified_timestamp;
    extent_t extent[4];                     // Direct file extents
    uint32_t single_indirect_block;         // Disk block for holding more extents
    uint32_t double_indirect_block;         // Disk block for holding more extents
    uint8_t ref_count;

    uint8_t padding[2];                     // Unused
} __attribute__ ((packed)) inode_t;         // sizeof(inode_t) should = 64
                                            
const uint8_t INODES_PER_SECTOR = FS_SECTOR_SIZE / sizeof(inode_t);

typedef struct {
    uint32_t id;                            // This will match the inode id
    char name[60];
} __attribute__ ((packed)) dir_entry_t;     // sizeof(dir_entry_t) = 64 bytes
                                            
const uint8_t DIR_ENTRIES_PER_BLOCK = FS_BLOCK_SIZE / sizeof(dir_entry_t);

typedef struct {
    uint8_t *address;           // Base virtual address file is loaded to
    int32_t offset;             // Current file position; used with seek()
    inode_t *inode;             // The underlying inode for the file, element in the open inode table
    uint32_t ref_count;         // Reference count, used for dup() or similar syscalls
    uint32_t flags;             // Open flags e.g. O_CREAT, O_RDONLY, O_WRONLY, O_RDWR, ...
    uint32_t pages_allocated;   // # of pages currently allocated
                            
    uint8_t padding[8];         // Unused
} __attribute__ ((packed)) open_file_table_t;       // sizeof(open_file_table_t) should = 32 bytes
                                                    
// Convert bytes to blocks
uint32_t bytes_to_blocks(const uint32_t bytes) {
    if (bytes == 0) return 0;
    if (bytes <= FS_BLOCK_SIZE) return 1;

    return (bytes / FS_BLOCK_SIZE) + ((bytes % FS_BLOCK_SIZE) > 0 ? 1 : 0);
}

// Convert bytes to sectors
uint32_t bytes_to_sectors(const uint32_t bytes) {
    if (bytes == 0) return 0;
    if (bytes <= FS_SECTOR_SIZE) return 1;

    return (bytes / FS_SECTOR_SIZE) + ((bytes % FS_SECTOR_SIZE) > 0 ? 1 : 0);
}

