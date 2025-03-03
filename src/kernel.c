// =========================================================================== 
// Kernel.c: basic 'kernel' loaded from 3rd stage bootloader
// ===========================================================================
#include "C/stdint.h"
#include "C/stdlib.h"
#include "C/string.h"
#include "C/stdio.h"
#include "C/stddef.h"
#include "C/stdbool.h"
#include "C/time.h"
#include "C/ctype.h"
#include "global/global_addresses.h"
#include "gfx/2d_gfx.h"
#include "screen/clear_screen.h"
#include "print/print_registers.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"
#include "memory/malloc.h"
#include "disk/file_ops.h"
#include "interrupts/idt.h"
#include "interrupts/exceptions.h"
#include "interrupts/pic.h"
#include "interrupts/syscalls.h"
#include "ports/io.h"
#include "keyboard/keyboard.h"
#include "sound/pc_speaker.h"
#include "sys/syscall_wrappers.h"
#include "fs/fs_impl.h"
#include "elf/elf.h"
#include "process/process.h"

// Open file table & inode table pointers
open_file_table_t *open_file_table;
uint32_t max_open_files;
uint32_t current_open_files;

inode_t *open_inode_table;
uint32_t max_open_inodes;
uint32_t current_open_inodes;

extern char current_dir[512];   // Current working directory string

uint32_t next_available_file_virtual_address;

// Forward function declarations
void init_open_file_table(void);                                        
void init_open_inode_table(void);                                        
void init_fs_vars(void);

void shell(bool, int32_t);

bool cmd_chgcolors(int32_t argc, char *argv[]);
bool cmd_chgfont(int32_t argc, char *argv[]);
bool cmd_cls(int32_t argc, char *argv[]);
bool cmd_date(int32_t argc, char *argv[]);
bool cmd_gfxtst(int32_t argc, char *argv[]);
bool cmd_msleep(int32_t argc, char *argv[]);
bool cmd_prtmemmap(int32_t argc, char *argv[]);
bool cmd_reboot(int32_t argc, char *argv[]);
bool cmd_runtests(int32_t argc, char *argv[]);
bool cmd_shutdown(int32_t argc, char *argv[]);
bool cmd_sleep(int32_t argc, char *argv[]);
bool cmd_soundtest(int32_t argc, char *argv[]);
bool cmd_touch(int32_t argc, char *argv[]);
bool cmd_type(int32_t argc, char *argv[]);

bool test_open_close(void); // Test functions...
bool test_seek(void);
bool test_write(void);
bool test_read(void);
bool test_malloc(void);

