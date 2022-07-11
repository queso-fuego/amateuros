/*
 * fs/fs_impl.h: Filesystem types & functions "implementations"
 */
#pragma once

#include "C/stdbool.h"
#include "C/math.h"
#include "fs/fs.h"
#include "disk/file_ops.h"
#include "global/global_addresses.h"

// Function declarations
inode_t *get_inode_from_id(const uint32_t id);
inode_t *get_inode_from_path(const char *path, inode_t *curr_dir);
inode_t *get_parent_inode_from_path(const char *path, inode_t *curr_dir);
void clear_data_bits(const extent_t extent);
dir_entry_sector_t find_dir_entry(const uint32_t id, inode_t inode);

// Global variables
static superblock_t *superblock = (superblock_t *)BOOTLOADER_SUPERBLOCK_ADDRESS;
static uint8_t tmp_sector[FS_SECTOR_SIZE] = {0};
static uint8_t tmp_block[FS_BLOCK_SIZE] = {0};
static inode_t *current_dir_inode = (inode_t *)CURRENT_DIR_INODE; // Current working directory inode

// Save a file from memory to disk
bool fs_save_file(inode_t *inode, uint32_t address) {
    // Write all file's blocks to disk
    uint32_t total_blocks = bytes_to_blocks(inode->size_bytes);
    uint32_t address_offset = 0;

    for (int i = 0; i < 4 && total_blocks > 0; i++) {
        rw_sectors(inode->extent[i].length_blocks * 8,
                   inode->extent[i].first_block * 8,
                   address + address_offset,
                   WRITE_WITH_RETRY);

        address_offset += inode->extent[i].length_blocks * FS_BLOCK_SIZE;
        total_blocks -= inode->extent[i].length_blocks;
    }

    // Grab any changes for current directory after writing new file data
    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (current_dir_inode->id / 8),
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    *current_dir_inode = *((inode_t *)tmp_sector + (current_dir_inode->id % 8));
    
    return true;
}

// Load a file from disk to memory
bool fs_load_file(inode_t *inode, uint32_t address) {
    // Read all file's blocks to memory
    uint32_t total_blocks = bytes_to_blocks(inode->size_bytes);
    uint32_t address_offset = 0;

    for (int i = 0; i < 4 && total_blocks > 0; i++) {
        rw_sectors(inode->extent[i].length_blocks * 8,
                   inode->extent[i].first_block * 8,
                   address + address_offset,
                   READ_WITH_RETRY);

        address_offset += inode->extent[i].length_blocks * FS_BLOCK_SIZE;
        total_blocks -= inode->extent[i].length_blocks;
    }

    if (total_blocks > 0) {
        // TODO: Load from single indirect block && double indirect block
    }

    return true;    // Success
}

