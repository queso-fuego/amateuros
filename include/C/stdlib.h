/*
 * stdlib.h: C standard libary functions?
 */
#pragma once

#include "C/stdint.h"

// Convert ascii string to integer
uint32_t atoi(const uint8_t *string)
{
    uint32_t result = 0;

    // TODO: Handle hex number string e.g. 1st 2 characters will be "0x" or "0X"
    
    for (uint32_t i = 0; string[i] != '\0'; i++)
        result = result * 10 + string[i] - '0';

    return result;
}

// Allocate uninitialized memory, uses syscall
void *malloc(const uint32_t size)
{
    void *ptr = 0;

    // TODO: Don't hardcode syscall numbers!
    __asm__ __volatile__ ("int $0x80" : : "a"(3), "b"(size) );

    __asm__ __volatile__ ("movl %%EAX, %0" : "=r"(ptr) ); 

    return ptr;
}

// Free allocated memory at a pointer, uses syscall
void free(const void *ptr)
{
    // TODO: Don't hardcode syscall numbers!
    __asm__ __volatile__ ("int $0x80" : : "a"(4), "b"(ptr) );
}