__attribute__ ((section ("kernel_entry"))) void kernel_main(void) {
    //uint8_t *windowsMsg     = "\r\n" "Oops! Something went wrong :(" "\r\n\0";
    uint8_t *menuString     = "------------------------------------------------------\r\n"
                              "Kernel Booted, Welcome to QuesOS - 32 Bit 'C' Edition!\r\n"
                              "------------------------------------------------------\r\n";
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
    set_idt_descriptor_32(0, (uint32_t)div_by_0_handler, TRAP_GATE_FLAGS); // Divide by 0 error #DE, ISR 0
    set_idt_descriptor_32(13, (uint32_t)protection_fault_handler, TRAP_GATE_FLAGS); // General protection fault #GP errors, ISR 13
    set_idt_descriptor_32(14, (uint32_t)page_fault_handler, TRAP_GATE_FLAGS); // Page fault #PF errors, ISR 14
    
    // Set up software interrupt system call handler/dispatcher
    set_idt_descriptor_32(0x80, (uint32_t)syscall_dispatcher, INT_GATE_USER_FLAGS);  

    // Mask off all hardware interrupts (disable the PIC)
    disable_pic();

    // Remap PIC interrupts to after exceptions (PIC1 starts at 0x20)
    remap_pic();

    // Add ISRs for PIC hardware interrupts
    set_idt_descriptor_32(0x20, (uint32_t)timer_irq0_handler, INT_GATE_FLAGS);  
    set_idt_descriptor_32(0x21, (uint32_t)keyboard_irq1_handler, INT_GATE_FLAGS);
    set_idt_descriptor_32(0x28, (uint32_t)cmos_rtc_irq8_handler, INT_GATE_FLAGS);
    
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

    // Set up kernel malloc variables 
    malloc_virt_address = 0x400000; // 4MB
    malloc_phys_address = (uint32_t)allocate_blocks(1);
    map_address(current_page_directory, malloc_phys_address, malloc_virt_address,
                PTE_PRESENT | PTE_READ_WRITE | PTE_USER);

    malloc_list_head       = (malloc_block_t *)malloc_virt_address;
    malloc_list_head->size = PAGE_SIZE - sizeof(malloc_block_t);
    malloc_list_head->free = true;
    malloc_list_head->next = 0;

    total_malloc_pages  = 1;

    // Set up initial file system variables
    init_fs_vars();

    // Set up kernel/system open file table and open inode table
    init_open_file_table();
    init_open_inode_table();

    // Set intial colors
    while (!user_gfx_info->fg_color) {
        if (gfx_mode->bits_per_pixel > 8) {
            printf("\033FG%#xBG%#x;", convert_color(0x00EEEEEE), convert_color(0x00222222)); 
        } else {
            // Assuming VGA palette
            printf("\033FG%#xBG%#x;", convert_color(0x02), convert_color(0x00)); 
        }
    }

    // Clear the screen
    printf("\033CLS;");

    // Print OS boot message
    printf("\033CSROFF;%s", menuString);

    shell(false, 0);
}

