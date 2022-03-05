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

__attribute__ ((section ("prekernel_entry"))) void prekernel_main(void)
{
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
    uint8_t ext[3];

    // Set up physical memory manager
    num_SMAP_entries = *(uint32_t *)0x8500;
    SMAP_entry       = (SMAP_entry_t *)0x8504;
    SMAP_entry += num_SMAP_entries - 1;

    total_memory = SMAP_entry->base_address + SMAP_entry->length - 1;

    // Initialize physical memory manager to all available memory, put it at some location
    // All memory will be set as used/reserved by default
    initialize_memory_manager(MEMMAP_AREA, total_memory);

    // Initialize memory regions for the available memory regions in the SMAP (type = 1)
    SMAP_entry = (SMAP_entry_t *)0x8504;
    for (uint32_t i = 0; i < num_SMAP_entries; i++) {
        if (SMAP_entry->type == 1)
            initialize_memory_region(SMAP_entry->base_address, SMAP_entry->length);

        SMAP_entry++;
    }

    // Set memory regions/blocks for the kernel and "OS" memory map areas as used/reserved
    deinitialize_memory_region(0x1000, 0xB000);                            // Reserve all memory below C000h for the kernel/OS
    deinitialize_memory_region(MEMMAP_AREA, max_blocks / BLOCKS_PER_BYTE); // Reserve physical memory map area 

    // Load kernel from disk so that virtual memory manager works
    // TODO: Add error handling later
    load_file("kernel", 6, KERNEL_ADDRESS, ext);

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
