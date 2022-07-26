/*
 * fs/fs_impl.h: File system implementation functions
 */
#pragma once

#include "C/stdbool.h"
#include "fs/fs.h"

// Load a file from disk to memory 
bool fs_load_file(inode_t *inode, uint32_t address) {
    // Read all of the file's blocks to memory
    uint32_t total_blocks = bytes_to_blocks(inode->size_bytes);
    uint32_t address_offset = 0;

    // Read inode direct extents
    for (uint32_t i = 0; i < 4 && total_blocks > 0; i++) {
        rw_sectors(inode->extent[i].length_blocks * 8,
                   inode->extent[i].first_block * 8,
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