// Kernel "shell"
void shell(bool from_process, int32_t return_status) {
    char cmdString[256] = {0};         // User input string  
    char *cmdString_ptr = cmdString;
    uint8_t input_char  = 0;            // User input character
    uint8_t input_length;               // Length of user input

    uint8_t *prompt = ">";

    int32_t argc = 0;
    char *argv[10] = {0};

    // !NOTE!: All commands here need to have their own functions at the 
    //   exact same array offset/element in the command_functions array below
    enum commands {
        CHDIR,
        CHGCOLOR,
        CHGFONT,
        CLS,
        DATE,
        GFXTST,
        LS,
        MKDIR,
        MSLEEP,
        PRTREG,
        PRTMEMMAP,
        RM,
        RMDIR,
        REBOOT,
        REN,
        RUNTESTS,
        SHUTDOWN,
        SLEEP,
        SOUNDTEST,
        TOUCH,
        TYPE,

        MAX_COMMAND
    };

    // Command names
    char *commands[MAX_COMMAND] = {
        [CHDIR]     = "chdir",
        [CHGCOLOR]  = "chgcolors",
        [CHGFONT]   = "chgfont",
        [CLS]       = "cls",
        [DATE]      = "date",
        [GFXTST]    = "gfxtst",
        [LS]        = "ls",
        [MKDIR]     = "mkdir",
        [MSLEEP]    = "msleep",
        [PRTREG]    = "prtreg",
        [PRTMEMMAP] = "prtmemmap",
        [RM]        = "rm",
        [RMDIR]     = "rmdir",
        [REBOOT]    = "reboot", 
        [REN]       = "ren", 
        [RUNTESTS]  = "runtests", 
        [SHUTDOWN]  = "shutdown", 
        [SLEEP]     = "sleep", 
        [SOUNDTEST] = "soundtest", 
        [TOUCH]     = "touch",
        [TYPE]      = "type",
    };

    // Commands will be similar to programs, and be called
    //   with the same argc/argv[]
    bool (*command_functions[MAX_COMMAND])(int32_t argc, char **) = {
        [CHDIR]     = fs_change_dir,
        [CHGCOLOR]  = cmd_chgcolors,
        [CHGFONT]   = cmd_chgfont,
        [CLS]       = cmd_cls,
        [DATE]      = cmd_date,
        [GFXTST]    = cmd_gfxtst,
        [LS]        = print_dir,
        [MKDIR]     = fs_make_dir,
        [MSLEEP]    = cmd_msleep,
        [PRTREG]    = print_registers,
        [PRTMEMMAP] = cmd_prtmemmap,
        [RM]        = fs_delete_file,
        [RMDIR]     = fs_delete_dir,
        [REBOOT]    = cmd_reboot,
        [REN]       = fs_rename_file,
        [RUNTESTS]  = cmd_runtests,
        [SHUTDOWN]  = cmd_shutdown,
        [SLEEP]     = cmd_sleep,
        [SOUNDTEST] = cmd_soundtest,
        [TOUCH]     = cmd_touch,
        [TYPE]      = cmd_type,
    };

    // If returned from a process, print return status code
    if (from_process) {
        from_process = false;
        printf("\r\nReturn Code: %d\r\n", return_status);
    }

    while (true) {
        // Print prompt
        printf("\r\n%s%s\033CSRON;", current_dir, prompt);
        
        input_length = 0;   // Reset input byte count

        // Key loop - get input from user
        while (true) {
            input_char = get_key();     // Get ascii char from scancode from keyboard data port 60h

            if (input_char == '\r') {   // enter key?
                printf("\033CSROFF; ");   // TODO: Add automatic newline here after prompt
                break;                  // go on to tokenize user input line
            }

            // TODO: Move all input back 1 char/byte after backspace, if applicable
            if (input_char == '\b') {       // backspace?
                if (input_length > 0) {                // Did user input anything?
                    input_length--;                    // yes, go back one char in cmdString
                    cmdString[input_length] = '\0';    // Blank out character    					

                    // Do a "visual" backspace; Move cursor back 1 space, print 2 spaces, move back 2 spaces
                    printf("\033BS;  \033BS;\033BS;");
                }
                continue;   // Get next character
            }

            cmdString[input_length] = input_char;   // Store input char to string
            input_length++;                         // Increment byte counter of input

            // Print input character to screen
            putchar(input_char);
        }

        if (input_length == 0) {
            // No input or command not found! boo D:
            printf("\r\nCommand/Program not found, Try again\r\n");
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

            // Found dbl quoted string, count the string as 1 full token
            if (*cmdString_ptr == '\"') {
                // Keep reading until ending dbl quote delimiter
                cmdString_ptr++;

                while (*cmdString_ptr != '\"') {
                    cmdString_ptr++;
                }
            }

            // Go to next space or end of string
            while (!isspace(*cmdString_ptr) && *cmdString_ptr != '\0') 
                cmdString_ptr++;
        }

        // Check commands 
        bool found_command = false;
        for (uint32_t i = 0; i < sizeof commands / sizeof commands[0]; i++) {
            if (!strncmp(argv[0], commands[i], strlen(argv[0]))) {
                found_command = true;

                if (!command_functions[i](argc, argv)) 
                    printf("\r\nError: command %s failed\r\n", argv[0]);
                break;
            }
        }

        // Found and ran command, loop for next user input
        if (found_command) continue;

        // Assuming user did not type in a command, but a program to run;
        //  First Check if file is a bin file
        if (strncmp(argv[0] + strlen(argv[0]) - 4, ".bin", 4) != 0) {
            printf("\r\nError: Cannot run program; File %s is not a bin file\r\n", argv[0]);
            continue;
        }

        // Check if this is a new file or not, don't run a newly created file
        inode_t program_inode = inode_from_path(argv[0]);
        if (program_inode.id == 0) {
            printf("\r\nError: Program %s does not exist.\r\n", argv[0]);
            continue;
        }

        int32_t pid = create_process(argc, argv);
        if (pid < 0) {
            printf("\r\nError: Could not create process for program %s\r\n", argv[0]);
            continue;
        }
        execute_process();  // Will return to shell() from exit() call when done
    }
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
    memset(open_inode_table, 0, sizeof(open_file_table_t) * max_open_inodes);

    *open_inode_table = (inode_t){0};
}

