//
// editor.c: text editor with "modes", from hexadecimal monitor to ascii editing
//
#include "C/stdint.h"
#include "C/string.h"
#include "C/stdlib.h"
#include "C/stdio.h"
#include "C/stddef.h"
#include "C/stdbool.h"
#include "gfx/2d_gfx.h"
#include "disk/file_ops.h"
#include "keyboard/keyboard.h"
#include "sys/syscall_wrappers.h"
#include "global/global_addresses.h"

#define ENDOFLINE  80
#define ESCAPE     0x1B  // Escape Key
#define SPACE      0x20  // ASCII space
#define LEFTARROW  0x4B	 // Keyboard scancodes...
#define RIGHTARROW 0x4D
#define UPARROW    0x48
#define DOWNARROW  0x50
#define HOMEKEY    0x47
#define ENDKEY     0x4F
#define DELKEY     0x53
#define CREATENEW  'c'   // Editor screen keybinds
#define LOADCURR   'l'
#define BINFILE    'b'
#define OTHERFILE  'o'
#define RUNINPUT   '$'   // Hex editor keybinds
#define ENDPGM     '?'
#define SAVEPGM    'S'

// File modes
enum file_modes {
    NEW    = 1,     // New file
    UPDATE = 2      // Updating an existing file
};

enum file_type {
    BIN,
    TXT,
};
 
void editor_load_file(char *filename);    // Function declarations
void text_editor(char *in_filename);
void hex_editor(void);
void save_hex_program(void);
void input_file_name(void);
void write_bottom_screen_message(char *msg);

int32_t fd = 0;
uint32_t editor_filesize = 0;
uint8_t input_char;
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;
uint8_t *file_ptr;
uint32_t file_offset;
uint8_t *file_address;
uint8_t file_mode;     
uint32_t current_line_length;
uint32_t file_length_lines;
uint32_t file_length_bytes;
uint8_t hex_count = 0;
uint8_t hex_byte = 0;   // 1 byte/2 hex digits
int32_t editor_filetype;
char editor_filename[32];
uint8_t font_height;
uint32_t screen_rows;
static char *load_file_error_msg = "Load file error occurred, press any key to go back...";
static char *keybinds_hex_editor = " $ = Run code ? = Return to kernel S = Save file to disk";
static char *digits              = "0123456789ABCDEF";

int main(int argc, char *argv[]) {
    // Initialize global variables
    font_height = *(uint8_t *)FONT_HEIGHT;
    screen_rows = gfx_mode->y_resolution / font_height;

    // If did not pass any arguments to editor, edit new file
    if (argc < 2) {
        file_mode = NEW;        // Creating a new file

        printf("\033CLS;(B)inary/hex file or (O)ther file type (.txt, etc)?");
        do {
            input_char = get_key();
            if (input_char == ESCAPE) return 1; // Return back to caller
        } while (input_char != BINFILE && input_char != OTHERFILE);

        // Allocate 1 blank sector for new file, and initialize cursor
        //   and file variables
        file_ptr            = calloc(1, 512); 
        file_address        = file_ptr;
        file_offset         = 0;
        file_length_lines   = 0;
        file_length_bytes   = 0;
        current_line_length = 0;
        editor_filesize     = 0; 
        cursor_x = cursor_y = 0;

        if (input_char == BINFILE) {
            // Fill out filetype (.bin)
            editor_filetype = BIN;
            hex_editor();
        } else {
            // Fill out filetype (.txt)
            editor_filetype = TXT;
            text_editor(NULL);
        }
    
    } else {
        // Otherwise load file
        file_mode = UPDATE;     // Updating an existing file
        editor_load_file(argv[1]); 
    }

    if (fd > 0) close(fd);
    printf("\033CLS;"); // Clear screen before returning
    return 0;
}

