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
    O_RDONLY = 0x0,     // Read only      - 1 of 3 required
    O_WRONLY = 0x1,     // Write only     - 1 of 3 required
    O_RDWR   = 0x2,     // Read and write - 1 of 3 required
    O_CREAT  = 0x4,     // Create if not exist
    O_APPEND = 0x8,     // Always write at end of file position
    O_TRUNC  = 0x10,    // Truncate file size and pos to 0 on open
} open_flags;

// Seek whence values: used in seek() as seek(<fd>, <offset>, <whence>)
// 
// SEEK_SET sets the current file offset to passed in offset
//   e.g. if open file table entry offset = 10000 and passed in offset = 200, SEEK_SET
//   will set file table entry offset = 200
//
// SEEK_CUR adds the offset value to the file table offset, and can be negative,
//   which will move the file position backwards.
//   e.g. seek(fd, 100, SEEK_CUR) will set file table offset += 100
//        seek(fd, -256, SEEK_CUR) will set file table offset -= 256, or until start of file
//
// SEEK_END sets the current file offset value to the end of the file (the current file size
//   in bytes) and then adds the given offset value to it.
//   e.g. seek(fd, 100, SEEK_END) will set file table offset = file_size_in_bytes + 100;

typedef enum {
    SEEK_SET, 
    SEEK_CUR,
    SEEK_END,
} whence_values;