// Change terminal colors
bool cmd_chgcolors(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;

    uint32_t fg = 0, bg = 0;

    printf("\033CSROFF;\r\nCurrent colors (32bpp ARGB):");
    printf("\r\nForeground: %#x", user_gfx_info->fg_color);
    printf("\r\nBackground: %#x", user_gfx_info->bg_color);

    // Foreground color
    if (gfx_mode->bits_per_pixel > 8)
        printf("\r\nNew Foregound (0xRRGGBB): 0x");
    else
        printf("\r\nNew Foregound (0xNN): 0x");

    printf("\033CSRON;");

    char input_char = get_key();
    while ((input_char = get_key()) != '\r') {
        if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

        if      (input_char >= '0' && input_char <= '9') fg += input_char - '0';          // Convert hex ascii 0-9 to integer
        else if (input_char >= 'A' && input_char <= 'F') fg += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
        else continue;

        putchar(input_char);
        fg *= 16;
    }

    printf("\033CSROFF;");

    // Background color
    if (gfx_mode->bits_per_pixel > 8)
        printf("\r\nNew Backgound (0xRRGGBB): 0x");
    else
        printf("\r\nNew Backgound (0xNN): 0x");

    printf("\033CSRON;");

    while ((input_char = get_key()) != '\r') {
        if (input_char >= 'a' && input_char <= 'f') input_char -= 0x20; // Convert lowercase to Uppercase

        if      (input_char >= 'A' && input_char <= 'F') bg += (input_char - 'A') + 10;   // Convert hex ascii 10-15 to integer
        else if (input_char >= '0' && input_char <= '9') bg += input_char - '0';          // Convert hex ascii 0-9 to integer
        else continue;

        putchar(input_char);
        bg *= 16;
    }

    // Set new colors
    printf("\033CSROFF;");
    printf("\033FG%#xBG%#x;\033CLS;", convert_color(fg), convert_color(bg));
    return true;
}

// Change current terminal font
bool cmd_chgfont(int32_t argc, char *argv[]) {
    (void)argc;

    // Open file
    int32_t fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("\r\nFont not found!\r\n");
        return false;
    }

    // Check if file has .fnt extension
    // TODO: Use different font file format, maybe "FNT" at start of file data?
    //if (strncmp(file_ptr+10, "fnt", 3) != 0) {
    //    printf("\r\nError: file is not a font\r\n"); 
    //    return false;
    //}

    // Load to font address
    uint8_t *font = (uint8_t *)FONT_ADDRESS;
    uint8_t buf[512];
    uint32_t bytes = 0, total = 0;
    while ((bytes = read(fd, buf, sizeof buf)) > 0) {
        memcpy(font, buf, bytes);
        font += bytes;
        total += bytes;
    }

    if (!total) {
        printf("\r\nError: file could not be loaded\r\n"); 
        close(fd);
        return false;
    }

    // New font should be loaded and in use now
    uint8_t font_width  = *(uint8_t *)FONT_WIDTH;
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;
    printf("\033CLS;Font loaded: %s\r\n", argv[1]);
    printf("\r\nWidth: %d Height: %d\r\n", font_width, font_height);

    close(fd);      // File cleanup
    return true;
}

// Clear the screen
bool cmd_cls(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;
    printf("\033CLS;");
    return true;
}

// Print current date/time from CMOS RTC
bool cmd_date(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;

    fs_datetime_t now = current_timestamp();
    printf("\r\n%.4d-%.2d-%.2d %.2d:%.2d:%.2d\r\n",
            now.year, now.month, now.day, now.hour, now.minute, now.second);
    return true;
}

