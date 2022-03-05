/*
 *  virtual_memory_manager.h: Definitions & functions for paging, virtual memory, mapping/unmapping pages, etc.
 */
#pragma once

#include "C/stdbool.h"
#include "C/string.h"
#include "memory/physical_memory_manager.h"
#include "global/global_addresses.h"

#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIRECTORY 1024
#define PAGE_SIZE 4096

#define PD_INDEX(address) ((address) >> 22)
#define PT_INDEX(address) (((address) >> 12) & 0x3FF) // Max index 1023 = 0x3FF
#define PAGE_PHYS_ADDRESS(dir_entry) ((*dir_entry) & ~0xFFF)    // Clear lowest 12 bits, only return frame/address
#define SET_ATTRIBUTE(entry, attr) (*entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (*entry &= ~attr)
#define TEST_ATTRIBUTE(entry, attr) (*entry & attr)
#define SET_FRAME(entry, address) (*entry = (*entry & ~0x7FFFF000) | address)   // Only set address/frame, not flags

typedef uint32_t pt_entry;  // Page table entry
typedef uint32_t pd_entry;  // Page directory entry
typedef uint32_t physical_address; 
typedef uint32_t virtual_address; 

typedef enum {
    PTE_PRESENT       = 0x01,
    PTE_READ_WRITE    = 0x02,
    PTE_USER          = 0x04,
    PTE_WRITE_THROUGH = 0x08,
    PTE_CACHE_DISABLE = 0x10,
    PTE_ACCESSED      = 0x20,
    PTE_DIRTY         = 0x40,
    PTE_PAT           = 0x80,
    PTE_GLOBAL        = 0x100,
    PTE_FRAME         = 0x7FFFF000,   // bits 12+
} PAGE_TABLE_FLAGS;

typedef enum {
    PDE_PRESENT       = 0x01,
    PDE_READ_WRITE    = 0x02,
    PDE_USER          = 0x04,
    PDE_WRITE_THROUGH = 0x08,
    PDE_CACHE_DISABLE = 0x10,
    PDE_ACCESSED      = 0x20,
    PDE_DIRTY         = 0x40,          // 4MB entry only
    PDE_PAGE_SIZE     = 0x80,          // 0 = 4KB page, 1 = 4MB page
    PDE_GLOBAL        = 0x100,         // 4MB entry only
    PDE_PAT           = 0x2000,        // 4MB entry only
    PDE_FRAME         = 0x7FFFF000,    // bits 12+
} PAGE_DIR_FLAGS;

// Page table: handle 4MB each, 1024 entries * 4096
typedef struct {
    pt_entry entries[PAGES_PER_TABLE];
} page_table;

// Page directory: handle 4GB each, 1024 page tables * 4MB
typedef struct {
    pd_entry entries[TABLES_PER_DIRECTORY];
} page_directory;

page_directory *current_page_directory = 0;

// Get entry in page table for given address
pt_entry *get_pt_entry(page_table *pt, virtual_address address)
{
    if (pt) return &pt->entries[PT_INDEX(address)];
    return 0;
}

// Get entry in page directory for given address
pd_entry *get_pd_entry(page_table *pd, virtual_address address)
{
    if (pd) return &pd->entries[PT_INDEX(address)];
    return 0;
}

// Return a page for a given virtual address in the current
//   page directory
pt_entry *get_page(const virtual_address address)
{
    // Get page directory
    page_directory *pd = current_page_directory; 

    // Get page table in directory
    pd_entry   *entry = &pd->entries[PD_INDEX(address)];
    page_table *table = (page_table *)PAGE_PHYS_ADDRESS(entry);

    // Get page in table
    pt_entry *page = &table->entries[PT_INDEX(address)];
    
    // Return page
    return page;
}

// Allocate a page of memory
void *allocate_page(pt_entry *page) 
{
    void *block = allocate_blocks(1);
    if (block) {
        // Map page to block
        SET_FRAME(page, (physical_address)block);
        SET_ATTRIBUTE(page, PTE_PRESENT);
    }

    return block;
}

