// 
// move_cursor.inc: moves hardware cursor to new row/col
// assuming VGA txt mode 03 - 80x25 characters, 16 colors
// Parms:
// input 1: col to move to
// input 2: row to move to
#pragma once

#define FONT_ADDRESS  0x6000

void move_cursor(uint16_t cursor_x, uint16_t cursor_y)
{
    uint8_t *framebuffer    = (uint8_t *)gfx_mode->physical_base_pointer; 
    uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;
    uint32_t row            = (cursor_y * 16 * gfx_mode->x_resolution) * bytes_per_pixel;           // Row to print to in pixels
    uint32_t col            = (cursor_x * 8) * bytes_per_pixel;                   // Col to print to in pixels
    uint8_t *font_char;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    // Draw cursor
    // Font memory address = 6000h, offset from start of font,
    // multiply char by 16 (length in bytes per char),
    // subtract 1 because 'cursor' is only 1 line
    font_char = (uint8_t *)(FONT_ADDRESS + ((127 * 16) - 1));

    framebuffer += (gfx_mode->x_resolution*15) * bytes_per_pixel;   // Go to last row of cursor character

    for (int8_t bit = 7; bit >= 0; bit--) {
        // If bit is set draw text color pixel, if not draw background color
        if (*font_char & (1 << bit)) {
            for (uint8_t temp = 0; temp < bytes_per_pixel; temp++)
                framebuffer[temp] = (uint8_t)(user_gfx_info->fg_color >> temp * 8);
        } else {
            for (uint8_t temp = 0; temp < bytes_per_pixel; temp++)
                framebuffer[temp] = (uint8_t)(user_gfx_info->bg_color >> temp * 8);
        }

        framebuffer+= bytes_per_pixel;  // Next pixel position
    }
}

void remove_cursor(uint16_t cursor_x, uint16_t cursor_y)
{
    uint8_t *framebuffer    = (uint8_t *)gfx_mode->physical_base_pointer; 
    uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;
    uint32_t row            = (cursor_y * 16 * gfx_mode->x_resolution) * bytes_per_pixel;           // Row to print to in pixels
    uint32_t col            = (cursor_x * 8) * bytes_per_pixel;                   // Col to print to in pixels
    uint8_t *font_char;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    framebuffer += (gfx_mode->x_resolution*15) * bytes_per_pixel;   // Go to last row of character

    // Draw background color for 1 row
    for (int8_t bit = 7; bit >= 0; bit--) {
        for (uint8_t temp = 0; temp < bytes_per_pixel; temp++) 
            framebuffer[temp] = (uint8_t)(user_gfx_info->bg_color >> temp * 8);

        framebuffer += bytes_per_pixel;
    }
}
