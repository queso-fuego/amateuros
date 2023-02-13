/*
 * terminal/terminal.h: Kind of a generic Terminal "driver", functions to write to screen
 */
#pragma once

#include "C/stdint.h"
#include "C/stdbool.h"
#include "C/string.h"
#include "gfx/2d_gfx.h"
#include "global/global_addresses.h"

/* Write len bytes from buf to screen
 * Supported escape sequences:
 * -----------------------------------------
 * !Start with '\e' or \x1b or \o33, etc. !
 * !End with semi-colon ';'               !
 * -----------------------------------------
 * Set cursor position: "\eX#Y#;"
 * Set FG/BG colors: "\eFG#BG#;"
 * Clear the screen, and set XY to 0: "\eCLS;
 * Turn cursor on: "\eCSRON;"
 * Turn cursor off: "\eCSROFF;"
 * Backspace (Move csr left by 1, and erase character at current position): "\eBS;"
*/
int32_t terminal_write(void *buf, const uint32_t len)
{
    const uint8_t FONT_W = *(uint8_t *)FONT_WIDTH; 
    const uint8_t FONT_H = *(uint8_t *)FONT_HEIGHT;
    const uint32_t X_LIMIT = gfx_mode->x_resolution / FONT_W;
    const uint32_t Y_LIMIT = gfx_mode->y_resolution / FONT_H;
    const uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;             // Get # of bytes per pixel, add 1 to fix 15bpp modes
    
    // Persistent terminal variables affected by control/escape sequences
    static uint32_t X = 0, Y = 0;
    static uint32_t fg_color = 0x00DDDDDD, bg_color = 0x00222222;
    static bool show_cursor = true;

    uint8_t *str = (uint8_t *)buf;
    uint8_t *scroll, *scroll2;
    uint8_t *font_char;
    uint8_t char_size;
    uint8_t bytes_per_char_line;
    uint8_t *framebuffer;
    uint32_t i;

    for (i = 0; i < len && str[i] != '\0'; i++) {
        if (str[i] == '\x1b') {
            // Found escape sequence
            i++;
            if (str[i] == 'X') { 
                // Set new X/Y cursor position
                X = 0;
                i++; 
                while (str[i] != 'Y') {
                    // TODO: Change to use atoi call here instead?
                    X = X * 10 + str[i++] - '0';
                }

                Y = 0;
                i++; 
                while (str[i] != ';') {
                    // TODO: Change to use atoi call here instead?
                    Y = Y * 10 + str[i++] - '0';
                }

            } else if (!strncmp(&str[i], "CLS;", 4)) {
               // Clear screen to background color, and set cursor X/Y position to 0 
               framebuffer = (uint8_t *)gfx_mode->physical_base_pointer;

                for (uint32_t i = 0; i < gfx_mode->x_resolution * gfx_mode->y_resolution; i++) {
                    // DEBUGGING
                    //*((uint32_t *)framebuffer) = bg_color;

                    if (bytes_per_pixel > 2) {
                        *((uint32_t *)framebuffer) = user_gfx_info->bg_color;
                    } else if (bytes_per_pixel == 2) {
                        *((uint16_t *)framebuffer) = (uint16_t)user_gfx_info->bg_color;
                    } else if (bytes_per_pixel == 1) {
                        *((uint8_t *)framebuffer) = (uint8_t)user_gfx_info->bg_color;
                    }
                    // DEBUGGING

                    framebuffer += bytes_per_pixel;
                }
                X = Y = 0;
                i += 3;

            } else if (!strncmp(&str[i], "FG", 2)) {
                // Set new foregound & background colors
                uint8_t base = 0;

                i += 2;     // Go to start of FG color number
                fg_color = 0;
                if (str[i] == '0' && str[i+1] == 'x') {
                    // Hex number
                    i += 2; // Start of hex number string
                    base = 16;
                } else {
                    // Decimal number
                    base = 10;
                }

                while (str[i] != 'B') {
                    fg_color = fg_color * base + str[i] - ((str[i] >= 'A' && str[i] <= 'F') ? 'A' - 10 : '0');
                    i++;
                }

                i += 2;     // Go to start of BG color number
                bg_color = 0;
                if (str[i] == '0' && str[i+1] == 'x') {
                    // Hex number
                    i += 2; // Start of hex number string
                    base = 16;
                } else {
                    // Decimal number
                    base = 10;
                }

                while (str[i] != ';') {
                    bg_color = bg_color * base + str[i] - ((str[i] >= 'A' && str[i] <= 'F') ? 'A' - 10 : '0');
                    i++;
                }

                user_gfx_info->fg_color = convert_color(fg_color);
                user_gfx_info->bg_color = convert_color(bg_color);

            } else if (!strncmp(&str[i], "CSRON;", 6)) {
                // Turn cursor on
                show_cursor = true;
                i += 5;     // Skip rest of escape sequence

            } else if (!strncmp(&str[i], "CSROFF;", 7)) {
                // Turn cursor off
                show_cursor = false;
                i += 6;     // Skip rest of escape sequence

            } else if (!strncmp(&str[i], "BS;", 3)) {
                // Back space
                // TODO: Extract character drawing to separate function and call it here, to print 2 spaces
                //   and then move cursor back 2 spaces afterwards
                X--;
                i += 2;     // Skip rest of escape sequence
            }

            continue; // Go on to next character in string
        } 

        if (str[i] == '\n') {     // Line feed
            // Increment cursor Y, go down 1 row
            if (++Y >= (uint32_t)(gfx_mode->y_resolution / FONT_H) - 1) {    // At bottom of screen? 
                // Copy screen lines 1-<last line> into lines 0-<last line - 1>,
                //   then clear out last line and continue printing
                scroll = (uint8_t *)gfx_mode->physical_base_pointer;

                // Char row 1 on screen 
                scroll2 = scroll + ((gfx_mode->x_resolution * FONT_H) * bytes_per_pixel);  // Multiply by FONT_H - char height in lines 

                memcpy32(scroll, scroll2, ((gfx_mode->y_resolution - FONT_H) * gfx_mode->linear_bytes_per_scanline));

                --Y;  // set Y = last line
            } 
            continue;

        } else if (str[i] == '\r') { // Carriage return
            X = 0;                   // New cursor X position = 0 / start of line
            continue;
        }

        // Font memory address = FONT_ADDRESS, offset from start of font,
        // multiply char by FONT_H (length in bytes per char),
        // subtract FONT_H to get start of char
        bytes_per_char_line = ((FONT_W - 1) / 8) + 1;
        char_size = bytes_per_char_line * FONT_H;
        font_char = (uint8_t *)(FONT_ADDRESS + ((str[i] * char_size) - char_size));

        // Print character
        framebuffer = (uint8_t *)gfx_mode->physical_base_pointer +
        ((Y * FONT_H * gfx_mode->x_resolution) * bytes_per_pixel) + 
        ((X * FONT_W) * bytes_per_pixel);
        
        // Char height is FONT_H lines
        for (uint8_t line = 0; line < FONT_H; line++) { 
            uint8_t px_drawn = 0;
            for (int8_t byte = bytes_per_char_line - 1; byte >= 0; byte--) {
                for (int8_t bit = 7; bit >= 0 && px_drawn < FONT_W; bit--, px_drawn++) {
                    // If bit is set draw text color pixel, if not draw background color
                    if (font_char[line * bytes_per_char_line + byte] & (1 << bit)) {
                        // DEBUGGING
                        if (bytes_per_pixel > 2) {
                            *((uint32_t *)framebuffer) = user_gfx_info->fg_color;
                        } else if (bytes_per_pixel == 2) {
                            *((uint16_t *)framebuffer) = (uint16_t)user_gfx_info->fg_color;
                        } else if (bytes_per_pixel == 1) {
                            *((uint8_t *)framebuffer) = (uint8_t)user_gfx_info->fg_color;
                        }
                        // DEBUGGING
                    } else {
                        // DEBUGGING
                        if (bytes_per_pixel > 2) {
                            *((uint32_t *)framebuffer) = user_gfx_info->bg_color;
                        } else if (bytes_per_pixel == 2) {
                            *((uint16_t *)framebuffer) = (uint16_t)user_gfx_info->bg_color;
                        } else if (bytes_per_pixel == 1) {
                            *((uint8_t *)framebuffer) = (uint8_t)user_gfx_info->bg_color;
                        }
                        // DEBUGGING
                    }

                    framebuffer += bytes_per_pixel;  // Next pixel position
                }
            }

            // Go down 1 line on screen - 1 char pixel width to line up next line of char
            framebuffer += (gfx_mode->x_resolution - FONT_W) * bytes_per_pixel;
        }

        // Increment cursor 
        if (++X < X_LIMIT)    // at end of line?
            continue;

        // Yes, at end of line, do a CR/LF
        // CR
        X = 0;      // New cursor X position = 0 / start of line

        // LF
        if (++Y >= Y_LIMIT - 1) {    // At bottom of screen? 
            // Copy screen lines 1-<last line> into lines 0-<last line - 1>,
            //   then clear out last line and continue printing
            scroll = (uint8_t *)gfx_mode->physical_base_pointer;

            // Char row 1 on screen 
            scroll2 = scroll + ((gfx_mode->x_resolution * FONT_H) * bytes_per_pixel);  // Multiply by FONT_H - char height in lines 

            memcpy32(scroll, scroll2, ((gfx_mode->y_resolution - FONT_H) * gfx_mode->linear_bytes_per_scanline));

            Y--;  // set Y = last line
        }
    }

    // Draw cursor
    if (show_cursor) {
        font_char = (uint8_t *)(FONT_ADDRESS + ((127 * char_size) - bytes_per_char_line));

        // Go to last row of cursor character 
        framebuffer = (uint8_t *)gfx_mode->physical_base_pointer +
        ((Y * FONT_H * gfx_mode->x_resolution) * bytes_per_pixel) + 
        ((X * FONT_W) * bytes_per_pixel);

        framebuffer += (gfx_mode->x_resolution * (FONT_H - 1)) * bytes_per_pixel;   // Last line of character data

        uint8_t px_drawn = 0;
        for (int8_t byte = bytes_per_char_line - 1; byte >= 0; byte--) {
            for (int8_t bit = 7; bit >= 0 && px_drawn < FONT_W; bit--, px_drawn++) {
                // If bit is set draw text color pixel, if not draw background color
                if (font_char[byte] & (1 << bit)) { 
                    *((uint32_t *)framebuffer) = user_gfx_info->fg_color;
                } else {
                    *((uint32_t *)framebuffer) = user_gfx_info->bg_color;
                }

                framebuffer += bytes_per_pixel;  // Next pixel position
            }
        }
    }

    return i;   // Number of bytes written
}