void editor_load_file(char *filename) {
    uint32_t file_size = 0;

    // Save filename
    strcpy(editor_filename, filename);

    fd = open(filename, O_RDWR);
    if (fd < 0) return;

    file_size = seek(fd, 0, SEEK_END);
    if (file_size > 0) {
        file_ptr     = malloc(file_size);    // Allocate memory for file 
        if (!file_ptr) {
            write_bottom_screen_message(load_file_error_msg);
            input_char = get_key();
            close(fd);
            return;
        }
        file_address = file_ptr;

        // Load file into buffer
        if (read(fd, file_ptr, file_size) < (int32_t)file_size) {
            write_bottom_screen_message(load_file_error_msg);
            input_char = get_key();
            close(fd);
            return;
        }

    } else {
        // Loading file error - file does not exist or could not be found
        write_bottom_screen_message(load_file_error_msg);
        input_char = get_key();

        printf("\033CLS;");
        cursor_x = cursor_y = 0;
        return; // Return to caller after failure :(
    }

    // Load file success
	// Go to editor depending on file type
    if (editor_filetype == BIN) {
        // Load hex file
        printf("\033CLS;");
        cursor_x = cursor_y = 0;
     
        // Load file bytes to screen
        for (uint32_t i = 0; i < file_size; i++) {
            input_char = (*file_ptr >> 4) & 0x0F;   // Read hex byte from file location - 2 nibbles!
            input_char = digits[input_char];  // 1st nibble as ascii

            // Print char
            putchar(input_char);

            input_char = *file_ptr & 0x0F;    // 2nd nibble
            input_char = digits[input_char];  // as ascii

            // Print char in AL
            putchar(input_char);

            if (cursor_x) putchar(' ');     // At start of line? Print space between hex bytes

            file_ptr++;
            editor_filesize++;
        }

        hex_count = 0;      // Reset byte counter
        file_ptr = file_address;  // Reset pointer to file 
        file_offset = 0;
        
        hex_editor();

    } else {
        // Load text file
        printf("\033CLS;");
        cursor_x = cursor_y = 0;

        current_line_length = 0;
        file_length_lines = 0;
        file_length_bytes = 0;

        // Load file bytes to screen - stop at EOF if less than file size
        for (uint32_t i = 0; i < file_size && file_ptr[i] != '\0'; i++) { 
            if (file_ptr[i] == '\n') {  
                putchar(' ');   // Newline = space visually

                // Go down 1 row
                cursor_x = 0;
                cursor_y++;
                file_length_lines++;

            } else if (file_ptr[i] <= 0x0F) {  
                input_char = digits[file_ptr[i]]; 
                putchar(input_char);

            } else {
                putchar(file_ptr[i]);
            }

            file_offset++;
            file_length_bytes++;
        }

        file_offset = 0;
        cursor_x = cursor_y = 0;    // Reset cursor position
        printf("\033X%uY%u;", cursor_x, cursor_y);

        // Get length of first line
        current_line_length = 0;

        for (uint32_t i = 0; file_ptr[i] != '\n' && file_offset != file_length_bytes; i++) { 
            file_offset++;
            current_line_length++;
        }

        current_line_length++;  // Include newline or last byte in file 

        file_offset = 0;

        text_editor(editor_filename);
    }
}

