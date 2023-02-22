/*
 * syscalls.h: Syscall handler/dispatcher and system call functions
 */
#pragma once

#include "C/stdint.h"
#include "C/stdlib.h"
#include "C/stdio.h"    
#include "sys/syscall_numbers.h"
#include "print/print_types.h"
#include "interrupts/pic.h"
#include "memory/malloc.h"
#include "memory/virtual_memory_manager.h" 
#include "terminal/terminal.h"
#include "fs/fs_impl.h"

// Registers pushed onto stack when a syscall function is called
//  from the syscall dispatcher
typedef struct {
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t eax;
} __attribute__ ((packed)) syscall_regs_t;

// These extern vars are from kernel.c
extern open_file_table_t *open_file_table;  
extern uint32_t max_open_files;
extern uint32_t current_open_files;         // FD 0/1/2 reserved for stdin/out/err

extern inode_t *open_inode_table;
extern uint32_t max_open_inodes;
extern uint32_t current_open_inodes;
extern uint32_t next_available_file_virtual_address;

// Test syscall 0
int32_t syscall_test0(syscall_regs_t regs) {
    printf("\r\nTest Syscall; Syscall # (EAX): %d\r\n", regs.eax);

    return EXIT_SUCCESS;
}

// Test syscall 1
int32_t syscall_test1(syscall_regs_t regs) {
    printf("\r\nTest Syscall; Syscall # (EAX): %d\r\n", regs.eax);

    return EXIT_SUCCESS;
}

// Sleep for a given number of milliseconds
// INPUTS:
//  EBX = # of milliseconds
int32_t syscall_sleep(syscall_regs_t regs) {
    *sleep_timer_ticks = regs.ebx;  // Set ticks value to sleep for

    // Wait ("Sleep") until # of ticks is 0
    while (*sleep_timer_ticks > 0) __asm__ __volatile__ ("sti;hlt;cli");

    return EXIT_SUCCESS;
}

// Allocate uninitialized memory
// INPUT:
//   EBX = size in bytes to allocate
int32_t syscall_malloc(syscall_regs_t regs) {
    uint32_t bytes = regs.ebx;

    // First malloc() from the calling program?
    if (!malloc_list_head)
        malloc_init(bytes); // Yes, set up initial memory/linked list

    void *ptr = malloc_next_block(bytes);

    merge_free_blocks();    // Combine consecutive free blocks of memory

    // Return pointer to malloc-ed memory
    __asm__ __volatile__ ("mov %0, %%EDX" : : "g"(ptr) );

    return EXIT_SUCCESS;
}

// Free allocated memory at a pointer
// INPUT:
//   EBX = pointer to malloc-ed bytes
int32_t syscall_free(syscall_regs_t regs) {
    void *ptr = (void *)regs.ebx;

    malloc_free(ptr);

    return EXIT_SUCCESS;
}

// Write system call: Write bytes from a buffer to a file descriptor
int32_t syscall_write(syscall_regs_t regs) {
    int32_t fd   = regs.ebx;
    void *buf    = (void *)regs.ecx;
    uint32_t len = regs.edx;

    uint32_t bytes_written = 0;

    if (fd < 0) {
        // Invalid FD
        return -1;
    }

    if (fd == stdin ) {
        // TODO: Remove error later
        // Can't write to stdin, invalid FD for writing
        return -1;
    }

    if (fd == stdout || fd == stderr) {
        // Terminal write will return bytes consumed/written,
        // Return early, don't run code below
        return terminal_write(buf, len);   
    } 

    // Get open file table entry for input FD
    open_file_table_t *oft = open_file_table + fd;

    // Error: file not found or is not open
    if (oft->inode == 0 || oft->ref_count == 0) {
        return -1;
    }

    // Check FD's open flags
    if (!(oft->flags & O_WRONLY) && 
        !(oft->flags & O_RDWR)) {
        // Error, FD is only open for reading
        return -1; 
    }

    // Check for O_APPEND flag, if used, set file offset to end of file (current file size)
    if (oft->flags & O_APPEND) {
        oft->offset = oft->inode->size_bytes;
    }

    // Check if current file offset is greater than length/size of file as pages allocated 
    //  in memory, if so, last seek() call probably went beyond end of file
    if ((uint32_t)oft->offset > oft->pages_allocated * PAGE_SIZE) {
        // Allocate additional pages to reach new end of file from previous seek(),
        //   and zero pad memory 
        const uint32_t bytes_to_allocate = oft->offset - oft->inode->size_bytes;
        uint32_t size_in_pages = bytes_to_blocks(bytes_to_allocate);
        if (size_in_pages == 0) size_in_pages = 1;  // Reserve 1 page by default for new/empty files

        // Allocate enough pages/blocks for file
        for (uint32_t i = 0; i < size_in_pages; i++) {
            pt_entry page = 0;
            uint32_t phys_addr = (uint32_t)allocate_page(&page);

            if (!map_page((void *)phys_addr, (void *)(next_available_file_virtual_address))) {
                // Error: Couldn't allocate enough additional memory for file
                return -1;
            }
            next_available_file_virtual_address += PAGE_SIZE;

            oft->pages_allocated++;  // File has another page allocated
        }

        // Set allocated memory to 0 to initialize it
        memset(oft->address + oft->inode->size_bytes, 0, bytes_to_allocate);

        // Check if new file size needs another data disk block added to file's extents
        const uint32_t current_blocks = bytes_to_blocks(oft->inode->size_bytes);
        const uint32_t new_blocks = bytes_to_blocks(oft->offset);

        if (new_blocks > current_blocks) {
            // TODO: Allocate more disk blocks to file's extents
        }
    }

    // Write data from input buffer to FD, at file offset
    memcpy32(oft->address + oft->offset, buf, len);

    // Set new file offset from data written
    oft->offset += len;

    bytes_written = len;    // Data written

    // Set new file size from data written
    if ((uint32_t)oft->offset > oft->inode->size_bytes) {
        oft->inode->size_bytes = oft->offset;
        oft->inode->size_sectors = bytes_to_sectors(oft->inode->size_bytes); 
    }

    // Update file's inode data
    update_inode_on_disk(*oft->inode);

    // Update file's data on disk
    if (!fs_save_file(oft->inode, (uint32_t)oft->address)) {
        // Error saving file
        return -1;
    }

    // Return number of bytes actually written to FD
    return bytes_written;
}

