// =========================================================================== 
// Kernel.c: basic 'kernel' loaded from 3rd stage bootloader
// ===========================================================================
#include "C/stdint.h"
#include "C/stdlib.h"
#include "C/string.h"
#include "C/stdio.h"
#include "C/time.h"
#include "C/ctype.h"
#include "global/global_addresses.h"
#include "gfx/2d_gfx.h"
#include "screen/clear_screen.h"
#include "screen/cursor.h"
#include "print/print_registers.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"
#include "memory/malloc.h"
#include "disk/file_ops.h"
#include "print/print_fileTable.h"
#include "type_conversions/hex_to_ascii.h"
#include "interrupts/idt.h"
#include "interrupts/exceptions.h"
#include "interrupts/pic.h"
#include "interrupts/syscalls.h"
#include "ports/io.h"
#include "keyboard/keyboard.h"
#include "sound/pc_speaker.h"
#include "sys/syscall_numbers.h"
#include "sys/syscall_wrappers.h"
#include "fs/fs_impl.h"

// Open file table & inode table pointers
open_file_table_t *open_file_table;
uint32_t max_open_files;
uint32_t current_open_files;

inode_t *open_inode_table;
uint32_t max_open_inodes;
uint32_t current_open_inodes;

uint32_t next_available_file_virtual_address;

// Forward function declarations
void print_physical_memory_info(void);  // Print information from the physical memory map (SMAP)
void init_open_file_table(void);                                        
void init_open_inode_table(void);                                        
void init_fs_vars(void);

