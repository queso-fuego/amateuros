/*
 * stdlib.h: C standard libary functions?
 */
#pragma once

#include "C/stdint.h"
#include "C/string.h"
#include "sys/syscall_numbers.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// Convert ascii string to integer
uint32_t atoi(const uint8_t *string) {
    uint32_t result = 0;

    // TODO: Handle hex number string e.g. 1st 2 characters will be "0x" or "0X"
    
    for (uint32_t i = 0; string[i] != '\0'; i++)
        result = result * 10 + string[i] - '0';

    return result;
}

// Allocate uninitialized memory, uses syscall
void *malloc(const uint32_t size) {
    void *ptr = 0;

    __asm__ __volatile__ ("int $0x80" : "=d"(ptr) : "a"(SYSCALL_MALLOC), "b"(size) );

    return ptr;
}

// Allocate memory and initialize to 0, uses syscall
void *calloc(const uint32_t num, const uint32_t size) {
    void *ptr = 0;

    __asm__ __volatile__ ("int $0x80" : "=d"(ptr) : "a"(SYSCALL_MALLOC), "b"(num*size) );

    memset(ptr, 0, num*size);
    return ptr;
}

// Free allocated memory at a pointer, uses syscall
void free(const void *ptr) {
    __asm__ __volatile__ ("int $0x80" : : "a"(SYSCALL_FREE), "b"(ptr) );
}

// Terminate running process
void exit(const int32_t status) {
    __asm__ __volatile__ ("int $0x80" : : "a"(SYSCALL_EXIT), "b"(status) );
}