// 2D Graphics test
bool cmd_gfxtst(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;

    Point p0, p1, p2;
    Point vertex_array[6];
    uint32_t xres = gfx_mode->x_resolution, yres = gfx_mode->y_resolution;

    uint32_t fg = user_gfx_info->fg_color, bg = user_gfx_info->bg_color;
    printf("\033FG%#xBG%#x;\033CLS;", fg, convert_color(LIGHT_GRAY));

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
    p0.X = xres/2 - 100;
    p0.Y = yres/2 - 100;
    p1.X = xres/2;
    p1.Y = yres/2 - 100;
    draw_line(p0, p1, convert_color(BLACK));

    // Vertical line
    p0.X = xres/2 + 100;
    p0.Y = yres/2 - 100;
    p1.X = xres/2 + 100;
    p1.Y = yres/2 + 100;
    draw_line(p0, p1, convert_color(PURPLE));
    
    // Diagonal line up-right
    p0.X = xres/2 + 150;
    p0.Y = yres/2;
    p1.X = xres/2 + 300;
    p1.Y = yres/2 - 100;
    draw_line(p0, p1, convert_color(DARK_GRAY));
    
    // Diagonal line down-right
    p0.X = xres/2 + 350;
    p0.Y = yres/2 - 100;
    p1.X = xres/2 + 500;
    p1.Y = yres/2 + 100;
    draw_line(p0, p1, convert_color(BLUE));

    // Draw triangle test - right angle
    p0.X = xres/2 - 600;
    p0.Y = yres/2 + 100;
    p1.X = xres/2 - 600;
    p1.Y = yres/2 + 200;
    p2.X = xres/2 - 450;
    p2.Y = yres/2 + 200;
    draw_triangle(p0, p1, p2, convert_color(0x00FF7233));  // Sort of "burnt orange"

    // Draw rectangle test
    p0.X = xres/2 - 400;
    p0.Y = yres/2 + 100;
    p1.X = xres/2 - 100;
    p1.Y = yres/2 + 200;
    draw_rect(p0, p1, convert_color(0x0033CEFF));  // Kind of teal maybe

    // Draw polygon test - hexagon
    vertex_array[0].X = xres/2;
    vertex_array[0].Y = yres/2 + 100;
    vertex_array[1].X = xres/2 + 100;
    vertex_array[1].Y = yres/2 + 100;
    vertex_array[2].X = xres/2 + 200;
    vertex_array[2].Y = yres/2 + 150;
    vertex_array[3].X = xres/2 + 100;
    vertex_array[3].Y = yres/2 + 200;
    vertex_array[4].X = xres/2;
    vertex_array[4].Y = yres/2 + 200;
    vertex_array[5].X = xres/2 - 100;
    vertex_array[5].Y = yres/2 + 150;
    draw_polygon(vertex_array, 6, convert_color(RED));

    // Draw circle test
    p0.X = xres/2 + 350;
    p0.Y = yres/2 + 200;
    draw_circle(p0, 50, convert_color(BLUE));
    
    // Draw ellipse test
    // TODO:
    //p0.X = xres/2 - 600;
    //p0.Y = yres/2 + 350;
    //draw_ellipse(p0, 100, 50, convert_color(GREEN - 0x00005500));

    // Fill triangle test
    p0.X = xres/2 - 400;
    p0.Y = yres/2 + 300;
    p1.X = xres/2 - 500;
    p1.Y = yres/2 + 350;
    p2.X = xres/2 - 420;
    p2.Y = yres/2 + 250;
    fill_triangle_solid(p0, p1, p2, convert_color(0x006315FD));  // Some sort of dark purple

    // Fill rectangle test
    p0.X = xres/2 - 350;
    p0.Y = yres/2 + 300;
    p1.X = xres/2 - 150;
    p1.Y = yres/2 + 350;
    fill_rect_solid(p0, p1, convert_color(0x0015FDCD));  // Mintish greenish

    // Fill polygon test - hexagon
    // TODO: Fix page fault from fill_polygon_solid()[
    //vertex_array[0].X = xres/2 - 50;
    //vertex_array[0].Y = yres/2 + 250;
    //vertex_array[1].X = xres/2 + 50;
    //vertex_array[1].Y = yres/2 + 250;
    //vertex_array[2].X = xres/2 + 100;
    //vertex_array[2].Y = yres/2 + 300;
    //vertex_array[3].X = xres/2 + 50;
    //vertex_array[3].Y = yres/2 + 350;
    //vertex_array[4].X = xres/2 - 50;
    //vertex_array[4].Y = yres/2 + 350;
    //vertex_array[5].X = xres/2 - 100;
    //vertex_array[5].Y = yres/2 + 300;
    //fill_polygon_solid(vertex_array, 6, convert_color(0x00FD9B15));  // Light orangey

    // Fill circle test
    p0.X = xres/2 + 200;
    p0.Y = yres/2 + 270;
    fill_circle_solid(p0, 50, convert_color(0x00FF0FE6));  // Magentish fuschish

    // Fill ellipse test
    // TODO:
    //p0.X = xres/2 + 400;
    //p0.Y = yres/2 + 350;
    //fill_ellipse_solid(p0, 100, 50, convert_color(0x00FFEE0F));  // I Love GOOOOOOoooolllddd

    get_key(); 
    printf("\033FG%#xBG%#x;\033CLS;", fg, bg);
    return true;
}

