/*
 * sys/syscall_wrappers.h: C function wrappers for system calls,
 *   this is a catch-all for misc system calls not in other files
 */
#pragma once

#include "C/stdint.h"
#include "sys/syscall_numbers.h"

int32_t write(int32_t fd, const void *buf, const uint32_t len)
{
    int32_t result = -1;

    __asm__ __volatile__ ("int $0x80" : "=a"(result) : "a"(SYSCALL_WRITE), "b"(fd), "c"(buf), "d"(len) );

    return result;
}
