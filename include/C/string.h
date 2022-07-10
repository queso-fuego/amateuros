/*
 *  string.h: String and memory movement functions
 */
#pragma once

#include "stdint.h"
#include "stdbool.h"

// strcmp: Compare NUL-terminated string1 to string2
//
// returns:
//   negative value if string1 is less than string2
//   0 if string1 = string2
//   positive value if string1 is greater than string2
int16_t strcmp(const uint8_t *string1, const uint8_t *string2)
{
    while (*string1 && *string1 == *string2) {
        string1++;
        string2++;
    }

    return *string1 - *string2;
}

// strncmp: Compare NUL-terminated string1 to string2, at most len characters
//
// returns:
//   negative value if string1 is less than string2
//   0 if string1 = string2
//   positive value if string1 is greater than string2
int16_t strncmp(const uint8_t *string1, const uint8_t *string2, uint8_t len)
{
    while (len > 0 && *string1 == *string2) {
        string1++;
        string2++;
        len--;
    }
    if (len == 0) return 0;
    else          return *string1 - *string2;
}

// strlen: Get length of a string up until the NUL terminator
//
// Returns:
//   length 
uint32_t strlen(const uint8_t *string)
{
    uint32_t len = 0;

    while (*string) {
        string++;
        len++;
    }

    return len;
}

// strcpy: Copy src string to dst string (assuming src is <= dst!)
//
// Returns:
//   dst string 
uint8_t *strcpy(uint8_t *dst, const uint8_t *src)
{
    uint8_t i = 0;
    for (; src[i]; i++)
        dst[i] = src[i];

    dst[i] = src[i];    // Copy null terminator

    return dst;
}

// strncpy: Copy at most len characters from src string to dst string
//
// Returns:
//   dst string 
uint8_t *strncpy(uint8_t *dst, const uint8_t *src, const uint8_t len)
{
    for (uint8_t i = 0; src[i] && i < len; i++)
        dst[i] = src[i];

    return dst;
}

// strcat: Append src string to dst string
//
// Returns:
//   dst string
uint8_t *strcat(uint8_t *dst, const uint8_t *src)
{
    uint8_t i = 0, j = 0;
    while (dst[i]) i++;

    while (src[j])
        dst[i++] = src[j++];

    return dst;
}

// strchr: Find first occurrence of character c in string s
//
// Returns:
//   Pointer to first occurrence of c in s, or NULL if not found
char *strchr(const char *s, char c)
{
    char *sc = (char *)s;

    while (*sc != c) {
        if (*sc == '\0') return 0;
        sc++;
    }

    return sc;
}

// strrchr: Find last occurrence of character c in string s
//
// Returns:
//   Pointer to last occurrence of c in s, or NULL if not found
char *strrchr(const char *s, char c)
{
    char *result = 0;

    for (char *sc = (char *)s; ; sc++) {
        if (*sc == c) result = sc;
        if (*sc == '\0') return result;
    }
}

// strstr: Find first occurrence of null-terminated string little
//   in string big
// Returns:
//   Big if little is empty string
//   Null if little not found in big
//   Pointer to first letter of little in big
char *strstr(const char *big, const char *little) {
    if (*little == '\0') return (char *)big;

    for (; (big = strchr(big, *little)) != 0; big++) {
        const char *bc = big, *lc = little;

        while (true) {
            if (*++lc == '\0') return (char *)bc;
            else if (*++bc != *lc) break;
        }
    }

    return 0;
}

// memset: Write len # of bytes to a buffer
//
// Returns:
//   buffer
void *memset(void *buffer, const uint8_t byte, const uint32_t len)
{
    uint8_t *ptr = (uint8_t *)buffer;

    for (uint32_t i = 0; i < len; i++)
        ptr[i] = byte;

    return buffer;
}

// memcpy: Copy len bytes from src buffer to dst buffer
//
// Returns:
//  buffer
void *memcpy(void *dst, const void *src, const uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];

    return dst;
}

// memcpy32: Copy len bytes from src buffer to dst buffer, 4 bytes at a time
//
// Returns:
//  buffer
void *memcpy32(void *dst, const void *src, const uint32_t len)
{
    for (uint32_t i = 0; i < len/4; i++)
        ((uint32_t *)dst)[i] = ((uint32_t *)src)[i];

    return dst;
}

