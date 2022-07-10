/*
 * syscalls.h: Syscall handler/dispatcher and system call functions
 */
#pragma once

#include "C/stdint.h"
#include "C/string.h"
#include "sys/syscall_numbers.h"
#include "print/print_types.h"
#include "interrupts/pic.h"
#include "memory/malloc.h"
#include "memory/kmalloc.h"
#include "terminal/terminal.h"
#include "fs/fs_impl.h"

extern open_file_table_entry_t *open_file_table;    // in kernel.c
extern uint32_t open_file_table_max_size;           // in kernel.c
extern uint32_t open_file_table_curr_size;          // in kernel.c

extern inode_t *open_inode_table;                   // In kernel.c
extern uint32_t open_inode_table_max_size;          // In kernel.c
extern uint32_t open_inode_table_curr_size;         // In kernel.c

// Test syscall 0
void syscall_test0(void)
{
    uint16_t x = 0, y = 20;
    uint32_t color = user_gfx_info->fg_color;   // Save current text color

    user_gfx_info->fg_color = convert_color(0x00FFFF00);    // YELLOW
    print_string(&x, &y, "Test system call 0");

    user_gfx_info->fg_color = color;    // Restore text color
}

// Test syscall 1
void syscall_test1(void)
{
    uint16_t x = 0, y = 21;
    uint32_t color = user_gfx_info->fg_color;   // Save current text color

    user_gfx_info->fg_color = convert_color(0x0000FF00);    // GREEN
    print_string(&x, &y, "Test system call 1");

    user_gfx_info->fg_color = color;    // Restore text color
}

// Sleep for a given number of milliseconds
// INPUTS:
//  EBX = # of milliseconds
void syscall_sleep(void)
{
    __asm__ __volatile__ ("mov %%ebx, %0" : "=r"(*sleep_timer_ticks) );

    // Wait ("Sleep") until # of ticks is 0
    while (*sleep_timer_ticks > 0) __asm__ __volatile__ ("sti;hlt;cli");
}

// Allocate uninitialized memory
// INPUT:
//   EBX = size in bytes to allocate
void syscall_malloc(void)
{
    uint32_t bytes = 0;

    __asm__ __volatile__ ("mov %%EBX, %0" : "=b"(bytes) );

    // First malloc() from the calling program?
    if (!malloc_list_head)
        malloc_init(bytes); // Yes, set up initial memory/linked list

    void *ptr = malloc_next_block(bytes);

    merge_free_blocks();    // Combine consecutive free blocks of memory

    // Return pointer to malloc-ed memory
    __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(ptr) );
}

// Free allocated memory at a pointer
// INPUT:
//   EBX = pointer to malloc-ed bytes
void syscall_free(void)
{
    void *ptr = 0;

    __asm__ __volatile__ ("mov %%EBX, %0" : "=b"(ptr) );

    malloc_free(ptr);
}

// Allocate uninitialized memory, for kernel use
// INPUT:
//   EBX = size in bytes to allocate
void syscall_kmalloc(void)
{
    uint32_t bytes = 0;

    __asm__ __volatile__ ("mov %%EBX, %0" : "=b"(bytes) );

    // First malloc() from the calling program?
    if (!kmalloc_list_head)
        kmalloc_init(bytes); // Yes, set up initial memory/linked list

    void *ptr = kmalloc_next_block(bytes);

    kmerge_free_blocks();    // Combine consecutive free blocks of memory

    // Return pointer to malloc-ed memory
    __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(ptr) );
}

// Free allocated memory at a pointer, for kernel use
// INPUT:
//   EBX = pointer to malloc-ed bytes
void syscall_kfree(void)
{
    void *ptr = 0;

    __asm__ __volatile__ ("mov %%EBX, %0" : "=b"(ptr) );

    kmalloc_free(ptr);
}

