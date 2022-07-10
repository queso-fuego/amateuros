/*
 * print_dir.h: Print directory contents; inode & dir_entry information
*/
#pragma once

#include "C/stdint.h"
#include "C/kstdio.h"
#include "fs/fs_impl.h"
#include "global/global_addresses.h"
#include "disk/file_ops.h"  // TODO: Rename to sata/sata.h?

void print_inode_info(const dir_entry_t *dir_entry) {
    superblock_t *superblock = (superblock_t *)BOOTLOADER_SUPERBLOCK_ADDRESS;

    // Get inode 
    // 8 inodes per sector
    // 8 sectors per block
    rw_sectors(1, 
            (superblock->first_inode_block * 8) + (dir_entry->id / 8), 
            (uint32_t)tmp_sector, 
            READ_WITH_RETRY);

    // Get correct inode within 8 inode sector
    inode_t *inode = (inode_t *)tmp_sector + (dir_entry->id % 8);

    // Print inode info for this file
    kprintf("%d %d/%d/%d %d:%d:%d %s", 
            inode->size_bytes, 
            inode->last_changed_timestamp.month, 
            inode->last_changed_timestamp.day, 
            inode->last_changed_timestamp.year, 
            inode->last_changed_timestamp.hour, 
            inode->last_changed_timestamp.minute, 
            inode->last_changed_timestamp.second, 
            dir_entry->name);

    if (inode->type == FILETYPE_DIR) 
        kputs("  [DIR]");

    kprintf("\r\n");
}

void print_dir(const inode_t *curr_dir) {
    // Read and print all files in this directory, in all blocks
    uint32_t total_bytes = curr_dir->size_bytes;
    uint32_t total_blocks = bytes_to_blocks(curr_dir->size_bytes);

    kprintf("\r\n");

    if (total_blocks == 0) return;

    for (int i = 0; i < 4 && total_blocks > 0; i++) {
        for (uint32_t j = 0; j < curr_dir->extent[i].length_blocks; j++) {
            rw_sectors(8, (curr_dir->extent[i].first_block + j) * 8, CURRENT_DIR_ADDRESS, READ_WITH_RETRY);

            uint8_t num_entries = FS_BLOCK_SIZE / sizeof(dir_entry_t);

            for (dir_entry_t *dir_entry = (dir_entry_t *)CURRENT_DIR_ADDRESS; 
                 total_bytes > 0 && num_entries > 0; 
                 dir_entry++, num_entries--) {

                // Only reduce bytes and read entry if valid, not deleted or empty
                //   this will skip over "holes" and get all valid entries 
                if (dir_entry->id != 0) {
                    print_inode_info(dir_entry);
                    total_bytes -= sizeof(dir_entry_t); 
                }
            }
        }
        total_blocks -= curr_dir->extent[i].length_blocks;
    }

    // TODO: Single indirect block and double indirect block
    //if (total_blocks == 0) return;
}