// Open system call: open a file
int32_t syscall_open(syscall_regs_t regs) {
    char *filepath = (char *)regs.ebx;
    int flags      = regs.ecx;

    int32_t fd = -1;

    // Grab inode for given file path
    inode_t file_inode = inode_from_path(filepath);

    // If file does not exist
    if (file_inode.id == 0) {

        if (!(flags & O_CREAT)) {
            // User did not say to create or otherwise does not have creation flag, error
            return -1;
        }

        // File doesn't exist, and flags does have O_CREAT,
        //   create file (incl. new inode, and update inode bitmap/data bitmap,
        //   write to inode blocks, data blocks for new data (dir data would be . and ..)
        file_inode = fs_create_file(filepath);

        if (file_inode.id == 0) {
            // Error, could not create file at path
            return -1;
        }
    }

    // Search for file inode in open inode table 
    inode_t *tmp_inode = open_inode_table;
    uint32_t inode_tbl_idx = 0;
    while (inode_tbl_idx < max_open_inodes &&
           tmp_inode->id != file_inode.id) {

        inode_tbl_idx++;
        tmp_inode++;
    }

    if (file_inode.id == tmp_inode->id) {
        // If already exists in inode table, increment ref_count in inode
        tmp_inode->ref_count++;
    } else {
        // File is not in inode table, add it
        // Find first open spot in inode table
        tmp_inode = open_inode_table;
        inode_tbl_idx = 0;
        while (inode_tbl_idx < max_open_inodes &&
               tmp_inode->id != 0) {

            inode_tbl_idx++;
            tmp_inode++;
        }

        if (inode_tbl_idx == max_open_inodes) {
            // Reached current end of open inode table,
            //   realloc() or expand table size and add a new entry for file
            void *malloc_ptr = malloc(sizeof(inode_t) * (max_open_inodes*2));
            memcpy(malloc_ptr, open_inode_table, sizeof(inode_t) * max_open_inodes);
            free(open_inode_table);
            open_inode_table = malloc_ptr;

            tmp_inode = open_inode_table + max_open_inodes; // Go to original max + 1
            *tmp_inode = file_inode;                        // Fill new inode table element

            inode_tbl_idx = max_open_inodes+1;
            max_open_inodes *= 2;

        } else {
            // Reached open position in inode table, add data to this position
            *tmp_inode = file_inode; 
        }
        current_open_inodes++;   
    }

    // Search for open spot in open file table
    open_file_table_t *tmp_ft_entry = open_file_table;
    uint32_t file_tbl_idx = 3;  // fd 0/1/2 are reserved for stdin/out/err
    tmp_ft_entry += 3;          // ""

    while (file_tbl_idx < max_open_files &&
           tmp_ft_entry->address   != 0  && 
           tmp_ft_entry->ref_count != 0) {

        file_tbl_idx++; 
        tmp_ft_entry++;
    }

    if (file_tbl_idx == max_open_files) {
        // Reached current end of open inode table,
        //   realloc() or expand table size and add a new entry for file
        void *malloc_ptr = malloc(sizeof(open_file_table_t) * (max_open_files*2));
        memcpy(malloc_ptr, open_file_table, sizeof(open_file_table_t) * max_open_files);
        free(open_file_table);
        open_file_table = malloc_ptr;

        tmp_ft_entry = open_file_table + max_open_files;    // Go to original max + 1
        
        file_tbl_idx = max_open_files+1;
        max_open_files *= 2;

    } 
    current_open_files++;   

    // Fill out file table entry data
    tmp_ft_entry->address   = 0;
    tmp_ft_entry->offset    = 0;
    tmp_ft_entry->inode     = tmp_inode;
    tmp_ft_entry->ref_count = 1;
    tmp_ft_entry->flags     = flags;

    // Return FD, which is index of open file table position/entry
    fd = file_tbl_idx;

    tmp_ft_entry->address = (uint8_t *)next_available_file_virtual_address;

    // Load file from disk to memory to fill out address of file table entry
    uint32_t size_in_pages = bytes_to_blocks(tmp_ft_entry->inode->size_bytes);
    if (size_in_pages == 0) size_in_pages = 1;  // Reserve 1 page by default for new/empty files

    // Allocate enough pages/blocks for file
    for (uint32_t i = 0; i < size_in_pages; i++) {
        pt_entry page = 0;
        uint32_t phys_addr = (uint32_t)allocate_page(&page);

        if (!map_page((void *)phys_addr, (void *)(next_available_file_virtual_address))) {
            fd = -1;
            break;
        }

        // Set page as read/write
        pt_entry *virt_page = get_page(next_available_file_virtual_address);
        SET_ATTRIBUTE(virt_page, PTE_READ_WRITE);

        next_available_file_virtual_address += PAGE_SIZE;

        tmp_ft_entry->pages_allocated++;    // Another page was mapped/allocated for file
    }

    if (flags & O_APPEND) {
        // File will be written to at end of file every time
        tmp_ft_entry->offset = tmp_ft_entry->inode->size_bytes;
    }

    // Load file to allocated addresses
    if (!fs_load_file(tmp_ft_entry->inode, (uint32_t)tmp_ft_entry->address)) {
      fd = -1; // Error, could not load file
    }

    return fd;
}