// Delete/remove a file 
int fs_delete_file(char *file_path, inode_t *dir_inode) {
    inode_t inode = *get_inode_from_path(file_path, dir_inode);
    inode_t parent_inode = {0};

    // Clear this file's data bits in data bitmap
    // NOTE: Could also zero out data blocks on disk, but clearing the bits means
    //   eventually another file can overwrite that space without needing extra
    //   disk I/O
    uint32_t total_blocks = bytes_to_blocks(inode.size_bytes);

    for (int i = 0; i < 4 && total_blocks > 0; i++) {
        clear_data_bits(inode.extent[i]);
        total_blocks -= inode.extent[i].length_blocks;
    }

    // TODO: Delete from single & double indirect blocks
    
    // Remove this file from its parent's directory
    char *parent_dir = strrchr(file_path, '/');    // Find last '/' in path
    if (!parent_dir) {
        // No slashes in file path, file is in current directory that was passed in 
        parent_inode = *dir_inode; 
    } else if (file_path[0] == '/') {
        // Parent is root
        parent_inode = *get_inode_from_id(1);
    } else {
        // Find parent directory 
        *parent_dir = '\0';
        parent_inode = *get_inode_from_path(file_path, dir_inode); 
        *parent_dir = '/';  // Restore file path
    }

    // Find dir_entry for this file in parent_dir's data blocks
    dir_entry_sector_t dir_entry_sector = find_dir_entry(inode.id, parent_inode);

    // TODO: Check single & double indirect blocks

    if (!dir_entry_sector.dir_entry) {
        // Could not find entry in parent directory
        return -1;
    }

    // Clear dir_entry for file
    memset(dir_entry_sector.dir_entry, 0, sizeof(dir_entry_t));

    // Write changed sector back to disk 
    rw_sectors(1, dir_entry_sector.disk_sector, (uint32_t)dir_entry_sector.sector_in_memory, WRITE_WITH_RETRY);

    // Update parent's inode size from removing the dir_entry
    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (parent_inode.id / 8),
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    inode_t *temp = (inode_t *)tmp_sector + (parent_inode.id % 8);
    temp->size_bytes -= sizeof(dir_entry_t);
    temp->size_sectors = bytes_to_sectors(temp->size_bytes); 

    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (parent_inode.id / 8),
               (uint32_t)tmp_sector,
               WRITE_WITH_RETRY);
    
    // Clear this file's inode bit in inode bitmap
    rw_sectors(1, 
               (superblock->first_inode_bitmap_block * 8) + ((inode.id / 8) / FS_SECTOR_SIZE),     // Bit -> byte -> sector
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    tmp_sector[inode.id / 8] &= ~(1 << (inode.id % 8));

    rw_sectors(1, 
               (superblock->first_inode_bitmap_block * 8) + ((inode.id / 8) / FS_SECTOR_SIZE),     // Bit -> byte -> sector
               (uint32_t)tmp_sector,
               WRITE_WITH_RETRY);

    // Clear inode data in inode block
    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (inode.id / 8),
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    temp = (inode_t *)tmp_sector + (inode.id % 8);
    memset(temp, 0, sizeof(inode_t));

    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (inode.id / 8),
               (uint32_t)tmp_sector,
               WRITE_WITH_RETRY);

    return 0;   // SUCCESS
}

