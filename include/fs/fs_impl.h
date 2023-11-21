/*
 * fs/fs_impl.h: File system implementation & helper functions
 */
#pragma once

#include "C/stdbool.h"
#include "C/stdio.h"
#include "fs/fs.h"
#include "sys/syscall_wrappers.h" 

#define MAX_PATH_SIZE 256

char *current_dir;                      // "Current working directory" string
inode_t current_dir_inode;              // Inode for current working dir
inode_t current_parent_inode;
uint8_t temp_sector[FS_SECTOR_SIZE];    // Temporary work sector
uint8_t temp_block[FS_BLOCK_SIZE];      // Temporary work block
inode_t root_inode;                     // Static place to put root dir 
superblock_t superblock;

// Load a file from disk to memory 
bool fs_load_file(inode_t *inode, uint32_t address) {
    // Read all of the file's blocks to memory
    uint32_t total_blocks = bytes_to_blocks(inode->size_bytes);
    uint32_t address_offset = 0;

    // Read inode direct extents
    for (uint32_t i = 0; i < superblock.direct_extents_per_inode && total_blocks > 0; i++) {
        rw_sectors(inode->extent[i].length_blocks * SECTORS_PER_BLOCK,
                   inode->extent[i].first_block * SECTORS_PER_BLOCK,
                   address + address_offset,
                   READ_WITH_RETRY);

        address_offset += inode->extent[i].length_blocks * FS_BLOCK_SIZE;
        total_blocks -= inode->extent[i].length_blocks;
    }

    if (total_blocks > 0) {
        // TODO: Also load single & double indirect extents as needed
    }

    return true;
}

// Save a file from memory to disk 
bool fs_save_file(inode_t *inode, uint32_t address) {
    // Write all of the file's blocks to disk
    uint32_t total_blocks = bytes_to_blocks(inode->size_bytes);
    uint32_t address_offset = 0;

    // Write inode direct extents
    for (uint32_t i = 0; i < superblock.direct_extents_per_inode && total_blocks > 0; i++) {
        rw_sectors(inode->extent[i].length_blocks * SECTORS_PER_BLOCK,
                   inode->extent[i].first_block * SECTORS_PER_BLOCK,
                   address + address_offset,
                   WRITE_WITH_RETRY);

        address_offset += inode->extent[i].length_blocks * FS_BLOCK_SIZE;
        total_blocks -= inode->extent[i].length_blocks;
    }

    if (total_blocks > 0) {
        // TODO: Also write single & double indirect extents as needed
    }

    return true;
}

// Get inode for a given string/file name contained in a given directory inode
inode_t inode_for_name_in_directory(const inode_t directory_inode, const char *file_name) {
    uint32_t total_blocks = bytes_to_blocks(directory_inode.size_bytes);
    dir_entry_t *dir_entry = 0;

    for (uint8_t i = 0; i < superblock.direct_extents_per_inode; i++) {
        // Search inode's extent's data blocks for file_name
        for (uint32_t next_block = directory_inode.extent[i].first_block; 
             next_block < directory_inode.extent[i].first_block + directory_inode.extent[i].length_blocks && 
                total_blocks > 0;
             next_block++, total_blocks--) {
        
            // Load next block to check
            rw_sectors(SECTORS_PER_BLOCK, 
                       next_block*SECTORS_PER_BLOCK, 
                       (uint32_t)temp_block, 
                       READ_WITH_RETRY);

            uint32_t count = 0;

            for (dir_entry = (dir_entry_t *)temp_block;
                 strncmp(dir_entry->name, file_name, strlen(file_name)) != 0 && 
                 count < DIR_ENTRIES_PER_BLOCK;
                 dir_entry++, count++)
                ;

            if (dir_entry->id == 0 || strncmp(dir_entry->name, file_name, strlen(file_name)) != 0) {
                // Did not find the file in this block, keep checking
                continue;
            }

            // Load inode for found file; 8 inode ids in 1 sector, offset from start of all
            //   inode sectors on disk.
            rw_sectors(1,
                       (superblock.first_inode_block*SECTORS_PER_BLOCK) + (dir_entry->id / SECTORS_PER_BLOCK),
                       (uint32_t)temp_sector,
                       READ_WITH_RETRY);

            // Get inode from sector
            return *((inode_t *)temp_sector + (dir_entry->id % SECTORS_PER_BLOCK));
        }
    }

    // Did not find file in directory
    return (inode_t){0};
}

