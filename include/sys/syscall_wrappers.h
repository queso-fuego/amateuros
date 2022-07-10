/*
 * sys/syscall_wrappers.h: C function wrappers for system calls,
 *   this is a catch-all for misc system calls not in other files
 */
#pragma once

#include "C/stdint.h"
#include "sys/syscall_numbers.h"

enum {
    O_CREAT  = 0x1,
    O_RDONLY = 0x2,
    O_WRONLY = 0x4,
    O_RDWR   = 0x8,
    O_APPEND = 0x10,
    O_TRUNC  = 0x20,
} open_flags;

// Open a file
int32_t open(const char *path, const uint8_t flags)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" : "=a"(result) : "a"(SYSCALL_OPEN), "b"(path), "c"(flags));

    return result;
}

// Close a file
int32_t close(const int fd)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" : "=a"(result) : "a"(SYSCALL_CLOSE), "b"(fd));

    return result;
}

// Read at most len bytes from file fd into buf
int32_t read(int32_t fd, const void *buf, const uint32_t len)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" : "=a"(result) : "a"(SYSCALL_READ), "b"(fd), "c"(buf), "d"(len) );

    return result;
}

// Write at most len bytes to file fd from buf
int32_t write(int32_t fd, const void *buf, const uint32_t len)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" : "=a"(result) : "a"(SYSCALL_WRITE), "b"(fd), "c"(buf), "d"(len) );

    return result;
}

// Change file position
int32_t seek(int fd, uint32_t offset, int whence) {
    int result = -1;

    __asm__ __volatile__ ("int $0x80" : "=a"(result) : "a"(SYSCALL_SEEK), "b"(fd), "c"(offset), "d"(whence) );

    return result;
}
