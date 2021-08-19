/*
 *  string.h: String and memory movement functions
 */
#pragma once

#include "stdint.h"

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
int16_t strncmp(const uint8_t *string1, const uint8_t *string2, const uint8_t len)
{
    uint8_t i = 0;

    while (*string1 && *string1 == *string2 && i < len) {
        string1++;
        string2++;
        i++;
    }

    return *string1 - *string2;
}

// strlen: Get length of a string up until the NUL terminator
//
// Returns:
//   length (255 max)
uint8_t strlen(const uint8_t *string)
{
    uint8_t len = 0;

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
    for (uint8_t i = 0; src[i]; i++)
        dst[i] = src[i];

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

