/*
 * C/stdio.h: Standard input/output functions
 */
#pragma once

#include "C/stdbool.h"
#include "sys/syscall_wrappers.h"

// Print single character
void putc(char c)
{
    write(1, &c, 1);
}

// Print a string
void puts(char *s)
{
    while (*s) {
        putc(*s);
        s++;
    }
}

// Print a string
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
        putc(buf[i]);
}

// Print formatted string
void printf(const char *fmt, ...)
{
    uint32_t *arg_ptr = (uint32_t *)&fmt;
    int state = 0;  // Inside a format string or not?

    arg_ptr++;

    for (uint32_t i = 0; fmt[i] != '\0'; i++) {
        char c = fmt[i];
        if (state == 0) {
            if (c == '%') state = '%'; // Found a format string
            else putc(c);

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
                puts("0x");
                printf_int(*(unsigned int *)arg_ptr, 16, false);
                //arg_ptr += sizeof(unsigned int *); 
                arg_ptr++;
                break;
            case 's':
                // String
                puts(*(char **)arg_ptr); 
                //arg_ptr += sizeof(char **);
                arg_ptr++;
                break;
            case 'c':
                // Single Character
                putc(*(char *)arg_ptr);
                //arg_ptr += sizeof(char *);  
                arg_ptr++;
                break;
            case '%':
                // Character literal '%'
                putc(c);
                break;
            default:
                // Unsupported format, print so user can see
                putc('%');
                putc(c);
                break;
            }

            state = 0;
        }
    }
}







