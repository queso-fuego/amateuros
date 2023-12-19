/*
 *  3rdstage.c: Pre-kernel memory setup
 */
#include "C/stdint.h"
#include "C/string.h"
#include "global/global_addresses.h"
#include "disk/file_ops.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"
#include "gfx/2d_gfx.h"
#include "fs/fs_impl.h"

__attribute__ ((section ("prekernel_entry"))) void prekernel_main(void) {
    // Physical memory map entry from Bios Int 15h EAX E820h
    typedef struct SMAP_entry {
        uint64_t base_address;
        uint64_t length;
        uint32_t type;
        uint32_t acpi;
    } __attribute__ ((packed)) SMAP_entry_t;

    uint32_t num_SMAP_entries; 
    uint32_t total_memory; 
    SMAP_entry_t *SMAP_entry;

    // Set up physical memory manager
    num_SMAP_entries = *(uint32_t *)SMAP_NUMBER_ADDRESS;        
    SMAP_entry       = (SMAP_entry_t *)SMAP_ENTRIES_ADDRESS;   
    SMAP_entry += num_SMAP_entries - 1;

    total_memory = SMAP_entry->base_address + SMAP_entry->length - 1;

    // Initialize physical memory manager to all available memory, put it at some location
    // All memory will be set as used/reserved by default
    initialize_memory_manager(MEMMAP_AREA, total_memory);

    // Initialize memory regions for the available memory regions in the SMAP (type = 1)
    SMAP_entry = (SMAP_entry_t *)SMAP_ENTRIES_ADDRESS; 
    for (uint32_t i = 0; i < num_SMAP_entries; i++) {
        if (SMAP_entry->type == 1)
            initialize_memory_region(SMAP_entry->base_address, SMAP_entry->length);

        SMAP_entry++;
    }

    // Set memory regions/blocks for the kernel and "OS" memory map areas as used/reserved
    deinitialize_memory_region(0x1000, 0x11000);                           // Reserve all memory below 12000h for the kernel/OS
    deinitialize_memory_region(MEMMAP_AREA, max_blocks / BLOCKS_PER_BYTE); // Reserve physical memory map area 

    // Load initial superblock state
    superblock = *(superblock_t *)SUPERBLOCK_ADDRESS;

    // Load root inode, root is always inode 1 
    rw_sectors(1, 
               (superblock.first_inode_block*8),   // 1 block = 8 sectors
               (uint32_t)temp_sector,
               READ_WITH_RETRY);

    root_inode = *((inode_t *)temp_sector + 1);
    superblock.root_inode_pointer = (uint32_t)&root_inode;

    // Set filesystem starting point
    char temp_buf[1024]; // Set starting size of current working directory string
    current_dir = temp_buf; 
    memset(current_dir, 0, 1024); // Initialize memory

    strcpy(current_dir, "/");   // Start in 'root' directory by default
    current_dir_inode = root_inode;
    current_parent_inode = root_inode;  // Root's parent is itself

    // Find kernel inode
    //inode_t inode = inode_from_path("/kernel.bin");
    inode_t inode = inode_from_path("/sys/bin/kernel.bin"); // TODO: Use this when make_disk.c is ready

    // Load kernel from disk
    fs_load_file(&inode, KERNEL_ADDRESS);
    
    // Find font inode
    //inode = inode_from_path("/ter-u32n.bin");
    inode = inode_from_path("/sys/bin/ter-u32n.bin"); // TODO: Use this when make_disk.c is ready

    // Load font from disk
    fs_load_file(&inode, FONT_ADDRESS);

    // Mark font memory as in use
    deinitialize_memory_region(FONT_ADDRESS, inode.extent[0].length_blocks * FS_BLOCK_SIZE);

    // Set up virtual memory & paging - TODO: Check if return value is true/false
    initialize_virtual_memory_manager();

    // Identity map VBE framebuffer
    const uint32_t fb_size_in_bytes = (gfx_mode->y_resolution * gfx_mode->linear_bytes_per_scanline);
    uint32_t fb_size_in_pages = fb_size_in_bytes / PAGE_SIZE;
    if (fb_size_in_pages % PAGE_SIZE > 0) fb_size_in_pages++;
    
    // For hardware, double size of framebuffer pages just in case    
    fb_size_in_pages *= 2;

    for (uint32_t i = 0, fb_start = gfx_mode->physical_base_pointer; i < fb_size_in_pages; i++, fb_start += PAGE_SIZE)
        map_page((void *)fb_start, (void *)fb_start);

    // Mark framebuffer as in use for physical memory manager
    deinitialize_memory_region(gfx_mode->physical_base_pointer, fb_size_in_pages * BLOCK_SIZE);

    // Set physical memory manager variables for kernel to use
    *(uint32_t *)PHYS_MEM_MAX_BLOCKS  = max_blocks;
    *(uint32_t *)PHYS_MEM_USED_BLOCKS = used_blocks;

    // Remove lower half kernel mapping 
    for (uint32_t virt = KERNEL_ADDRESS; virt < 0x400000; virt += PAGE_SIZE)
        unmap_page((void *)virt);

    // Reload CR3 register to flush TLB to update unmapped pages
    __asm__ __volatile__ ("movl %CR3, %ECX; movl %ECX, %CR3");

    // Store current page directory for kernel to use
    *(uint32_t *)CURRENT_PAGE_DIR_ADDRESS = (uint32_t)current_page_directory;

    // Call/execute higher half kernel
    ((void (*)(void))0xC0000000)();
}
