/*
 *  kmalloc.h: Malloc() & Free() C function implementations, for kernel use
 */
#pragma once

#include "C/stdint.h"
#include "C/stdbool.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"

#define PAGE_SIZE 4096

// Singly linked list nodes for blocks of memory
typedef struct kmalloc_block {
    uint32_t size;  // Size of this memory block in bytes
    bool free;      // Is this block of memory free?
    struct kmalloc_block *next;  // Next block of memory
} kmalloc_block_t;

static kmalloc_block_t *kmalloc_list_head = 0;    // Start of linked list
static uint32_t kmalloc_virt_address      = 0x300000;
static uint32_t kmalloc_phys_address      = 0;
static uint32_t kmalloc_total_pages       = 0;

// Initialize malloc bytes/linked list for first malloc() call from a program
void kmalloc_init(const uint32_t bytes)
{
    kmalloc_total_pages = bytes / PAGE_SIZE;
    if (bytes % PAGE_SIZE > 0) kmalloc_total_pages++;    // Partial page

    // TODO: Change to allocate individual pages in case available physical memory is not contiguous
    kmalloc_phys_address = (uint32_t)allocate_blocks(kmalloc_total_pages);
    kmalloc_list_head    = (kmalloc_block_t *)kmalloc_virt_address;

    // Map in pages
    for (uint32_t i = 0, virt = kmalloc_virt_address; i < kmalloc_total_pages; i++, virt += PAGE_SIZE) {
        map_page((void *)(kmalloc_phys_address + i*PAGE_SIZE), (void *)virt);

        pt_entry *page = get_page(virt);

        SET_ATTRIBUTE(page, PTE_READ_WRITE); // Read/write access for malloc-ed pages
    }

    if (kmalloc_list_head) {
        kmalloc_list_head->size = (kmalloc_total_pages * PAGE_SIZE) - sizeof(kmalloc_block_t);
        kmalloc_list_head->free = true;
        kmalloc_list_head->next = 0;
    }
}

// Split linked list node/memory block into two by inserting a new block with
//   the size of the difference of original block and requested size
void kmalloc_split(kmalloc_block_t *node, const uint32_t size)
{
    kmalloc_block_t *new_node = (kmalloc_block_t *)((void *)node + size + sizeof(kmalloc_block_t));

    new_node->size = node->size - size - sizeof(kmalloc_block_t);
    new_node->free = true;
    new_node->next = node->next;

    node->size = size;
    node->free = false;
    node->next = new_node;
}

// Find & allocate next block of memory
void *kmalloc_next_block(const uint32_t size)
{
    kmalloc_block_t *cur = 0, *prev = 0; 

    // Nothing to malloc
    if (size == 0) return 0;

    // If no bytes in list to malloc, init it first
    if (!kmalloc_list_head->size) kmalloc_init(size);

    // Find first available block in list
    cur = kmalloc_list_head;
    while (((cur->size < size) || !cur->free) && cur->next) {
        prev = cur;
        cur = cur->next;
    }

    if (size == cur->size) {
        // Node size is what we are looking for
        cur->free = false;
    } else if (cur->size > size + sizeof(kmalloc_block_t)) {
        // Node is larger than what we need, split it into 2 nodes,
        //   will return current node for requested size
        kmalloc_split(cur, size);
    } else {
        // Node is too small, allocate more pages to current node, and split
        //   to return a node of requested size
        uint8_t num_pages = 1;
        while (cur->size + num_pages*PAGE_SIZE < size + sizeof(kmalloc_block_t))
            num_pages++;

        // Allocate & map in new pages starting at next virtual address page boundary (next 0x1000/4KB)
        uint32_t virt = kmalloc_virt_address + kmalloc_total_pages*PAGE_SIZE;

        for (uint8_t i = 0; i < num_pages; i++) {
            pt_entry page = 0;
            uint32_t *temp = allocate_page(&page);

            map_page((void *)temp, (void *)virt);
            SET_ATTRIBUTE(&page, PTE_READ_WRITE);
            virt += PAGE_SIZE;
            cur->size += PAGE_SIZE;
            kmalloc_total_pages++;
        }

        kmalloc_split(cur, size);
    }
        
    // Return node pointing to requested bytes, casted to char * to remove pointer
    //   arithmetic
    return (char *)cur + sizeof(kmalloc_block_t);
}

// Merge consecutive free list nodes to prevent (some) memory fragmentation
void kmerge_free_blocks(void)
{
    kmalloc_block_t *cur = kmalloc_list_head, *prev = 0;

    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += (cur->next->size) + sizeof(kmalloc_block_t);
            cur->next = cur->next->next;
        }
        prev = cur;
        cur = cur->next;
    }
}

void kmalloc_free(void *ptr)
{
    for (kmalloc_block_t *cur = kmalloc_list_head; cur->next; cur = cur->next) 
        if ((void *)cur + sizeof(kmalloc_block_t) == ptr) {
            cur->free = true;
            kmerge_free_blocks();
            break;
        }
}
