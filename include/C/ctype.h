/*
 * ctype.h: Character functions, to operate/handle invidual characters
 */
#pragma once

#include "C/stdbool.h"

// Check whitespace characters
bool isspace(const char c) {
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
            c == '\b');
}

// Check digits
bool isdigit(const char c) {
    return (c >= '0' && c <= '9');
}

// Check hex digits
bool isxdigit(const char c) {
    return (c >= '0' && c <= '9') || 
           (c >= 'A' && c <= 'F') || 
           (c >= 'a' && c <= 'f');
}

// Check lowercase characters
bool islower(const char c) {
    return (c >= 'a' && c <= 'f');
}

// Check uppercase characters
bool isupper(const char c) {
    return (c >= 'A' && c <= 'F');
}

// Check alphabetic 
bool isalpha(const char c) {
    return (islower(c) || isupper(c));
}

// Check alphanumeric
bool isalnum(const char c) {
    return (isalpha(c) || isdigit(c));
}

// Convert uppercase to lowercase
char tolower(const char c) {
    if (isupper(c)) return c - 0x20;
    return c;
}

// Convert lowercase to uppercase
char toupper(const char c) {
    if (islower(c)) return c + 0x20;
    return c;
}

