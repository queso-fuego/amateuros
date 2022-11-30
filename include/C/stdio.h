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

uint8_t *write_buffer = 0;
uint32_t len = 0;

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

// Print single character
void putc(char c)
{
    printf("%c", c);
}

// Print a string
void puts(char *s)
{
    printf("%s", s);
}


