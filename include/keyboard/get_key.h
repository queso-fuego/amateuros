//
// get_key.inc: Returns an ascii character translated from a scancode from PS/2 controller 
//                data port 60h
// Output 1: Ascii char in AL
#pragma once

uint8_t get_key(void)
{
    // Scancode -> Ascii lookup table
    const uint8_t *scancode_to_ascii = "\x00\x1B" "1234567890-=" "\x08"
    "\x00" "qwertyuiop[]" "\x0D\x1D" "asdfghjkl;'`" "\x00" "\\"
    "zxcvbnm,./" "\x00\x00\x00" " ";

    // Shift key pressed on number row lookup table (0-9 keys)
    uint8_t *num_row_shifts = ")!@#$%^&*(";

    uint8_t left_shift = 0;  // Left shift key pressed y/n
    uint8_t left_ctrl  = 0;  // Left control key pressed y/n
    uint8_t scancode   = 0;  // Scancode from keyboard data port
    uint8_t input_char = 0;      
    uint8_t test_byte  = 0;
    uint8_t oldKey     = 0;

    while (1) {
        oldKey = scancode;

        // Read PS/2 controller status register until output buffer is full
        while (1) {
            __asm__ __volatile__ ("inb $0x64, %%al" : "=a"(test_byte) );
            if (test_byte & 1) break;   // Bit 0 = output buffer status; 0 = empty, 1 = full
        }

        // Get keyboard scancode from data port
        __asm__ __volatile__ ("inb $0x60, %%al" : "=a"(scancode) );

        if (scancode == oldKey) {    // Keep going until we get a new key
            continue;
        } else if (scancode == 0x2A) {          // Left Shift make code
            left_shift = 1;
            continue;
        } else if (scancode == 0xAA) {   // Left Shift break code
            left_shift = 0;
            continue;
        } else if (scancode == 0x1D) {   // Left Ctrl make code
            left_ctrl = 1;
            continue;
        } else if (scancode == 0x9D) {   // Left Ctrl break code
            left_ctrl = 0;
            continue;
        } else if (scancode & (1 << 7)) {  // is it a break code? Bit 7 set
            continue;
        } else if (scancode > 0x53) {    // Current end of scancode lookup table (delete key)
            continue;               // Beyond lookup table, keep going
        } 

        input_char = scancode_to_ascii[scancode];

        if (left_shift != 1)        // Is shift pressed?
            break;                  // No, return

        // Yes, check which key shift is modifying
        if (input_char >= 'a' && input_char <= 'z') {   // Letter
            input_char -= 0x20;     // Convert lowercase letters to uppercase
            break;
        }

        if (input_char >= '0' && input_char <= '9') {   // Number
            input_char -= 0x30;                         // Convert ascii -> integer
            input_char = num_row_shifts[input_char];    // Get shifted key code value
            break;
        }

        // Maybe it's a '+' or '"' or '?' or other things
        if (input_char == '=')
            input_char = '+';
        else if (input_char == '/')
            input_char = '?';

        // TODO: Get next key if invalid?

        break;
    }

    // 'struct' in memory at address 3000h holds last key/scancode pressed and shift/ctrl key status
    *(uint8_t *)0x1600 = input_char;
    *(uint8_t *)0x1601 = scancode;
    *(uint8_t *)0x1602 = left_shift;    // 1 = pressed, 0 = released
    *(uint8_t *)0x1603 = left_ctrl;     // 1 = pressed, 0 = released

    // Reset shift & ctrl key status just in case
    left_shift = 0;
    left_ctrl  = 0;

    return input_char;
}