// Clear bits in data bitmap given file blocks from an extent
void clear_data_bits(const extent_t extent) {
    const uint8_t size = 8; // Number of maximum bits/blocks to clear at once TODO: Use 32 instead?

    // TODO: Check for additional sectors beyond 1 sector, add loop for each sector

    const uint32_t first_bit = extent.first_block;
    uint32_t num_blocks = extent.length_blocks;
    rw_sectors(1, 
               (superblock->first_data_bitmap_block * 8) + ((first_bit / 8) / FS_SECTOR_SIZE),
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    // Clear bits from data_bit % 8 to data_bit % 8 + (length_blocks - 1)
    // First byte
    uint8_t *sector_ptr = &tmp_sector[first_bit / 8];

    uint8_t data_bits = 0;
    if (num_blocks >= size) {
        // Will clear <size> bits first
        data_bits = uipow(2, size - (first_bit % size)) - 1;    

    } else if ((num_blocks % size) > (size - (first_bit % size))) {
        // Will clear partial <size> bits first, then the remaining partial <size> bits last
        data_bits = uipow(2, ((num_blocks % size) + 1) - (first_bit % size)) - 1;

    } else {
        // Can clear all bits 
        data_bits = uipow(2, num_blocks) - 1;
    }

    uint8_t shift = first_bit % size;
    uint8_t mask = data_bits << shift;
    *sector_ptr &= ~mask;

    if (num_blocks >= shift) {
        // Second+ bytes
        num_blocks -= shift;
        mask = -1;  // All bits set, all 0xFFs
        while (num_blocks > size) {
            sector_ptr++;
            *sector_ptr &= ~mask; 
            num_blocks -= size;
        }

        // Last byte
        sector_ptr++;
        mask = uipow(2, first_bit % size) - 1;
        *sector_ptr &= ~mask;
    }

    rw_sectors(1, 
               (superblock->first_data_bitmap_block * 8) + ((first_bit / 8) / FS_SECTOR_SIZE),
               (uint32_t)tmp_sector,
               WRITE_WITH_RETRY);
}

// Read all files in data blocks from extent, if found dir, return it and mark
//   added, if not, continue and delete file.
void fs_delete_dir_files(inode_t *curr_dir, inode_t *next_dir, bool *added_dir) {
    uint32_t total_bytes = curr_dir->size_bytes;

    for (int i = 0; i < 4; i++) {
        for (uint32_t j = 0; j < curr_dir->extent[i].length_blocks; j++) {
            rw_sectors(8, (curr_dir->extent[i].first_block + j) * 8, CURRENT_DIR_ADDRESS, READ_WITH_RETRY);

            uint8_t num_entries = FS_BLOCK_SIZE / sizeof(dir_entry_t);

            for (dir_entry_t *dir_entry = (dir_entry_t *)CURRENT_DIR_ADDRESS;
                 num_entries > 0 && total_bytes > 0;
                 dir_entry++, num_entries--, total_bytes -= sizeof(dir_entry_t)) {

                if (dir_entry->id != 0) {
                    // Skip . and .. dir_entries, do not delete, no need to
                    if (!((!strcmp(dir_entry->name, ".")  && strlen(dir_entry->name) == 1) ||
                          (!strcmp(dir_entry->name, "..") && strlen(dir_entry->name) == 2))) {

                        // Get inode for file
                        rw_sectors(1, 
                                   (superblock->first_inode_block * 8) + (dir_entry->id / 8),
                                   (uint32_t)tmp_sector,
                                   READ_WITH_RETRY);

                        inode_t *inode = (inode_t *)tmp_sector + (dir_entry->id % 8);

                        if (inode->type == FILETYPE_DIR) {
                            // Add directory to delete files from, push onto stack
                            *next_dir = *inode;
                            *added_dir = true;
                            return;
                        } else {
                            // Regular file, delete it. This will use the dir on the stack 
                            //   as the 'current' directory and the file name as being in 
                            //   that current directory.
                            fs_delete_file(dir_entry->name, curr_dir);
                        }
                    }
                }
            }
        }
    }
}

// Rename a file
void fs_rename_file(char *path, char *new_name) {
    // Get inode of path file & parent directory of file
    inode_t *inode = get_inode_from_path(path, current_dir_inode);
    inode_t parent_inode = *get_parent_inode_from_path(path, current_dir_inode);

    // Get dir_entry of path file
    dir_entry_sector_t dir_entry_sector = find_dir_entry(inode->id, parent_inode);

    // Rename dir_entry to new name
    strcpy(dir_entry_sector.dir_entry->name, new_name);

    // Write changes to disk
    rw_sectors(1, 
               dir_entry_sector.disk_sector, 
               (uint32_t)dir_entry_sector.sector_in_memory,
               WRITE_WITH_RETRY);

    // Update current directory to get any changes
    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (current_dir_inode->id / 8),
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    *current_dir_inode = *((inode_t *)tmp_sector + (current_dir_inode->id % 8));
}

// Get inode from a given file's inode id
inode_t *get_inode_from_id(const uint32_t id) {
    if (id == 0) return 0;

    // Get sector with inode
    rw_sectors(1, (superblock->first_inode_block * 8) + (id / 8), (uint32_t)tmp_sector, READ_WITH_RETRY);

    // Return inode within sector
    return (inode_t *)tmp_sector + (id % 8);
}

// Get inode for last file at end of a given path, relative to the 
//   current working directory.
// OUTPUT: 
//   Pointer to inode 
//   0/Null if not found or doesn't exist
inode_t *get_inode_from_path(const char *path, inode_t *curr_dir) {
    dir_entry_t *dir_entry = 0;
    char *path_ptr = (char *)path;
    char name[60] = {0};
    int i = 0;

    // Default to starting in current directory
    inode_t *inode = curr_dir; 

    if (*path_ptr == '/') {
        // Absolute path from root
        path_ptr++;

        inode = get_inode_from_id(1); // Root inode is ID 1
    }

    while (*path_ptr) { 
        if (*path_ptr == '/') { 
            path_ptr++;   // Move past divider
            continue;
        }

        if (path_ptr[0] == '.' && (path_ptr[1] == '\0' || path_ptr[1] == '/')) {
            // Relative path from current directory
            path_ptr++; 
            if (*path_ptr) path_ptr++;   // Handle only . and not ./

            continue;
        } 

        if (!strncmp(path_ptr, "..", 2) && (path_ptr[2] == '\0' || path_ptr[2] == '/')) {
            // Relative path from parent directory
            path_ptr += 2;
            if (*path_ptr) path_ptr++;   // Handle only .. and not ../

            rw_sectors(1, (inode->extent[0].first_block * 8), (uint32_t)tmp_sector, READ_WITH_RETRY);
            dir_entry = (dir_entry_t *)tmp_sector + 1;  // ".." is always 2nd dir_entry

            // Get inode for file
            inode = get_inode_from_id(dir_entry->id);

            continue;
        } 

        // Get next file name at path_ptr
        for (i = 0; path_ptr[i] != '/' && path_ptr[i] != '\0'; i++) 
            name[i] = path_ptr[i];  

        name[i] = '\0';

        path_ptr += i;  // Move past name

        // Get inode for file from current dir
        bool found = false;

        // Check extents
        const uint32_t max_entries = FS_BLOCK_SIZE / sizeof(dir_entry_t);

        for (int i = 0; i < 4; i++) {
            for (uint32_t j = 0; !found && j < inode->extent[i].length_blocks; j++) {
                rw_sectors(8, (inode->extent[i].first_block + j) * 8, (uint32_t)tmp_block, READ_WITH_RETRY);

                uint32_t num_entries = 0;
                for (dir_entry = (dir_entry_t *)tmp_block; 
                     num_entries < max_entries; 
                     dir_entry++, num_entries++) {

                    if (!strcmp(dir_entry->name, name)) {
                        found = true;
                        break;
                    }
                }
            }

            if (found) break;
        }

        if (found) {
            // Get inode for file
            inode = get_inode_from_id(dir_entry->id);
            continue;
        }

        // Check single indirect block and double indirect block extents
        // TODO:
        
        inode = 0;  // Did not find inode, return null
        break;
    }
    
    return inode;
}

// Get inode for the parent directory of the last name for a given file path
inode_t *get_parent_inode_from_path(const char *path, inode_t *curr_dir) {
    char *parent_dir = strrchr(path, '/');    // Find last '/' in path
    inode_t *parent_inode = 0;

    if (!parent_dir) {
        // No slashes in file path, file is in current directory that was passed in 
        return curr_dir; 

    } else if (path[0] == '/' && path[1] == '\0') {
        // Path is root dir
        parent_inode = get_inode_from_id(1);    // Root is inode 1

    } else {
        // Find parent directory 
        *parent_dir = '\0';
        parent_inode = get_inode_from_path(path, curr_dir); 
        *parent_dir = '/';  // Restore file path

        if (!parent_inode) 
            return 0; // Parent does not exist
    }

    return parent_inode; 
}

// Set input path to resolved path
char *resolve_path(char *start_path, char *rel_to_path) {
    int i = 0;
    char *path_ptr = rel_to_path;
    char *result_path = start_path;
    
    // Start at end of starting path
    while (result_path[i]) i++;

    while (*path_ptr) {
        if (*path_ptr == '/') { 
            path_ptr++;   // Move past divider
            continue;
        }

        if (path_ptr[0] == '.' && (path_ptr[1] == '\0' || path_ptr[1] == '/')) {
            // Current directory
            path_ptr++; 
            if (*path_ptr) path_ptr++;   // Handle only . and not ./

            continue;
        } 

        if (!strncmp(path_ptr, "..", 2) && (path_ptr[2] == '\0' || path_ptr[2] == '/')) {
            // Parent directory
            path_ptr += 2;
            if (*path_ptr) path_ptr++;   // Handle only .. and not ../

            // End name one level up
            while (result_path[i] != '/') i--;

            if (i == 0) i++;    // Don't erase root
            result_path[i] = '\0';  

            continue;
        } 

        // Copy next dir name from path
        if (result_path[i-1] != '/') result_path[i++] = '/';  // Add divider

        while (*path_ptr != '/' && *path_ptr != '\0')
            result_path[i++] = *path_ptr++;

        result_path[i] = '\0'; // Stop in case this is last dir in path
    }

    return result_path;
}

// Return last file/dir name from a given path
char *get_last_name_in_path(char *path) {
    // Find last directory
    char *last_name = strrchr(path, '/');

    // If no slashes, use input name 
    if (!last_name) return path;
    
    // Else last name starts after final slash
    return last_name+1;
}

// Find next free bit in a bitmap
uint32_t get_next_free_bit(const uint16_t first_block, const uint16_t last_block) {
    const uint32_t chunks = FS_BLOCK_SIZE / 32;

    for (uint16_t next_block = first_block; next_block <= last_block; next_block++) {
        // Load next block to memory
        rw_sectors(8, next_block*8, (uint32_t)tmp_block, READ_WITH_RETRY);

        uint32_t *chunk = (uint32_t *)tmp_block;

        // Search for free bit in chunk
        for (uint32_t i = 0; i < chunks; i++) {
            if (chunk[i] != 0xFFFFFFFF) {
                // 1 bit is free, find it
                for (uint8_t j = 0; j < 32; j++) {
                    if (!(chunk[i] & (1 << j))) 
                        return i*32 + j;
                }
            }
        }
    }

    return 0;   // No free bits left! :^(
}

// Create a file at the given path
// OUTPUT:
//   Inode id
uint32_t fs_create_file(char *path, const uint8_t file_type) {
    // Get next parent inode to add directory to
    inode_t parent_inode = *get_parent_inode_from_path(path, current_dir_inode);

    // Update directory's inode extents for new data, if needed
    const uint32_t prev_block_size = bytes_to_blocks(parent_inode.size_bytes);
    parent_inode.size_bytes  += sizeof(dir_entry_t);
    parent_inode.size_sectors = bytes_to_sectors(parent_inode.size_bytes);
    const uint32_t curr_dir_blocks = bytes_to_blocks(parent_inode.size_bytes);

    if (curr_dir_blocks > prev_block_size) {
        // Get Next free data bit/block
        uint32_t data_bit = superblock->first_free_bit_in_data_bitmap; 

        // Mark data bit as in use in data bitmap
        const uint32_t data_bit_sector = (data_bit / 8) / FS_SECTOR_SIZE; // Bit -> byte -> sector
        rw_sectors(1, 
                (superblock->first_data_bitmap_block * 8) + data_bit_sector, 
                (uint32_t)tmp_sector, 
                READ_WITH_RETRY);

        tmp_sector[data_bit / 8] |= 1 << (data_bit % 8);

        rw_sectors(1, 
                (superblock->first_data_bitmap_block * 8) + data_bit_sector, 
                (uint32_t)tmp_sector, 
                WRITE_WITH_RETRY);

        // Set next free bit in data bitmap
        superblock->first_free_bit_in_data_bitmap = 
            get_next_free_bit(superblock->first_data_bitmap_block, 
                              (superblock->first_data_bitmap_block + 
                               superblock->number_of_data_bitmap_blocks));

        // Add another data block to current directory's inode
        // TODO:
    }

    // Change inode in sector
    rw_sectors(1, 
            (superblock->first_inode_block * 8) + (parent_inode.id / 8), 
            (uint32_t)tmp_sector, 
            READ_WITH_RETRY);    

    inode_t *old = (inode_t *)tmp_sector + (parent_inode.id % 8);
    *old = parent_inode;

    rw_sectors(1, 
            (superblock->first_inode_block * 8) + (parent_inode.id / 8), 
            (uint32_t)tmp_sector, 
            WRITE_WITH_RETRY);

    // Write new dir_entry in directory's data blocks
    //   Load dir's last data block 
    const uint32_t total_blocks = bytes_to_blocks(parent_inode.size_bytes);

    uint32_t last_block = 0;
    uint32_t blocks = 0;

    for (uint32_t i = 0; i < 4; i++) { 
        blocks += parent_inode.extent[i].length_blocks; 
        if (blocks == total_blocks) {
            last_block = (parent_inode.extent[i].first_block + parent_inode.extent[i].length_blocks) - 1;
            break;
        }
    }

    // TODO: Check first indirect block & double indirect block extents

    rw_sectors(8, last_block * 8, CURRENT_DIR_ADDRESS, READ_WITH_RETRY);

    // Append new dir_entry to data_block
    const uint32_t inode_bit = superblock->first_free_bit_in_inode_bitmap;  // Next free inode number

    // Find next free spot or end of entries
    dir_entry_t *new_entry = (dir_entry_t *)CURRENT_DIR_ADDRESS;
    uint32_t offset = 0;

    while (new_entry->id != 0 && offset < parent_inode.size_bytes - sizeof(dir_entry_t)) {
        new_entry++; 
        offset += sizeof(dir_entry_t);
    }

    // New file inode ID
    new_entry->id = inode_bit;          

    // New file name
    resolve_path(new_entry->name, path);
    strcpy(new_entry->name, get_last_name_in_path(new_entry->name));

    // Write changed sector
    offset /= FS_SECTOR_SIZE;   // Convert offset in bytes to # of sectors from start of block (0-7)

    rw_sectors(1, 
            (last_block * 8) + offset, 
            CURRENT_DIR_ADDRESS + (offset * FS_SECTOR_SIZE),
            WRITE_WITH_RETRY);

    // Create new inode for file
    const uint32_t data_bit = superblock->first_free_bit_in_data_bitmap; // Next free data bit/block
    fs_datetime_t *datetime = (fs_datetime_t *)RTC_DATETIME_AREA;

    inode_t new_inode = {
        .id = inode_bit,
        .type = file_type,
        .size_bytes = 0,
        .size_sectors = 0,
        .last_changed_timestamp = {
            .second = datetime->second,
            .minute = datetime->minute,
            .hour = datetime->hour,
            .day = datetime->day,
            .month = datetime->month,
            .year = datetime->year,
        },
        .extent[0] = {
            .first_block = data_bit,
            .length_blocks = 1,
        },
        .extent[1] = {0},
        .extent[2] = {0},
        .extent[3] = {0},
        .single_indirect_pointer = 0,
        .double_indirect_pointer = 0,
        .unused = 0,
    };

    if (file_type == FILETYPE_DIR) {
        // Fill directory data
        new_inode.size_bytes = sizeof(dir_entry_t) * 2;   // Default directory only contains "." & ".."
        new_inode.size_sectors = 1;
    }

    // Write new inode to inode bitmap's location in inode blocks
    rw_sectors(1, 
            (superblock->first_inode_block * 8) + (new_inode.id / 8), 
            (uint32_t)tmp_sector,
            READ_WITH_RETRY);

    inode_t *new_inode_ptr = (inode_t *)tmp_sector + (new_inode.id % 8);
    *new_inode_ptr = new_inode;

    rw_sectors(1, 
            (superblock->first_inode_block * 8) + (new_inode.id / 8), 
            (uint32_t)tmp_sector,
            WRITE_WITH_RETRY);

    if (file_type == FILETYPE_DIR) {
        // Write new directory's contents (dir entries) to data bitmap's location in data blocks;
        //   by default a new directory will contain 2 entries: "." and ".." for current/parent dir
        rw_sectors(1,
                new_inode.extent[0].first_block * 8,
                (uint32_t)tmp_block,
                READ_WITH_RETRY);

        // "."
        new_entry = (dir_entry_t *)tmp_block;
        new_entry->id = new_inode.id;
        strcpy(new_entry->name, ".");

        // ".."
        new_entry++;
        new_entry->id = parent_inode.id;     // Parent directory's id
        strcpy(new_entry->name, "..");

        rw_sectors(1,
                new_inode.extent[0].first_block * 8,
                (uint32_t)tmp_block,
                WRITE_WITH_RETRY);
    }

    // Mark inode bit as in use in inode bitmap
    const uint32_t inode_bit_sector = (inode_bit / 8) / FS_SECTOR_SIZE; // Bit -> byte -> sector
    rw_sectors(1, 
            (superblock->first_inode_bitmap_block * 8) + inode_bit_sector, 
            (uint32_t)tmp_sector, 
            READ_WITH_RETRY);

    tmp_sector[inode_bit / 8] |= 1 << (inode_bit % 8);

    rw_sectors(1, 
            (superblock->first_inode_bitmap_block * 8) + inode_bit_sector, 
            (uint32_t)tmp_sector, 
            WRITE_WITH_RETRY);

    superblock->first_free_bit_in_inode_bitmap = 
        get_next_free_bit(superblock->first_inode_bitmap_block, 
                          (superblock->first_inode_bitmap_block + 
                           superblock->number_of_inode_bitmap_blocks));

    // Mark data bit as in use in data bitmap
    const uint32_t data_bit_sector = (data_bit / 8) / FS_SECTOR_SIZE; // Bit -> byte -> sector
    rw_sectors(1, 
            (superblock->first_data_bitmap_block * 8) + data_bit_sector, 
            (uint32_t)tmp_sector, 
            READ_WITH_RETRY);

    tmp_sector[data_bit / 8] |= 1 << (data_bit % 8);

    rw_sectors(1, 
            (superblock->first_data_bitmap_block * 8) + data_bit_sector, 
            (uint32_t)tmp_sector, 
            WRITE_WITH_RETRY);

    superblock->first_free_bit_in_data_bitmap = 
        get_next_free_bit(superblock->first_data_bitmap_block, 
                          (superblock->first_data_bitmap_block + 
                           superblock->number_of_data_bitmap_blocks));

    // Write changed superblock to disk
    rw_sectors(1, 8, (uint32_t)superblock, WRITE_WITH_RETRY);

    // Grab any changes for current directory after creating new file 
    rw_sectors(1, 
               (superblock->first_inode_block * 8) + (current_dir_inode->id / 8),
               (uint32_t)tmp_sector,
               READ_WITH_RETRY);

    *current_dir_inode = *((inode_t *)tmp_sector + (current_dir_inode->id % 8));

    return new_inode.id;
}

// Find dir_entry for an inode id within another inode's data blocks
dir_entry_sector_t find_dir_entry(const uint32_t id, inode_t inode) {
    dir_entry_sector_t result = {0};

    const uint32_t dir_entries_per_block = FS_BLOCK_SIZE / sizeof(dir_entry_t);
    const uint32_t dir_entries_per_sector = FS_SECTOR_SIZE / sizeof(dir_entry_t);

    for (int i = 0; i < 4; i++) {
        for (uint32_t j = 0; j < inode.extent[i].length_blocks; j++) {
            rw_sectors(8, (inode.extent[i].first_block + j) * 8, CURRENT_DIR_ADDRESS, READ_WITH_RETRY);

            uint32_t entries = 0;
            for (dir_entry_t *dir_entry = (dir_entry_t *)CURRENT_DIR_ADDRESS; 
                 entries < dir_entries_per_block; 
                 dir_entry++, entries++) {

                if (dir_entry->id == id) {
                    result.dir_entry = dir_entry;
                    result.disk_sector = ((inode.extent[i].first_block + j) * 8) + (entries / dir_entries_per_sector);
                    result.sector_in_memory = (uint8_t *)(CURRENT_DIR_ADDRESS + ((entries / dir_entries_per_sector) * 512)); 

                    return result;
                }
            }
        }
    }

    // TODO: Check single indirect block and double indirect blocks

    return result;   // Did not find entry
}

