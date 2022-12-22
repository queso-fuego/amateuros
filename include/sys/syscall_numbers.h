/*
 * sys/syscall_numbers.h: System call numbers
 */
#pragma once

#define MAX_SYSCALLS 10 

typedef enum {
    SYSCALL_TEST0  = 0,
    SYSCALL_TEST1  = 1,
    SYSCALL_SLEEP  = 2,
    SYSCALL_MALLOC = 3,
    SYSCALL_FREE   = 4,
    SYSCALL_WRITE  = 5,
    SYSCALL_OPEN   = 6,
    SYSCALL_CLOSE  = 7,
    SYSCALL_READ   = 8,
    SYSCALL_SEEK   = 9,
} system_call_numbers;

typedef enum {
    O_RDONLY = 0x0,
    O_WRONLY = 0x1,     // Write only     - 1 of 3 required
    O_RDWR   = 0x2,     // Read and write - 1 of 3 required
    O_CREAT  = 0x4,     // Create if not exist
    O_APPEND = 0x8,     // Always write at end of file position
    O_TRUNC  = 0x10,    // Truncate file size and pos to 0 on open
} open_flags;
