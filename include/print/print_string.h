//
// print_string.h: Print a string to video memory
// Parms:
//  input 1: column to print to (address, not value)
//  input 2: row to print to	(address, not value)
//  input 3: address of string
//  output 1: return code in AX
//----------------------------------------------
#pragma once

uint16_t print_string(uint16_t *cursor_x, uint16_t *cursor_y, uint8_t *in_string)
{
    const uint32_t text_bg_color = 0x000000FF;  // Blue
    const uint32_t text_fg_color = 0x00EEEEEE;  // Light gray / almost white

    uint32_t *framebuffer       = (uint32_t *)*(uint32_t *)0x9028;   // Framebuffer pointer
    uint32_t row                = (*cursor_y * 16 * 1920);           // Row to print to in pixels
    uint32_t col                = (*cursor_x * 8);                   // Col to print to in pixels
    uint8_t *font_char;
    uint8_t *string = in_string;
    uint32_t *scroll_start, *scroll_start2;

    // Text Cursor Position in screen in pixels to print to
    framebuffer += (row + col);

    for (; *string != '\0'; string++) {
        if (*string == 0x0A) {     // Line feed
            (*cursor_y)++;    // Increment cursor Y, go down 1 row
            if (*cursor_y >= 66) {    // At bottom of screen? (66 * 16 = 1080) 
                // Copy screen lines 1-<last line> into lines 0-<last line - 1>,
                //   then clear out last line and continue printing
                scroll_start = (uint32_t *)*(uint32_t *)0x9028;

                // Byte location of char row 1 on screen
                scroll_start2 = scroll_start + (1920 * 16);  // Multiply by 16 - char height in lines

                // 1080 lines - 16 lines character height = 1064 lines
                // 1064 * bytes per scanline (1920 pixels * 4 bytes per pixel = 7680 bytes)
                // 1064 * 7680 = 8171520 or 7CB000h bytes
                // 8171520 / 4 = 2042880 or 1F2C00h dbl words
                for (uint32_t pixel = 0; pixel < 0x1F2C00; pixel++)
                    *scroll_start++ = *scroll_start2++; // Move screen data 1 line up

                // scroll_start pointing at last character line now (last 16 lines)
                // 1920px per line * 16 lines = 30720px
                // 30720 = 7800h dbl words
                for (uint32_t pixel = 0; pixel < 0x7800; pixel++)
                    *scroll_start++ = text_bg_color;  // Clear last line

                (*cursor_y)--;  // set Y = last line

            } else {
                // Go down 1 char row on screen
                framebuffer += (1920 * 16);     // Line of pixels * char height 
            }
            continue;

        } else if (*string == 0x0D) { // Carriage return
            framebuffer -= (*cursor_x * 8);  // Go to start of line; Multiply cursor X by char pixel width
            *cursor_x = 0;                   // New cursor X position = 0 / start of line
            continue;
        }

        // Font memory address = 6000h, offset from start of font,
        // multiply char by 16 (length in bytes per char),
        // subtract 16 to get start of char
        font_char = (uint8_t *)(0x6000 + ((*string * 16) - 16));

        // Print character
        // Char height is 16 lines
        for (uint8_t line = 0; line < 16; line++) {
            for (int8_t bit = 7; bit >= 0; bit--) {
                // If bit is set draw text color pixel, if not draw background color
                *framebuffer = (font_char[line] & (1 << bit)) ? text_fg_color : text_bg_color;
                framebuffer++;  // Next pixel position
            }
            // Go down 1 line on screen - 1 char pixel width to line up next line of char
            framebuffer += (1920 - 8);
        }

        // Increment cursor 
        // Move screen position back up 1 char height to start next char
        framebuffer -= (1920 * 16);  // Char height = 16 lines

        (*cursor_x)++;    // Update cursor X position
        framebuffer += 8;
        if (*cursor_x != 80)    // at end of line?
            continue;           // No, go on

        // Yes, at end of line, do a CR/LF
        // CR
        framebuffer -= (*cursor_x * 8);  // Go to start of line; Multiply cursor X by char pixel width
        *cursor_x = 0;      // New cursor X position = 0 / start of line

        // LF
        (*cursor_y)++;    // Go down 1 row
        if (*cursor_y >= 66) {    // At bottom of screen? (66 * 16 = 1080) 
			// Copy screen lines 1-<last line> into lines 0-<last line - 1>,
			//   then clear out last line and continue printing
       		scroll_start = (uint32_t *)*(uint32_t *)0x9028;

            // Byte location of char row 1 on screen
            scroll_start2 = scroll_start + (1920 * 16);  // Multiply by 16 - char height in lines

            // 1080 lines - 16 lines character height = 1064 lines
            // 1064 * bytes per scanline (1920 pixels * 4 bytes per pixel = 7680 bytes)
            // 1064 * 7680 = 8171520 or 7CB000h bytes
            // 8171520 / 4 = 2042880 or 1F2C00h dbl words
            for (uint32_t pixel = 0; pixel < 0x1F2C00; pixel++)
                *scroll_start++ = *scroll_start2++; // Move screen data 1 line up

            // scroll_start pointing at last character line now (last 16 lines)
            // 1920px per line * 16 lines = 30720px
            // 30720 = 7800h dbl words
            for (uint32_t pixel = 0; pixel < 0x7800; pixel++)
                *scroll_start++ = text_bg_color;  // Clear last line

            (*cursor_y)--;  // set Y = last line

        } else {
            // Go down 1 char row on screen
            // Multiply by char height in lines
            framebuffer += (1920 * 16); 
        }
    }

    return 0;
}