void text_editor(char *in_filename) {
    uint32_t save_x, save_y;
    uint8_t *save_file_ptr;
    uint8_t save_file_offset;
    uint8_t changed_filename[10];
    uint8_t unsaved = 0;

    if (!in_filename) strcpy(editor_filename, "(new file)");

    key_info_t *key_info = (key_info_t *)KEY_INFO_ADDRESS;

    printf("\033CLS;");     // Clear screen, resets cursor to 0,0

    while (true) {
        // Write info and keybinds at bottom of screen
        printf("\033CSROFF;\033X%uY%u;", 1u, screen_rows-2);
        printf("X:%x Y:%x ", cursor_x, cursor_y);
        printf("LEN:%x ", current_line_length);
        printf("SIZE:%x ", file_length_bytes);

        printf("[%s]  ", editor_filename);
        if (unsaved) putchar('*');  // Unsaved changes
        printf("%s", (file_mode == NEW) ? "NEW" : "UPD");

        write_bottom_screen_message(
            "Ctrl-R: Return | Ctrl-S: Save | Ctrl-C: Chg name/ext | Ctrl-D: Del line");

        // Restore cursor position: show cursor for user input
        printf("\033CSRON;\033X%uY%u; \033BS;", cursor_x, cursor_y);

		// Get next key, check text editor keybinds
        input_char = get_key();
        if (key_info->ctrl) {    // CTRL Key pressed
            if (input_char == 'r') return; // CTRL-R Return to kernel

            if (input_char == 's') {    // CTRL-S Save file to disk
                // If new file, input file name and extension
                if (file_mode == NEW) {
                    printf("\033CSROFF;");  // Erase cursor 
                    write_bottom_screen_message("Enter file name: "); 
                    printf("\033CSRON;");  

                    input_file_name(); 
                    file_mode = UPDATE;

                    fd = open(editor_filename, O_CREAT | O_RDWR);
                    if (fd < 0) {
                        // Errored on creating new file
                        write_bottom_screen_message("Error: open() file");   
                        get_key();
                        continue;
                    }
                }

                // Save file data
                if (write(fd, file_address, file_length_bytes) < 0) {
                    write_bottom_screen_message("Error: write() file");   
                    get_key();
                    continue;
                } 

                unsaved = 0;    // User saved file, no more unsaved changes
                continue;
            }

            // Ctrl-C: Change file name 
            if (input_char == 'c') {
                printf("\033CSROFF;");  // Remove cursor
                write_bottom_screen_message("Enter file name: "); 
                printf("\033CSRON;");  

                // Get old file name
                strncpy(changed_filename, editor_filename, 10);

                // Input new file name
                input_file_name();

                // Call rename_file() with new file name/ext to overwrite filetable
                rename_file(changed_filename, 10, editor_filename, 10);

                // Call save_file() to update file and data on disk
                // TODO: Use actual file size, not hardcoded 1
                //if (!save_file(editor_filename, editor_filetype, 1, (uint32_t)file_address)) {
                    // TODO: handle save errors here
                //}

                printf("\033X%uY%u;", cursor_x, cursor_y);  // Restore cursor position
                unsaved = 0;  // Saved file, no unsaved changes now
                continue;
            }

            // Ctrl-D: Delete current line
            if (input_char == 'd') {
                // If on last line of file, skip TODO: Put in deleting last line
                if (cursor_y == file_length_lines) continue;

                // Move all lines after this one up 1 line
                file_ptr        -= cursor_x;
                file_offset     -= cursor_x;
                save_file_ptr    = file_ptr;
                save_file_offset = file_offset;
                while (file_offset < file_length_bytes - current_line_length + 1) {
                    *file_ptr = *(file_ptr + current_line_length);
                    file_ptr++;
                    file_offset++;
                }
                
                // Blank out all lines from current line to end of file
                save_y = cursor_y;
                while (cursor_y <= file_length_lines) {
                    cursor_x = 0;
                    for (uint8_t j = 0; j < 79; j++)
                        putchar(' ');

                    cursor_y++;
                }

                file_length_lines--;    // 1 less line in the file now
                file_length_bytes -= current_line_length;

                // Redraw all characters from current line to end of file
                // TODO: Look into refactoring all of this for better performance?
                cursor_x = 0;
                cursor_y = save_y;
                file_ptr = save_file_ptr;

                while (*file_ptr != 0x00) {
                    if (*file_ptr == 0x0A) {
                        // Go to next line
                        putchar(' ');
                        cursor_x = 0;
                        cursor_y++;

                    } else putchar(*file_ptr);

                    file_ptr++;
                }

                // Restore cursor position to start of new line
                cursor_x = 0;
                cursor_y = save_y;
                printf("\033X%uY%u;", cursor_x, cursor_y);

                file_ptr    = save_file_ptr;
                file_offset = save_file_offset;

                // Get new current line length
                current_line_length = 0;
                while (*file_ptr != 0x0A && *file_ptr != 0x00) {
                    file_ptr++;
                    current_line_length++;
                }

                file_ptr = save_file_ptr;   // Restore file data at cursor
                continue;
            }
        }
			
        // Backspace or delete
        // TODO: May not be consistent with multiple lines and deleting at different positions
        if (input_char == 0x08 || input_char == DELKEY) {
            if (input_char == 0x08 && cursor_x == 0) continue;  // Skip backspace at start of line

            // TODO: Handle newline deletion
            if (*file_ptr == 0x0A) continue;

            // At end of file? Move cursor back
            if (*file_ptr == 0x00 && file_offset != 0) {
                printf("\033CSROFF;");

                // Go back 1 character
                cursor_x--;
                file_ptr--;
                file_offset--;
                current_line_length--;
                continue;
            }

            // Backspace, move back 1 character/byte
            if (input_char == 0x08) {
                cursor_x--;
                file_ptr--;
                file_offset--;
            }

            // Move all file data ahead of cursor back 1 byte
            for (uint32_t i = 0; i < (file_length_bytes - file_offset); i++)
                file_ptr[i] = file_ptr[i+1];

            file_length_bytes--;    // Deleted a char/byte from file data

            // Rewrite this line
            save_file_ptr = file_ptr;
            save_x = cursor_x;

            file_ptr -= cursor_x;   // Start of line
            cursor_x = 0;
            // Redraw line until end of line or end of file
            while (*file_ptr != 0x0A && *file_ptr != 0x00) {
                putchar(*file_ptr);
                file_ptr++;
            }
            putchar(' ');   // Previous end of line is now a space
            current_line_length--;

            // Restore cursor_file position
            cursor_x = save_x;
            file_ptr = save_file_ptr;
            printf("\033X%uY%u;", cursor_x, cursor_y);

            unsaved = 1;    // File now has unsaved changes
            continue;
        }

        if (input_char == LEFTARROW) {    // Left arrow key
            // Move 1 byte left (till beginning of line)
            if (cursor_x != 0) {
                printf("\033CSROFF;");

                file_ptr--;     // Move file data to previous byte
                file_offset--;
                cursor_x--;     // Move cursor to previous character
                printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);
            }
            continue;
        }

        if (input_char == RIGHTARROW) {    // Right arrow key
            // Move 1 byte right (till end of line)
            if ((uint32_t)cursor_x+1 < current_line_length) { 
                printf("\033CSROFF;");

                file_ptr++;
                file_offset++;
                cursor_x++;
                printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);
            }
            continue;
        }

        if (input_char == UPARROW) {        
            if (cursor_y == 0)  continue;   // On 1st line, can't go up

            printf("\033CSROFF;");

			cursor_y--;  // Move cursor 1 line up

            current_line_length = 0;
            file_ptr--;
            file_offset--;

			// Search for end of previous line above current line (newline 0Ah)
            // TODO: End of line could be char position 80, not always a line feed
			while (*file_ptr != 0x0A) {
				file_ptr--;
                file_offset--;
            }

            file_ptr--;             // Move past newline
            file_offset--;
            current_line_length++;  // Include newline as end of current line

			// Search for either start of file (if 1st line) or end of line above
			//  previous line
            // TODO: End of line could be char position 80, not always a line feed
            while (*file_ptr != 0x0A && file_offset != 0) {
				file_ptr--;
                file_offset--;
                current_line_length++;
            }
            
            if (*file_ptr == 0x0A) {
                file_ptr++;         // Move to start of current line
                file_offset++;

            } else if (file_offset == 0) {
                current_line_length++;  // Include 1st byte of file
            }

            // If line is shorter than where cursor is, move cursor to end of shorter line
            if (current_line_length < (uint32_t)cursor_x + 1)  // Cursor is 0-based
                cursor_x = current_line_length - 1;

            file_ptr    += cursor_x;      // offset into line
            file_offset += cursor_x;
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);
            continue;
        }

        if (input_char == DOWNARROW) {    // Down arrow key
            if (cursor_y == file_length_lines)  // On last line of file
                continue;
                
            printf("\033CSROFF;");

            cursor_y++;  // Move cursor down 1 line

            current_line_length = 0;

			// Search file data forwards for a newline (0Ah)
            // TODO: End of line could be char position 80, not always a line feed
			while (*file_ptr != 0x0A) {
				file_ptr++;
                file_offset++;
            }

			// Found end of current line, move past newline
            file_ptr++;
            file_offset++;

            // Now search for end of next line or end of file
            //   File length is 1-based, offset is 0-based
            // TODO: End of line could be char position 80, not always a line feed
			while ((*file_ptr != 0x0A && *file_ptr != 0x00) && (file_offset != file_length_bytes - 1)) {
                file_ptr++;
                file_offset++;
                current_line_length++;
			}

            // Include end of file byte
            current_line_length++;

            // If line is shorter than where cursor is, move cursor to end of shorter line
            if (current_line_length < (uint32_t)cursor_x + 1)  // Cursor is 0-based
                cursor_x = current_line_length - 1;

            // Move to start of current line
            file_ptr    -= current_line_length - 1;    
            file_offset -= current_line_length - 1;    

            file_ptr    += cursor_x;   // Move to cursor position in line
            file_offset += cursor_x;

            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);
			continue;
        }

        if (input_char == HOMEKEY) {      // Home key
            printf("\033CSROFF;");

            // Move to beginning of line
            file_ptr    -= cursor_x;   // Move file data to start of line
            file_offset -= cursor_x;

            // Move cursor to start of line
            cursor_x = 0;           
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);
            continue;
        }

        if (input_char == ENDKEY) {       // End key
            printf("\033CSROFF;");

            // Move to end of line
            // Get difference of current_line_length and cursor_x (0-based),
            //   add this difference to cursor_x, and file data
            file_ptr    += ((current_line_length - 1) - cursor_x);
            file_offset += ((current_line_length - 1) - cursor_x);
            cursor_x    += ((current_line_length - 1) - cursor_x);

            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);
            continue;
        }

		// Else print out user input character to screen (assuming "insert" mode)
        current_line_length++;  // Update line length
        file_length_bytes++;    // Update file length
        if (input_char == 0x0D) file_length_lines++;  // Update file length

        // Move all file data forward 1 byte then fill in current character
        for (uint32_t i = (file_length_bytes - file_offset); i > 0; i--)
            file_ptr[i] = file_ptr[i-1];

        if (input_char == 0x0D) input_char = 0x0A;  // Convert CR to LF

        *file_ptr = input_char;     // Overwrite current character at cursor

        // Reprint file data, starting from cursor 
        save_x = cursor_x;
        save_y = cursor_y;
        save_file_ptr    = file_ptr;
        save_file_offset = file_offset;

        // Newline, reprint all lines until new end of file
        if (input_char == 0x0A) {
            // Blank out rest of current line
            for (uint8_t i = cursor_x; i < 80; i++)
                putchar(' ');

            // Blank out rest of file
            while (cursor_y <= file_length_lines) {
                for (uint8_t i = 0; i < 80; i++) {
                    cursor_x = i;
                    putchar(' ');
                }
            }

            // Reprint file data from current position
            cursor_x = save_x;
            cursor_y = save_y;
            while (*file_ptr != 0x00) {
                if (*file_ptr == 0x0A) {
                    // Newline
                    putchar(' ');
                    cursor_x = 0;
                    cursor_y++;

                } else putchar(*file_ptr);

                file_ptr++;
            }

        } else {
            // Not a newline, only print current line
            while (*file_ptr != 0x0A && *file_ptr != 0x00) {
                    putchar(*file_ptr);
                    file_ptr++;
            }
        }

        cursor_x = save_x;
        cursor_y = save_y;
        file_ptr = save_file_ptr;
        file_offset = save_file_offset;

        file_ptr++;
        file_offset++;  // Inserted new character, move forward 1

        // If inserted newline, go to start of next line
        if (input_char == 0x0A) {
            cursor_x = 0;
            cursor_y++;

            // Get length of new current line
            current_line_length = 0;
            while ((*file_ptr != 0x0A && *file_ptr != 0x00)) {
                file_ptr++;
                file_offset++;
                current_line_length++;
            }

            // Include end of line or end of file byte
            current_line_length++;

            // Move file data to start of line
            file_ptr    -= current_line_length - 1;
            file_offset -= current_line_length - 1;

        } else
            cursor_x++;

        printf("\033X%uY%u;", cursor_x, cursor_y);  // Move cursor
        unsaved = 1;        // Inserted new character, unsaved changes
    }

    if (fd >= 0) close(fd);     // File cleanup
}