// Free a page of memory
void free_page(pt_entry *page)
{
    void *address = (void *)PAGE_PHYS_ADDRESS(page);
    if (address) free_blocks(address, 1);

    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

// Set the current page directory
bool set_page_directory(page_directory *pd)
{
    if (!pd) return false;

    current_page_directory = pd;

    // CR3 (Control register 3) holds address of the current page directory
    __asm__ __volatile__ ("movl %%EAX, %%CR3" : : "a"(current_page_directory) );

    return true;
}

// Flush a single page from the TLB (translation lookaside buffer)
void flush_tlb_entry(virtual_address address)
{
    __asm__ __volatile__ ("cli; invlpg (%0); sti" : : "r"(address) );
}

// Map a page
bool map_page(void *phys_address, void *virt_address)
{
    // Get page
    page_directory *pd = current_page_directory;

    // Get page table
    pd_entry *entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

    // TODO: Use TEST_ATTRIBUTE for this check?
    if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
        // Page table not present allocate it
        page_table *table = (page_table *)allocate_blocks(1);
        if (!table) return false;   // Out of memory

        // Clear page table
        memset(table, 0, sizeof(page_table));

        // Create new entry
        pd_entry *entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

        // Map in the table & enable attributes
        SET_ATTRIBUTE(entry, PDE_PRESENT);
        SET_ATTRIBUTE(entry, PDE_READ_WRITE);
        SET_FRAME(entry, (physical_address)table);
    }

    // Get table 
    page_table *table = (page_table *)PAGE_PHYS_ADDRESS(entry);

    // Get page in table
    pt_entry *page = &table->entries[PT_INDEX((uint32_t)virt_address)];

    // Map in page
    SET_ATTRIBUTE(page, PTE_PRESENT);
    SET_FRAME(page, (uint32_t)phys_address);

    return true;
}

// Unmap a page
void unmap_page(void *virt_address)
{
    pt_entry *page = get_page((uint32_t)virt_address);

    SET_FRAME(page, 0);     // Set physical address to 0 (effectively this is now a null pointer)
    CLEAR_ATTRIBUTE(page, PTE_PRESENT); // Set as not present, will trigger a #PF
}

// Initialize virtual memory manager
bool initialize_virtual_memory_manager(void)
{
    // Create a default page directory
    page_directory *dir = (page_directory *)allocate_blocks(3);

    if (!dir) return false; // Out of memory

    // Clear page directory and set as current
    memset(dir, 0, sizeof(page_directory));
    for (uint32_t i = 0; i < 1024; i++)
        dir->entries[i] = 0x02; // Supervisor, read/write, not present

    // Allocate page table for 0-4MB
    page_table *table = (page_table *)allocate_blocks(1);

    if (!table) return false;   // Out of memory

    // Allocate a 3GB page table
    page_table *table3G = (page_table *)allocate_blocks(1);

    if (!table3G) return false;   // Out of memory

    // Clear page tables
    memset(table, 0, sizeof(page_table));
    memset(table3G, 0, sizeof(page_table));

    // Identity map 1st 4MB of memory
    for (uint32_t i = 0, frame = 0x0, virt = 0x0; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        // Create new page
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        // Add page to 3GB page table
        table3G->entries[PT_INDEX(virt)] = page;
    }

    // Map kernel to 3GB+ addresses (higher half kernel)
    for (uint32_t i = 0, frame = KERNEL_ADDRESS, virt = 0xC0000000; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        // Create new page
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        // Add page to 0-4MB page table
        table->entries[PT_INDEX(virt)] = page;
    }

    pd_entry *entry = &dir->entries[PD_INDEX(0xC0000000)];
    SET_ATTRIBUTE(entry, PDE_PRESENT);
    SET_ATTRIBUTE(entry, PDE_READ_WRITE);
    SET_FRAME(entry, (physical_address)table); // 3GB directory entry points to default page table

    pd_entry *entry2 = &dir->entries[PD_INDEX(0x00000000)];
    SET_ATTRIBUTE(entry2, PDE_PRESENT);
    SET_ATTRIBUTE(entry2, PDE_READ_WRITE);   
    SET_FRAME(entry2, (physical_address)table3G);    // Default dir entry points to 3GB page table

    // Switch to page directory
    set_page_directory(dir);

    // Enable paging: Set PG (paging) bit 31 and PE (protection enable) bit 0 of CR0
    __asm__ __volatile__ ("movl %CR0, %EAX; orl $0x80000001, %EAX; movl %EAX, %CR0");

    return true;
}