// Get inode for last file in given path
// e.g. /folderA/./.././fileB -> fileB's inode
inode_t inode_from_path(const char *starting_path) {
    char *pos = (char *)starting_path;  
    inode_t current_inode;

    // Traverse path, walking down each inode and resolving relative references as needed
    // Special case, starting at root
    if (*pos == '/') {
        pos++;
        // Set current traversing dir to root
        current_inode = root_inode;
    } else {
        // Not starting at root, assume starting at
        //  current directory
        current_inode = current_dir_inode;
    }

    while (*pos != '\0') {
        if (*pos == '/') {
            pos++; // Skip dir separator
            continue;
        }
                           
        // Relative current directory
        if (pos[0] == '.' && (pos[1] == '\0' || pos[1] == '/')) {
            pos++; 
            continue;
        }

        // Relative parent directory
        if (!strncmp(pos, "..", 2) && (pos[3] == '\0' || pos[3] == '/')) {
            current_inode = inode_for_name_in_directory(current_inode, "..");

            pos += 2;
            continue;
        }

        // Not relative current/working directory, get next full name and load its inode
        char *name = pos; 
        while (*pos != '/' && *pos != '\0') pos++; 

        const char temp = *pos;
        *pos = '\0';    // Set to null to properly null-terminate name string as needed
        
        current_inode = inode_for_name_in_directory(current_inode, name);

        *pos = temp;    // Restore slash/other character
    }

    return current_inode;
}

// Get inode for a given inode(file) id
inode_t inode_from_id(const uint32_t id) {
    if (id == 0) return (inode_t){0};

    // Load sector containing inode with given id
    rw_sectors(1,
               (superblock.first_inode_block*SECTORS_PER_BLOCK) + (id / INODES_PER_SECTOR),
               (uint32_t)temp_sector,
               READ_WITH_RETRY);

    // Return inode from sector
    return *((inode_t *)temp_sector + (id % INODES_PER_SECTOR));
}

// Get inode for parent directory of last file in given path
// e.g. /folderA/./.././folderB/folderC -> folderB's inode
inode_t parent_inode_from_path(const char *starting_path) {
    char *pos = (char *)starting_path;

    pos = strrchr(pos, '/'); // Get end of last directory in path
    if (!pos) {
        // No slashes in path, parent is assumed to be current dir
        return current_dir_inode;
    }

    *pos = '\0';     // Cut off last name

    if (strlen(starting_path) == 0) {
        // parent was root dir/1st char in path
        *pos = '/'; // Restore root dir
        return inode_from_id(1); // Return root inode, id = 1
    }

    inode_t result = inode_from_path(starting_path);   // Inode for last name in path
    *pos = '/';  // Restore last name 

    return result;
}

// Grab the last file name from a path (string)
char *last_name_in_path(const char *path) {
    char *pos = (char *)path;
    pos = strrchr(path, '/'); // End of last dir in path
                              
    if (pos) {
        return pos+1;   // Start of last name in path
    } else {
        return (char *)path;    // Assumed current dir
    }
}

// Get current timestamp
fs_datetime_t current_timestamp(void) {
    return *(fs_datetime_t *)RTC_DATETIME_AREA;
}

// Generic helper function to set a given bit in a chunk
//  of data
void set_bit_in_disk_block(const uint32_t block, uint32_t bit) {
    // Load block to memory
    rw_sectors(SECTORS_PER_BLOCK, block * SECTORS_PER_BLOCK, (uint32_t)temp_block, READ_WITH_RETRY);

    // Set bit in block, using correct 4 byte chunk of bits
    bit %= BITS_PER_BLOCK; // Constrain bit number to this BITS_PER_BLOCK sized block
    uint32_t *chunk = (uint32_t *)temp_block;

    chunk[bit / 32] |= 1 << (bit % 32);

    // Write updated block back to disk
    rw_sectors(SECTORS_PER_BLOCK, block * SECTORS_PER_BLOCK, (uint32_t)temp_block, WRITE_WITH_RETRY);
}

