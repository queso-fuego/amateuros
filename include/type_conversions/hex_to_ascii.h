// =========================================================================== 
// hex_to_ascii.h: Convert a hex character to its ascii equivalent
// ===========================================================================
// Parms: 
//  input 1: hex character to convert
#pragma once

uint8_t hex_to_ascii(uint8_t hex_char)
{
    uint8_t *hex_to_ascii = "0123456789ABCDEF"; // Hex digits as Ascii
    hex_char = hex_to_ascii[hex_char];

    return hex_char;
}