__attribute__ ((section ("kernel_entry"))) void kernel_main(void)
{
    char cmdString[256] = {0};         // User input string  
    char *cmdString_ptr = cmdString;
    uint8_t input_char   = 0;       // User input character
    uint8_t input_length;           // Length of user input
    uint16_t idx         = 0;
    uint8_t *cmdDir      = "dir";         // Directory command; list all files/pgms on disk
    uint8_t *cmdReboot   = "reboot";      // 'warm' reboot option
    uint8_t *cmdPrtreg   = "prtreg";      // Print register values
    uint8_t *cmdGfxtst   = "gfxtst";      // Graphics mode test
    uint8_t *cmdHlt      = "hlt";         // E(n)d current program by halting cpu
    uint8_t *cmdCls	     = "cls";         // Clear screen by scrolling
    uint8_t *cmdShutdown = "shutdown";    // Close QEMU emulator
    uint8_t *cmdDelFile  = "del";		    // Delete a file from disk
    uint8_t *cmdRenFile  = "ren";         // Rename a file in the file table
    uint8_t *cmdPrtmemmap = "prtmemmap";  // Print physical memory map info
    uint8_t *cmdChgColors = "chgcolors";  // Change current fg/bg colors
    uint8_t *cmdChgFont   = "chgfont";    // Change current font
    uint8_t *cmdSleep     = "sleep";      // Sleep for a # of seconds
    uint8_t *cmdMSleep    = "msleep";     // Sleep for a # of milliseconds
    uint8_t *cmdShowDateTime = "datetime"; // Show CMOS RTC date/time values
    uint8_t *cmdSoundTest = "soundtest";   // Test pc speaker square wave sound
    uint8_t *cmdOpenTest = "opentest";     // DEBUGGING, but may keep
    uint8_t *cmdCloseTest = "closetest";   // DEBUGGING, but may keep
    uint8_t fileExt[3];
    uint8_t *fileBin = "bin";
    uint8_t *file_ptr;
    //uint8_t *windowsMsg     = "\r\n" "Oops! Something went wrong :(" "\r\n\0";
    uint8_t *menuString     = "------------------------------------------------------\r\n"
                              "Kernel Booted, Welcome to QuesOS - 32 Bit 'C' Edition!\r\n"
                              "------------------------------------------------------\r\n\r\n\0";
    uint8_t *failure        = "\r\n" "Command/Program not found, Try again" "\r\n\0";
    uint8_t *prompt         = ">";
    uint8_t *pgmNotLoaded   = "\r\n" "Program/file found but not loaded, Try Again" "\r\n\0";
    uint8_t *fontNotFound   = "\r\n" "Font not found!" "\r\n\0";

    uint32_t needed_pages;
    uint8_t font_width  = *(uint8_t *)FONT_WIDTH;
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;
    int argc = 0;
    char *argv[10] = {0};

    // --------------------------------------------------------------------
    // Initial hardware, interrupts, etc. setup
    // --------------------------------------------------------------------
    // Get & set current kernel page directory
    current_page_directory = (page_directory *)*(uint32_t *)CURRENT_PAGE_DIR_ADDRESS;

    // Set physical memory manager variables
    memory_map  = (uint32_t *)MEMMAP_AREA;
    max_blocks  = *(uint32_t *)PHYS_MEM_MAX_BLOCKS;
    used_blocks = *(uint32_t *)PHYS_MEM_USED_BLOCKS;

    // Set up interrupts
    init_idt_32();  

    // Set up exception handlers (ISRs 0-31)
    set_idt_descriptor_32(0, div_by_0_handler, TRAP_GATE_FLAGS); // Divide by 0 error #DE, ISR 0
    set_idt_descriptor_32(14, page_fault_handler, TRAP_GATE_FLAGS); // Page fault #PF errors, ISR 14
    
    // Set up software interrupts
    set_idt_descriptor_32(0x80, syscall_dispatcher, INT_GATE_USER_FLAGS);  // System call handler/dispatcher

    // Mask off all hardware interrupts (disable the PIC)
    disable_pic();

    // Remap PIC interrupts to after exceptions (PIC1 starts at 0x20)
    remap_pic();

    // Add ISRs for PIC hardware interrupts
    set_idt_descriptor_32(0x20, timer_irq0_handler, INT_GATE_FLAGS);  
    set_idt_descriptor_32(0x21, keyboard_irq1_handler, INT_GATE_FLAGS);
    set_idt_descriptor_32(0x28, cmos_rtc_irq8_handler, INT_GATE_FLAGS);
    
    // Clear out PS/2 keyboard buffer: check status register and read from data port
    //   until clear
    // This may fix not getting keys on boot
    while (inb(0x64) & 1) inb(0x60);    // Status register 0x64 bit 0 is set, output buffer is full

    // Enable PIC IRQ interrupts after setting their descriptors
    clear_irq_mask(0); // Enable timer (will tick every ~18.2/s)
    clear_irq_mask(1); // Enable keyboard IRQ1, keyboard interrupts
    clear_irq_mask(2); // Enable PIC2 line
    clear_irq_mask(8); // Enable CMOS RTC IRQ8
    
    // Enable CMOS RTC
    enable_rtc();

    // Set default PIT Timer IRQ0 rate - ~1000hz
    // 1193182 MHZ / 1193 = ~1000
    set_pit_channel_mode_frequency(0, 2, 1193);

    // After setting up hardware interrupts & PIC, set IF to enable 
    //   non-exception and not NMI hardware interrupts
    __asm__ __volatile__("sti");

    // Set up kernel malloc variables for e.g. printf() calls
    uint32_t kernel_malloc_virt_address = 0x300000;
    uint32_t kernel_malloc_phys_address = (uint32_t)allocate_blocks(1);
    uint32_t kernel_total_malloc_pages  = 1;

    map_page((void *)kernel_malloc_phys_address, (void *)kernel_malloc_virt_address);
    pt_entry *kernel_malloc_page = get_page(kernel_malloc_virt_address);
    SET_ATTRIBUTE(kernel_malloc_page, PTE_READ_WRITE);  // Add read/write flags for malloc-ed memory

    malloc_block_t *kernel_malloc_list_head = (malloc_block_t *)kernel_malloc_virt_address;

    kernel_malloc_list_head->size = PAGE_SIZE - sizeof(malloc_block_t);
    kernel_malloc_list_head->free = true;
    kernel_malloc_list_head->next = 0;
    
    malloc_list_head    = kernel_malloc_list_head;
    malloc_virt_address = kernel_malloc_virt_address;
    malloc_phys_address = kernel_malloc_phys_address;
    total_malloc_pages  = kernel_total_malloc_pages;

    // Set up initial file system variables
    init_fs_vars();

    // Set up kernel/system open file table and open inode table
    init_open_file_table();
    init_open_inode_table();

    // Set intial colors
    while (!user_gfx_info->fg_color) {
        if (gfx_mode->bits_per_pixel > 8) {
            printf("\eFG%xBG%x;", convert_color(0x00EEEEEE), convert_color(0x00222222)); 
            user_gfx_info->fg_color = convert_color(0x00EEEEEE);
            user_gfx_info->bg_color = convert_color(0x00222222);
        } else {
            // Assuming VGA palette
            printf("\eFG%xBG%x;", convert_color(0x02), convert_color(0x00)); 
            user_gfx_info->fg_color = convert_color(0x02);
            user_gfx_info->bg_color = convert_color(0x00);
        }
    }

    // Clear the screen
    clear_screen_esc();

    // Print OS boot message
    printf("\eCSROFF;%s", menuString);

    // --------------------------------------------------------------------
    // Get user input, print to screen & run command/program  
    // --------------------------------------------------------------------
    // TODO: Change printing everywhere to use printf/write instead of print_types functions, 
    //   then remove kernel cursor variables
    printf("\eX%dY%d;\eCSROFF;TESTING PRINTF; Char: %c, Int: %d, String: %s, Hex: %x, %: %%\r\n", 0, 3, 'f', 123, "Hello", 0xAB12);

    while (1) {
        // Reset tokens data, arrays, and variables for next input line
        memset(cmdString, 0, sizeof cmdString);

        // Print prompt
        printf("\eCSRON;%s%s", current_dir, prompt);
        
        input_length = 0;   // reset byte counter of input

        // Key loop - get input from user
        while (1) {
            input_char = get_key();     // Get ascii char from scancode from keyboard data port 60h

            if (input_char == '\r') {   // enter key?
                printf("\eCSROFF; ");
                break;                  // go on to tokenize user input line
            }

            // TODO: Move all input back 1 char/byte after backspace, if applicable
            if (input_char == '\b') {       // backspace?
                if (input_length > 0) {                // Did user input anything?
                    input_length--;                    // yes, go back one char in cmdString
                    cmdString[input_length] = '\0';    // Blank out character    					

                    // Do a "visual" backspace; Move cursor back 1 space, print 2 spaces, move back 2 spaces
                    printf("\eBS;  \eBS;\eBS;");
                }

                continue;   // Get next character
            }

            cmdString[input_length] = input_char;   // Store input char to string
            input_length++;                         // Increment byte counter of input

            // Print input character to screen
            putc(input_char);
        }

        if (input_length == 0) {
            // No input or command not found! boo D:
            printf(failure);

            continue;
        }

        cmdString[input_length] = '\0';     // else null terminate cmdString from si

        // Tokenize input string "cmdString" into separate tokens
        cmdString_ptr = cmdString;      // Reset pointers...
        argc = 0;                       // Reset argument count
        memset(argv, 0, sizeof argv);

        // Get token loop
        while (*cmdString_ptr != '\0') { 
            // Skip whitespace between tokens
            while (isspace(*cmdString_ptr)) *cmdString_ptr++ = '\0';

            // Found next non space character, start of next input token/argument
            argv[argc++] = cmdString_ptr; 

            // Go to next space or end of string
            while (!isspace(*cmdString_ptr) && *cmdString_ptr != '\0') 
                cmdString_ptr++;
        }

        // Check commands 
        // Get first token (command to run) & second token (if applicable e.g. file name)
        if (strncmp(argv[0], cmdDir, strlen(cmdDir)) == 0) {
            if (!argv[1]) {
                if (!print_dir(current_dir))
                    printf("\r\nError printing dir %s\r\n", current_dir);
            } else {
                if (!print_dir(argv[1])) 
                    printf("\r\nError printing dir %s\r\n", argv[1]);
            }

            continue;
        }

        // Test Open() syscall
        if (strncmp(argv[0], cmdOpenTest, strlen(cmdOpenTest)) == 0) {
            const char file[] = "opentest.txt";

            if (open(file, O_CREAT) != -1) {
                printf("\r\nCreated file %s\r\n", file);
            } else {
                printf("\r\nError: could not create file %s\r\n", file);
            }

            continue;
        }

        // Test close() syscall
        if (strncmp(argv[0], cmdCloseTest, strlen(cmdCloseTest)) == 0) {
            const char file[] = "closetest.txt";

            int32_t fd = open(file, O_CREAT);
            if (fd == -1)
                printf("\r\nError: could not create file %s\r\n", file);
            else
                printf("\r\nCreated file %s\r\n", file);

            if (close(fd) == 0) {
                printf("Closed file %s\r\n", file);
            } else {
                printf("Error: could not close file %s\r\n", file);
            }

            continue;
        }

        if (strncmp(argv[0], cmdReboot, strlen(cmdReboot)) == 0) {
            // --------------------------------------------------------------------
            // Reboot: Reboot the PC
            // --------------------------------------------------------------------
            outb(0x64, 0xFE);  // Send "Reset CPU" command to PS/2 keyboard controller port
        }

        if (strncmp(argv[0], cmdPrtreg, strlen(cmdPrtreg)) == 0) {
            // --------------------------------------------------------------------
            // Print Register Values
            // --------------------------------------------------------------------
            print_registers(); 
            continue;   
        }

        if (strncmp(argv[0], cmdGfxtst, strlen(cmdGfxtst)) == 0) {
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
            // TODO: Add back in later when no floating point
            //p0.X = 1920/2 - 600;
            //p0.Y = 1080/2 + 350;
            //draw_ellipse(p0, 100, 50, convert_color(GREEN - 0x00005500));

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
            // TODO: Add back in later when no floating point
            //p0.X = 1920/2 + 400;
            //p0.Y = 1080/2 + 350;
            //fill_ellipse_solid(p0, 100, 50, convert_color(0x00FFEE0F));  // I Love GOOOOOOoooolllddd

            input_char = get_key(); 
            clear_screen_esc();

            continue;
        }

        if (strncmp(argv[0], cmdHlt, strlen(cmdHlt)) == 0) {
            // --------------------------------------------------------------------
            // End Program  
            // --------------------------------------------------------------------
            __asm__ ("cli;hlt");   // Clear interrupts & Halt cpu
        }

        if (strncmp(argv[0], cmdCls, strlen(cmdCls)) == 0) {
            // --------------------------------------------------------------------
            // Clear Screen
            // --------------------------------------------------------------------
            clear_screen_esc();

            continue;
        }

        if (strncmp(argv[0], cmdShutdown, strlen(cmdShutdown)) == 0) {
            // --------------------------------------------------------------------
            // Shutdown (QEMU)
            // --------------------------------------------------------------------
            __asm__ ("outw %%ax, %%dx" : : "a"(0x2000), "d"(0x604) );

            // TODO: Get shutdown command for bochs, user can uncomment which one they want to use
        }

        if (strncmp(argv[0], cmdDelFile, strlen(cmdDelFile)) == 0) {
            // TODO: Fix deleting erasing filetable from "dir" command until reboot
            // --------------------------------------------------------------------
            // Delete a file from the disk
            // --------------------------------------------------------------------
            //	Input 1 - File name to delete
            //	      2 - Length of file name
            if (!delete_file(argv[1], strlen(argv[1]))) {
                //	;; TODO: Add error message or code here
                printf("ERROR: Delete file failed!");
            }

            // Print newline when done
            printf("\r\n");

            continue;
        }

        if (strncmp(argv[0], cmdRenFile, strlen(cmdRenFile)) == 0) {
            // --------------------------------------------------------------------
            // Rename a file in the file table
            // --------------------------------------------------------------------
            // 1 - File to rename
            // 2 - Length of name to rename
            // 3 - New file name // 4 - New file name length
            if (rename_file(argv[1], strlen(argv[1]), argv[2], strlen(argv[2])) != 0) {
                //	;; TODO: Add error message or code here
            }

            // Print newline when done
            printf("\r\n");

            continue;
        }

        // Print memory map command
        if (strncmp(argv[0], cmdPrtmemmap, strlen(cmdPrtmemmap)) == 0) {
            // Print out physical memory map info
            printf("\r\n-------------------\r\nPhysical Memory Map"
                   "\r\n-------------------\r\n\r\n");
            print_physical_memory_info();
            continue;
        }

        // Change colors command
        // TODO: Change to use terminal control codes to change FG/BG color when debugged
        if (strncmp(argv[0], cmdChgColors, strlen(cmdChgColors)) == 0) {
            uint32_t fg_color = 0;
            uint32_t bg_color = 0;

            printf("\eCSROFF;\r\nCurrent colors (32bpp ARGB):");
            printf("\r\nForeground: %x", user_gfx_info->fg_color);
            printf("\r\nBackground: %x", user_gfx_info->bg_color);

            // Foreground color
            if (gfx_mode->bits_per_pixel > 8)
                printf("\r\nNew Foregound (0xRRGGBB): 0x");
            else
                printf("\r\nNew Foregound (0xNN): 0x");

            printf("\eCSRON;");

            while ((input_char = get_key()) != '\r') {
                if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

                putc(input_char);

                fg_color *= 16;
                if      (input_char >= '0' && input_char <= '9') fg_color += input_char - '0';          // Convert hex ascii 0-9 to integer
                else if (input_char >= 'A' && input_char <= 'F') fg_color += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
            }

            printf("\eCSROFF;");

            // Background color
            if (gfx_mode->bits_per_pixel > 8)
                printf("\r\nNew Backgound (0xRRGGBB): 0x");
            else
                printf("\r\nNew Backgound (0xNN): 0x");

            printf("\eCSRON;");

            while ((input_char = get_key()) != '\r') {
                if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

                putc(input_char);

                bg_color *= 16;

                if      (input_char >= '0' && input_char <= '9') bg_color += input_char - '0';          // Convert hex ascii 0-9 to integer
                else if (input_char >= 'A' && input_char <= 'F') bg_color += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
            }

            printf("\eCSROFF;");

            printf("\eFG%xBG%x;", convert_color(fg_color), convert_color(bg_color));
            user_gfx_info->fg_color = convert_color(fg_color);  // Convert colors first before setting new values
            user_gfx_info->bg_color = convert_color(bg_color);

            printf("\r\n");
            continue;
        }

        // Change font command; Usage: chgFont <name of font>
        if (strncmp(argv[0], cmdChgFont, strlen(cmdChgFont)) == 0) {
            // First check if font exists - name is 2nd token
            file_ptr = check_filename(argv[1], strlen(argv[1]));
            if (*file_ptr == 0) {  
                printf(fontNotFound);  // File not found in filetable, error

                continue;
            }

            // Check if file has .fnt extension
            if (strncmp(file_ptr+10, "fnt", 3) != 0) {
                printf("\r\nError: file is not a font\r\n"); 
                continue;
            }

            // File is a valid font, try to load it to memory
            if (!load_file(argv[1], strlen(argv[1]), (uint32_t)FONT_ADDRESS, fileExt)) {
                printf("\r\nError: file could not be loaded\r\n"); 

                continue;
            }

            // New font should be loaded and in use now
            font_width  = *(uint8_t *)FONT_WIDTH;
            font_height = *(uint8_t *)FONT_HEIGHT;
            printf("\r\nFont loaded: %s\r\n", argv[1]);
            printf("\r\nWidth: %d Height: %d\r\n", font_width, font_height);

            continue;
        }

        // Sleep command - sleep for given number of seconds
        if (strncmp(argv[0], cmdSleep, strlen(cmdSleep)) == 0) {
            sleep_seconds(atoi(argv[1]));  

            printf("\r\n");
            continue;
        }

        // MSleep command - sleep for given number of milliseconds
        if (strncmp(argv[0], cmdMSleep, strlen(cmdMSleep)) == 0) {
            sleep_milliseconds(atoi(argv[1]));

            printf("\r\n");
            continue;
        }

        // Show CMOS RTC date/time values
        if (strncmp(argv[0], cmdShowDateTime, strlen(cmdShowDateTime)) == 0) {
            fs_datetime_t now = current_timestamp();
            printf("\r\n%d-%d-%d %d:%d:%d\r\n",
                    now.year, now.month, now.day, now.hour, now.minute, now.second);
            continue;
        }

        // Test Sound
        if (strncmp(argv[0], cmdSoundTest, strlen(cmdSoundTest)) == 0) {
            enable_pc_speaker();

            /* ADD SOUND THINGS HERE */
            play_note(A4, 500);
            play_note(B4, 500);
            play_note(C5, 500);
            play_note(D5, 500);
            play_note(C5, 500);
            play_note(B4, 500);
            play_note(A4, 500);

            disable_pc_speaker();

            printf("\r\n");
            continue;
        }

        // If command not input, search file table entries for user input file
        file_ptr = check_filename(argv[0], strlen(argv[0]));
        if (*file_ptr == 0) {  
            printf(failure);  // File not found in filetable, error

            continue;
        }

        // file_ptr is pointing to filetable entry, get number of pages needed to load the file
        //   num_pages = (file size in sectors * # of bytes in a sector) / size of a page in bytes
        needed_pages = (file_ptr[15] * 512) / PAGE_SIZE;  // Convert file size in bytes to pages
        if ((file_ptr[15] * 512) % PAGE_SIZE > 0) needed_pages++;   // Allocate extra page for partial page of memory

        printf("\r\nAllocating %d page(s)\r\n", needed_pages);
        
        // Load files/programs to this starting address
        const uint32_t entry_point = 0x400000;  // Put entry point right after identity mapped 4MB page table

        // Allocate & map pages
        for (uint32_t i = 0; i < needed_pages; i++) {
            pt_entry page = 0;
            uint32_t phys_addr = (uint32_t)allocate_page(&page);

            if (!map_page((void *)phys_addr, (void *)(entry_point + i*PAGE_SIZE)))
                printf("\r\nCouldn't map pages, may be out of memory :'(");
        }

        printf("\r\nAllocated to virtual address %x\r\n", entry_point);
        
        // Call load_file function to load program/file to starting memory address
        // Input 1: File name (address)
        //       2: File name length
        //       3: Memory offset to load file to
        //       4: File extension variable
        // Return value - 0 = Success, !0 = error
        if (!load_file(argv[0], strlen(argv[0]), entry_point, fileExt)) {
            // Error, program did not load correctly
            printf(pgmNotLoaded);

            continue;
        }

        // Check file extension in file table entry, if 'bin'/binary, far jump & run
        if (strncmp(fileExt, fileBin, 3) == 0) {
            // Reset malloc variables before calling program
            malloc_list_head    = 0;    // Start of linked list
            malloc_virt_address = entry_point + (needed_pages * PAGE_SIZE);
            malloc_phys_address = 0;
            total_malloc_pages  = 0;

            // Void function pointer to jump to and execute code at specific address in C
            ((int (*)(int argc, char *argv[]))entry_point)(argc, argv);     // Execute program, this can return

            // If used malloc(), free remaining memory to prevent memory leaks
            for (uint32_t i = 0, virt = malloc_virt_address; i < total_malloc_pages; i++, virt += PAGE_SIZE) {
                pt_entry *page = get_page(virt);

                if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
                    free_page(page);
                    unmap_page((uint32_t *)virt);
                    flush_tlb_entry(virt);  // Invalidate page as it is no longer present
                }
            }

            // Reset malloc variables after calling program
            malloc_list_head    = kernel_malloc_list_head;
            malloc_virt_address = kernel_malloc_virt_address;
            malloc_phys_address = kernel_malloc_phys_address;
            total_malloc_pages  = kernel_total_malloc_pages;
            
            // TODO: In the future, if using a backbuffer, restore screen data from that buffer here instead
            //  of clearing
            
            // Clear the screen and reset cursor position before going back
            clear_screen_esc();

            // Free previously allocated pages when done
            printf("\r\n" "Freeing %d page(s)\r\n", needed_pages);
            
            for (uint32_t i = 0, virt = entry_point; i < needed_pages; i++, virt += PAGE_SIZE) {
                pt_entry *page = get_page(virt);

                if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
                    free_page(page);
                    unmap_page((uint32_t *)virt);
                    flush_tlb_entry(virt);  // Invalidate page as it is no longer present or mapped
                }
            }

            printf("\r\nFreed at address %x\r\n", entry_point);

            continue;   // Loop back to prompt for next input
        }

        // Else print text file to screen
        // TODO: Put this behind a "shell" command like 'typ'/'type' or other
        file_ptr = (uint8_t *)entry_point;   // File location to print from

        // Print newline first
        printf("\r\n");
        
        // Get size of filesize in bytes (512 byte per sector)
        // TODO: Change this later - currently assuming text files are only 1 sector
        //	<fileSize> * 512 
        
        // print_file_char:
        for (idx = 0; idx < 512; idx++) {
            // TODO: Handle newlines (byte 0x0A in txt file data)
            if (*file_ptr <= 0x0F)          // Convert to hex
                *file_ptr = hex_to_ascii(*file_ptr);

            // Print file character to screen
            putc(*file_ptr);

            file_ptr++;
        }

        // Print newline after printing file contents
        printf("\r\n");

        // Free pages when done
        printf("\r\nFreeing %d page(s)\r\n", needed_pages);
        
        for (uint32_t i = 0, virt = entry_point; i < needed_pages; i++, virt += PAGE_SIZE) {
            pt_entry *page = get_page(virt);

            if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
                free_page(page);
                unmap_page((uint32_t *)virt);
                flush_tlb_entry(virt);  // Invalidate page as it is no longer present or mapped
            }
        }

        printf("\r\nFreed at address %x\r\n", entry_point);
    }
}