// Generic helper function to find the first unset bit within
//  a chunk of data
uint32_t find_first_free_bit_in_disk_blocks(const uint32_t start_block, const uint32_t length_blocks) {
    for (uint32_t i = 0; i < length_blocks; i++) {
        // Read next block to memory
        rw_sectors(SECTORS_PER_BLOCK, 
                   (start_block + i) * SECTORS_PER_BLOCK, 
                   (uint32_t)temp_block, 
                   READ_WITH_RETRY);

        const uint32_t *chunk = (uint32_t *)temp_block;
        for (uint32_t j = 0; j < FS_BLOCK_SIZE / sizeof(uint32_t); j++) {
           if (chunk[j] != 0xFFFFFFFF) {
               // Found at least 1 bit that is not set/1
               for (uint8_t k = 0; k < 32; k++) {
                   // Check if each bit is 0, not set  
                   if (!(chunk[j] & (1 << k))) {
                       // Found free bit
                       return (j*32) + k;
                   }
               }
           }
        }
    }

    return 0;   // Could not find free bit, error
}

// Update superblock on disk
void update_superblock(void) {
    // Update memory with current data
    *(superblock_t *)SUPERBLOCK_ADDRESS = superblock;   

    // Write updated memory to disk
    rw_sectors(1, SUPERBLOCK_DISK_SECTOR, SUPERBLOCK_ADDRESS, WRITE_WITH_RETRY);
}

// Helper function to update an inode on disk, in the inode disk blocks
void update_inode_on_disk(const inode_t inode) {
    rw_sectors(1, 
               (superblock.first_inode_block * SECTORS_PER_BLOCK) + 
                   (inode.id / INODES_PER_SECTOR),
               (uint32_t)temp_sector,
               READ_WITH_RETRY);

    inode_t *tmp_inode = (inode_t *)temp_sector + (inode.id % INODES_PER_SECTOR);
    *tmp_inode = inode;

    rw_sectors(1, 
               (superblock.first_inode_block * SECTORS_PER_BLOCK) + 
                   (inode.id / INODES_PER_SECTOR),
               (uint32_t)temp_sector,
               WRITE_WITH_RETRY);
}