//Sleep for given number of milliseconds
bool cmd_msleep(int32_t argc, char *argv[]) {
    (void)argc;
    sleep_milliseconds(atoi(argv[1]));
    return true;
}

// Print information from the physical memory map (SMAP)
bool cmd_prtmemmap(int32_t argc, char *argv[])  {
    (void)argc, (void)argv;

    // Physical memory map entry from Bios Int 15h EAX E820h
    typedef struct SMAP_entry {
        uint64_t base_address; 
        uint64_t length;
        uint32_t type;
        uint32_t acpi;
    } __attribute__ ((packed)) SMAP_entry_t;

    uint32_t num_entries = *(uint32_t *)SMAP_NUMBER_ADDRESS;          // Number of SMAP entries
    SMAP_entry_t *SMAP_entry = (SMAP_entry_t *)SMAP_ENTRIES_ADDRESS;  // Memory map entries start point

    char *entry_type_strings[] = {
        "Unknown",         // 0
        "Available",       // 1
        "Reserved",        // 2
        "ACPI Reclaim",    // 3
        "ACPI NVS Memory", // 4 
    };

    printf("\r\n-------------------"
           "\r\nPhysical Memory Map"
           "\r\n-------------------\r\n");

    for (uint32_t i = 0; i < num_entries; i++, SMAP_entry++) {
        printf("Region: %#x ", i);
        printf("base: %#x ", (uint32_t)SMAP_entry->base_address);
        printf("length: %#x ", (uint32_t)SMAP_entry->length);
        printf("type: %#x ", SMAP_entry->type);

        printf("(%s)\r\n", 
               (SMAP_entry->type <= 4) ? entry_type_strings[SMAP_entry->type]
                                       : "Reserved");
    }

    // Print total amount of memory
    SMAP_entry--;   // Get last SMAP entry
    printf("\r\nTotal memory in bytes: %#x", 
           SMAP_entry->base_address + SMAP_entry->length - 1);

    // Print out memory manager block info:
    //   total memory in 4KB blocks, total # of used blocks, total # of free blocks
    printf("\r\nTotal 4KB blocks: %d", max_blocks);
    printf("\r\nUsed or reserved blocks: %d", used_blocks);
    printf("\r\nFree or available blocks: %d\r\n\r\n", max_blocks - used_blocks);
    return true;
}

// Reboot
bool cmd_reboot(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;
    outb(0x64, 0xFE);   // Send "Reset CPU" command to PS/2 keyboard controller port
    return false;       // Should never reach here!
}

