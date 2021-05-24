// =========================================================================== 
// Kernel.c: basic 'kernel' loaded from 2nd stage bootloader
// ===========================================================================
#include "../include/C/stdint.h"
#include "../include/C/string.h"
#include "../include/screen/clear_screen.h"
#include "../include/print/print_string.h"
#include "../include/print/print_char.h"
#include "../include/screen/cursor.h"
#include "../include/keyboard/get_key.h"
#include "../include/print/print_hex.h"
#include "../include/print/print_registers.h"
#include "../include/disk/file_ops.h"
#include "../include/print/print_fileTable.h"
#include "../include/type_conversions/hex_to_ascii.h"
#include "../include/gfx/2d_gfx.h"

__attribute__ ((section ("kernel_entry"))) void kernel_main(void)
{
    uint16_t kernel_cursor_x = 0;   // Text cursor X position
    uint16_t kernel_cursor_y = 0;   // Text cursor Y position
    uint8_t tokens[50];             // tokens array, equivalent-ish to tokens[5][10]
    uint8_t *tokens_ptr = tokens;
    uint16_t tokens_length[5];      // length of each token in tokens array (0-10)
    uint16_t *tokens_length_ptr = tokens_length;
    uint8_t token_count;            // How many tokens did the user enter?
    uint8_t token_file_name1[10];   // Filename 1 for commands
    uint8_t token_file_name2[10];   // Filename 2 for commands
    uint8_t cmdString[256];         // User input string
    uint8_t *cmdString_ptr = cmdString;
    uint8_t input_char   = 0;       // User input character
    uint8_t input_length;           // Length of user input
    uint16_t idx         = 0;
    uint8_t *cmdDir      = "dir\0";         // Directory command; list all files/pgms on disk
    uint8_t *cmdReboot   = "reboot\0";      // 'warm' reboot option
    uint8_t *cmdPrtreg   = "prtreg\0";      // Print register values
    uint8_t *cmdGfxtst   = "gfxtst\0";      // Graphics mode test
    uint8_t *cmdHlt      = "hlt\0";         // E(n)d current program by halting cpu
    uint8_t *cmdCls	     = "cls\0";         // Clear screen by scrolling
    uint8_t *cmdShutdown = "shutdown\0";    // Close QEMU emulator
    uint8_t *cmdEditor   = "editor\0";	    // Launch editor program
    uint8_t *cmdDelFile  = "del\0";		    // Delete a file from disk
    uint8_t *cmdRenFile  = "ren\0";         // Rename a file in the file table
    uint8_t fileExt[3];
    uint8_t *fileBin = "bin\0";
    uint8_t *fileTxt = "txt\0";
    uint8_t fileSize = 0;
    uint8_t *txt_file_ptr;
    uint8_t *windowsMsg     = "\x0A\x0D" "Oops! Something went wrong :(" "\x0A\x0D\0";
    uint8_t *notFoundString = "\x0A\x0D" "Program/file not found!, Try again? (Y)" "\x0A\x0D\0";
    uint8_t *sectNotFound   = "\x0A\x0D" "Sector not found!, Try again? (Y)" "\x0A\x0D\0";
    uint8_t *menuString     = "------------------------------------------------------\x0A\x0D"
                              "Kernel Booted, Welcome to QuesOS - 32 Bit 'C' Edition!\x0A\x0D"
                              "------------------------------------------------------\x0A\x0D\x0A\x0D\0";
    uint8_t *failure        = "\x0A\x0D" "Command/Program not found, Try again" "\x0A\x0D\0";
    uint8_t *prompt         = ">:\0";
    uint8_t *pgmNotLoaded   = "\x0A\x0D" "Program found but not loaded, Try Again" "\x0A\x0D\0";

    // --------------------------------------------------------------------
    // Initial setup
    // --------------------------------------------------------------------
    // Clear the screen
    clear_screen(BLUE);

    // Print OS boot message
    print_string(&kernel_cursor_x, &kernel_cursor_y, menuString);

    // --------------------------------------------------------------------
    // Get user input, print to screen & run command/program  
    // --------------------------------------------------------------------
    while (1) {
        // Reset tokens data, arrays, and variables for next input line
        memset(tokens, 0, 50);
        memset(tokens_length, 0, 10);
        token_count = 0;
        memset(token_file_name1, 0 , 10);
        memset(token_file_name2, 0 , 10);
        memset(cmdString, 0 , 255);

        // Print prompt
        print_string(&kernel_cursor_x, &kernel_cursor_y, prompt);
        move_cursor(kernel_cursor_x, kernel_cursor_y);
        
        input_length = 0;   // reset byte counter of input

        // Key loop - get input from user
        while (1) {
            input_char = get_key();     // Get ascii char from scancode from keyboard data port 60h

            if (input_char == 0x0D) {   // enter key?
                remove_cursor(kernel_cursor_x, kernel_cursor_y);
                break;                  // go on to tokenize user input line
            }

            // TODO: Move all input back 1 char/byte after backspace, if applicable
            if (input_char == 0x08) {       // backspace?
                if (input_length > 0) {                // Did user input anything?
                    input_length--;                     // yes, go back one char in cmdString
                    cmdString[input_length] = '\0';    // Blank out character    					
                }

                if (kernel_cursor_x > 2) {     // At start of line (at prompt)?
                    // TODO: change to use remove_cursor, move back 1 space, print a space, move back 1 space
                    // Move cursor back 1 space
                    kernel_cursor_x--;

                    // Print 2 spaces at cursor
                    print_char(&kernel_cursor_x, &kernel_cursor_y, 0x20);
                    print_char(&kernel_cursor_x, &kernel_cursor_y, 0x20);

                    // Move cursor back 2 spaces
                    kernel_cursor_x -= 2;
                    move_cursor(kernel_cursor_x, kernel_cursor_y);
                }

                continue;   // Get next character
            }

            cmdString[input_length] = input_char;   // Store input char to string
            input_length++;                         // Increment byte counter of input

            // Print input character to screen
            print_char(&kernel_cursor_x, &kernel_cursor_y, input_char);
            move_cursor(kernel_cursor_x, kernel_cursor_y);
        }

        if (input_length == 0) {
            // No input or command not found! boo D:
            print_string(&kernel_cursor_x, &kernel_cursor_y, failure);
            move_cursor(kernel_cursor_x, kernel_cursor_y);

            continue;
        }

        cmdString[input_length] = '\0';     // else null terminate cmdString from si

        // Tokenize input string "cmdString" into separate tokens
        cmdString_ptr     = cmdString;      // Reset pointers...
        tokens_ptr        = tokens;
        tokens_length_ptr = tokens_length;

        // Get token loop
        while (*cmdString_ptr != '\0') { 

            // Skip whitespace between tokens
            while (*cmdString_ptr == ' ') cmdString_ptr++;

            // If alphanumeric or underscore, add to current token
            while ((*cmdString_ptr >= '0' && *cmdString_ptr <= '9') || 
                  (*cmdString_ptr >= 'A' && *cmdString_ptr <= 'Z')  ||
                  (*cmdString_ptr == '_')                           ||
                  (*cmdString_ptr >= 'a' && *cmdString_ptr <= 'z')) {

                *tokens_ptr++ = *cmdString_ptr++;
                (*tokens_length_ptr)++;     // increment length of current token
            } 

            // Go to next token
            token_count++;                              // Next token
            tokens_ptr = tokens + (token_count * 10);   // Next position in tokens array
            tokens_length_ptr++;                        // Next position in tokens_length array
        }

        // Check commands 
        // Get first token (command to run) & second token (if applicable e.g. file name)
        if (strncmp(tokens, cmdDir, strlen(cmdDir)) == 0) {
            // --------------------------------------------------------------------
            // File/Program browser & loader   
            // --------------------------------------------------------------------
            print_fileTable(&kernel_cursor_x, &kernel_cursor_y);
            continue;
        }

        if (strncmp(tokens, cmdReboot, strlen(cmdReboot)) == 0) {
            // --------------------------------------------------------------------
            // Reboot: far jump to reset vector
            // --------------------------------------------------------------------
            __asm ("jmpl $0xFFFF, $0x0000");
        }

        if (strncmp(tokens, cmdPrtreg, strlen(cmdPrtreg)) == 0) {
            // --------------------------------------------------------------------
            // Print Register Values
            // --------------------------------------------------------------------
            print_registers(&kernel_cursor_x, &kernel_cursor_y); 

            continue;   
        }

        if (strncmp(tokens, cmdGfxtst, strlen(cmdGfxtst)) == 0) {
            // --------------------------------------------------------------------
            // Graphics Mode Test(s)
            // --------------------------------------------------------------------
            clear_screen(LIGHT_GRAY);

            Point p0, p1, p2;
            Point vertex_array[6];

            // Draw pixel test
            for (uint8_t i = 0; i < 100; i ++) {
                for (uint8_t j = 0; j < 100; j++) {
                    p0.X = 200 + j;
                    p0.Y = 200 + i;
                    draw_pixel(p0.X, p0.Y, RED);
                }
            } 

            // Draw line tests
            // Horizontal line
            p0.X = 1920/2 - 100;
            p0.Y = 1080/2 - 100;
            p1.X = 1920/2;
            p1.Y = 1080/2 - 100;
            draw_line(p0, p1, BLACK);

            // Vertical line
            p0.X = 1920/2 + 100;
            p0.Y = 1080/2 - 100;
            p1.X = 1920/2 + 100;
            p1.Y = 1080/2 + 100;
            draw_line(p0, p1, PURPLE);
            
            // Diagonal line up-right
            p0.X = 1920/2 + 150;
            p0.Y = 1080/2;
            p1.X = 1920/2 + 300;
            p1.Y = 1080/2 - 100;
            draw_line(p0, p1, DARK_GRAY);
            
            // Diagonal line down-right
            p0.X = 1920/2 + 350;
            p0.Y = 1080/2 - 100;
            p1.X = 1920/2 + 500;
            p1.Y = 1080/2 + 100;
            draw_line(p0, p1, BLUE);

            // Draw triangle test - right angle
            p0.X = 1920/2 - 600;
            p0.Y = 1080/2 + 100;
            p1.X = 1920/2 - 600;
            p1.Y = 1080/2 + 200;
            p2.X = 1920/2 - 450;
            p2.Y = 1080/2 + 200;
            draw_triangle(p0, p1, p2, 0x00FF7233);  // Sort of "burnt orange"

            // Draw rectangle test
            p0.X = 1920/2 - 400;
            p0.Y = 1080/2 + 100;
            p1.X = 1920/2 - 100;
            p1.Y = 1080/2 + 200;
            draw_rect(p0, p1, 0x0033CEFF);  // Kind of teal maybe

            // Draw polygon test - hexagon
            vertex_array[0].X = 1920/2;
            vertex_array[0].Y = 1080/2 + 100;
            vertex_array[1].X = 1920/2 + 100;
            vertex_array[1].Y = 1080/2 + 100;
            vertex_array[2].X = 1920/2 + 200;
            vertex_array[2].Y = 1080/2 + 150;
            vertex_array[3].X = 1920/2 + 100;
            vertex_array[3].Y = 1080/2 + 200;
            vertex_array[4].X = 1920/2;
            vertex_array[4].Y = 1080/2 + 200;
            vertex_array[5].X = 1920/2 - 100;
            vertex_array[5].Y = 1080/2 + 150;
            draw_polygon(vertex_array, 6, RED);

            // Draw circle test
            p0.X = 1920/2 + 350;
            p0.Y = 1080/2 + 200;
            draw_circle(p0, 50, BLUE);
            
            // Draw ellipse test
            p0.X = 1920/2 - 600;
            p0.Y = 1080/2 + 350;
            draw_ellipse(p0, 100, 50, GREEN - 0x00005500);

            // Fill triangle test
            p0.X = 1920/2 - 400;
            p0.Y = 1080/2 + 300;
            p1.X = 1920/2 - 500;
            p1.Y = 1080/2 + 350;
            p2.X = 1920/2 - 420;
            p2.Y = 1080/2 + 250;
            fill_triangle_solid(p0, p1, p2, 0x006315FD);  // Some sort of dark purple

            // Fill rectangle test
            p0.X = 1920/2 - 350;
            p0.Y = 1080/2 + 300;
            p1.X = 1920/2 - 150;
            p1.Y = 1080/2 + 350;
            fill_rect_solid(p0, p1, 0x0015FDCD);  // Mintish greenish

            // Fill polygon test - hexagon
            vertex_array[0].X = 1920/2 - 50;
            vertex_array[0].Y = 1080/2 + 250;
            vertex_array[1].X = 1920/2 + 50;
            vertex_array[1].Y = 1080/2 + 250;
            vertex_array[2].X = 1920/2 + 100;
            vertex_array[2].Y = 1080/2 + 300;
            vertex_array[3].X = 1920/2 + 50;
            vertex_array[3].Y = 1080/2 + 350;
            vertex_array[4].X = 1920/2 - 50;
            vertex_array[4].Y = 1080/2 + 350;
            vertex_array[5].X = 1920/2 - 100;
            vertex_array[5].Y = 1080/2 + 300;
            fill_polygon_solid(vertex_array, 6, 0x00FD9B15);  // Light orangey

            // Fill circle test
            p0.X = 1920/2 + 200;
            p0.Y = 1080/2 + 270;
            fill_circle_solid(p0, 50, 0x00FF0FE6);  // Magentish fuschish

            // Fill ellipse test
            p0.X = 1920/2 + 400;
            p0.Y = 1080/2 + 350;
            fill_ellipse_solid(p0, 100, 50, 0x00FFEE0F);  // I Love GOOOOOOoooolllddd

            input_char = get_key(); 
            clear_screen(BLUE);
            kernel_cursor_x = 0;
            kernel_cursor_y = 0;
            continue;
        }

        if (strncmp(tokens, cmdHlt, strlen(cmdHlt)) == 0) {
            // --------------------------------------------------------------------
            // End Program  
            // --------------------------------------------------------------------
            // __asm ("cli");   // Clear interrupts
            __asm ("hlt");   // Halt cpu
        }

        if (strncmp(tokens, cmdCls, strlen(cmdCls)) == 0) {
            // --------------------------------------------------------------------
            // Clear Screen
            // --------------------------------------------------------------------
            clear_screen(BLUE);

            // Update cursor values for new position
            kernel_cursor_x = 0;
            kernel_cursor_y = 0;

            continue;
        }

        if (strncmp(tokens, cmdShutdown, strlen(cmdShutdown)) == 0) {
            // --------------------------------------------------------------------
            // Shutdown (QEMU)
            // --------------------------------------------------------------------
            __asm ("outw %%ax, %%dx" : : "a"(0x2000), "d"(0x604) );

            // TODO: Get shutdown command for bochs, user can uncomment which one they want to use
        }

        if (strncmp(tokens, cmdDelFile, strlen(cmdDelFile)) == 0) {
            // --------------------------------------------------------------------
            // Delete a file from the disk
            // --------------------------------------------------------------------
            //	Input 1 - File name to delete
            //	      2 - Length of file name
            // File name is 2nd token in array, each token is 10 char max
            tokens_ptr = tokens + 10;
            for (idx = 0; idx < tokens_length[1]; idx++)
                token_file_name1[idx] = *tokens_ptr++; 

            if (delete_file(token_file_name1, tokens_length[1]) != 0) {
                //	;; TODO: Add error message or code here
            }

            // Print newline when done
            print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0A); 
            print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0D); 

            continue;
        }

        if (strncmp(tokens, cmdRenFile, strlen(cmdRenFile)) == 0) {
            // --------------------------------------------------------------------
            // Rename a file in the file table
            // --------------------------------------------------------------------
            // 1 - File to rename
            // 2 - Length of name to rename
            // 3 - New file name // 4 - New file name length
            // File name is 2nd token in array, each token is 10 char max
            for (idx = 0; idx < tokens_length[1]; idx++)
                token_file_name1[idx] = tokens[10+idx]; 

            // New file name is 3rd token in array, each token is 10 char max
            for (idx = 0; idx < tokens_length[2]; idx++)
                token_file_name2[idx] = tokens[20+idx];

            if (rename_file(token_file_name1, tokens_length[1], token_file_name2, tokens_length[2]) != 0) {
                //	;; TODO: Add error message or code here
            }

            // Print newline when done
            print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0A); 
            print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0D); 

            continue;
        }

        // If command not input, search file table entries for user input file
        // Call load_file function to load program/file to memory address
        // Input 1: File name (address)
        //       2: File name length
        //       3: Memory offset to load file to
        //       4: File extension variable
        // Return value - 0 = Success, !0 = error
        if (load_file(cmdString, tokens_length[0], 0x10000, fileExt) != 0) {
            // Error, program did not load correctly
            print_string(&kernel_cursor_x, &kernel_cursor_y, pgmNotLoaded);
            move_cursor(kernel_cursor_x, kernel_cursor_y);

            continue;
        }

        // Check file extension in file table entry, if 'bin'/binary, far jump & run
        if (strncmp(fileExt, fileBin, 3) == 0) {
            // Void function pointer to jump to and execute code at specific address in C
            ((void (*)(void))0x10000)();     // Execute program, this can return

            // TODO: In the future, if using a backbuffer, restore screen data from that buffer here instead
            //  of clearing
            
            // Clear the screen before going back
            clear_screen(BLUE);

            // Reset cursor position
            kernel_cursor_x = 0;
            kernel_cursor_y = 0;

            continue;   // Loop back to prompt for next input
        }

        // Else print text file to screen
        // TODO: Put this behind a "shell" command like 'typ'/'type' or other
        txt_file_ptr = (uint8_t *)0x10000;   // File location to print from

        // Print newline first
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0A);
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0D);
        
        // Get size of filesize in bytes (512 byte per sector)
        // TODO: Change this later - currently assuming text files are only 1 sector
        //	<fileSize> * 512 
        
        // print_file_char:
        for (idx = 0; idx < 512; idx++) {
            // TODO: Handle newlines (byte 0x0A in txt file data)
            if (*txt_file_ptr <= 0x0F)          // Convert to hex
                *txt_file_ptr = hex_to_ascii(*txt_file_ptr);

            // Print file character to screen
            print_char(&kernel_cursor_x, &kernel_cursor_y, *txt_file_ptr);

            txt_file_ptr++;
        }

        // Print newline after printing file contents
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0A);
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0D);
    }
}