// Create a new file in the filesystem given a file path
// including: new inode, updating inode bitmap blocks, data bitmap blocks,
// inode blocks, and data blocks for new file. Will probably need to also
// update superblock
inode_t fs_create_file(const char *path) {
    // Disallow creating files with special names 
    if (!strncmp(path, ".", strlen(path))  || 
        !strncmp(path, "..", strlen(path)) ||
        !strncmp(path, "/", strlen(path)))
        return (inode_t){0};

    // Get parent directory of file at path
    inode_t parent_inode = parent_inode_from_path(path);

    if (parent_inode.id == 0) 
        return (inode_t){0};  // Did not find containing dir, error
                              
    char *file_name = last_name_in_path(path); // Will use later for dir_entry->name
    if (!file_name) {
        return (inode_t){0}; // No final name in path or other error
    }

    // Make new inode for file
    inode_t new_inode = {0};

    // Get next free inode bit, use as new inode.id
    new_inode.id = superblock.first_free_inode_bit;

    // Set inode bit as in use in inode bitmap blocks
    set_bit_in_disk_block(superblock.first_inode_bitmap_block + (new_inode.id / BITS_PER_BLOCK), 
                          new_inode.id);

    // Find next free inode bit, this updates superblock 
    superblock.first_free_inode_bit = find_first_free_bit_in_disk_blocks(superblock.first_inode_bitmap_block,
                                                                         superblock.num_inode_bitmap_blocks);
    new_inode.type = FILETYPE_FILE;
    new_inode.last_modified_timestamp = current_timestamp();

    // Get next free data bit, use as extent[0].first_block value
    new_inode.extent[0].first_block = superblock.first_free_data_bit;
    new_inode.extent[0].length_blocks = 1;

    // Set data bit as in use in data bitmap blocks
    set_bit_in_disk_block(superblock.first_data_bitmap_block + (superblock.first_free_data_bit / BITS_PER_BLOCK), 
                          superblock.first_free_data_bit);

    // Find & set next free data bit, this updates superblock
    superblock.first_free_data_bit = find_first_free_bit_in_disk_blocks(superblock.first_data_bitmap_block,
                                                                         superblock.num_data_bitmap_blocks);

    // Update inode blocks for new file/inode
    update_inode_on_disk(new_inode);

    // Update fs info for parent directory inode
    // Update data bitmap blocks for new file/inode in parent dir,
    //   if it takes up another block of data from adding new dir_entry
    const uint32_t current_blocks = bytes_to_blocks(parent_inode.size_bytes);
    const uint32_t new_blocks = bytes_to_blocks(parent_inode.size_bytes + sizeof(dir_entry_t));

    if (new_blocks > current_blocks) {
        // TODO: Abstract this code out into a general helper function,
        //   as there is more than 1 place this logic can be used,
        //   e.g. adding an additional block for files being written to in
        //     write() syscall

        // TODO: Add new file dir_entry data in new block being added, vs. at first found location
        //   in dir's data blocks
        
        // Find the end of the last used extent's data blocks in parent inode
        uint32_t last_used_block = 0;   // Start of last used area on disk for file data
        uint8_t last_used_extent = 0;
        for (last_used_extent = 0; last_used_extent < superblock.direct_extents_per_inode; last_used_extent++) {
            if (parent_inode.extent[last_used_extent].first_block != 0)
                last_used_block = (parent_inode.extent[last_used_extent].first_block + 
                                   parent_inode.extent[last_used_extent].length_blocks) - 1;
        }

        // TODO: Also check single and double indirect extents

        // Check if next block at end of extent's data is available
        // Convert bit to byte to sector
        const uint32_t bit_to_check = last_used_block+1;    // 1 based indexing
        const uint32_t data_bitmap_sector = ((bit_to_check) / 8) / FS_SECTOR_SIZE;
        rw_sectors(1, 
                   (superblock.first_data_bitmap_block * SECTORS_PER_BLOCK) + data_bitmap_sector,
                   (uint32_t)temp_sector,
                   READ_WITH_RETRY);
        
        // Set new bit/data block if NOT in use
        if (!(temp_sector[bit_to_check / 8] & (1 << (bit_to_check % 8)))) {
            parent_inode.extent[last_used_extent].length_blocks++;
            temp_sector[bit_to_check / 8] |= (1 << (bit_to_check % 8));

            rw_sectors(1, 
                       (superblock.first_data_bitmap_block * SECTORS_PER_BLOCK) + data_bitmap_sector,
                       (uint32_t)temp_sector,
                       WRITE_WITH_RETRY);
        } else {
            // Find a new data bitmap bit for a new data block,
            //   and add new extent to parent_inode's extents, first block = new data bit found,
            //   length_blocks = 1
            // Add to next direct extent if available
            if (last_used_extent < superblock.direct_extents_per_inode-1) {
                last_used_extent++;
                parent_inode.extent[last_used_extent].first_block = superblock.first_free_data_bit;
                parent_inode.extent[last_used_extent].length_blocks = 1;
            } else {
                // TODO: Use single or double indirect extents
            }

            // Set data bit as in use in data bitmap blocks
            set_bit_in_disk_block(superblock.first_data_bitmap_block + (superblock.first_free_data_bit / BITS_PER_BLOCK), 
                                  superblock.first_free_data_bit);

            superblock.first_free_data_bit = find_first_free_bit_in_disk_blocks(superblock.first_data_bitmap_block,
                                                                                superblock.num_data_bitmap_blocks);
        }

        // Initialize new data block on disk so that checking for free dir_entry space works later
        const uint32_t block_to_use = (parent_inode.extent[last_used_extent].first_block +
                                       parent_inode.extent[last_used_extent].length_blocks) - 1;

        rw_sectors(SECTORS_PER_BLOCK,
                   block_to_use * SECTORS_PER_BLOCK,
                   (uint32_t)temp_block,
                   READ_WITH_RETRY);

        memset(temp_block, 0, sizeof temp_block);   // Init block to all 0s

        rw_sectors(SECTORS_PER_BLOCK,
                   block_to_use * SECTORS_PER_BLOCK,
                   (uint32_t)temp_block,
                   WRITE_WITH_RETRY);
    }

    // Update remaining parent_inode data
    parent_inode.size_bytes += sizeof(dir_entry_t);
    parent_inode.size_sectors = bytes_to_sectors(parent_inode.size_bytes);
    parent_inode.last_modified_timestamp = current_timestamp();

    // Update inode blocks for parent_dir inode
    update_inode_on_disk(parent_inode);
    
    // Update data block for parent_inode, by adding new dir_entry for new file name and id
    //   use the first empty dir_entry available, or end of list
    const uint32_t total_entries = parent_inode.size_bytes / sizeof(dir_entry_t);
    uint32_t num_entries = 0;
    dir_entry_t *tmp_dir_entry = 0;
    bool wrote_data = false;  

    for (uint8_t i = 0; i < superblock.direct_extents_per_inode && num_entries < total_entries; i++) {
        extent_t tmp_extent = parent_inode.extent[i];
        for (uint32_t j = tmp_extent.first_block; j < tmp_extent.first_block + tmp_extent.length_blocks; j++) {
            rw_sectors(SECTORS_PER_BLOCK, 
                       j * SECTORS_PER_BLOCK,
                       (uint32_t)temp_block,
                       READ_WITH_RETRY);

            uint8_t entries_in_block = 0;
            for (tmp_dir_entry = (dir_entry_t *)temp_block; 
                 num_entries < total_entries && entries_in_block < DIR_ENTRIES_PER_BLOCK; 
                 tmp_dir_entry++, entries_in_block++) {

                if (tmp_dir_entry->id != 0) num_entries++;
                else {
                    // Found empty spot, add new file info here
                    tmp_dir_entry->id = new_inode.id;
                    strcpy(tmp_dir_entry->name, file_name);

                    rw_sectors(SECTORS_PER_BLOCK, 
                               j * SECTORS_PER_BLOCK,
                               (uint32_t)temp_block,
                               WRITE_WITH_RETRY);

                    wrote_data = true;
                    goto done;          // End loops early
                }
            }
        } 
    }

done:   
    if (!wrote_data) {  
        // TODO: Check single and double indirect extents
    }

    // Update current dir inode and root dir inode, in case new file 
    //   was added to either
    if (current_dir_inode.id == parent_inode.id)
        current_dir_inode = parent_inode;

    if (root_inode.id == parent_inode.id)
        root_inode = parent_inode;

    // Update superblock info, at least for inode/data bitmap info,
    //   if not other stuff
    update_superblock();
    
    return new_inode;
};