// Print information from the physical memory map (SMAP)
void print_physical_memory_info(void)  
{
    // Physical memory map entry from Bios Int 15h EAX E820h
    typedef struct SMAP_entry {
        uint64_t base_address;
        uint64_t length;
        uint32_t type;
        uint32_t acpi;
    } __attribute__ ((packed)) SMAP_entry_t;

    uint32_t num_entries = *(uint32_t *)SMAP_NUMBER_ADDRESS;          // Number of SMAP entries
    SMAP_entry_t *SMAP_entry = (SMAP_entry_t *)SMAP_ENTRIES_ADDRESS;  // Memory map entries start point

    for (uint32_t i = 0; i < num_entries; i++) {
        printf("Region: %x", i);
        printf(" base: %x", SMAP_entry->base_address);
        printf(" length: %x", SMAP_entry->length);
        printf(" type: %x", SMAP_entry->type);

        switch(SMAP_entry->type) {
            case 1:
                printf(" (Available)");
                break;
            case 2: 
                printf(" (Reserved)");
                break;
            case 3: 
                printf(" (ACPI Reclaim)");
                break;
            case 4: 
                printf(" (ACPI NVS Memory)");
                break;
            default: 
                printf(" (Reserved)");
                break;
        }

        printf("\r\n");
        SMAP_entry++;   // Go to next entry
    }

    // Print total amount of memory
    SMAP_entry--;   // Get last SMAP entry
    printf("\r\nTotal memory in bytes: %x", SMAP_entry->base_address + SMAP_entry->length - 1);

    // Print out memory manager block info:
    //   total memory in 4KB blocks, total # of used blocks, total # of free blocks
    printf("\r\nTotal 4KB blocks: %d", max_blocks);
    printf("\r\nUsed or reserved blocks: %d", used_blocks);
    printf("\r\nFree or available blocks: %d\r\n\r\n", max_blocks - used_blocks);
}

