/*
 * sys/syscall_numbers.h: System call numbers
 */
#pragma once

#define MAX_SYSCALLS 12

#define SYSCALL_INT 0x80    // 'int 0x80' is the syscall instruction

typedef enum {
    SYSCALL_TEST0   = 0,
    SYSCALL_TEST1   = 1,
    SYSCALL_SLEEP   = 2,
    SYSCALL_MALLOC  = 3,
    SYSCALL_FREE    = 4,
    SYSCALL_KMALLOC = 5,    // Kernel malloc
    SYSCALL_KFREE   = 6,    // Kernel free
    SYSCALL_OPEN    = 7,
    SYSCALL_CLOSE   = 8,
    SYSCALL_READ    = 9,
    SYSCALL_WRITE   = 10,
    SYSCALL_SEEK    = 11, 
} system_call_numbers;