// Read a given directory's file data, and print to screen
bool print_dir(const char *path) {
    const inode_t dir = inode_from_path(path);
    if (dir.id == 0) return false;

    const uint32_t total_entries = dir.size_bytes / sizeof(dir_entry_t);
    uint32_t num_entries = 0;
    dir_entry_t *dir_entry = 0;

    if (dir.type != FILETYPE_DIR) return false;

    // Load dir's file data
    for (uint8_t i = 0; i < superblock.direct_extents_per_inode; i++) {
        for (uint32_t j = dir.extent[i].first_block; j < dir.extent[i].first_block + dir.extent[i].length_blocks; j++) {
            rw_sectors(SECTORS_PER_BLOCK,
                       j * SECTORS_PER_BLOCK,
                       (uint32_t)temp_block,
                       READ_WITH_RETRY);

            uint8_t entries_in_block = 0;
            for (dir_entry = (dir_entry_t *)temp_block; 
                 num_entries < total_entries && entries_in_block < DIR_ENTRIES_PER_BLOCK;
                 entries_in_block++, dir_entry++) {

                if (dir_entry->id != 0) {
                    num_entries++;

                    // Print out file name from this dir 
                    // TODO: Change printf() to be able to use fixed width with padding and left/right adjust
                    printf("\r\n%s ", dir_entry->name);

                    // Print out inode info for file
                    rw_sectors(1,
                               (superblock.first_inode_block * SECTORS_PER_BLOCK) + 
                                   (dir_entry->id / INODES_PER_SECTOR),
                               (uint32_t)temp_sector,
                               READ_WITH_RETRY);

                    inode_t *inode = (inode_t *)temp_sector + (dir_entry->id % INODES_PER_SECTOR);

                    if (inode->type == FILETYPE_DIR) puts("[DIR] ");

                    printf("%d %d-%d-%d %d:%d:%d  %d", 
                           inode->size_bytes, 

                           inode->last_modified_timestamp.month,
                           inode->last_modified_timestamp.day,
                           inode->last_modified_timestamp.year,
                           inode->last_modified_timestamp.hour,
                           inode->last_modified_timestamp.minute,
                           inode->last_modified_timestamp.second,

                           inode->ref_count);
                }
            }
        }
    }

    // TODO: Also load single/double indirect extents for inode

    // Ended without errors
    puts("\r\n"); 
    return true;
}

