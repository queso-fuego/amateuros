// print_types.h: print different types of values, strings/numbers/etc.
#pragma once

#define FONT_ADDRESS 0xA000  
#define FONT_WIDTH   0xA000 
#define FONT_HEIGHT  (FONT_WIDTH+1)

//----------------------------------------------
// print_char.h: Print a single character to video memory
// Parms:
//  input 1: column to print to (address, not value)
//  input 2: row to print to    (address, not value)
//  input 3: char to print
//----------------------------------------------
void print_char(uint16_t *cursor_x, uint16_t *cursor_y, uint8_t in_char)
{
    uint8_t *framebuffer    = (uint8_t *)gfx_mode->physical_base_pointer;   // Framebuffer pointer
    uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;             // Get # of bytes per pixel, add 1 to fix 15bpp modes
    uint8_t font_width      = *(uint8_t *)FONT_WIDTH; 
    uint8_t font_height     = *(uint8_t *)FONT_HEIGHT;
    uint32_t row            = (*cursor_y * font_height * gfx_mode->x_resolution) * bytes_per_pixel;           // Row to print to in pixels
    uint32_t col            = (*cursor_x * font_width) * bytes_per_pixel;            // Col to print to in pixels
    uint8_t *font_char;
    uint8_t bytes_per_char_line;
    uint8_t char_size;
    uint8_t *scroll, *scroll2;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    if (in_char == 0x0A) {     // Line feed
        (*cursor_y)++;    // Increment cursor Y, go down 1 row
        if (*cursor_y >= (gfx_mode->y_resolution / font_height) - 1) {    // At bottom of screen? 
            // Copy screen lines 1-<last line> into lines 0-<last line - 1>,
            //   then clear out last line and continue printing
            scroll = (uint8_t *)gfx_mode->physical_base_pointer;

            // Char row 1 on screen 
            scroll2 = scroll + ((gfx_mode->x_resolution * font_height) * bytes_per_pixel);  // Multiply by font_height - char height in lines 

            memcpy32(scroll, scroll2, ((gfx_mode->y_resolution - font_height) * gfx_mode->linear_bytes_per_scanline));

            (*cursor_y)--;  // set Y = last line

        } 
        goto ret;

    } else if (in_char == 0x0D) { // Carriage return
        *cursor_x = 0;                   // New cursor X position = 0 / start of line
        goto ret;
    }

    // Font memory address = FONT_ADDRESS, offset from start of font,
    // multiply char by font_height (length in bytes per char),
    // subtract font_height to get start of char
    bytes_per_char_line = ((font_width - 1) / 8) + 1;
    char_size = bytes_per_char_line * font_height;
    font_char = (uint8_t *)(FONT_ADDRESS + ((in_char * char_size) - char_size));

    // Print character
    // Char height is font_height lines
    for (uint8_t line = 0; line < font_height; line++) { 
        uint8_t num_px_drawn = 0;
        for (int8_t byte = bytes_per_char_line - 1; byte >= 0; byte--) {
            for (int8_t bit = 7; bit >= 0; bit--) {
                // If bit is set draw text color pixel, if not draw background color
                if (font_char[line * bytes_per_char_line + byte] & (1 << bit)) {
                    *((uint32_t *)framebuffer) = user_gfx_info->fg_color;
                } else {
                    *((uint32_t *)framebuffer) = user_gfx_info->bg_color;
                }

                framebuffer += bytes_per_pixel;  // Next pixel position
                num_px_drawn++;

                // If done drawing all pixels in current line of char, stop and go on
                if (num_px_drawn == font_width) {
                    num_px_drawn = 0;
                    break;
                }
            }
        }

        // Go down 1 line on screen - 1 char pixel width to line up next line of char
        framebuffer += (gfx_mode->x_resolution - font_width) * bytes_per_pixel;
    }

    // Increment cursor 
    (*cursor_x)++;    // Update cursor X position
    if (*cursor_x != 80)    // at end of line?
        goto ret;           // No, go on

    // Yes, at end of line, do a CR/LF
    // CR
    *cursor_x = 0;      // New cursor X position = 0 / start of line

    // LF
    (*cursor_y)++;    // Go down 1 row
    if (*cursor_y >= (gfx_mode->y_resolution / font_height) - 1) {    // At bottom of screen? 
        // Copy screen lines 1-<last line> into lines 0-<last line - 1>,
        //   then clear out last line and continue printing
        scroll = (uint8_t *)gfx_mode->physical_base_pointer;

        // Char row 1 on screen 
        scroll2 = scroll + ((gfx_mode->x_resolution * font_height) * bytes_per_pixel);  // Multiply by font_height - char height in lines 

        memcpy32(scroll, scroll2, ((gfx_mode->y_resolution - font_height) * gfx_mode->linear_bytes_per_scanline));

        (*cursor_y)--;  // set Y = last line
    }

    ret:
    return;
}

