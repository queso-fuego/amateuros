/*
 * sys/syscall_wrappers.h: C function wrappers for system calls,
 *   this is a catch-all for misc system calls not in other files
 */
#pragma once

#include "C/stdint.h"
#include "sys/syscall_numbers.h"

// Open()
// RETURNS:
//   fd of 3+, or -1 on error
int32_t open(char *filepath, open_flag_t flags) {
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" 
                          : "=a"(result) 
                          : "a"(SYSCALL_OPEN), "b"(filepath), "c"(flags) 
                          : "memory");
    return result;
}

// Close an open file
int32_t close(const int32_t fd) {
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" 
                          : "=a"(result) 
                          : "a"(SYSCALL_CLOSE), "b"(fd) 
                          : "memory");
    return result;
}

// Read()
int32_t read(const int32_t fd, const void *buf, const uint32_t len)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" 
                          : "=a"(result) 
                          : "a"(SYSCALL_READ), "b"(fd), "c"(buf), "d"(len) 
                          : "memory");
    return result;
}

// Write()
int32_t write(const int32_t fd, const void *buf, const uint32_t len)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" 
                          : "=a"(result) 
                          : "a"(SYSCALL_WRITE), "b"(fd), "c"(buf), "d"(len) 
                          : "memory");
    return result;
}

// Seek a file, updating it's offset in the open file table
int32_t seek(const int32_t fd, const int32_t offset, const whence_value_t whence) {
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" 
                          : "=a"(result) 
                          : "a"(SYSCALL_SEEK), "b"(fd), "c"(offset), "d"(whence) 
                          : "memory");
    return result;
}




















