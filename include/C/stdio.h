/*
 * C/stdio.h: Standard input/output functions
 */
#pragma once

#include "C/stdbool.h"
#include "C/stdlib.h"
#include "C/stdint.h"
#include "C/string.h"
#include "sys/syscall_wrappers.h"

#define stdin  0
#define stdout 1
#define stderr 2

char write_buffer[1024];
uint32_t write_len = 0;

// Print a hex integer
void printf_hex(const uint32_t num, int32_t min_digits) {
    const char digits[] = "0123456789ABCDEF";
    char buf[32] = {0};
    int32_t i = 0;
    uint32_t n = num;
    bool pad = (n < 0x10);

    do {
        buf[i++] = digits[n % 16];
    } while ((n /= 16) > 0);

    while (i < min_digits) buf[i++] = '0';

    if (pad || !((i-1) & 1)) write_buffer[write_len++] = '0';  // Add padding 0

    while (--i >= 0) write_buffer[write_len++] = buf[i];
}

// Print a decimal integer
void printf_int(const int32_t num, const uint8_t base, const bool sign, int32_t min_digits) {
    const char digits[] = "0123456789ABCDEF";
    bool negative = false;
    char buf[32] = {0};
    int32_t i = 0;
    int32_t n = num;

    if (sign == true && num < 0) {
        negative = true;
        n = -num;
    }

    do {
        buf[i++] = digits[n % base];
    } while ((n /= base) > 0);

    while (i < min_digits) buf[i++] = '0';

    if (negative) buf[i++] = '-';

    while (--i >= 0) write_buffer[write_len++] = buf[i];
}

// Print formatted string
void printf(const char *fmt, ...) {
    uint32_t *arg_ptr = (uint32_t *)&fmt;
    int state = 0;  // Inside a format string or not?
    char *s = 0; 
    bool alternate_form = false, left_adjust = false;
    int32_t precision = 0;
    int32_t field_width = 0;

    arg_ptr++;  // Move to first arg after format string on the stack

    // Initialize buffer
    write_buffer[0] = '\0';
    write_len = 0;

    for (uint32_t i = 0; fmt[i] != '\0'; i++) {
        char c = fmt[i];
        if (state == 0) {
            if (c == '%') state = '%'; // Found a format string
            else write_buffer[write_len++] = c;

        } else if (state == '%') {
            // Flags
            alternate_form = false;
            left_adjust    = false;
            while (true) {
                switch (c) {
                    case '#':
                        alternate_form = true;
                        c = fmt[++i];
                        continue;
                    case '-':
                        left_adjust = true;
                        c = fmt[++i];
                        continue;
                    default:
                        break;
                }
                break;
            }

            // Width
            field_width = 0;
            while (c >= '0' && c <= '9') {
                field_width = field_width * 10 + (c - '0');
                c = fmt[++i];
            }

            // Precision
            precision = 0;
            if (c == '.') {
                c = fmt[++i];
                while (c >= '0' && c <= '9') {
                    precision = precision * 10 + (c - '0');
                    c = fmt[++i];
                }
            }

            // Conversion specifier
            switch (c) {
            case 'd': 
                // Signed decimal integer e.g. "-123"
                {
                    int32_t number = *(int32_t *)arg_ptr, tmp = number;
                    arg_ptr++;

                    int32_t num_digits = 0;
                    do {
                       num_digits++; 
                    } while ((tmp /= 10) != 0);

                    int32_t diff = field_width - num_digits - (number < 0);
                    if (precision - num_digits > 0) 
                        diff -= (precision - num_digits);

                    printf_int(number, 10, true, precision);

                    // Left adjust; pad with spaces on right
                    if (diff > 0 && left_adjust) 
                        while (diff-- > 0) write_buffer[write_len++] = ' ';
                }
                break;
            case 'u':
                // Unsigned decimal integer e.g. "123"
                {
                    uint32_t number = *(uint32_t *)arg_ptr, tmp = number;
                    arg_ptr++;

                    int32_t num_digits = 0;
                    do {
                       num_digits++; 
                    } while ((tmp /= 10) != 0);

                    int32_t diff = field_width - num_digits;
                    if (precision - num_digits > 0) 
                        diff -= (precision - num_digits);

                    printf_int(number, 10, false, precision);

                    // Left adjust; pad with spaces on right
                    if (diff > 0 && left_adjust) 
                        while (diff-- > 0) write_buffer[write_len++] = ' ';
                }
                break;
            case 'x':
                // Hex integer e.g. "0xFFFF"
                {
                    uint32_t number = *(uint32_t *)arg_ptr, tmp = number;
                    arg_ptr++;

                    int32_t num_digits = 0;
                    do {
                       num_digits++; 
                    } while ((tmp /= 16) != 0);

                    int32_t diff = field_width - num_digits - ((alternate_form)*2);
                    if (precision - num_digits > 0) 
                        diff -= (precision - num_digits);

                    if (alternate_form) {
                        write_buffer[write_len++] = '0';  // Add "0x" prefix
                        write_buffer[write_len++] = 'x';
                    }
                    printf_hex(number, precision);

                    // Left adjust; pad with spaces on right
                    if (diff > 0 && left_adjust) 
                        while (diff-- > 0) write_buffer[write_len++] = ' ';
                }
                break;
            case 's': 
                {
                    // String
                    s = *(char **)arg_ptr; 
                    arg_ptr++;

                    if (!s) s = "(null)";

                    int32_t diff = field_width - strlen(s);

                    // Right adjust; pad with spaces on left
                    if (diff > 0 && !left_adjust) 
                        while (diff-- > 0) write_buffer[write_len++] = ' ';

                    while (*s) write_buffer[write_len++] = *s++; 

                    // Left adjust; pad with spaces on right
                    if (diff > 0 && left_adjust) 
                        while (diff-- > 0) write_buffer[write_len++] = ' ';
                }
                break;
            case 'c':
                // Single Character
                write_buffer[write_len++] = *(char *)arg_ptr;
                arg_ptr++;
                break;
            case '%':
                // Character literal '%'
                write_buffer[write_len++] = c;
                break;
            default:
                // Unsupported format, print so user can see
                write_buffer[write_len++] = '%';
                write_buffer[write_len++] = c;
                break;
            }

            state = 0;
        }
    }

    write_buffer[write_len] = '\0';         // Null terminate string
    write(stdout, write_buffer, write_len); // Call write system call
}

// Print a single character to stdout
void putchar(char c) {
    printf("%c", c);
}

// Print a string with ending newline to stdout
void puts(char *s) {
    printf("%s\n", s);
}


