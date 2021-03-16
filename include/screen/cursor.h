// 
// move_cursor.inc: moves hardware cursor to new row/col
// assuming VGA txt mode 03 - 80x25 characters, 16 colors
// Parms:
// input 1: col to move to
// input 2: row to move to
// output 1: error/return code in AX
#pragma once

#define text_bg_color 0x000000FF  // Blue
#define text_fg_color 0x00EEEEEE  // Light gray / almost white

uint16_t move_cursor(uint16_t cursor_x, uint16_t cursor_y)
{
    uint32_t *framebuffer       = (uint32_t *)*(uint32_t *)0x9028;   // Framebuffer pointer
    uint16_t bytes_per_scanline = *(uint16_t *)0x9010;               // bytes per scanline
    uint32_t row                = (cursor_y * 16 * 1920);           // Row to print to in pixels
    uint32_t col                = (cursor_x * 8);                   // Col to print to in pixels
    uint8_t *font_char;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    // Draw cursor
    // Font memory address = 6000h, offset from start of font,
    // multiply char by 16 (length in bytes per char),
    // subtract 1 because 'cursor' is only 1 line
    font_char = (uint8_t *)(0x6000 + ((127 * 16) - 1));

    framebuffer += (1920*15);   // Go to last row of cursor character

    for (int8_t bit = 7; bit >= 0; bit--) {
        // If bit is set draw text color pixel, if not draw background color
        *framebuffer = (*font_char & (1 << bit)) ? text_fg_color : text_bg_color;
        framebuffer++;  // Next pixel position
    }

	return 0;   // Success
}

uint16_t remove_cursor(uint16_t cursor_x, uint16_t cursor_y)
{
    uint32_t *framebuffer       = (uint32_t *)*(uint32_t *)0x9028;   // Framebuffer pointer
    uint16_t bytes_per_scanline = *(uint16_t *)0x9010;               // bytes per scanline
    uint32_t row                = (cursor_y * 16 * 1920);           // Row to print to in pixels
    uint32_t col                = (cursor_x * 8);                   // Col to print to in pixels
    uint8_t *font_char;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    framebuffer += (1920*15);   // Go to last row of character

    // Draw background color for 1 row
    for (int8_t bit = 7; bit >= 0; bit--)
        *framebuffer++ = text_bg_color;

	return 0;   // Success
}
