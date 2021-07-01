/*
 *  physical_memory_manager.h: Provide functions to allocate/free or handle blocks of physical
 *      memory
 */
#pragma once

#define BLOCK_SIZE      4096     // Size of 1 block of memory, 4KB
#define BLOCKS_PER_BYTE 8        // Using a bitmap, each byte will hold 8 bits/blocks

// Global variables
static uint32_t *memory_map = 0;
static uint32_t max_blocks  = 0;
static uint32_t used_blocks = 0;

// Sets a block/bit in the memory map (mark block as used)
void set_block(uint32_t bit)
{
    // Divide bit by 32 to get 32bit "chunk" of memory containing bit to be set;
    // Shift 1 by remainder of bit divided by 32 to get bit to set within the
    //   32 bit chunk
    // OR to set that bit
    memory_map[bit/32] |= (1 << (bit % 32));
}

// Unsets a block/bit in the memory map (mark block as free)
void unset_block(uint32_t bit)
{
    // Divide bit by 32 to get 32bit "chunk" of memory containing bit to be set;
    // Shift 1 by remainder of bit divided by 32 to get bit to set within the
    //   32 bit chunk
    // AND with the inverse of those bits to clear the bit to 0
    memory_map[bit/32] &= ~(1 << (bit % 32));
}

// Test if a block/bit in the memory map is set/used
uint8_t test_block(uint32_t bit)
{
    // Divide bit by 32 to get 32bit "chunk" of memory containing bit to be set;
    // Shift 1 by remainder of bit divided by 32 to get bit within the 32bit chunk;
    // AND and return that bit (will be 1 or 0)
    return memory_map[bit/32] & (1 << (bit % 32));
}

// Find the first free blocks of memory for a given size
int32_t find_first_free_blocks(uint32_t num_blocks)
{
    if (num_blocks == 0) return -1; // Can't return no memory, error

    // Test 32 blocks at a time
    for (uint32_t i = 0; i < max_blocks / 32;  i++) {
        if (memory_map[i] != 0xFFFFFFFF) {
            // At least 1 bit is not set within this 32bit chunk of memory,
            //   find that bit by testing each bit
            for (int32_t j = 0; j < 32; j++) {
                int32_t bit = 1 << j;

                // If bit is unset/0, found start of a free region of memory
                if (!(memory_map[i] & bit)) {
                    int32_t start_bit = i*32 + bit;  // Get bit at index i within memory map
                    uint32_t free_blocks = 0;

                    for (uint32_t count = 0; count <= num_blocks; count++) {
                        if (!test_block(start_bit + count)) free_blocks++;

                        if (free_blocks == num_blocks) // Found enough free space
                            return i*32 + j;
                    }
                }
            }
        }
    }

    return -1;  // No free region of memory large enough
}

// Initialize memory manager, given an address and size to put the memory map
void initialize_memory_manager(uint32_t start_address, uint32_t size)
{
    memory_map = (uint32_t *)start_address;
    max_blocks  = size / BLOCK_SIZE;
    used_blocks = max_blocks;       // To start off, every block will be "used/reserved"

    // By default, set all memory in use (used blocks/bits = 1, every block is set)
    // Each byte of memory map holds 8 bits/blocks
    memset(memory_map, 0xFF, max_blocks / BLOCKS_PER_BYTE);
}

// Initialize region of memory (sets blocks as free/available)
void initialize_memory_region(uint32_t base_address, uint32_t size)
{
    int32_t align      = base_address / BLOCK_SIZE;  // Convert memory address to blocks
    int32_t num_blocks = size / BLOCK_SIZE;          // Convert size to blocks

    for (; num_blocks > 0; num_blocks--) {
        unset_block(align++);
        used_blocks--;
    }

    // Always going to set the 1st block, ensure we can't alloc 0, and can't overwrite
    //   interrupt tables IDT/IVT, Bios data areas, etc.
    set_block(0);
}

// De-initialize region of memory (sets blocks as used/reserved)
void deinitialize_memory_region(uint32_t base_address, uint32_t size)
{
    int32_t align      = base_address / BLOCK_SIZE;  // Convert memory address to blocks
    int32_t num_blocks = size / BLOCK_SIZE;          // Convert size to blocks

    for (; num_blocks > 0; num_blocks--) {
        set_block(align++);
        used_blocks++;
    }
}

// Allocate blocks of memory
uint32_t *allocate_blocks(uint32_t num_blocks)
{
    // If # of free blocks left is not enough, we can't allocate any more, return
    if ((max_blocks - used_blocks) <= num_blocks) return 0;   

    int32_t starting_block = find_first_free_blocks(num_blocks);
    if (starting_block == -1) return 0;     // Couldn't find that many blocks in a row to allocate

    // Found free blocks, set them as used
    for (uint32_t i = 0; i < num_blocks; i++)
        set_block(starting_block + i);

    used_blocks += num_blocks;  // Blocks are now used/reserved, increase count

    // Convert blocks to bytes to get start of actual RAM that is now allocated
    uint32_t address = starting_block * BLOCK_SIZE; 

    return (uint32_t *)address;  // Physical memory location of allocated blocks
}

// Free blocks memory
void free_blocks(uint32_t *address, uint32_t num_blocks)
{
    int32_t starting_block = (uint32_t)address / BLOCK_SIZE;   // Convert address to blocks 

    for (uint32_t i = 0; i < num_blocks; i++) 
        unset_block(starting_block + i);    // Unset bits/blocks in memory map, to free

    used_blocks -= num_blocks;  // Decrease used block count
}