// Close system call: close an open file
int32_t syscall_close(syscall_regs_t regs) {
    int32_t fd = regs.ebx;

    // Error for invalid file descriptor
    if (fd < 0) {
        return -1;
    }

    // Get open file table entry corresponding to given file descriptor/fd
    open_file_table_t *oft = open_file_table + fd;

    // Error if file not found or is not open
    if (oft->inode == 0 || oft->ref_count == 0) {
        return -1;
    }

    // Found fd in table, decrement ref count for file in file table and inode table
    oft->ref_count--;
    oft->inode->ref_count--;

    // Clear open file table entry if no longer in use and free memory used
    //   for file
    // TODO: This assumes virtual addresses are contiguous for each file, that is not
    //  guaranteed to be the case. Update to take this into account to get the actual virtual
    //  addresses that were mapped/allocated for the file
    if (oft->ref_count == 0) {
        uint32_t size_in_pages = bytes_to_blocks(oft->inode->size_bytes);
        if (size_in_pages == 0) size_in_pages = 1;  // Files use 1 page by default 

        uint32_t file_address = (uint32_t)oft->address;

        while (size_in_pages > 0) {
            free_page(get_page(file_address));
            size_in_pages--;
            file_address += PAGE_SIZE;
        }

        // If inode ref count = 0, clear open inode table entry as file is no longer open/in use
        if (oft->inode->ref_count == 0) {
            memset(oft->inode, 0, sizeof(inode_t));
        }

        memset(oft, 0, sizeof(open_file_table_t));  // Clear file table entry
    }

    return 0;   // Success
}

// Read system call: read bytes from an open file to a buffer
int32_t syscall_read(syscall_regs_t regs) {
    int32_t fd   = regs.ebx;
    void *buf    = (void *)regs.ecx;
    uint32_t len = regs.edx;

    uint32_t bytes_read = 0;

    if (fd < 0) {
        // Invalid FD
        return -1;
    }

    // Get open file table entry for FD
    open_file_table_t *oft = open_file_table + fd;
    
    // Check if file is open and valid
    if (oft->address == 0 || oft->ref_count == 0) {
        // Error, FD is not loaded to memory/invalid or file is not open anymore
        return -1;
    }

    // Check FD's open flags
    if (oft->flags & O_WRONLY) {
        // Error, FD is only open for writing
        return -1;
    }

    // Only read up to file len in bytes, do not read past end of file
    if (oft->inode->size_bytes < oft->offset + len) {
        bytes_read = oft->inode->size_bytes - oft->offset;
    } else {
        // Can read up to full length in bytes from file
        bytes_read = len;
    }

    // Copy from file to input buffer
    memcpy(buf, oft->address + oft->offset, bytes_read);

    // Update file offset, adding bytes read from file
    oft->offset += bytes_read;

    // Return # of bytes actually read
    return bytes_read;
}

