/*
 * kstdlib.h: C standard libary functions, for kernel use
 */
#pragma once

#include "C/stdint.h"
#include "sys/syscall_numbers.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// Allocate uninitialized memory, uses syscall
void *kmalloc(const uint32_t size)
{
    void *ptr = 0;

    // Need to use =a for return ptr to fix bugs, as EAX has return value
    __asm__ __volatile__ ("int $0x80" : "=a"(ptr) : "a"(SYSCALL_KMALLOC), "b"(size) );

    return ptr;
}

// Free allocated memory at a pointer, uses syscall
void kfree(const void *ptr)
{
    __asm__ __volatile__ ("int $0x80" : : "a"(SYSCALL_KFREE), "b"(ptr) );
}

