/*
 * C/stdio.h: Standard input/output functions
 */
#pragma once

#include "C/stdbool.h"
#include "C/stdlib.h"
#include "C/string.h"
#include "sys/syscall_wrappers.h"

#define stdin  0
#define stdout 1
#define stderr 2

enum {
    STDIN_FILENO  = 0,
    STDOUT_FILENO = 1,
    STDERR_FILENO = 2,
};

enum {
    SEEK_SET = 0,
    SEEK_CUR = 1,
    SEEK_END = 2,
};

typedef struct {
    uint32_t fd;    // File descriptor
    uint32_t pos;   // File position
    int flags;      // File access flags
} FILE;

uint8_t *write_buffer = 0;
uint32_t len = 0;

// Print single character
void putc(char c)
{
    write(stdout, &c, 1);
}

// Print a string
void puts(char *s)
{
    while (*s) {
        putc(*s);
        s++;
    }
}

// Print a hex integer
void printf_hex(const uint32_t num)
{
    const uint8_t digits[] = "0123456789ABCDEF";
    uint8_t buf[16] = {0};
    int32_t i = 0;
    uint32_t n = num;
    bool pad = (n < 0x10);

    do {
        buf[i++] = digits[n % 16];
    } while ((n /= 16) > 0);

    if (pad) write_buffer[len++] = '0';  // Add padding 0

    while (--i >= 0)  
        write_buffer[len++] = buf[i];
}

// Print a decimal integer
void printf_int(const int32_t num, const uint8_t base, const bool sign)
{
    const uint8_t digits[] = "0123456789ABCDEF";
    bool negative = false;
    uint8_t buf[16] = {0};
    int32_t i = 0;
    int32_t n = num;

    if (sign == true && num < 0) {
        negative = true;
        n = -num;
    }

    do {
        buf[i++] = digits[n % base];
    } while ((n /= base) > 0);

    if (negative) buf[i++] = '-';

    while (--i >= 0)  
        write_buffer[len++] = buf[i];
}

// Print formatted string
void printf(const char *fmt, ...)
{
    uint32_t *arg_ptr = (uint32_t *)&fmt;
    int state = 0;  // Inside a format string or not?
    uint32_t max = 256; // Max string length
    char *s = 0; 

    arg_ptr++;  // Move to first arg after format string on the stack

    len = 0;
    write_buffer = malloc(max);

    for (uint32_t i = 0; fmt[i] != '\0'; i++) {
        while (len > max) {
            // Allocate and move to a larger buffer
            max *= 2;

            uint8_t *temp_buffer = malloc(max);

            strcpy(temp_buffer, write_buffer);

            free(write_buffer); // Free smaller buffer as it's no longer needed

            write_buffer = temp_buffer;
        }

        char c = fmt[i];
        if (state == 0) {
            if (c == '%') state = '%'; // Found a format string
            else write_buffer[len++] = c;

        } else if (state == '%') {
            switch(c) {
            case 'd':
                // Decimal integer e.g. "123"
                printf_int(*(int *)arg_ptr, 10, true);
                //arg_ptr += sizeof(int *);
                arg_ptr++;
                break;
            case 'x':
                // Hex integer e.g. "0xFFFF"
                write_buffer[len++] = '0';  // Add "0x" prefix
                write_buffer[len++] = 'x';

                printf_hex(*(unsigned int *)arg_ptr);
                //arg_ptr += sizeof(unsigned int *); 
                arg_ptr++;
                break;
            case 's': 
                // String
                s = *(char **)arg_ptr; 
                //arg_ptr += sizeof(char **);
                arg_ptr++;

                if (*s == '\0') s = "(null)";

                while (*s) 
                    write_buffer[len++] = *s++;
                break;
            case 'c':
                // Single Character
                write_buffer[len++] = *(char *)arg_ptr;
                //arg_ptr += sizeof(char *);  
                arg_ptr++;
                break;
            case '%':
                // Character literal '%'
                write_buffer[len++] = c;
                break;
            default:
                // Unsupported format, print so user can see
                write_buffer[len++] = '%';
                write_buffer[len++] = c;
                break;
            }

            state = 0;
        }
    }

    write_buffer[len] = '\0';       // Null terminate string
    write(stdout, write_buffer, len);    // Call write system call
    free(write_buffer);
}

// Open a file
FILE *fopen(char *path, char *mode) {
    FILE *fp = malloc(sizeof(FILE));
    if (!fp) return 0;

    // Set file access flags
    for (int i = 0; mode[i] != '\0'; i++) {
        switch (mode[i]) {
            case 'r':
              if (fp->flags & O_WRONLY)
                  fp->flags = O_RDWR;
              else
                  fp->flags = O_RDONLY;
              break;
            case 'w':
              if (fp->flags & O_RDONLY)
                  fp->flags = O_RDWR;
              else
                  fp->flags = O_WRONLY;
              break;
            case 'a':
              fp->flags |= O_APPEND;
              break;
            case '+':
              fp->flags = O_RDWR;
              break;
            default:
              return 0; // Invalid/unsupported mode
              break;
        }
    }

    // In case this is a new file, add create flag
    fp->flags |= O_CREAT;

    // Get file descriptor 
    fp->fd = open(path, fp->flags);

    // Set initial file position
    fp->pos = 0; 

    return fp;
} 

// Close a file
int fclose(FILE *fp) {
    close(fp->fd);

    return 0;
}

// Read nmemb objects, each size bytes long, from the file pointed to
//   by fp, to the pointer ptr
uint32_t fread(void *ptr, uint32_t size, uint32_t nmemb, FILE *fp) {
    uint32_t num_read = 0;

    for (uint32_t i = 0; i < nmemb; i++) { 
        uint32_t bytes_read = read(fp->fd, ptr, size);
        fp->pos += bytes_read;
        if (bytes_read != size) break; // Only count read as full size read
        num_read++;
    }

    return num_read;
}

// Write nmemb objects, each size bytes long, to the file pointed to
//   by fp, from the pointer ptr
uint32_t fwrite(void *ptr, uint32_t size, uint32_t nmemb, FILE *fp) {
    uint32_t num_written = 0;

    for (uint32_t i = 0; i < nmemb; i++) { 
        uint32_t bytes_written = write(fp->fd, ptr, size);
        fp->pos += bytes_written;
        if (bytes_written != size) break; // Only count read as full size read
        num_written++;
    }

    return num_written;
}

// Change file position
int fseek(FILE *fp, uint32_t offset, int whence) {
    int position = seek(fp->fd, offset, whence);

    if (position >= 0) {
        // Set fp->pos value
        fp->pos = position;
        return 0;   // Success
    } else {
      return -1;    // Error
    }
}

// Reset file to starting offset/position
void rewind(FILE *fp) {
    fseek(fp, 0, SEEK_SET);
}

// Return value of current file position
int ftell(FILE *fp) {
    if (!fp) return -1;
    if (fp->fd == 0) return -1;

    return fp->pos;
}