// Seek system call: update an open file's position 
int32_t syscall_seek(syscall_regs_t regs) {
    int32_t fd            = regs.ebx;
    int32_t offset        = regs.ecx;
    whence_value_t whence = regs.edx;

    // Error for invalid file descriptor
    if (fd < 0) {
        return -1;
    }

    // Get open file table entry corresponding to given file descriptor/fd
    open_file_table_t *oft = open_file_table + fd;

    // Error if file not found or is not open
    if (oft->inode == 0 || oft->ref_count == 0) {
        return -1;
    }

    switch(whence) {
        // Set file offset to function arg offset
        case SEEK_SET:
            if (offset < 0) {
                oft->offset = 0; // Prevent further errors, just go to the start
                // ERROR: Can not seek before start of file
                return -1;
            }
            oft->offset = offset;
            break;

        // Set file offset += function arg offset
        case SEEK_CUR:
            oft->offset += offset;
            if (oft->offset < 0) oft->offset = 0;   // Don't go before start of file
            break;

        // Set file offset to end of file, then add function arg offset
        case SEEK_END:
            oft->offset = oft->inode->size_bytes + offset;
            if (oft->offset < 0) oft->offset = 0;   // Don't go before start of file
            break;

        default: 
            // ERROR: Did not pass valid whence value
            return -1;
            break;
    }

    // Return file offset
    return oft->offset;
}

// Syscall table
int32_t (*syscalls[MAX_SYSCALLS])(syscall_regs_t) = {
    [SYSCALL_TEST1]  = syscall_test1,
    [SYSCALL_TEST0]  = syscall_test0,
    [SYSCALL_SLEEP]  = syscall_sleep,
    [SYSCALL_MALLOC] = syscall_malloc,
    [SYSCALL_FREE]   = syscall_free,
    [SYSCALL_OPEN]   = syscall_open,
    [SYSCALL_CLOSE]  = syscall_close,
    [SYSCALL_READ]   = syscall_read,
    [SYSCALL_WRITE]  = syscall_write,
    [SYSCALL_SEEK]   = syscall_seek,
};

// Syscall dispatcher
// naked attribute means no function prologue/epilogue, and only allows inline asm
__attribute__ ((naked)) void syscall_dispatcher(void) {
    // "basic" syscall handler, push everything we want to save, call the syscall by
    //   offsetting into syscalls table with value in eax, then pop everything back 
    //   and return using "iret" (d/q), NOT regualar "ret" as this is technically
    //   an interrupt (software interrupt)
    //
    // Already on stack: SS, SP, FLAGS, CS, IP
    // Need to push: AX, GS, FS, ES, DS, BP, DI, SI, DX, CX, BX
    //
    // NOTE: Easier to do call in intel syntax, I'm not sure how to do it in att syntax
    __asm__ __volatile__ (".intel_syntax noprefix\n"

                          ".equ MAX_SYSCALLS, 10\n"     // Have to define again, inline asm does not see the #define

                          "cmp eax, MAX_SYSCALLS-1\n"   // syscalls table is 0-based
                          "ja invalid_syscall\n"        // invalid syscall number, skip and return

                          "push eax\n"
                          "push gs\n"
                          "push fs\n"
                          "push es\n"
                          "push ds\n"
                          "push ebp\n"
                          "push edi\n"
                          "push esi\n"
                          "push edx\n"
                          "push ecx\n"
                          "push ebx\n"
                          "push esp\n"
                          "call [syscalls+eax*4]\n"
                          "add esp, 4\n"    // Do not overwrite esp
                          "pop ebx\n"
                          "pop ecx\n"

                          // Do not overwrite EDX, as some syscalls e.g. malloc() will need it
                          "add esp, 4\n"    

                          "pop esi\n"
                          "pop edi\n"
                          "pop ebp\n"
                          "pop ds\n"
                          "pop es\n"
                          "pop fs\n"
                          "pop gs\n"
                          "add esp, 4\n"    // Save eax value in case; don't overwrite it
                          "iretd\n"         // Need interrupt return here! iret, NOT ret

                          "invalid_syscall:\n"
                          "mov eax, -1\n"   // Error will be -1
                          "iretd\n"

                          ".att_syntax");
}
