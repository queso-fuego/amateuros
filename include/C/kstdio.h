/*
 * C/stdio.h: Standard input/output functions
 */
#pragma once

#include "C/stdbool.h"
#include "C/kstdlib.h"
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
    uint32_t fd;        // File descriptor
    uint32_t pos;       // File position
    uint8_t attr;       // File attributes
} FILE;

enum {
    READ = 0x1,
    WRITE = 0x2,
    READWRITE = 0x3,
} file_attributes;

uint8_t *write_buffer = 0;
uint32_t len = 0;

// Print single character
void kputc(char c)
{
    write(stdout, &c, 1);
}

// Print a string
void kputs(char *s)
{
    while (*s) {
        kputc(*s);
        s++;
    }
}

// Print a hex integer
void kprintf_hex(const uint32_t num)
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
void kprintf_int(const int32_t num, const uint8_t base, const bool sign)
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
void kprintf(const char *fmt, ...)
{
    uint32_t *arg_ptr = (uint32_t *)&fmt;
    int state = 0;      // Inside a format string or not?
    uint32_t max = 512; // Max string length
    char *s = 0; 

    arg_ptr++;  // Move to first arg after format string on the stack

    len = 0;

    write_buffer = kmalloc(max);

    for (uint32_t i = 0; fmt[i] != '\0'; i++) {
        while (len > max) {
            // Allocate and move to a larger buffer
            max *= 2;

            uint8_t *temp_buffer = kmalloc(max);

            strcpy(temp_buffer, write_buffer);

            kfree(write_buffer); // Free smaller buffer as it's no longer needed

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
                kprintf_int(*(int *)arg_ptr, 10, true);
                //arg_ptr += sizeof(int *);
                arg_ptr++;
                break;
            case 'x':
                // Hex integer e.g. "0xFFFF"
                write_buffer[len++] = '0';  // Add "0x" prefix
                write_buffer[len++] = 'x';

                kprintf_hex(*(unsigned int *)arg_ptr);
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

    write_buffer[len] = '\0';           // Null terminate string
    write(stdout, write_buffer, len);   // Call write system call
    kfree(write_buffer);
}

