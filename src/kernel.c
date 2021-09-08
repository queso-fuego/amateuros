// =========================================================================== 
// Kernel.c: basic 'kernel' loaded from 2nd stage bootloader
// ===========================================================================
#include "../include/C/stdint.h"
#include "../include/C/string.h"
#include "../include/gfx/2d_gfx.h"
#include "../include/screen/clear_screen.h"
#include "../include/print/print_types.h"
#include "../include/screen/cursor.h"
#include "../include/keyboard/get_key.h"
#include "../include/print/print_registers.h"
#include "../include/memory/physical_memory_manager.h"
#include "../include/disk/file_ops.h"
#include "../include/print/print_fileTable.h"
#include "../include/type_conversions/hex_to_ascii.h"
#include "../include/interrupts/idt.h"
#include "../include/interrupts/exceptions.h"
#include "../include/interrupts/syscalls.h"
#include "../include/interrupts/pic.h"
#include "../include/ports/io.h"

// Constants
#define MEMMAP_AREA 0x30000
// TODO: Fill out more constants to replace magic numbers

void print_physical_memory_info(void);  // Print information from the physical memory map (SMAP)

// Physical memory map entry from Bios Int 15h EAX E820h
typedef struct SMAP_entry {
    uint64_t base_address;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__ ((packed)) SMAP_entry_t;

uint16_t kernel_cursor_x = 0;   // Text cursor X position
uint16_t kernel_cursor_y = 0;   // Text cursor Y position

__attribute__ ((section ("kernel_entry"))) void kernel_main(void)
{
    uint8_t tokens[50];             // tokens array, equivalent-ish to tokens[5][10]
    uint8_t *tokens_ptr = tokens;
    uint16_t tokens_length[5];      // length of each token in tokens array (0-10)
    uint16_t *tokens_length_ptr = tokens_length;
    uint8_t token_count;            // How many tokens did the user enter?
    uint8_t token_file_name1[11] = {0};   // Filename 1 for commands
    uint8_t token_file_name2[11] = {0};   // Filename 2 for commands
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
    uint8_t *cmdPrtmemmap = "prtmemmap\0";  // Print physical memory map info
    uint8_t *cmdChgColors = "chgColors\0";  // Change current fg/bg colors
    uint8_t *cmdChgFont   = "chgFont\0";    // Change current font
    uint8_t fileExt[3];
    uint8_t *fileBin = "bin\0";
    uint8_t *fileTxt = "txt\0";
    uint8_t fileSize = 0;
    uint8_t *file_ptr;
    uint8_t *windowsMsg     = "\x0A\x0D" "Oops! Something went wrong :(" "\x0A\x0D\0";
    uint8_t *notFoundString = "\x0A\x0D" "Program/file not found!, Try again? (Y)" "\x0A\x0D\0";
    uint8_t *sectNotFound   = "\x0A\x0D" "Sector not found!, Try again? (Y)" "\x0A\x0D\0";
    uint8_t *menuString     = "------------------------------------------------------\x0A\x0D"
                              "Kernel Booted, Welcome to QuesOS - 32 Bit 'C' Edition!\x0A\x0D"
                              "------------------------------------------------------\x0A\x0D\x0A\x0D\0";
    uint8_t *failure        = "\x0A\x0D" "Command/Program not found, Try again" "\x0A\x0D\0";
    uint8_t *prompt         = ">:\0";
    uint8_t *pgmNotLoaded   = "\x0A\x0D" "Program found but not loaded, Try Again" "\x0A\x0D\0";
    uint8_t *fontNotFound   = "\x0A\x0D" "Font not found!" "\x0A\x0D\0";

    uint32_t num_SMAP_entries; 
    uint32_t total_memory; 
    SMAP_entry_t *SMAP_entry;
    uint32_t needed_blocks;
    uint32_t *allocated_address;
    uint8_t font_width  = *(uint8_t *)FONT_WIDTH;
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;

    // --------------------------------------------------------------------
    // Initial setup
    // --------------------------------------------------------------------
    // Set intial colors
    if (gfx_mode->bits_per_pixel > 8) {
        user_gfx_info->fg_color = convert_color(0x00EEEEEE);
        user_gfx_info->bg_color = convert_color(0x00222222);
    } else {
        // Assuming VGA palette
        user_gfx_info->fg_color = convert_color(0x02);
        user_gfx_info->bg_color = convert_color(0x00);
    }

    // Clear the screen
    clear_screen(user_gfx_info->bg_color);

    // Print OS boot message
    print_string(&kernel_cursor_x, &kernel_cursor_y, menuString);

    // Physical memory manager setup
    num_SMAP_entries = *(uint32_t *)0x8500;
    SMAP_entry       = (SMAP_entry_t *)0x8504;
    SMAP_entry += num_SMAP_entries - 1;

    total_memory = SMAP_entry->base_address + SMAP_entry->length - 1;

    // Initialize physical memory manager to all available memory, put it at some location
    // All memory will be set as used/reserved by default
    initialize_memory_manager(MEMMAP_AREA, total_memory);

    // TODO: First free location seems to be at 0x20000, find why, is this wrong? Should be at 0xA000

    // Initialize memory regions for the available memory regions in the SMAP (type = 1)
    SMAP_entry = (SMAP_entry_t *)0x8504;
    //for (uint32_t i = 0; i < num_SMAP_entries; i++, SMAP_entry++) DEBUGGING
    //    if (SMAP_entry->type == 1)
    //        initialize_memory_region(SMAP_entry->base_address, SMAP_entry->length);
    for (uint32_t i = 0; i < num_SMAP_entries; i++) {
        if (SMAP_entry->type == 1)
            initialize_memory_region(SMAP_entry->base_address, SMAP_entry->length);

        SMAP_entry++;
    }

    // Set memory regions/blocks for the kernel and "OS" memory map areas as used/reserved
    deinitialize_memory_region(0x1000, 0xB000);                            // Reserve all memory below C000h for the kernel/OS
    deinitialize_memory_region(MEMMAP_AREA, max_blocks / BLOCKS_PER_BYTE); // Reserve physical memory map area 

    // Successfully set up and initialized the physical memory manager
    print_string(&kernel_cursor_x, &kernel_cursor_y, "Physical Memory Manager initialized.\x0A\x0D\x0A\x0D");

    // Print memory map info
    print_physical_memory_info();

    // Set up interrupts
    init_idt_32();  

    // Set up exception handlers (ISRs 0-31)
    set_idt_descriptor_32(0, div_by_0_handler, TRAP_GATE_FLAGS); // Divide by 0 error, ISR 0
    // Set up additional exceptions here...
    
    // Set up software interrupts
    set_idt_descriptor_32(0x80, syscall_dispatcher, INT_GATE_USER_FLAGS);  // System call handler/dispatcher
    // Set up additional interrupts here...

    // Debugging/testing system calls, uncomment these lines to test
    __asm__ __volatile__ ("movl $0, %eax; int $0x80");
    __asm__ __volatile__ ("movl $1, %eax; int $0x80");

    __asm__ __volatile__ ("movl $2, %eax; int $0x80");   // Should do nothing

    // Mask off all hardware interrupts (disable the PIC)
    disable_pic();

    // Remap PIC interrupts to after exceptions (PIC1 starts at 0x20)
    remap_pic();

    // TODO: Add ISRs for PIC hardware interrupts
    //set_idt_descriptor_32(0x20, <timer_irq_handler>, INT_FLAGS);  
    //set_idt_descriptor_32(0x21, <keyboard_irq_handler>, INT_FLAGS);
    // Put more PIC IRQ handlers here...
    
    // TODO: Enable PIC IRQ interrupts after setting their descriptors
    // clear_irq_mask(0); // Enable timer (will tick every ~18.2/s)
    // clear_irq_mask(1); // Enable keyboard interrupts

    // TODO: After setting up hardware interrupts & PIC, set IF to enable 
    //   non-exception and not NMI hardware interrupts
    __asm__ __volatile__("sti");

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
            clear_screen(convert_color(LIGHT_GRAY));

            Point p0, p1, p2;
            Point vertex_array[6];

            // Draw pixel test
            for (uint8_t i = 0; i < 100; i ++) {
                for (uint8_t j = 0; j < 100; j++) {
                    p0.X = 200 + j;
                    p0.Y = 200 + i;
                    draw_pixel(p0.X, p0.Y, convert_color(RED));
                }
            } 

            // Draw line tests
            // Horizontal line
            p0.X = 1920/2 - 100;
            p0.Y = 1080/2 - 100;
            p1.X = 1920/2;
            p1.Y = 1080/2 - 100;
            draw_line(p0, p1, convert_color(BLACK));

            // Vertical line
            p0.X = 1920/2 + 100;
            p0.Y = 1080/2 - 100;
            p1.X = 1920/2 + 100;
            p1.Y = 1080/2 + 100;
            draw_line(p0, p1, convert_color(PURPLE));
            
            // Diagonal line up-right
            p0.X = 1920/2 + 150;
            p0.Y = 1080/2;
            p1.X = 1920/2 + 300;
            p1.Y = 1080/2 - 100;
            draw_line(p0, p1, convert_color(DARK_GRAY));
            
            // Diagonal line down-right
            p0.X = 1920/2 + 350;
            p0.Y = 1080/2 - 100;
            p1.X = 1920/2 + 500;
            p1.Y = 1080/2 + 100;
            draw_line(p0, p1, convert_color(BLUE));

            // Draw triangle test - right angle
            p0.X = 1920/2 - 600;
            p0.Y = 1080/2 + 100;
            p1.X = 1920/2 - 600;
            p1.Y = 1080/2 + 200;
            p2.X = 1920/2 - 450;
            p2.Y = 1080/2 + 200;
            draw_triangle(p0, p1, p2, convert_color(0x00FF7233));  // Sort of "burnt orange"

            // Draw rectangle test
            p0.X = 1920/2 - 400;
            p0.Y = 1080/2 + 100;
            p1.X = 1920/2 - 100;
            p1.Y = 1080/2 + 200;
            draw_rect(p0, p1, convert_color(0x0033CEFF));  // Kind of teal maybe

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
            draw_polygon(vertex_array, 6, convert_color(RED));

            // Draw circle test
            p0.X = 1920/2 + 350;
            p0.Y = 1080/2 + 200;
            draw_circle(p0, 50, convert_color(BLUE));
            
            // Draw ellipse test
            p0.X = 1920/2 - 600;
            p0.Y = 1080/2 + 350;
            draw_ellipse(p0, 100, 50, convert_color(GREEN - 0x00005500));

            // Fill triangle test
            p0.X = 1920/2 - 400;
            p0.Y = 1080/2 + 300;
            p1.X = 1920/2 - 500;
            p1.Y = 1080/2 + 350;
            p2.X = 1920/2 - 420;
            p2.Y = 1080/2 + 250;
            fill_triangle_solid(p0, p1, p2, convert_color(0x006315FD));  // Some sort of dark purple

            // Fill rectangle test
            p0.X = 1920/2 - 350;
            p0.Y = 1080/2 + 300;
            p1.X = 1920/2 - 150;
            p1.Y = 1080/2 + 350;
            fill_rect_solid(p0, p1, convert_color(0x0015FDCD));  // Mintish greenish

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
            fill_polygon_solid(vertex_array, 6, convert_color(0x00FD9B15));  // Light orangey

            // Fill circle test
            p0.X = 1920/2 + 200;
            p0.Y = 1080/2 + 270;
            fill_circle_solid(p0, 50, convert_color(0x00FF0FE6));  // Magentish fuschish

            // Fill ellipse test
            p0.X = 1920/2 + 400;
            p0.Y = 1080/2 + 350;
            fill_ellipse_solid(p0, 100, 50, convert_color(0x00FFEE0F));  // I Love GOOOOOOoooolllddd

            input_char = get_key(); 
            clear_screen(user_gfx_info->bg_color);  
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
            clear_screen(user_gfx_info->bg_color); 

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

        // Print memory map command
        if (strncmp(tokens, cmdPrtmemmap, strlen(cmdPrtmemmap)) == 0) {
            // Print out physical memory map info
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D-------------------\x0A\x0DPhysical Memory Map"
                                                             "\x0A\x0D-------------------\x0A\x0D\x0A\x0D");
            print_physical_memory_info();
            continue;
        }

        // Change colors command
        if (strncmp(tokens, cmdChgColors, strlen(cmdChgColors)) == 0) {
            uint32_t fg_color = 0;
            uint32_t bg_color = 0;

            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Current colors (32bpp ARGB):");
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Foregound: ");
            print_hex(&kernel_cursor_x, &kernel_cursor_y, user_gfx_info->fg_color);
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Backgound: ");
            print_hex(&kernel_cursor_x, &kernel_cursor_y, user_gfx_info->bg_color);

            // Foreground color
            if (gfx_mode->bits_per_pixel > 8)
                print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "New Foregound (0xRRGGBB): 0x");
            else
                print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "New Foregound (0xNN): 0x");

            move_cursor(kernel_cursor_x, kernel_cursor_y);

            while ((input_char = get_key()) != 0x0D) {
                if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

                print_char(&kernel_cursor_x, &kernel_cursor_y, input_char);
                move_cursor(kernel_cursor_x, kernel_cursor_y);

                fg_color *= 16;
                if      (input_char >= '0' && input_char <= '9') fg_color += input_char - '0';          // Convert hex ascii 0-9 to integer
                else if (input_char >= 'A' && input_char <= 'F') fg_color += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
            }

            remove_cursor(kernel_cursor_x, kernel_cursor_y);

            // Background color
            if (gfx_mode->bits_per_pixel > 8)
                print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "New Backgound (0xRRGGBB): 0x");
            else
                print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "New Backgound (0xNN): 0x");

            move_cursor(kernel_cursor_x, kernel_cursor_y);

            while ((input_char = get_key()) != 0x0D) {
                if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

                print_char(&kernel_cursor_x, &kernel_cursor_y, input_char);
                move_cursor(kernel_cursor_x, kernel_cursor_y);

                bg_color *= 16;

                if      (input_char >= '0' && input_char <= '9') bg_color += input_char - '0';          // Convert hex ascii 0-9 to integer
                else if (input_char >= 'A' && input_char <= 'F') bg_color += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
            }

            remove_cursor(kernel_cursor_x, kernel_cursor_y);

            user_gfx_info->fg_color = convert_color(fg_color);  // Convert colors first before setting new values
            user_gfx_info->bg_color = convert_color(bg_color);

            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D");

            continue;
        }

        // Change font command; Usage: chgFont <name of font>
        if (strncmp(tokens, cmdChgFont, strlen(cmdChgFont)) == 0) {
            // First check if font exists - name is 2nd token
            file_ptr = check_filename(tokens+10, tokens_length[1]);
            if (*file_ptr == 0) {  
                print_string(&kernel_cursor_x, &kernel_cursor_y, fontNotFound);  // File not found in filetable, error
                move_cursor(kernel_cursor_x, kernel_cursor_y);

                continue;
            }

            // Check if file has .fnt extension
            if (strncmp(file_ptr+10, "fnt", 3) != 0) {
                print_string(&kernel_cursor_x, &kernel_cursor_y, "\r\nError: file is not a font\r\n"); 
                move_cursor(kernel_cursor_x, kernel_cursor_y);

                continue;
            }

            // Save file name
            strncpy(token_file_name1, file_ptr, 10);
            token_file_name1[10] = '\0';

            // File is a valid font, try to load it to memory
            if (load_file(tokens+10, tokens_length[1], (uint32_t)FONT_ADDRESS, fileExt) != 0) {
                print_string(&kernel_cursor_x, &kernel_cursor_y, "\r\nError: file could not be loaded\r\n"); 
                move_cursor(kernel_cursor_x, kernel_cursor_y);

                continue;
            }

            // New font should be loaded and in use now
            font_width  = *(uint8_t *)FONT_WIDTH;
            font_height = *(uint8_t *)FONT_HEIGHT;
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\r\nFont loaded: \r\n");
            print_string(&kernel_cursor_x, &kernel_cursor_y, token_file_name1);
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\r\nWidth: ");
            print_dec(&kernel_cursor_x, &kernel_cursor_y, font_width);
            print_string(&kernel_cursor_x, &kernel_cursor_y, " Height: ");
            print_dec(&kernel_cursor_x, &kernel_cursor_y, font_height);
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\r\n");

            move_cursor(kernel_cursor_x, kernel_cursor_y);

            continue;
        }

        // If command not input, search file table entries for user input file
        file_ptr = check_filename(cmdString, tokens_length[0]);
        if (*file_ptr == 0) {  
            print_string(&kernel_cursor_x, &kernel_cursor_y, failure);  // File not found in filetable, error
            move_cursor(kernel_cursor_x, kernel_cursor_y);

            continue;
        }

        // file_ptr is pointing to filetable entry, get number of blocks needed to load the file
        //   num_blocks = (file size in sectors * # of bytes in a sector) / size of a block in bytes
        needed_blocks = (file_ptr[15] * 512) / BLOCK_SIZE;  // Convert file size in bytes to blocks
        if (needed_blocks == 0) needed_blocks = 1;          // If file size < 1 block, use 1 block default

        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Allocating ");
        print_dec(&kernel_cursor_x, &kernel_cursor_y, needed_blocks);
        print_string(&kernel_cursor_x, &kernel_cursor_y, " block(s)\x0A\x0D");
        
        allocated_address = allocate_blocks(needed_blocks); // Allocate 4KB blocks of memory, get address to memory

        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Allocated to address ");
        print_hex(&kernel_cursor_x, &kernel_cursor_y, (uint32_t)allocated_address);
        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D");
        
        // Call load_file function to load program/file to memory address
        // Input 1: File name (address)
        //       2: File name length
        //       3: Memory offset to load file to
        //       4: File extension variable
        // Return value - 0 = Success, !0 = error
        if (load_file(cmdString, tokens_length[0], (uint32_t)allocated_address, fileExt) != 0) {
            // Error, program did not load correctly
            print_string(&kernel_cursor_x, &kernel_cursor_y, pgmNotLoaded);
            move_cursor(kernel_cursor_x, kernel_cursor_y);

            continue;
        }

        // Check file extension in file table entry, if 'bin'/binary, far jump & run
        if (strncmp(fileExt, fileBin, 3) == 0) {
            // Void function pointer to jump to and execute code at specific address in C
            ((void (*)(void))allocated_address)();     // Execute program, this can return

            // TODO: In the future, if using a backbuffer, restore screen data from that buffer here instead
            //  of clearing
            
            // Clear the screen before going back
            clear_screen(user_gfx_info->bg_color);  

            // Reset cursor position
            kernel_cursor_x = 0;
            kernel_cursor_y = 0;

            // Free memory when done
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Freeing ");
            print_dec(&kernel_cursor_x, &kernel_cursor_y, needed_blocks);
            print_string(&kernel_cursor_x, &kernel_cursor_y, " block(s)\x0A\x0D");
            
            free_blocks(allocated_address, needed_blocks); // Free previously allocated blocks

            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Freed at address ");
            print_hex(&kernel_cursor_x, &kernel_cursor_y, (uint32_t)allocated_address);
            print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D");

            continue;   // Loop back to prompt for next input
        }

        // Else print text file to screen
        // TODO: Put this behind a "shell" command like 'typ'/'type' or other
        file_ptr = (uint8_t *)allocated_address;   // File location to print from

        // Print newline first
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0A);
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0D);
        
        // Get size of filesize in bytes (512 byte per sector)
        // TODO: Change this later - currently assuming text files are only 1 sector
        //	<fileSize> * 512 
        
        // print_file_char:
        for (idx = 0; idx < 512; idx++) {
            // TODO: Handle newlines (byte 0x0A in txt file data)
            if (*file_ptr <= 0x0F)          // Convert to hex
                *file_ptr = hex_to_ascii(*file_ptr);

            // Print file character to screen
            print_char(&kernel_cursor_x, &kernel_cursor_y, *file_ptr);

            file_ptr++;
        }

        // Print newline after printing file contents
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0A);
        print_char(&kernel_cursor_x, &kernel_cursor_y, 0x0D);

        // Free memory when done
        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Freeing ");
        print_dec(&kernel_cursor_x, &kernel_cursor_y, needed_blocks);
        print_string(&kernel_cursor_x, &kernel_cursor_y, " block(s)\x0A\x0D");
        
        free_blocks(allocated_address, needed_blocks); // Free previously allocated blocks

        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Freed at address ");
        print_hex(&kernel_cursor_x, &kernel_cursor_y, (uint32_t)allocated_address);
        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D");
    }
}