// Open system call: Open a file
// INPUT:
//   EBX: file path
// OUTPUT:
//   EAX: File descriptor or 0 if error
void syscall_open(void)
{
    int fd = 0;
    char *path = 0;
    int flags = 0;

    __asm__ __volatile__ ("mov %%EBX, %0;"
                          "mov %%ECX, %1;"
                          : "=b"(path), "=c"(flags));
                          
    inode_t *file_inode = get_inode_from_path(path, current_dir_inode);
    open_file_table_entry_t *ft_ptr = open_file_table;

    if (!file_inode) {
        // New file, need create flag set
        if (!(flags & O_CREAT)){
            __asm__ __volatile__ ("mov $0, %EAX");
            return;
        }

        // Create new file at input path
        fd = fs_create_file(path, FILETYPE_FILE);

        // Add to open file table
        open_file_table_curr_size++;
        if (open_file_table_curr_size == open_file_table_max_size) {
          // TODO: Use realloc() to expand table?
        }

        // Find next open spot in table
        for (uint32_t i = 0; i < open_file_table_curr_size && ft_ptr->id != 0; i++, ft_ptr++) 
            ;

        // Add to table
        ft_ptr->id = fd;
        ft_ptr->status_flags = flags;
        ft_ptr->ref_count = 1;
        ft_ptr->offset = 0;

        // Get inode for file
        rw_sectors(1, 
                   (superblock->first_inode_block * 8) + (fd / 8),
                   (uint32_t)tmp_sector,
                   READ_WITH_RETRY);

        inode_t *inode = (inode_t *)tmp_sector + (fd % 8);

        // Add to open inode table
        open_inode_table_curr_size++;
        if (open_inode_table_curr_size == open_inode_table_max_size) {
          // TODO: Use realloc() to expand table?
        }

        // Find next open spot in table
        inode_t *temp = open_inode_table;
        for (uint32_t i = 0; i < open_inode_table_curr_size && temp->id != 0; i++, temp++) 
            ;

        // Add to table
        *temp = *inode;

        // Load file
        if (inode->size_bytes < PAGE_SIZE) 
            ft_ptr->address = malloc(PAGE_SIZE);    // Start with one page of memory
        else
            ft_ptr->address = malloc(inode->size_bytes); 

        fs_load_file(inode, (uint32_t)ft_ptr->address);

    } else {
        // Return fd for file at input path
        fd = file_inode->id;

        // Add to open file table
        open_file_table_curr_size++;
        if (open_file_table_curr_size == open_file_table_max_size) {
          // TODO: Use realloc() to expand table?
        }

        // Get next open spot in table
        ft_ptr = open_file_table;
        for (uint32_t i = 0; i < open_file_table_curr_size && ft_ptr->id != 0; i++, ft_ptr++) 
            ;

        // Add table entry 
        ft_ptr->id = fd;
        ft_ptr->status_flags = flags;
        ft_ptr->ref_count = 1;
        ft_ptr->offset = 0;

        // Get inode for file
        rw_sectors(1, 
                   (superblock->first_inode_block * 8) + (fd / 8),
                   (uint32_t)tmp_sector,
                   READ_WITH_RETRY);

        inode_t *inode = (inode_t *)tmp_sector + (fd % 8);

        // Check if inode is in open inode table
        inode_t *temp = open_inode_table;
        for (uint32_t i = 0; i < open_inode_table_curr_size && temp->id != inode->id; i++, temp++) 
            ;

        if (temp->id != inode->id) {
            // Add to open inode table
            open_inode_table_curr_size++;
            if (open_inode_table_curr_size == open_inode_table_max_size) {
              // TODO: Use realloc() to expand table?
            }

            temp = open_inode_table;
            for (uint32_t i = 0; i < open_inode_table_curr_size && temp->id != 0; i++, temp++) 
                ;

            // Add table entry
            *temp = *inode;
        }

        // Load file
        if (inode->size_bytes < PAGE_SIZE) 
            ft_ptr->address = malloc(PAGE_SIZE);    // Start with one page of memory
        else
            ft_ptr->address = malloc(inode->size_bytes); 

        fs_load_file(inode, (uint32_t)ft_ptr->address);
    }

    // Return file descriptor
    __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(fd) );
}

// Close system call: Close a file
// INPUT:
//   EBX: file descriptor
// OUTPUT: 
//   EAX: -1 on error
void syscall_close(void) {
    int fd = 0;

    __asm__ __volatile__ ("mov %%EBX, %0;"
                          : "=b"(fd));

    open_file_table_entry_t *ft_ptr = open_file_table;

    // Check if file is in open file table
    for (uint32_t i = 0; i < open_file_table_curr_size && ft_ptr->id != fd; i++, ft_ptr++) 
        ;

    if (ft_ptr->id != fd) {
        // Did not find file in open file table
        __asm__ __volatile__ ("mov $-1, %EAX");
        return;

    } else {
        // Decrement reference count
        if (--ft_ptr->ref_count == 0) {
            // Remove from file table and free memory
            free(ft_ptr->address);
            memset(ft_ptr, 0, sizeof(open_file_table_entry_t));
            open_file_table_curr_size--;

            // Remove from inode table 
            inode_t *temp = open_inode_table;
            for (uint32_t i = 0; i < open_inode_table_curr_size && temp->id != fd; i++, temp++) 
                ;

            memset(temp, 0, sizeof(inode_t));
            open_inode_table_curr_size--;
        }
    }

    // Return 0: Success
    __asm__ __volatile__ ("mov $0, %EAX");
}