// Run test functions and present results
bool cmd_runtests(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;

    typedef struct {
        char *name;
        bool (*function)(void);
    } test_function_t;

    test_function_t tests[] = {
        { "Open() & Close() Syscalls",        test_open_close },
        { "Seek() Syscall on New/Empty file", test_seek },
        { "Write() Syscall for New file",     test_write },
        { "Read() Syscall on file",           test_read },
        { "Malloc() & Free() tests",          test_malloc },
        // TODO: { "Seek() Syscall on file with data", test_seek },
    };

    const uint32_t fg = user_gfx_info->fg_color, bg = user_gfx_info->bg_color;
    const uint32_t num_tests = sizeof tests / sizeof tests[0];  
    uint32_t num_passed = 0;

    printf("\r\nRunning tests...\r\n----------------");

    for (uint32_t i = 0; i < num_tests; i++) {
        printf("\r\n%s: ", tests[i].name);
        if (tests[i].function()) {
            printf("\033FG%#xBG%#x;OK", GREEN, bg);
            num_passed++;
        } else {
            printf("\033FG%#xBG%#x;FAIL", RED, bg);
        }

        // Reset default colors
        printf("\033FG%#xBG%#x;", fg, bg);
    }

    printf("\r\nTests passed: %d/%d\r\n", num_passed, num_tests);
    return true;
}

// Shutdown
bool cmd_shutdown(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;

    // Shutdown (QEMU)
    __asm__ ("outw %%ax, %%dx" : : "a"(0x2000), "d"(0x604) );
    // TODO: Get shutdown command for bochs, user can uncomment which one they want to use
    // TODO: Look into ACPI and how to shutdown from there

    return false;   // Should never reach here!
}

// Sleep for given number of seconds
bool cmd_sleep(int32_t argc, char *argv[]) {
    (void)argc;
    sleep_seconds(atoi(argv[1]));  
    return true;
}

// Test Sound
bool cmd_soundtest(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;
    
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
    return true;
}

// Touch command: Create new empty file
bool cmd_touch(int32_t argc, char *argv[]) {
    if (argc < 1) return false;

    int32_t fd = open(argv[1], O_CREAT);
    if (fd < 0) return false;
    if (close(fd) != 0) return false;
    return true;
}

// Type out a file to screen
bool cmd_type(int32_t argc, char *argv[]) {
    (void)argc;

    // Check if text file
    if (strncmp(argv[1] + strlen(argv[1]) - 4, ".txt", 4) != 0) {
        printf("\r\nError: File %s is not a text file\r\n", argv[1]);
        return false;
    }

    // Open file
    const int32_t fd = open(argv[1], O_RDONLY);
    if (fd < 3) {
        printf("\r\nError: Could not open file %s for reading\r\n", argv[1]);
        return false;
    }

    // Read each sector of file and print to screen
    char buf[512] = {0};    // 1 sector
    int32_t len = 0;

    printf("\r\n");
    while ((len = read(fd, buf, sizeof buf)) > 0) {
        for (int32_t i = 0; i < len; i++) {
            if (buf[i] == '\n') { putchar('\r'); putchar('\n'); } 
            else putchar(buf[i]);
        }
    }
    close(fd);  // File cleanup
    return true;
}

// Probably should move these to a separate file or something later...
// Test open() & close() syscalls
bool test_open_close(void) {
    char *file = "openclosetest.txt";
    int32_t fd = open(file, O_CREAT);
    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    // Test close() syscall
    if (close(fd) != 0) {
        printf("Error: could not close file %s\r\n", file);
        return false;
    }
    return true;
}

