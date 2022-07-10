/*
 * math.h: Math functions
 */
#pragma once

#include "C/stdint.h"

// Pow for positive integers (NOT a normal C math.h function)
// taken from https://stackoverflow.com/questions/101439/
unsigned int uipow(unsigned int base, unsigned int exp) {
    unsigned int result = 1;

    while (true) {
        if (exp & 1) result *= base;
        exp >>= 1;
        if (!exp) break;
        base *= base;
    }

    return result;
}