// Read system call: Read bytes from a file to a buffer
// INPUT:
//   EBX = file descriptor
//   ECX = buffer
//   EDX = length in bytes
// OUTPUT:
//   EAX = # of bytes read
void syscall_read(void)
{
    int fd = 0;
    void *buf = 0;
    uint32_t len = 0;

    __asm__ __volatile__ ("mov %%EBX, %0;"
                          "mov %%ECX, %1;"
                          "mov %%EDX, %2;"
                          : "=b"(fd), "=c"(buf), "=d"(len) );

    // Check if file in open file table
    open_file_table_entry_t *ft_ptr = open_file_table;
    for (uint32_t i = 0; i < open_file_table_curr_size && ft_ptr->id != fd; i++, ft_ptr++) 
        ;

    if (ft_ptr->id != fd) {
        __asm__ __volatile__ ("mov $0, %EAX");
        return;  // File not found, need to open() it first
    }

    if (ft_ptr->status_flags & O_WRONLY) {
        __asm__ __volatile__ ("mov $0, %EAX");
        return;   // File is write only, can't read 
    }

    // Get file inode from open inode table
    inode_t *inode = open_inode_table;
    for (uint32_t i = 0; i < open_inode_table_curr_size && inode->id != ft_ptr->id; i++, inode++) 
        ;

    // Read bytes from file at file address into buffer 
    if (ft_ptr->offset + len <= inode->size_bytes) {
        // All bytes can be read
        memcpy(buf, ft_ptr->address + ft_ptr->offset, len);
        ft_ptr->offset += len;

        __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(len) );
        return;

    } else {
        // Only some bytes can be read up until EOF
        memcpy(buf, 
               ft_ptr->address + ft_ptr->offset, 
               inode->size_bytes - ft_ptr->offset);

        ft_ptr->offset += (inode->size_bytes - ft_ptr->offset);

        __asm__ __volatile__ ("mov %0, %%EAX" 
                              : 
                              : "r"(inode->size_bytes - ft_ptr->offset) );
        return;
    }
}

// Write system call: Write bytes from a buffer to a file descriptor
// INPUT:
//   EBX = file descriptor
//   ECX = buffer
//   EDX = length in bytes
// OUTPUT:
//   EAX = number of bytes written
void syscall_write(void)
{
    int fd = 0;
    void *buf = 0;
    uint32_t len = 0;

    __asm__ __volatile__ ("mov %%EBX, %0;"
                          "mov %%ECX, %1;"
                          "mov %%EDX, %2;"
                          : "=b"(fd), "=c"(buf), "=d"(len) );

    if (fd == 1) {
        __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(terminal_write(buf, len)) );
        return;
    } 

    // Check if file in open file table
    open_file_table_entry_t *ft_ptr = open_file_table;
    for (uint32_t i = 0; i < open_file_table_curr_size && ft_ptr->id != fd; i++, ft_ptr++) 
        ;

    if (ft_ptr->id != fd) {
        __asm__ __volatile__ ("mov $0, %EAX");
        return;  // File not found, need to open() it first
    }

    if (ft_ptr->status_flags & O_RDONLY) {
        __asm__ __volatile__ ("mov $0, %EAX");
        return;  // File is read only, can't write 
    }

    // Get file inode from open inode table
    inode_t *inode = open_inode_table;
    for (uint32_t i = 0; i < open_inode_table_curr_size && inode->id != ft_ptr->id; i++, inode++) 
        ;

    if (ft_ptr->status_flags & O_APPEND) {
        // Write at end of file, appending new data. Move file
        //   position/offset to end of file before writing
        ft_ptr->offset = (uint32_t)ft_ptr->address + inode->size_bytes;
    }

    // Write bytes to file at file address from buffer 
    uint32_t file_blocks = bytes_to_blocks(inode->size_bytes);
    if (ft_ptr->offset + len > file_blocks * FS_BLOCK_SIZE) {
        // Allocate more memory to write file to
        // TODO: Use realloc() to move ft_ptr->address to larger size
        // A simple, not good realloc() could be:
        // 1) Malloc() with bytes large enough for new size,
        //   current size + 1 page? Or enough pages to cover new
        //   size?
        // 2) Copy current address buffer for file to new larger
        //   buffer.
        // 3) Free() old, smaller buffer
        //
        // Better realloc() could be:
        // 1) Get the difference in size between current buffer
        //   and new larger size
        // 2) Allocate up to next page size of memory for this 
        //   difference, and put it as the next or current malloc
        //   block for input pointer address.

        // TODO: Add extra block to inode, and mark extra data
        //   bits in use, as needed
    } 

    // Write bytes from buffer to file address
    memcpy(ft_ptr->address + ft_ptr->offset, 
           buf,
           len);

    if (ft_ptr->offset + len > inode->size_bytes) {
        // File size increased, add difference of 
        //   ((offset+len) - size_bytes) to inode->size_bytes
        inode->size_bytes += 
            ((ft_ptr->offset + len) - inode->size_bytes);

        inode->size_sectors = bytes_to_sectors(inode->size_bytes);

        // Update file's inode on disk
        rw_sectors(1, 
                   (superblock->first_inode_block * 8) + 
                   (inode->id / 8),
                   (uint32_t)tmp_sector,
                   READ_WITH_RETRY);

        inode_t *temp_inode = (inode_t *)tmp_sector + (inode->id % 8);
        *temp_inode = *inode;

        rw_sectors(1, 
                   (superblock->first_inode_block * 8) + 
                   (inode->id / 8),
                   (uint32_t)tmp_sector,
                   WRITE_WITH_RETRY);
    }

    ft_ptr->offset += len;  // Update file offset

    // Write updated file data to disk
    // TODO: Do this later unless file has O_SYNC flag set?
    fs_save_file(inode, (uint32_t)ft_ptr->address);
    
    // Return Length bytes written to file
    __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(len) );
}

