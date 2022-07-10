/* 
 * =========================================================================== 
 * Kernel.c: basic 'kernel' loaded from 3rd stage bootloader
 * =========================================================================== 
*/
#include "C/stdint.h"
#include "C/stdlib.h"
#include "C/string.h"
#include "C/kstdio.h"   // Kernel stdio
#include "C/time.h"
#include "C/ctype.h"
#include "C/math.h"
#include "global/global_addresses.h"
#include "gfx/2d_gfx.h"
#include "screen/clear_screen.h"
#include "screen/cursor.h"
#include "print/print_registers.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"
#include "memory/kmalloc.h" // Kernel malloc()
#include "disk/file_ops.h"
#include "print/print_dir.h"
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

void print_physical_memory_info(void);  // Print information from the physical memory map (SMAP)

open_file_table_entry_t *open_file_table = 0;   // Pointer to system-wide Open File table 
uint32_t open_file_table_max_size = 0;          // Max # of entries in open file table
uint32_t open_file_table_curr_size = 0;         // Current # of entries in Open File table

inode_t *open_inode_table = 0;                  // Pointer to system-wide open inode table 
uint32_t open_inode_table_max_size = 0;         // Max # of entries in open inode table
uint32_t open_inode_table_curr_size = 0;        // Current # of entries in open inode table

