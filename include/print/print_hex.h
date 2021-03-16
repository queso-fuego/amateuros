//
// Prints hexidecimal values using register DX and print_string.asm
//
// input 1: cursor Y position	(address, not value)
// input 2: cursor X position	(address, not value)
//
// Ascii '0'-'9' = hex 30h-39h
// Ascii 'A'-'F' = hex 41h-46h
// Ascii 'a'-'f' = hex 61h-66h
#pragma once

void print_hex(uint16_t *cursor_x, uint16_t *cursor_y, uint16_t hex_num)
{
    uint8_t *hexString     = "0x0000\0";
    uint8_t *ascii_numbers = "0123456789ABCDEF";
    uint8_t nibble;

    for (uint8_t i = 0; i < 4; i++) {
        // Convert hex values to ascii string
        nibble = (uint8_t)hex_num & 0x0F;  // Get lowest 4 bits
        nibble = ascii_numbers[nibble];    // Hex to ascii 
        hexString[5-i] = nibble;           // Move ascii char into string
        hex_num >>= 4;                     // Shift right by 4 for next nibble
    }
        
    // Print hex string
    print_string(cursor_x, cursor_y, hexString);
}