// Print information from the physical memory map (SMAP)
void print_physical_memory_info(void)  
{
    uint32_t num_entries = *(uint32_t *)0x8500;         // Number of SMAP entries
    SMAP_entry_t *SMAP_entry = (SMAP_entry_t *)0x8504;  // Memory map entries start point

    for (uint32_t i = 0; i < num_entries; i++) {
        print_string(&kernel_cursor_x, &kernel_cursor_y, "Region ");
        print_hex(&kernel_cursor_x, &kernel_cursor_y, i);

        print_string(&kernel_cursor_x, &kernel_cursor_y, ": base: ");
        print_hex(&kernel_cursor_x, &kernel_cursor_y, SMAP_entry->base_address);

        print_string(&kernel_cursor_x, &kernel_cursor_y, " length: ");
        print_hex(&kernel_cursor_x, &kernel_cursor_y, SMAP_entry->length);

        print_string(&kernel_cursor_x, &kernel_cursor_y, " type: ");
        print_hex(&kernel_cursor_x, &kernel_cursor_y, SMAP_entry->type);

        switch(SMAP_entry->type) {
            case 1:
                print_string(&kernel_cursor_x, &kernel_cursor_y, " (Available)");
                break;
            case 2: 
                print_string(&kernel_cursor_x, &kernel_cursor_y, " (Reserved)");
                break;
            case 3: 
                print_string(&kernel_cursor_x, &kernel_cursor_y, " (ACPI Reclaim)");
                break;
            case 4: 
                print_string(&kernel_cursor_x, &kernel_cursor_y, " (ACPI NVS Memory)");
                break;
            default: 
                print_string(&kernel_cursor_x, &kernel_cursor_y, " (Reserved)");
                break;
        }

        print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D");
        SMAP_entry++;   // Go to next entry
    }

    // Print total amount of memory
    print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D");
    print_string(&kernel_cursor_x, &kernel_cursor_y, "Total memory in bytes: ");

    SMAP_entry--;   // Get last SMAP entry
    print_hex(&kernel_cursor_x, &kernel_cursor_y, SMAP_entry->base_address + SMAP_entry->length - 1);
  
    // TODO: Print out memory manager block info:
    //   total memory in 4KB blocks, total # of used blocks, total # of free blocks
    print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0DTotal 4KB blocks: ");
    print_dec(&kernel_cursor_x, &kernel_cursor_y, max_blocks);

    print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0DUsed or reserved blocks: ");
    print_dec(&kernel_cursor_x, &kernel_cursor_y, used_blocks);

    print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D" "Free or available blocks: ");
    print_dec(&kernel_cursor_x, &kernel_cursor_y, max_blocks - used_blocks);

    print_string(&kernel_cursor_x, &kernel_cursor_y, "\x0A\x0D\x0A\x0D");
}