// Test seek() syscall
bool test_seek(void) {
    // TODO: Add more to this after read() and write() are filled out,
    //   to be able to actually write data to a file

    char *file = "seektest.txt";
    int32_t fd = open(file, O_CREAT);
    int32_t seek_val = 0;

    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    // Test seeking on a new/empty file
    if (0 != seek(fd, 0, SEEK_SET)) {
        printf("\r\nError: could not SEEK_SET to 0 for %s\r\n", file);
        return false;
    }

    if (100 != seek(fd, 100, SEEK_SET)) {
        printf("\r\nError: could not SEEK_SET to 100 for %s\r\n", file);
        return false;
    }

    if (-1 != seek(fd, -100, SEEK_SET)) {
        printf("\r\nError: could not SEEK_SET to -100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, 0, SEEK_CUR)) {
        printf("Error: could not SEEK_CUR to 0 for %s\r\n", file);
        printf("seek result: %d\r\n", seek_val);
        return false;
    }

    if (100 != seek(fd, 100, SEEK_CUR)) {
        printf("\r\nError: could not SEEK_CUR to 100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, -100, SEEK_CUR)) {
        printf("\r\nError: could not SEEK_CUR to -100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, 0, SEEK_END)) {
        printf("\r\nError: could not SEEK_END to 0 for %s\r\n", file);
        return false;
    }

    if (100 != seek(fd, 100, SEEK_END)) {
        printf("\r\nError: could not SEEK_END to 100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, -100, SEEK_END)) {
        printf("\r\nError: could not SEEK_END to -100 for %s\r\n", file);
        return false;
    }

    close(fd);
    return true;
}

// Test write() syscall
bool test_write(void) {
    char *file = "writetest.txt";
    int32_t fd = open(file, O_CREAT | O_WRONLY); 

    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    char buf[] = "Hello, World!";

    if (14 != write(fd, buf, sizeof buf)) {
        printf("\r\nError: could not write \"%s\" to file %s\r\n", 
               buf, file);
        return false;
    }

    // TODO: Test O_APPEND

    close(fd);
    return true;
}

// Test read() syscall
bool test_read(void) {
    char *file = "readtest.txt";
    int32_t fd = open(file, O_CREAT | O_RDWR);

    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    char str_buf[] = "Hello, World!";

    if (14 != write(fd, str_buf, sizeof str_buf)) {
        printf("\r\nError: could not write \"%s\" to file %s\r\n", 
               str_buf, file);
        return false;
    }

    char read_buf[14];

    // read() from end of file
    if (read(fd, read_buf, sizeof read_buf) > 0) {
        printf("\r\nError: Read from end of file %s should return 0\r\n", file);
        return false;
    }

    // Move back to start of file
    seek(fd, 0, SEEK_SET);

    // read() from start of file
    if (read(fd, read_buf, sizeof read_buf) < 1) {
        printf("\r\nError: could not read from file %s into buffer\r\n", file);
        return false;
    }

    if (strncmp(read_buf, "Hello, World!", strlen(read_buf)) != 0) {
        printf("\r\nError: could not read file %s into buffer correctly\r\n", file);
        return false;
    }

    close(fd);

    return true;
}

// Test malloc() & free() function calls
bool test_malloc(void) {
    void *buf;
    void *buf2;
    void *buf3;
    void *buf4;
    void *buf5;

    buf = malloc(100);
    printf("\r\nMalloc-ed 100 bytes to address %x\r\n", (uint32_t)buf);
    printf("Free-ing bytes...\r\n");
    free(buf);

    buf2 = malloc(42);
    printf("Malloc-ed 42 bytes to address %x\r\n", (uint32_t)buf2);
    printf("Free-ing bytes...\r\n");
    free(buf2);

    buf3 = malloc(250);
    printf("Malloc-ed 250 bytes to address %x\r\n", (uint32_t)buf3);

    buf4 = malloc(6000);
    printf("Malloc-ed 6000 bytes to address %x\r\n", (uint32_t)buf4);

    buf5 = malloc(333);
    printf("Malloc-ed 333 bytes to address %x\r\n", (uint32_t)buf5);

    free(buf4);
    free(buf5);
    free(buf3);

    return true;
}