// Create a new directory file in the filesystem given a file path
bool fs_make_dir(char *path) {
    if (!path) return false;

    // Create new empty default file
    int32_t fd = open(path, O_CREAT | O_WRONLY);
    if (fd < 0) return false;

    // Grab this new directory's inode and its containing directory's inode
    //   for . and .. dir_entry's
    inode_t dir_inode = inode_from_path(path);
    inode_t parent_inode = parent_inode_from_path(path);

    // Change filetype of new file to DIR
    dir_inode.type = FILETYPE_DIR;

    // Write default dir entries for . and ..
    // "." = this directory itself
    dir_entry_t dir_entry = { .id = dir_inode.id, .name = "." };
    write(fd, &dir_entry, sizeof dir_entry);

    // ".." = parent directory, which contains this new directory
    dir_entry.id = parent_inode.id;
    memcpy(&dir_entry.name, "..", 3);
    write(fd, &dir_entry, sizeof dir_entry);

    // Update size of directory
    dir_inode.size_bytes = sizeof(dir_entry) * 2;
    dir_inode.size_sectors = bytes_to_sectors(dir_inode.size_bytes);

    // Update inode for new dir on disk
    update_inode_on_disk(dir_inode);

    // Update current dir inode and root dir inode, in case new dir 
    //   was added to either
    if (current_dir_inode.id == parent_inode.id)
        current_dir_inode = parent_inode;

    if (root_inode.id == parent_inode.id)
        root_inode = parent_inode;

    // Close file created by open()
    close(fd);

    return true;
}

// Change current directory
bool fs_change_dir(char *path) {
    if (!path) return false;

    // TODO: Create multiple directories in path if not exist?

    // Change directory to current directory
    if (!strncmp(path, ".", 2)) return true;

    inode_t dir_inode = inode_from_path(path);
    if (dir_inode.id == 0) return false;    // Dir does not exist

    if (dir_inode.type != FILETYPE_DIR) return false;   // Not a directory

    char *curr = current_dir;
    if (*path == '/') {
        // Changing to explicit path starting at root
        *curr = *path;

    } else {
        // Start path from current dir path,
        //   get pointer to end of current path
        while (*curr) curr++;
        curr--; // Go back before ending null terminator
    }

    // Update current directory 
    current_dir_inode = dir_inode;

    for (char *p = path; *p != '\0'; p++) {
        // '/' = directory separator, skip over
        if (*p == '/') continue;

        // .. = parent directory, skip over, set current dir string
        //   to end on next parent directory (up by 1 level)
        if (!strncmp(p, "..", 2)) {
            if (*curr == '/') curr--;   // Go before current separator first

            while (*curr != '/') curr--;    

            p++;
            continue;
        }

        // . = current directory, skip over
        if (*p == '.') continue;

        // Else found next folder name, copy to current_dir string
        curr++; // Move past slash in current_dir string
        while (*p != '\0' && *p != '/') *curr++ = *p++;

        *curr = '/';    // Always end current_dir path with a slash
    }

    // Will always end current_dir path string with a slash
    if (*curr != '/') *curr = '/';
    *(curr + 1) = '\0';   // Null terminate new string

    return true;
}