// Initialize file system variables
void init_fs_vars(void) {
    // Load initial superblock state
    superblock = *(superblock_t *)SUPERBLOCK_ADDRESS;

    // Load root inode, root is always inode 1 
    rw_sectors(1, 
               (superblock.first_inode_block*8),   // 1 block = 8 sectors
               (uint32_t)temp_sector,
               READ_WITH_RETRY);

    root_inode = *((inode_t *)temp_sector + 1);
    superblock.root_inode_pointer = (uint32_t)&root_inode;

    // Set filesystem starting point
    current_dir = malloc(1024); // Set starting size of current working directory string
    strcpy(current_dir, "/");   // Start in 'root' directory by default
    current_dir_inode = root_inode;
    current_parent_inode = root_inode;  // Root's parent is itself
                                        //
    next_available_file_virtual_address = 0x40000000; // Default to ~1GB
}

// Initialize open file table
void init_open_file_table(void) {
    max_open_files = 256;

    open_file_table = malloc(sizeof(open_file_table_t) * max_open_files);
    memset(open_file_table, 0, sizeof(open_file_table_t) * max_open_files);

    *open_file_table = (open_file_table_t){0};

    current_open_files = 3;    // FD 0/1/2 reserved for stdin/out/err
}

// Initialize open inode table
void init_open_inode_table(void) {
    max_open_inodes = 256;
    current_open_inodes = 0;

    open_inode_table = malloc(sizeof(inode_t) * max_open_inodes);
    memset(open_inode_table, 0, sizeof(open_file_table_t) * max_open_files);

    *open_inode_table = (inode_t){0};
}