// Change file position/offset
// INPUT:
//   EBX: file descriptor
//   ECX: offset 
//   EDX: whence
// OUTPUT:
//   EAX: file position
void syscall_seek(void) {
    int fd = 0;
    uint32_t offset = 0;
    int whence = 0;

    __asm__ __volatile__ ("mov %%EBX, %0;"
                          "mov %%ECX, %1;"
                          "mov %%EDX, %2;"
                          : "=b"(fd), "=c"(offset), "=d"(whence) );

    // Check if file in open file table
    open_file_table_entry_t *ft_ptr = open_file_table;
    for (uint32_t i = 0; i < open_file_table_curr_size && ft_ptr->id != fd; i++, ft_ptr++) 
        ;

    if (ft_ptr->id != fd) {
        __asm__ __volatile__ ("mov $-1, %EAX");
        return;  // File not found
    }

    // Get file inode from open inode table
    inode_t *inode = open_inode_table;
    for (uint32_t i = 0; i < open_inode_table_curr_size && inode->id != ft_ptr->id; i++, inode++) 
        ;

    switch(whence) {
        case SEEK_SET:
            // Offset from start of file
            ft_ptr->offset = 0 + offset;    
            break;
        case SEEK_CUR:
            // Offset from current position 
            ft_ptr->offset += offset;    
            break;
        case SEEK_END:
            // Offset from end of file
            ft_ptr->offset = inode->size_bytes + offset;
            break;
        default:
            __asm__ __volatile__ ("mov $-1, %EAX");
            return;  // Invalid whence value
            break;
    }

    // Success
    __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(ft_ptr->offset) );
}

// Syscall table
void (*syscalls[MAX_SYSCALLS])(void) = {
    [SYSCALL_TEST1]   = syscall_test1,
    [SYSCALL_TEST0]   = syscall_test0,
    [SYSCALL_SLEEP]   = syscall_sleep,
    [SYSCALL_MALLOC]  = syscall_malloc,
    [SYSCALL_FREE]    = syscall_free,
    [SYSCALL_KMALLOC] = syscall_kmalloc,
    [SYSCALL_KFREE]   = syscall_kfree,
    [SYSCALL_OPEN]    = syscall_open,
    [SYSCALL_CLOSE]   = syscall_close,
    [SYSCALL_READ]    = syscall_read,
    [SYSCALL_WRITE]   = syscall_write,
    [SYSCALL_SEEK]    = syscall_seek,
};

// Syscall dispatcher
// naked attribute means no function prologue/epilogue, and only allows inline asm
__attribute__ ((naked, interrupt)) void syscall_dispatcher(int_frame_32_t *frame)
{
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

                          ".equ MAX_SYSCALLS, 12\n"  // Have to define again, inline asm does not see the #define

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
                          "add esp, 4\n"
                          "pop ebx\n"
                          "pop ecx\n"
                          "pop edx\n"
                          "pop esi\n"
                          "pop edi\n"
                          "pop ebp\n"
                          "pop ds\n"
                          "pop es\n"
                          "pop fs\n"
                          "pop gs\n"
                          "add esp, 4\n"    // Save eax value in case
                          "iretd\n"         // Need interrupt return here! iret, NOT ret

                          "invalid_syscall:\n"
                          "mov eax, -1\n"   // Error will be -1
                          "iretd\n"

                          ".att_syntax");
}