//
// print_string.h: Print a string to video memory
// Parms:
//  input 1: column to print to (address, not value)
//  input 2: row to print to	(address, not value)
//  input 3: address of string
//----------------------------------------------
void print_string(uint16_t *cursor_x, uint16_t *cursor_y, const uint8_t *in_string)
{
    for (; *in_string != '\0'; in_string++)
        print_char(cursor_x, cursor_y, *in_string);
}

/*
 * print_dec.h: Print a base 10 integer as a string of characters (maybe an itoa or itostr?)
 */
void print_dec(uint16_t *cursor_x, uint16_t *cursor_y, int32_t number)
{
    uint8_t dec_string[80];
    uint8_t i = 0, j, temp;
    uint8_t negative = 0;       // Is number negative?

    if (number == 0) dec_string[i++] = '0'; // If passed in 0, print a 0
    else if (number < 0)  {
        negative = 1;       // Number is negative
        number = -number;   // Easier to work with positive values
    }

    while (number > 0) {
        dec_string[i] = (number % 10) + '0';    // Store next digit as ascii
        number /= 10;                           // Remove last digit
        i++;
    }

    if (negative) dec_string[i++] = '-';    // Add negative sign to front

    dec_string[i] = '\0';   // Null terminate

    // Number stored backwards in dec_string, reverse the string by swapping each end
    //   until they meet in the middle
    i--;    // Skip the null byte
    for (j = 0; j < i; j++, i--) {
        temp          = dec_string[j];
        dec_string[j] = dec_string[i];
        dec_string[i] = temp;
    }

    // Print out result
    print_string(cursor_x, cursor_y, dec_string);
}
//
// Prints hexadecimal values using register DX and print_string.asm
//
// input 1: cursor Y position	(address, not value)
// input 2: cursor X position	(address, not value)
// input 3: number to print     (value)
//
// Ascii '0'-'9' = hex 30h-39h
// Ascii 'A'-'F' = hex 41h-46h
// Ascii 'a'-'f' = hex 61h-66h
void print_hex(uint16_t *cursor_x, uint16_t *cursor_y, uint32_t hex_num)
{
    uint8_t hex_string[80];
    uint8_t *ascii_numbers = "0123456789ABCDEF";
    uint8_t nibble;
    uint8_t i = 0, j, temp;
    uint8_t pad = 0;
    
    // If passed in 0, print a 0
    if (hex_num == 0) {
        strncpy(hex_string, "0\0", 2);
        i = 1;
    }

    if (hex_num < 0x10) pad = 1;    // If one digit, will pad out to 2 later

    while (hex_num > 0) {
        // Convert hex values to ascii string
        nibble = (uint8_t)hex_num & 0x0F;  // Get lowest 4 bits
        nibble = ascii_numbers[nibble];    // Hex to ascii 
        hex_string[i] = nibble;             // Move ascii char into string
        hex_num >>= 4;                     // Shift right by 4 for next nibble
        i++;
    }
        
    if (pad) hex_string[i++] = '0';  // Pad out string with extra 0    

    // Add initial "0x" to front of hex string
    hex_string[i++] = 'x';
    hex_string[i++] = '0';
    hex_string[i] = '\0';   // Null terminate string

    // Number is stored backwards in hex_string, reverse the string by swapping ends
    //   until they meet in the middle
    i--;    // Skip null byte
    for (j = 0; j < i; j++, i--) {
        temp          = hex_string[j];
        hex_string[j] = hex_string[i];
        hex_string[i] = temp;
    }

    // Print hex string
    print_string(cursor_x, cursor_y, hex_string);
}