void hex_editor(void) {
    write_bottom_screen_message(keybinds_hex_editor);  // Write keybinds to screen
    cursor_x = cursor_y = 0;            // Reset cursor
    printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor

    while (1) {
        input_char = get_key();

        // Check for hex editor keybinds
        // TODO: Use switch(input_char) here instead of if blocks
        if (input_char == RUNINPUT) {
            *(file_address + editor_filesize) = 0xCB; // CB = far return

            // Jump to and execute input
            //   Get around function pointer <-> object pointer warnings
            void (*entry)(void) = NULL; 
            *(void **)&entry = file_address;    
            entry();

            hex_count = 0;              // Reset byte counter
            file_ptr = file_address;    // Reset to hex memory location 20,000h
            file_offset = 0;
            
            // Reset cursor x/y
            cursor_x = cursor_y = 0;
            continue;
        } 

        if (input_char == ENDPGM) return;   // End program, return to caller

        if (input_char == SAVEPGM) {   // Does user want to save?
            printf("\033CSROFF;");
            save_hex_program();
            continue;
        }

        // Check backspace
        // TODO: Move all file data back 1 byte after blanking out current byte
        if (input_char == 0x08) {
            if (cursor_x >= 3) {
                // Blank out 1st and 2nd nibbles of hex byte 
                putchar(' ');
                putchar(' ');

                // Move cursor 1 full hex byte left on screen
                cursor_x -= 5;  // Cursor is after hex byte, go to start of previous hex byte
                printf("\033X%uY%u;", cursor_x, cursor_y);  // Move cursor

                *file_ptr = 0;  // Make current byte 0 in file
                file_ptr--;     // Move file data to previous byte
                file_offset--;
            }
            continue;
        }

        // Check delete key
        // TODO: Move all file data back 1 byte after blanking out current byte
        if (input_char == DELKEY) {
            // Blank out 1st and 2nd nibbles of hex byte
            putchar(' ');
            putchar(' ');

            *file_ptr = 0;  // Make current byte 0 in file
            cursor_x -= 2;  // Cursor is after hex byte, move back to 1st nibble of hex byte
            printf("\033X%uY%u;", cursor_x, cursor_y);  // Move cursor
            continue;
        }
        
        // Check navigation keys (arrows + home/end)
        if (input_char == LEFTARROW) {     // Left arrow key
            printf("\033CSROFF;");

            // Move 1 byte left (till beginning of line)
            if (cursor_x >= 3) {
                cursor_x -= 3;
                file_ptr--;     // Move file data to previous byte
                file_offset--;
            }
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor
            continue;
        }

        if (input_char == RIGHTARROW) {    // Right arrow key
            printf("\033CSROFF;");

            // Move 1 byte right (till end of line)
            if (cursor_x <= 75) {
                cursor_x += 3;
                file_ptr++;             // Move file data to next byte
                file_offset++;
            }
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor
            continue;
        }

        if (input_char == UPARROW) {     // Up arrow key
            printf("\033CSROFF;");

            // Move 1 line up
            if (cursor_y != 0) {
                cursor_y--;
                file_ptr -= 27;  // # of hex bytes (2 nibbles + space) in 1 line
                file_offset -= 27;
            }
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor
            continue;
        }

        if (input_char == DOWNARROW) {   // Down arrow key
            printf("\033CSROFF;");

            // Move 1 line down
            if (cursor_y != ((uint32_t)gfx_mode->y_resolution / 16) - 1) {  // At bottom row of screen?
                cursor_y++;
                file_ptr += 27;		// # of hex bytes (2 nibbles + space) in 1 line
                file_offset += 27;
            }
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor
            continue;
        }

        if (input_char == HOMEKEY) {     // Home key pressed
            printf("\033CSROFF;");

            // Move to beginning of line
            file_ptr -= (cursor_x / 3);       // Each hex byte on screen is 2 nibbles + space
            file_offset -= (cursor_x / 3);
            cursor_x = 0;
            
            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor
            continue;
        }

        if (input_char == ENDKEY) {       // End key pressed
            printf("\033CSROFF;");

            // Move to end of line
            file_ptr += (79 - cursor_x / 3);  // Each hex byte on screen is 2 nibbles + space
            file_offset += (79 - cursor_x / 3);
            cursor_x = 78;

            printf("\033X%uY%u;\033CSRON;", cursor_x, cursor_y);  // Move cursor
            continue;
        }

        // Check for valid hex digits
        if (!(input_char >= '0' && input_char <= '9') &&
            !(input_char >= 'A' && input_char <= 'F') &&
            !(input_char >= 'a' && input_char <= 'f')) {

            continue;
        } 

        if (input_char >= 'a' && input_char <= 'f')
            input_char -= 0x20;             // Convert lowercase to uppercase

        // Print character
        putchar(input_char);
        printf("\033X%uY%u;", cursor_x, cursor_y);  // Move cursor

        // Convert input_char to hexidecimal 
        if (input_char > '9') input_char -= 0x37;   // Ascii 'A'-'F', get hex A-F/decimal 10-15
        else                  input_char -= 0x30;   // Ascii '0'-'9', get hex/decimal 0-9 

        hex_count++;            // Increment byte counter
        if (hex_count == 2) {   // 2 ascii bytes = 1 hex byte
            hex_byte <<= 4;             // Move digit 4 bits to the left, make room for 2nd digit
            hex_byte |= input_char;     // Move 2nd ascii byte/hex digit into memory
            input_char = hex_byte; // TODO: Is this needed?
            *file_ptr++ = hex_byte;   // Put hex byte(2 hex digits) into 10000h memory area, and inc di/point to next byte
            file_offset++;
            editor_filesize++;          // Increment file size byte counter
            hex_count = 0;              // Reset byte counter

            // Print space to screen if not at start of line
            if (cursor_x != 0) { 
                putchar(' ');
                printf("\033X%uY%u;", cursor_x, cursor_y);  // Move cursor
            }

        } else {
            hex_byte = input_char;  // Put input into hex byte variable
        }
    }
}
	
