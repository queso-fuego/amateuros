// 
// move_cursor.inc: moves hardware cursor to new row/col
// assuming VGA txt mode 03 - 80x25 characters, 16 colors
// Parms:
// input 1: col to move to
// input 2: row to move to
#pragma once

#define FONT_ADDRESS  0xA000    
#define FONT_WIDTH    0xA000   
#define FONT_HEIGHT (FONT_WIDTH+1)

void move_cursor(uint16_t cursor_x, uint16_t cursor_y)
{
    uint8_t *framebuffer    = (uint8_t *)gfx_mode->physical_base_pointer; 
    uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;
    uint8_t font_width      = *(uint8_t *)FONT_WIDTH; 
    uint8_t font_height     = *(uint8_t *)FONT_HEIGHT;
    uint32_t row            = (cursor_y * font_height * gfx_mode->x_resolution) * bytes_per_pixel; // Row to print to in pixels
    uint32_t col            = (cursor_x * font_width) * bytes_per_pixel;                           // Col to print to in pixels
    uint8_t *font_char;
    uint8_t bytes_per_char_line;
    uint8_t char_size;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    // Draw cursor
    // Font memory address = FONT_ADRESS, offset from start of font,
    // subtract 1 because 'cursor' is only 1 line
    bytes_per_char_line = ((font_width - 1) / 8) + 1;
    char_size = bytes_per_char_line * font_height;
    font_char = (uint8_t *)(FONT_ADDRESS + ((127 * char_size) - bytes_per_char_line));

    // Go to last row of cursor character 
    framebuffer += (gfx_mode->x_resolution * (font_height-1)) * bytes_per_pixel;   

    uint8_t num_px_drawn = 0;
    for (int8_t byte = bytes_per_char_line - 1; byte >= 0; byte--) {
        for (int8_t bit = 7; bit >= 0; bit--) {
            // If bit is set draw text color pixel, if not draw background color
            if (font_char[byte] & (1 << bit)) {
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
}

void remove_cursor(uint16_t cursor_x, uint16_t cursor_y)
{
    uint8_t *framebuffer    = (uint8_t *)gfx_mode->physical_base_pointer; 
    uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;
    uint8_t font_width      = *(uint8_t *)FONT_WIDTH; 
    uint8_t font_height     = *(uint8_t *)FONT_HEIGHT;
    uint32_t row            = (cursor_y * font_height * gfx_mode->x_resolution) * bytes_per_pixel;           // Row to print to in pixels
    uint32_t col            = (cursor_x * font_width) * bytes_per_pixel;                   // Col to print to in pixels
    uint8_t *font_char;
    uint8_t bytes_per_char_line;
    uint8_t char_size;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    bytes_per_char_line = ((font_width - 1) / 8) + 1;

    // Go to last row of cursor character 
    framebuffer += (gfx_mode->x_resolution * (font_height-1)) * bytes_per_pixel;   

    // Draw background color for 1 row
    uint8_t num_px_drawn = 0;
    for (int8_t byte = bytes_per_char_line - 1; byte >= 0; byte--) {
        for (int8_t bit = 7; bit >= 0; bit--) {
            // If bit is set draw text color pixel, if not draw background color
            *((uint32_t *)framebuffer) = user_gfx_info->bg_color;

            framebuffer += bytes_per_pixel;  // Next pixel position
            num_px_drawn++;

            // If done drawing all pixels in current line of char, stop and go on
            if (num_px_drawn == font_width) {
                num_px_drawn = 0;
                break;
            }
        }
    }
}
