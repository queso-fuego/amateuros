/*
 * sys/syscall_numbers.h: System call numbers
 */
#pragma once

#define MAX_SYSCALLS 6

typedef enum {
    SYSCALL_TEST0  = 0,
    SYSCALL_TEST1  = 1,
    SYSCALL_SLEEP  = 2,
    SYSCALL_MALLOC = 3,
    SYSCALL_FREE   = 4,
    SYSCALL_WRITE  = 5,
} system_call_numbers;