void save_hex_program(void) {
	// Fill rest of sector with null data 
    memset(file_ptr, 0, 512 - (editor_filesize % 512)); 

	// Print filename string
    write_bottom_screen_message("Enter file name: ");
    printf("\033CSRON;");

	// Save file type - 'bin'
    editor_filetype = BIN;

	// Input file name to save
	input_file_name();

	// Call save_file function
    // file name, file ext, file size (hex sectors), address to save from
    // TODO: Allow file size > 1, don't hardcode 0x0001
    //if (!save_file(editor_filename, editor_filetype, 0x0001, (uint32_t)file_address)) {
    //    write_bottom_screen_message("Save file error occurred");   // Error on save_file()

    //} else {
        write_bottom_screen_message(keybinds_hex_editor);   // Write keybinds at bottom

        // Restore cursor position
        printf("\033X%uY%u;", cursor_x, cursor_y);
    //}
}

// Have user input a file name
void input_file_name(void) {
    uint8_t i;
    for (i = 0; i < sizeof editor_filename-1; i++) {
        input_char = get_key();
        if (input_char == '\r') break;
        editor_filename[i] = input_char;
        putchar(input_char);
    }
    editor_filename[i] = '\0';
}

// Blank out bottom line of screen, then write message 
void write_bottom_screen_message(char *msg) {
    printf("\033X%uY%u;%80s\r%s", 0u, screen_rows - 1, "", msg);
}
