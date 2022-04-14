/*
 * ctype.h: Character functions, to operate/handle invidual characters
 */
#pragma once

#include "C/stdbool.h"

// Is C a whitespace character?
bool isspace(const char c)
{
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
            c == '\b');
}