__attribute__ ((section ("kernel_entry"))) void kernel_main(void)
{
    char cmdString[256] = {0};              // User input string  
    char *cmdString_ptr = cmdString;
    uint8_t input_char   = 0;               // User input character
    uint8_t input_length;                   // Length of user input
    uint8_t *cmdDir      = "dir";           // Directory command; list all files/pgms on disk
    uint8_t *cmdReboot   = "reboot\0";      // 'warm' reboot option
    uint8_t *cmdPrtreg   = "prtreg\0";      // Print register values
    uint8_t *cmdGfxtst   = "gfxtst\0";      // Graphics mode test
    uint8_t *cmdHlt      = "hlt\0";         // E(n)d current program by halting cpu
    uint8_t *cmdCls	     = "cls\0";         // Clear screen by scrolling
    uint8_t *cmdShutdown = "shutdown\0";    // Close QEMU emulator
    uint8_t *cmdDelFile  = "del";		    // Delete a file from disk
    uint8_t *cmdRenFile  = "ren";           // Rename a file in the file table
    uint8_t *cmdPrtmemmap = "prtmemmap\0";  // Print physical memory map info
    uint8_t *cmdChgColors = "chgcolors\0";  // Change current fg/bg colors
    uint8_t *cmdChgFont   = "chgfont\0";    // Change current font
    uint8_t *cmdSleep     = "sleep\0";      // Sleep for a # of seconds
    uint8_t *cmdMSleep    = "msleep\0";     // Sleep for a # of milliseconds
    uint8_t *cmdShowDateTime = "showdatetime\0";  // Show CMOS RTC date/time values
    uint8_t *cmdSoundTest = "soundtest\0";  // Test pc speaker square wave sound
    uint8_t *cmdHelp      = "help";         // Help: List all available commands & descriptions (sans aliases)
    uint8_t *cmdChdir     = "chdir";        // Change current directory
    uint8_t *cmdMkdir     = "mkdir";        // Make a new directory
    uint8_t *cmdRmdir     = "rmdir";        // Remove a directory and all of its subfiles/directories
    uint8_t *cmdMov       = "mov";          // Move a file from one directory to another
    uint8_t *file_ptr;
    uint8_t *windowsMsg     = "\r\n" "Oops! Something went wrong :(" "\r\n\0";
    uint8_t *notFoundString = "\r\n" "Program/file not found!, Try again? (Y)" "\r\n\0";
    uint8_t *sectNotFound   = "\r\n" "Sector not found!, Try again? (Y)" "\r\n\0";
    uint8_t *menuString     = "------------------------------------------------------\r\n"
                              "Kernel Booted, Welcome to QuesOS - 32 Bit 'C' Edition!\r\n"
                              "------------------------------------------------------\r\n"
                              "Type 'help' to list available commands, or type the name of a file to run or print it.\r\n";
    uint8_t *failure        = "\r\n" "Command/Program not found, Try again" "\r\n\0";
    uint8_t *prompt         = ">:\0";
    uint8_t *pgmNotLoaded   = "\r\n" "Program found but not loaded, Try Again" "\r\n\0";
    uint8_t *fontNotFound   = "\r\n" "Font not found!" "\r\n\0";

    uint32_t *allocated_address;
    uint8_t font_width  = *(uint8_t *)FONT_WIDTH;
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;
    int argc = 0;
    char *argv[10] = {0};
    char *working_dir = (char *)CURRENT_DIR_NAME;
    static bool first_boot = true;

    // TODO: !!! Clean up, simplify, and refactor code for new filesystem !!!
    
    // Nice to haves/Optional right now:
    // ---------------------------------
    // TODO: Make a helper function to read over all dir_entries for an extent or extents for an inode,
    //   that uses a callback function (function pointer) for each dir_entry? i.e. this helper function would
    //   call the input function pointer with/for each dir_entry, like
    //   void (*function_ptr)(dir_entry_t dir_entry) ... function_ptr = &print_info ... function_ptr(dir_entry)
    // TODO: Add 'alias' command to add alternative names for a command
    // TODO: Save command info for 'help' and 'alias'es in a new file, and read/check this file for entered in 
    //   shell commands 
    
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
    show_datetime = false;  // Don't show date/time on boot 
    enable_rtc();

    // Set default PIT Timer IRQ0 rate - ~1000hz
    // 1193182 MHZ / 1193 = ~1000
    set_pit_channel_mode_frequency(0, 2, 1193);

    // After setting up hardware interrupts & PIC, set IF to enable 
    //   non-exception and not NMI hardware interrupts
    __asm__ __volatile__("sti");

    // Set up kernel malloc variables for e.g. kprintf() calls
    // Initialize open file table
    open_file_table_max_size = sizeof(open_file_table_entry_t) * 256;  // 256 entries to start
    open_file_table = kmalloc(open_file_table_max_size);
    open_file_table_curr_size = 0;  // No files open currently

    // TODO: Use calloc()?
    if (open_file_table) 
        memset(open_file_table, 0, open_file_table_max_size);

    // Initialize open inode table
    open_inode_table_max_size = sizeof(inode_t) * 256;  // 256 entries to start
    open_inode_table = kmalloc(open_inode_table_max_size);
    open_inode_table_curr_size = 0;  // No files open currently

    // TODO: Use calloc()?
    if (open_inode_table) 
        memset(open_inode_table, 0, open_inode_table_max_size);

    // Set intial colors
    while (!user_gfx_info->fg_color) {
        if (gfx_mode->bits_per_pixel > 8) {
            kprintf("\eFG%xBG%x;", convert_color(0x00EEEEEE), convert_color(0x00222222)); 
            user_gfx_info->fg_color = convert_color(0x00EEEEEE);
            user_gfx_info->bg_color = convert_color(0x00222222);
        } else {
            // Assuming VGA palette
            kprintf("\eFG%xBG%x;", convert_color(0x02), convert_color(0x00)); 
            user_gfx_info->fg_color = convert_color(0x02);
            user_gfx_info->bg_color = convert_color(0x00);
        }
    }

    // Clear the screen
    clear_screen_esc();

    // Print OS boot message
    kprintf("\eCSROFF;%s", menuString);

    // --------------------------------------------------------------------
    // Get user input, print to screen & run command/program  
    // --------------------------------------------------------------------
    // Set current directory to root inode
    rw_sectors(1, superblock->first_inode_block * 8, (uint32_t)tmp_sector, READ_WITH_RETRY);
    *current_dir_inode = *((inode_t *)tmp_sector + 1); // Root inode id = 1
    strcpy(working_dir, "/");   // Currently at root '/'

    // Initial file setup, rename font files to have .fnt extension
    if (first_boot) {
        fs_rename_file("termu16n.bin", "termu16n.fnt");
        fs_rename_file("termu18n.bin", "termu18n.fnt");
        fs_rename_file("testfont.bin", "testfont.fnt");
        first_boot = false;
    }

    while (true) {
        // Reset tokens data, arrays, and variables for next input line
        memset(cmdString, 0, sizeof cmdString);

        // Print prompt
        kprintf("\eCSRON;%s %s", working_dir, prompt);
        
        input_length = 0;   // reset byte counter of input

        // Key loop - get input from user
        while (1) {
            input_char = get_key();     // Get ascii char from scancode from keyboard data port 60h

            if (input_char == '\r') {   // enter key?
                kprintf("\eCSROFF;");
                break;                  // go on to tokenize user input line
            }

            // TODO: Move all input back 1 char/byte after backspace, if applicable
            if (input_char == '\b') {       // backspace?
                if (input_length > 0) {                // Did user input anything?
                    input_length--;                    // yes, go back one char in cmdString
                    cmdString[input_length] = '\0';    // Blank out character    					

                    // Do a "visual" backspace; Move cursor back 1 space, print 2 spaces, move back 2 spaces
                    kprintf("\eBS;  \eBS;\eBS;");
                }

                continue;   // Get next character
            }

            cmdString[input_length] = input_char;   // Store input char to string
            input_length++;                         // Increment byte counter of input

            // Print input character to screen
            kputc(input_char);
        }

        if (input_length == 0) {
            // No input or command not found! boo D:
            kprintf(failure);

            continue;
        }

        cmdString[input_length] = '\0';     // else null terminate cmdString from si

        // Tokenize input string "cmdString" into separate tokens
        cmdString_ptr = cmdString;      // Reset pointers...
        argc = 0;   // Reset argument count
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
        if (strncmp(argv[0], cmdHelp, strlen(cmdHelp)) == 0) {
            // Help: List all available commands
            kputs("\r\n"
                 "----------------------------\r\n"
                 "Commands:\r\n"
                 "         \r\n"
                 "[parameter] = optional\r\n"
                 "<parameter> = required\r\n"
                 "----------------------------\r\n"
                 "cls                         | Clear the screen and start text output at top left.\r\n"
                 "chdir   <path>              | Change the current working directory.\r\n"
                 "chgcolors                   | Change the current text and background color.\r\n"
                 "chgfont <path>              | Change the current bitmapped font to use a given font file.\r\n"
                 "del     <path>              | Remove/delete a given file. For deleting directories, use rmdir.\r\n"
                 "dir     [path]              | Print the contents of the current directory or a given directory.\r\n"
                 "gfxtst                      | Test drawing 2D graphics primitives. WIP: Only for 1920x1080 resolution for now.\r\n"
                 "mkdir   <name>              | Make a new directory.\r\n"
                 "msleep  <mseconds>          | Sleep for a number of milliseconds. 1000ms = 1s.\r\n"
                 "mov     <oldpath> <newpath> | Move a file from one directory to another directory\r\n"
                 "help                        | Print this help message.\r\n"
                 "hlt                         | Clear interrupt flag and halt cpu. This will make your machine hang indefinitely.\r\n"
                 "prtreg                      | Print register contents. WIP: These are not guarenteed to be useful at all.\r\n"
                 "prtmemmap                   | Print the system memory map SMAP entries and current memory use\r\n"
                 "reboot                      | Reboot the machine, should also work on hardware\r\n"
                 "ren     <oldpath> <newpath> | Rename a file or directory\r\n"
                 "rmdir   <path>              | Remove/delete a directory and all of its contents\r\n"
                 "showdatetime                | Print current date & time as MM/DD/YY HH:MM:SS\r\n"
                 "shutdown                    | Turn off the QEMU virtual machine, does not work on bochs or hardware\r\n"
                 "sleep   <seconds>           | Sleep for a number of seconds\r\n"
                 "soundtest                   | Play sounds/music if available\r\n"
            );
            continue;
        }
        
        if (strncmp(argv[0], cmdChdir, strlen(cmdChdir)) == 0) {
            // Change directory: Change current directory
            inode_t *inode = get_inode_from_path(argv[1], current_dir_inode);

            if (!inode || inode->id == 0) {
                kputs("\r\nERROR: Invalid directory or path");
            } else {
                // Update current_dir to point to new current directory inode
                *current_dir_inode = *inode;
                set_name_from_path(working_dir, argv[1]); 
            }

            kprintf("\r\n");
            continue;
        }

        if (strncmp(argv[0], cmdMkdir, strlen(cmdMkdir)) == 0) {
            // Make directory: Make new directory
            // Verify this is a new file/directory
            inode_t *temp = get_inode_from_path(argv[1], current_dir_inode);

            if (temp) {
                kputs("\r\nDirectory already exists!\r\n");
                continue;
            }

            // LIFO "stack" of directories to create 
            const int max_dirs = 32;
            char *path_stack[max_dirs];
            int path_stack_ptr = 0;

            // Start by adding initial directory's parent directory
            char *initial_path = argv[1];
            path_stack[path_stack_ptr] = initial_path;

            // Get parent dir of initial dir to create
            temp = get_parent_inode_from_path(initial_path, current_dir_inode);

            while (!temp) {
                // Did not find parent directory, create each missing directory in given path,
                //   cut path short at next directory boundary to search for next level up parent dir
                char *next_dir = strrchr(initial_path, '/');
                *next_dir = '\0';       
                path_stack_ptr++;
                path_stack[path_stack_ptr] = next_dir;  // Add next boundary marker to stack

                // Try to get this parent directory
                temp = get_parent_inode_from_path(initial_path, current_dir_inode);
            }

            while (path_stack_ptr >= 0) {
                // Create new dir file at path
                fs_create_file(initial_path, FILETYPE_DIR);

                // Reset current directory path to handle next dir to be created
                *path_stack[path_stack_ptr] = '/';
                path_stack_ptr--;       // "Pop" stack to move to next element
            }

            kprintf("\r\n");
            continue;
        }

        if (strncmp(argv[0], cmdRmdir, strlen(cmdRmdir)) == 0) {
            // Remove directory: Delete a directory and all of its containing subfiles/directories
            const inode_t *test = get_inode_from_path(argv[1], current_dir_inode);
            if (!test) {
                kputs("\r\nDirectory does not exist!\r\n");
                continue;
            }

            if (test->type != FILETYPE_DIR) {
                kputs("\r\nFile is not a directory! Use 'del <file>' instead.\r\n");
                continue;
            }

            const inode_t temp = *test;
            const inode_t temp_parent = *get_parent_inode_from_path(argv[1], current_dir_inode);

            // LIFO "stack" of directories to delete all files from
            inode_t dir_stack[32];
            int dir_stack_ptr = 0;

            // Start by adding initial directory & its parent
            dir_stack[dir_stack_ptr++] = temp_parent;  
            dir_stack[dir_stack_ptr] = temp; 

            // Loop over all files in each directory and delete them
            while (dir_stack_ptr > 0) {
                // Read through all dir_entries for all blocks in inode
                inode_t next_dir_inode = dir_stack[dir_stack_ptr];

                bool added_dir = false;

                fs_delete_dir_files(&next_dir_inode, &dir_stack[dir_stack_ptr+1], &added_dir);

                if (added_dir) {
                    dir_stack_ptr++;    // New dir inode is stored at this position in dir stack
                    continue;
                }

                // TODO: Check single & double indirect blocks

                // Delete this dir itself, from itself, and "pop" stack
                fs_delete_file("./", &dir_stack[dir_stack_ptr]);

                // Update this dir's parent directory now that this dir is deleted
                inode_t parent_inode = {0};
                if (dir_stack_ptr > 0)
                    parent_inode = dir_stack[dir_stack_ptr-1];
                else
                    parent_inode = *get_inode_from_id(1);    // Use root

                dir_entry_sector_t dir_entry_sector = find_dir_entry(dir_stack[dir_stack_ptr].id, parent_inode);

                // Clear dir_entry
                memset(dir_entry_sector.dir_entry, 0, sizeof(dir_entry_t));

                // Write changed sector to disk 
                rw_sectors(1, dir_entry_sector.disk_sector, (uint32_t)dir_entry_sector.sector_in_memory, WRITE_WITH_RETRY);

                // Update parent's inode size from removing the dir_entry
                rw_sectors(1, 
                           (superblock->first_inode_block * 8) + (parent_inode.id / 8),
                           (uint32_t)tmp_sector,
                           READ_WITH_RETRY);

                inode_t *temp = (inode_t *)tmp_sector + (parent_inode.id % 8);
                temp->size_bytes -= sizeof(dir_entry_t);
                temp->size_sectors = bytes_to_sectors(temp->size_bytes); 

                rw_sectors(1, 
                           (superblock->first_inode_block * 8) + (parent_inode.id / 8),
                           (uint32_t)tmp_sector,
                           WRITE_WITH_RETRY);
                
                dir_stack_ptr--;
            } 

            // Grab any changes for current directory after deleting
            rw_sectors(1, 
                       (superblock->first_inode_block * 8) + (current_dir_inode->id / 8),
                       (uint32_t)tmp_sector,
                       READ_WITH_RETRY);

            *current_dir_inode = *((inode_t *)tmp_sector + (current_dir_inode->id % 8));

            kprintf("\r\n");
            continue;
        }

        if (strncmp(argv[0], cmdDir, strlen(cmdDir)) == 0) {
            // Directory: List files in a directory
            if (argc > 1) {
                // Print files in given directory 
                inode_t *inode = get_inode_from_path(argv[1], current_dir_inode);

                if (!inode)
                    kputs("\r\nFile does not exist!\r\n");
                else if (inode->type != FILETYPE_DIR)
                    kputs("\r\nFile is not a directory!\r\n");
                else
                    print_dir(inode); 
            } else {
                // Print files in current directory
                print_dir(current_dir_inode); 
            }

            continue;
        }

        if (strncmp(argv[0], cmdMov, strlen(cmdMov)) == 0) {
            if (argc < 3) {
                kputs("\r\nUsage: mov <oldpath> <newpath>\r\n");
                continue;
            }

            // Get inode and parent dir of file to move
            inode_t inode = *get_inode_from_path(argv[1], current_dir_inode);
            inode_t parent_inode = *get_parent_inode_from_path(argv[1], current_dir_inode);

            // Get dir_entry of argv[1] path
            dir_entry_sector_t dir_entry_sector = find_dir_entry(inode.id, parent_inode);
            dir_entry_t dir_entry = *dir_entry_sector.dir_entry;

            // Delete from current path
            memset(dir_entry_sector.dir_entry, 0, sizeof(dir_entry_t));

            // Write changes to disk
            rw_sectors(1, 
                       dir_entry_sector.disk_sector, 
                       (uint32_t)dir_entry_sector.sector_in_memory,
                       WRITE_WITH_RETRY);

            // Update dir inode for path
            parent_inode.size_bytes -= sizeof(dir_entry_t);
            parent_inode.size_sectors = bytes_to_sectors(inode.size_bytes);

            rw_sectors(1, 
                       (superblock->first_inode_block * 8) + (parent_inode.id / 8),
                       (uint32_t)tmp_sector,
                       READ_WITH_RETRY);

            inode_t *temp = (inode_t *)tmp_sector + (parent_inode.id % 8);
            *temp = parent_inode;

            rw_sectors(1, 
                       (superblock->first_inode_block * 8) + (parent_inode.id / 8),
                       (uint32_t)tmp_sector,
                       WRITE_WITH_RETRY);

            // Get parent dir of new path
            parent_inode = *get_parent_inode_from_path(argv[2], current_dir_inode);

            // Update dir inode for new path
            parent_inode.size_bytes += sizeof(dir_entry_t);
            parent_inode.size_sectors = bytes_to_sectors(inode.size_bytes);

            rw_sectors(1, 
                       (superblock->first_inode_block * 8) + (parent_inode.id / 8),
                       (uint32_t)tmp_sector,
                       READ_WITH_RETRY);

            temp = (inode_t *)tmp_sector + (parent_inode.id % 8);
            *temp = parent_inode;

            rw_sectors(1, 
                       (superblock->first_inode_block * 8) + (parent_inode.id / 8),
                       (uint32_t)tmp_sector,
                       WRITE_WITH_RETRY);

            // Write new dir_entry in directory's data blocks
            //   Load dir's last data block 
            const uint32_t total_blocks = bytes_to_blocks(parent_inode.size_bytes);

            uint32_t last_block = 0;
            uint32_t blocks = 0;

            for (uint32_t i = 0; i < 4; i++) { 
                blocks += parent_inode.extent[i].length_blocks; 
                if (blocks == total_blocks) {
                    last_block = (parent_inode.extent[i].first_block + parent_inode.extent[i].length_blocks) - 1;
                    break;
                }
            }

            // TODO: Check first indirect block & double indirect block extents

            rw_sectors(8, last_block * 8, (uint32_t)tmp_block, READ_WITH_RETRY);

            // Find next free spot or end of entries
            dir_entry_t *new_entry = (dir_entry_t *)tmp_block;
            uint32_t offset = 0;

            while (new_entry->id != 0 && offset < parent_inode.size_bytes - sizeof(dir_entry_t)) {
                new_entry++; 
                offset += sizeof(dir_entry_t);
            }

            // Add new dir entry
            new_entry->id = dir_entry.id;
            strcpy(new_entry->name, dir_entry.name);

            // Write changed sector
            offset /= FS_SECTOR_SIZE;   // Convert offset in bytes to # of sectors from start of block (0-7)

            rw_sectors(1, 
                       (last_block * 8) + offset, 
                       (uint32_t)(tmp_block + (offset * FS_SECTOR_SIZE)),
                       WRITE_WITH_RETRY);

            // Update file name if user input different name in new path
            if (argv[2][strlen(argv[2]) - 1] != '/') {
                // Only change to new name if last name in path is not a dir
                char old_name[60], new_name[60];
                strcpy(old_name, get_last_name_in_path(old_name, argv[1]));
                strcpy(new_name, get_last_name_in_path(new_name, argv[2]));
                
                if (strcmp(old_name, new_name) != 0) {
                    // Construct "old" path at new directory
                    char *last_name_pos = strrchr(argv[2], '/');
                    *++last_name_pos = '\0';
                    strcat(last_name_pos, old_name);

                    // Rename file to new name
                    fs_rename_file(argv[2], new_name);
                }
            }
            
            // Update current directory to get any changes
            rw_sectors(1, 
                       (superblock->first_inode_block * 8) + (current_dir_inode->id / 8),
                       (uint32_t)tmp_sector,
                       READ_WITH_RETRY);

            *current_dir_inode = *((inode_t *)tmp_sector + (current_dir_inode->id % 8));

            kprintf("\r\n");
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
            // --------------------------------------------------------------------
            // Delete a file 
            // --------------------------------------------------------------------
            inode_t *inode = get_inode_from_path(argv[1], current_dir_inode);
            if (!inode) {
                kputs("\r\nFile does not exist.\r\n");
                continue;
            }

            if (inode->type == FILETYPE_DIR) {
                kputs("\r\nCan not delete a directory! Use 'rmdir' instead.");
                continue;
            }

            fs_delete_file(argv[1], current_dir_inode); // Deleting normal file, not a directory

            kprintf("\r\n");

            continue;
        }

        if (strncmp(argv[0], cmdRenFile, strlen(cmdRenFile)) == 0) {
            // --------------
            // Rename a file 
            // --------------
            // Validate input
            if (argc < 3) {
                kputs("\r\nUsage: ren [path] [new_name]\r\n");
                continue;
            }

            if (strchr(argv[2], '/')) {
                kputs("\r\nUsage: New name cannot contain '/'!\r\n");
                continue;
            }

            fs_rename_file(argv[1], argv[2]);

            // Print newline when done
            kprintf("\r\n");
            continue;
        }

        // Print memory map command
        if (strncmp(argv[0], cmdPrtmemmap, strlen(cmdPrtmemmap)) == 0) {
            // Print out physical memory map info
            kprintf("\r\n-------------------\r\nPhysical Memory Map"
                   "\r\n-------------------\r\n");
            print_physical_memory_info();
            continue;
        }

        // Change colors command
        // TODO: Change to use terminal control codes to change FG/BG color when debugged
        if (strncmp(argv[0], cmdChgColors, strlen(cmdChgColors)) == 0) {
            uint32_t fg_color = 0;
            uint32_t bg_color = 0;

            kprintf("\eCSROFF;\r\nCurrent colors (32bpp ARGB):");
            kprintf("\r\nForeground: %x", user_gfx_info->fg_color);
            kprintf("\r\nBackground: %x", user_gfx_info->bg_color);

            // Foreground color
            if (gfx_mode->bits_per_pixel > 8)
                kprintf("\r\nNew Foregound (0xRRGGBB): 0x");
            else
                kprintf("\r\nNew Foregound (0xNN): 0x");

            kprintf("\eCSRON;");

            while ((input_char = get_key()) != '\r') {
                if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

                kputc(input_char);

                fg_color *= 16;
                if      (input_char >= '0' && input_char <= '9') fg_color += input_char - '0';          // Convert hex ascii 0-9 to integer
                else if (input_char >= 'A' && input_char <= 'F') fg_color += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
            }

            kprintf("\eCSROFF;");

            // Background color
            if (gfx_mode->bits_per_pixel > 8)
                kprintf("\r\nNew Backgound (0xRRGGBB): 0x");
            else
                kprintf("\r\nNew Backgound (0xNN): 0x");

            kprintf("\eCSRON;");

            while ((input_char = get_key()) != '\r') {
                if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

                kputc(input_char);

                bg_color *= 16;

                if      (input_char >= '0' && input_char <= '9') bg_color += input_char - '0';          // Convert hex ascii 0-9 to integer
                else if (input_char >= 'A' && input_char <= 'F') bg_color += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
            }

            kprintf("\eCSROFF;");

            kprintf("\eFG%xBG%x;", convert_color(fg_color), convert_color(bg_color));
            user_gfx_info->fg_color = convert_color(fg_color);  // Convert colors first before setting new values
            user_gfx_info->bg_color = convert_color(bg_color);

            kprintf("\r\n");
            continue;
        }

        // Change font command; Usage: chgFont <name of font>
        if (strncmp(argv[0], cmdChgFont, strlen(cmdChgFont)) == 0) {
            // First check if font exists - name is 2nd token
            inode_t *font_inode = get_inode_from_path(argv[1], current_dir_inode);
            if (!font_inode) {
                kprintf(fontNotFound); // File not found
                continue;
            }

            // Check if file has .fnt extension
            char *name_pos = strrchr(argv[1], '/');
            if (!name_pos) 
                name_pos = argv[1];
            else
                name_pos++;

            if (!strstr(name_pos, ".fnt")) {
                kprintf("\r\nError: file is not a font\r\n"); 
                continue;
            }

            // File is a valid font, try to load it to memory
            if (!fs_load_file(font_inode, FONT_ADDRESS)) {
                kprintf("\r\nError: file could not be loaded\r\n"); 
                continue;
            }

            // New font should be loaded and in use now
            font_width  = *(uint8_t *)FONT_WIDTH;
            font_height = *(uint8_t *)FONT_HEIGHT;
            kprintf("\r\nFont loaded: %s\r\n", argv[1]);
            kprintf("\r\nWidth: %d Height: %d\r\n", font_width, font_height);

            continue;
        }

        // Sleep command - sleep for given number of seconds
        if (strncmp(argv[0], cmdSleep, strlen(cmdSleep)) == 0) {
            sleep_seconds(atoi(argv[1]));  

            kprintf("\r\n");
            continue;
        }

        // MSleep command - sleep for given number of milliseconds
        if (strncmp(argv[0], cmdMSleep, strlen(cmdMSleep)) == 0) {
            sleep_milliseconds(atoi(argv[1]));

            kprintf("\r\n");
            continue;
        }

        // Show CMOS RTC date/time values
        if (strncmp(argv[0], cmdShowDateTime, strlen(cmdShowDateTime)) == 0) {
            show_datetime = !show_datetime;  

            if (!show_datetime) {
                // Blank out date/time
                // TODO: Make "save/restore" screen or terminal variables control code
                uint16_t x = 50, y = 30;
                print_string(&x, &y, "                   "); // Overwrite date/time with spaces
            }

            kprintf("\r\n");
            continue;
        }

        // Test Sound
        if (strncmp(argv[0], cmdSoundTest, strlen(cmdSoundTest)) == 0) {
            enable_pc_speaker();

            /* ADD SOUND THINGS HERE */

            disable_pc_speaker();

            kprintf("\r\n");
            continue;
        }

        // If command not input, try to load file to run/print
        inode_t *file_inode = get_inode_from_path(argv[0], current_dir_inode);
        if (!file_inode) {
            kprintf(failure);  // File not found
            continue;
        }

        // Get number of pages needed to load file
        //   num_pages = (file size in sectors * # of bytes in a sector) / size of a page in bytes
        uint32_t needed_pages = (file_inode->size_sectors * FS_SECTOR_SIZE) / PAGE_SIZE;
        if ((file_inode->size_sectors * FS_SECTOR_SIZE) % PAGE_SIZE > 0) needed_pages++;   // Allocate extra page for partial page of memory

        kprintf("\r\nAllocating %d page(s)\r\n", needed_pages);
        
        // Load files/programs to this starting address
        const uint32_t entry_point = 0x400000;  // Put entry point right after identity mapped 4MB page table

        // Allocate & map pages
        for (uint32_t i = 0; i < needed_pages; i++) {
            pt_entry page = 0;
            uint32_t phys_addr = (uint32_t)allocate_page(&page);

            if (!map_page((void *)phys_addr, (void *)(entry_point + i*PAGE_SIZE)))
                kprintf("\r\nCouldn't map pages, may be out of memory :'(");
        }

        kprintf("\r\nAllocated to virtual address %x\r\n", entry_point);

        // Call load_file function to load program/file to starting memory address
        // Input 1: File name (address)
        //       2: Memory offset to load file to
        // Return value: true = Success, false = Error
        if (!fs_load_file(file_inode, entry_point)) {
            kputs(pgmNotLoaded);    // Error, program did not load correctly
            continue;
        }

        // Check file extension, if 'bin'/binary or 'exe'/executable, execute as a program
        dir_entry_t file_entry = {0};
        strcpy(file_entry.name, get_last_name_in_path(file_entry.name, argv[0]));

        if (!strcmp(&file_entry.name[strlen(file_entry.name) - 4], ".bin") || 
            !strcmp(&file_entry.name[strlen(file_entry.name) - 4], ".exe")) {
            // Reset malloc variables before calling program
            malloc_list_head    = 0;    // Start of linked list
            malloc_virt_address = entry_point + (needed_pages * PAGE_SIZE);
            malloc_phys_address = 0;
            total_malloc_pages  = 0;

            // Function pointer to jump to and execute code at specific address in C
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
            malloc_list_head    = 0; 
            malloc_virt_address = 0;
            malloc_phys_address = 0;
            total_malloc_pages  = 0;
            
            // TODO: In the future, if using a backbuffer, restore screen data from that buffer here instead
            //  of clearing
            
            // Clear the screen and reset cursor position before going back
            kprintf("\eCSROFF;");
            clear_screen_esc();

            // Free previously allocated pages when done
            kprintf("\r\nFreeing %d page(s)\r\n", needed_pages);
            
            for (uint32_t i = 0, virt = entry_point; i < needed_pages; i++, virt += PAGE_SIZE) {
                pt_entry *page = get_page(virt);

                if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
                    free_page(page);
                    unmap_page((uint32_t *)virt);
                    flush_tlb_entry(virt);  // Invalidate page as it is no longer present or mapped
                }
            }

            kprintf("\r\nFreed at address %x\r\n", entry_point);

            continue;   // Loop back to prompt for next input
        }

        // Else print text file to screen
        // TODO: Put this behind a "shell" command like 'typ'/'type' or other
        // Print newline first
        kprintf("\r\n");

        // Print characters from file
        file_ptr = (uint8_t *)entry_point;   // File location to print from

        for (uint32_t idx = 0; idx < file_inode->size_bytes; idx++, file_ptr++) {
            if (*file_ptr <= 0x0F) {         
                if (*file_ptr == 0x0A) {
                    // Newline
                    kputs("\r\n");
                    continue;
                }

                *file_ptr = hex_to_ascii(*file_ptr); // Convert to hex
            }

            // Print file character to screen
            kputc(*file_ptr);
        }

        // Print newline after printing file contents
        kprintf("\r\n");

        // Free pages when done
        kprintf("\r\nFreeing %d page(s)\r\n", needed_pages);
        
        for (uint32_t i = 0, virt = entry_point; i < needed_pages; i++, virt += PAGE_SIZE) {
            pt_entry *page = get_page(virt);

            if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
                free_page(page);
                unmap_page((uint32_t *)virt);
                flush_tlb_entry(virt);  // Invalidate page as it is no longer present or mapped
            }
        }

        kprintf("\r\nFreed at address %x\r\n", entry_point);
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

    uint32_t num_entries = *(uint32_t *)SMAP_ENTRIES_NUM;     // Number of SMAP entries
    SMAP_entry_t *SMAP_entry = (SMAP_entry_t *)SMAP_ENTRIES;  // Memory map entries start point

    for (uint32_t i = 0; i < num_entries; i++) {
        kprintf("Region: %x", i);
        kprintf(" base: %x", SMAP_entry->base_address);
        kprintf(" length: %x", SMAP_entry->length);
        kprintf(" type: %x", SMAP_entry->type);

        switch(SMAP_entry->type) {
            case 1:
                kprintf(" (Available)");
                break;
            case 2: 
                kprintf(" (Reserved)");
                break;
            case 3: 
                kprintf(" (ACPI Reclaim)");
                break;
            case 4: 
                kprintf(" (ACPI NVS Memory)");
                break;
            default: 
                kprintf(" (Reserved)");
                break;
        }

        kprintf("\r\n");
        SMAP_entry++;   // Go to next entry
    }

    // Print total amount of memory
    SMAP_entry--;   // Get last SMAP entry
    kprintf("\r\nTotal memory in bytes: %x", SMAP_entry->base_address + SMAP_entry->length - 1);

    // Print out memory manager block info:
    //   total memory in 4KB blocks, total # of used blocks, total # of free blocks
    kprintf("\r\nTotal 4KB blocks: %d", max_blocks);
    kprintf("\r\nUsed or reserved blocks: %d", used_blocks);
    kprintf("\r\nFree or available blocks: %d\r\n\r\n", max_blocks - used_blocks);
}

