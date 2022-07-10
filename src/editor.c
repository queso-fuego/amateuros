//
// editor.c: text editor with "modes", from hexidecimal monitor to ascii editing
//
#include "C/stdint.h"
#include "C/string.h"
#include "C/stdlib.h"
#include "C/stdio.h"
#include "gfx/2d_gfx.h"
#include "print/print_types.h"
#include "screen/cursor.h"
#include "screen/clear_screen.h"
#include "keyboard/keyboard.h"
#include "type_conversions/hex_to_ascii.h"
#include "fs/fs_impl.h"
#include "global/global_addresses.h"

#define ENDOFLINE  80
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

// Function declarations
void editor_load_file(char *filename);    
void text_editor(char *editor_filename, uint8_t *file_buf, FILE *file_ptr);
void hex_editor(char *editor_filename, uint8_t *file_buf, FILE *file_ptr);
void save_hex_program(char *filename, uint8_t *file_buf, FILE *file_ptr);
void input_file_name(char *editor_filename);
void fill_out_bottom_editor_message(const uint8_t *msg);
void write_bottom_screen_message(const uint8_t *msg);

// Global variables
uint32_t editor_filesize = 0;
uint8_t input_char = 0;
uint32_t current_line_length;
uint32_t file_length_lines;
uint32_t file_length_bytes;
static const uint8_t *filename_string = "Enter file name: ";
static const uint8_t *choose_file_msg = "File to load: ";
static const uint8_t *load_file_error_msg = "Load file error occurred, press any key to go back...";
static const uint8_t *save_file_error_msg = "Save file error occurred";

__attribute__ ((section ("editor_entry"))) int editor_main(int argc, char *argv[])
{
    const char new_or_current_string[] = "(C)reate new file or (L)oad existing file?";
    const char keybinds_text_editor[] = " Ctrl-R = Return to kernel Ctrl-S = Save file to disk";
    uint8_t editor_filename[60] = {0};
    FILE *file_ptr = 0;
    uint8_t *file_buf = 0;

    // If did not pass any arguments to editor, edit new file
    if (argc < 2) {
        clear_screen(user_gfx_info->bg_color);

        // Choose filetype string
        printf("\eX0Y0;\eCSRON;(B)inary/hex file or (O)ther file type (.txt, etc)?");

        // Get user input
        while ((input_char = get_key()) != BINFILE && input_char != OTHERFILE)
            ;

        printf("\eX0Y0;\eCSROFF;                                                    ");

        strcpy(editor_filename, "temp.txt");

        file_ptr = fopen(editor_filename, "rw"); 

        // Allocate default file size 
        file_buf = malloc(FS_BLOCK_SIZE);
        memset(file_buf, 0, FS_BLOCK_SIZE);

        file_length_lines = 0;
        file_length_bytes = 0;

        if (input_char == BINFILE) {
            hex_editor(editor_filename, file_buf, file_ptr);

        } else {
            fill_out_bottom_editor_message(keybinds_text_editor);   // Write keybinds & filetype at bottom of screen

            text_editor(editor_filename, file_buf, file_ptr);
        }

        // File cleanup
        fclose(file_ptr);
    
    } else {
        // Otherwise load file
        editor_load_file(argv[1]); 
    }

    return 0;
}

void editor_load_file(char *load_file)
{
    const uint8_t keybinds_text_editor[] = " Ctrl-R = Return to kernel Ctrl-S = Save file to disk";
    uint32_t file_size = 0;
    uint16_t cursor_x, cursor_y;
    uint8_t filename[60] = {0};

    strcpy(filename, load_file);

    FILE *file_ptr = fopen(filename, "rw");

    if (!file_ptr) {
        // Loading file error
        write_bottom_screen_message(load_file_error_msg);

        input_char = get_key();

        clear_screen(user_gfx_info->bg_color);

        return; 
    } 

    // Load file success
    // Get file size
    fseek(file_ptr, 0, SEEK_END);
    file_size = ftell(file_ptr);
    rewind(file_ptr);

    // Read file into buffer to work with
    uint8_t *file_buf = malloc(bytes_to_sectors(file_size) * FS_SECTOR_SIZE);
    fread(file_buf, bytes_to_sectors(file_size) * FS_SECTOR_SIZE, 1, file_ptr);
    rewind(file_ptr);   // Reset file just in case

    clear_screen(user_gfx_info->bg_color);

    // Reset cursor position
    cursor_x = 0;
    cursor_y = 0;

	// Go to editor depending on file type
    if (strcmp(&filename[strlen(filename) - 3], "bin") == 0) {
        // Load hex file
        // Load file bytes to screen
        for (uint32_t i = 0; i < file_size; i++) {
            uint8_t temp = (file_buf[i] >> 4) & 0x0F;   // Read hex byte from file location - 2 nibbles!
            temp = hex_to_ascii(temp);  // 1st nibble as ascii

            print_char(&cursor_x, &cursor_y, temp);

            temp = file_buf[i] & 0x0F;   // 2nd nibble
            temp = hex_to_ascii(temp);  // as ascii

            print_char(&cursor_x, &cursor_y, temp);

            if (cursor_x)    // At start of line? Print space between hex bytes
                print_char(&cursor_x, &cursor_y, SPACE);

            editor_filesize++;
        }

        hex_editor(filename, file_buf, file_ptr);

    } else {
        // Load text file
        file_length_lines = 0;
        file_length_bytes = 0;

        // Load file bytes to screen - stop at EOF if less than file size
        for (uint32_t i = 0; i < file_size; i++) { 
            if (file_buf[i] == '\0') break;

            if (file_buf[i] == '\n') {  
                print_char(&cursor_x, &cursor_y, SPACE);  // Newline = space visually

                // Go down 1 row
                cursor_x = 0;
                cursor_y++;
                file_length_lines++;

            } else if (file_buf[i] <= 0x0F) {  
                input_char = hex_to_ascii(file_buf[i]); 
                print_char(&cursor_x, &cursor_y, input_char);

            } else {
                print_char(&cursor_x, &cursor_y, file_buf[i]);
            }

            file_length_bytes++;
        }

        // Write keybinds at bottom of screen
        fill_out_bottom_editor_message(keybinds_text_editor);

        cursor_x = 0;                   // Reset cursor position
        cursor_y = 0;
        move_cursor(cursor_x, cursor_y);

        // Get length of first line
        current_line_length = 0;

        for (uint32_t i = 0; i != file_length_bytes; i++) { 
            if (file_buf[i] == '\n') break;

            current_line_length++;
        }

        current_line_length++;  // Include newline or last byte in file 

        text_editor(filename, file_buf, file_ptr);
    }

    // File cleanup
    fclose(file_ptr);
}

void text_editor(char *editor_filename, uint8_t *file_buf, FILE *file_ptr)
{
    const char keybinds_text_editor[] = " Ctrl-R: Return | Ctrl-S: Save | Ctrl-C: Chg name/ext | Ctrl-D: Del line";
    const char blank_line[] = "                                                                                ";
    const char file_ext_string[] = "Enter file extension: "; 
    uint16_t cursor_x, cursor_y, save_x, save_y;
    uint8_t save_file_offset;
    uint8_t changed_filename[10];
    uint8_t unsaved = 0;
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;
    uint8_t file_mode = 0;
    uint32_t file_offset = 0;

    if (file_length_bytes == 0) 
        file_mode = NEW;
    else 
        file_mode = UPDATE;

    // Get length of first line
    current_line_length = 0;
    for (uint32_t i = 0; file_buf[i] != '\0' && file_buf[i] != '\n'; i++)
        current_line_length++;

    // Write keybinds at bottom of screen
    fill_out_bottom_editor_message(keybinds_text_editor);
    cursor_x = 0;
    cursor_y = 0;
    move_cursor(cursor_x, cursor_y);

    while (true) {
        // Print cursor X/Y position and current line length
        printf("\eCSROFF;\eX%dY%d;", 1, (gfx_mode->y_resolution / font_height) - 2); // Above last line

        printf("X:%d Y:%d LEN:%d SIZE:%d ", 
               cursor_x, cursor_y, current_line_length, file_length_bytes);

        // Print filename & extension
        putc('[');
        for (uint8_t i = 0; editor_filename[i] != '\0'; i++) {
            putc(editor_filename[i]);   
        }

        puts("] ");

        // If unsaved changes, put asterisk
        if (unsaved) putc('*');

        // Print current file mode
        if (file_mode == NEW)
            puts("NEW ");
        else if (file_mode == UPDATE)
            puts("UPD ");

        // Get next input char
        input_char = get_key();

		// Check for text editor keybinds
        if (key_info->ctrl) {    // CTRL Key pressed
            if (input_char == 'r')  // CTRLR Return to kernel
                return; 

            if (input_char == 's') {    // CTRLS Save file to disk
                // Save cursor position
                save_x = cursor_x;
                save_y = cursor_y;

                printf("\eX%dY%d;", cursor_x, cursor_y); // Restore cursor position

                // Save file
                if (fwrite(file_buf, file_length_bytes, 1, file_ptr) != 1) {
                    write_bottom_screen_message(save_file_error_msg);   // Errored on saving file

                } else {
                    // If new file, input file name 
                    if (file_mode == NEW) {
                        remove_cursor(cursor_x, cursor_y);  // Erase cursor first

                        write_bottom_screen_message(blank_line);
                        write_bottom_screen_message(filename_string); // Enter file name at bottom of screen
                        printf("\eCSRON;");

                        input_file_name(editor_filename);  // Enter file name

                        // Change filename from "temp.txt" to input name
                        fs_rename_file("temp.txt", editor_filename);

                        file_mode = UPDATE;
                    }

                    printf("\eX%dY%d;", cursor_x, cursor_y); // Restore cursor position

                    unsaved = 0;    // User saved file, no more unsaved changes

                    fill_out_bottom_editor_message(blank_line);
                    fill_out_bottom_editor_message(keybinds_text_editor); // Write keybinds at bottom

                    cursor_x = save_x;
                    cursor_y = save_y;
                    move_cursor(cursor_x, cursor_y);
                }

                continue;
            }

            // Ctrl-C: Change file name & extension
            if (input_char == 'c') {
                save_x = cursor_x;
                save_y = cursor_y;
                remove_cursor(cursor_x, cursor_y);  // Erase cursor

                write_bottom_screen_message(blank_line);
                write_bottom_screen_message(filename_string);   // Enter file name 
                move_cursor(cursor_x, cursor_y);

                // Get old file name
                strncpy(changed_filename, editor_filename, 10);

                // Input new file name
                input_file_name(editor_filename);

                write_bottom_screen_message(blank_line);

                // Call rename_file() with new file name/ext to overwrite filetable
                fs_rename_file(editor_filename, changed_filename);

                fill_out_bottom_editor_message(keybinds_text_editor);

                cursor_x = save_x;
                cursor_y = save_y;
                move_cursor(cursor_x, cursor_y);

                continue;
            }

            // Ctrl-D: Delete current line
            if (input_char == 'd') {
                // If on last line of file, skip TODO: Put in deleting last line
                if (cursor_y == file_length_lines) continue;

                // Move all lines after this one up 1 line
                file_offset -= cursor_x;
                save_file_offset = file_offset;

                while (file_offset < file_length_bytes - current_line_length + 1) {
                    file_buf[file_offset] = file_buf[file_offset + current_line_length];
                    file_offset++;
                }
                
                // Blank out all lines from current line to end of file
                save_y = cursor_y;
                while (cursor_y <= file_length_lines) {
                    cursor_x = 0;
                    for (uint8_t j = 0; j < 79; j++)
                        print_char(&cursor_x, &cursor_y, SPACE);

                    cursor_y++;
                }

                file_length_lines--;    // 1 less line in the file now
                file_length_bytes -= current_line_length;

                // Redraw all characters from current line to end of file
                // TODO: Look into refactoring all of this for better performance?
                cursor_x = 0;
                cursor_y = save_y;

                while (file_buf[file_offset] != 0x00) {
                    if (file_buf[file_offset] == 0x0A) {
                        // Go to next line
                        print_char(&cursor_x, &cursor_y, SPACE); // Print space visually
                        cursor_x = 0;
                        cursor_y++;

                    } else
                        print_char(&cursor_x, &cursor_y, file_buf[file_offset]);

                    file_offset++;
                }

                // Restore cursor position to start of new line
                cursor_x = 0;
                cursor_y = save_y;
                move_cursor(cursor_x, cursor_y);

                file_offset = save_file_offset;

                // Get new current line length
                current_line_length = 0;
                while (file_buf[file_offset] != 0x0A && file_buf[file_offset] != 0x00) {
                    file_offset++;
                    current_line_length++;
                }

                file_offset = save_file_offset;
                continue;
            }
        }
			
        // Backspace or delete
        // TODO: May not be consistent with multiple lines and deleting at different positions
        if (input_char == 0x08 || input_char == DELKEY) {
            if (input_char == 0x08 && cursor_x == 0) continue;  // Skip backspace at start of line

            // TODO: Handle newline deletion
            if (file_buf[file_offset] == 0x0A) continue;

            // At end of file? Move cursor back
            if (file_buf[file_offset] == 0x00 && file_offset != 0) {
                remove_cursor(cursor_x, cursor_y);

                // Go back 1 character
                cursor_x--;
                file_offset--;
                current_line_length--;

                continue;
            }

            // Backspace, move back 1 character/byte
            if (input_char == 0x08) {
                cursor_x--;
                file_offset--;
            }

            // Move all file data ahead of cursor back 1 byte
            for (uint32_t i = 0; i < (file_length_bytes - file_offset); i++)
                file_ptr[i] = file_ptr[i+1];

            file_length_bytes--;    // Deleted a char/byte from file data

            // Rewrite this line
            save_x = cursor_x;

            file_offset -= cursor_x;   // Start of line
            cursor_x = 0;
            save_file_offset = file_offset;

            // Redraw line until end of line or end of file
            while (file_buf[file_offset] != 0x0A && file_buf[file_offset] != 0x00) {
                print_char(&cursor_x, &cursor_y, file_buf[file_offset]);
                file_offset++;
            }
            print_char(&cursor_x, &cursor_y, SPACE);    // Previous end of line now = space
            current_line_length--;

            // Restore cursor_file position
            file_offset = save_file_offset;
            cursor_x = save_x;
            move_cursor(cursor_x, cursor_y);

            unsaved = 1;    // File now has unsaved changes

            continue;
        }

        if (input_char == LEFTARROW) {    // Left arrow key
            // Move 1 byte left (till beginning of line)
            if (cursor_x != 0) {
                remove_cursor(cursor_x, cursor_y);

                file_offset--;
                cursor_x--;     // Move cursor to previous character
                move_cursor(cursor_x, cursor_y);
            }
            continue;
        }

        if (input_char == RIGHTARROW) {    // Right arrow key
            // Move 1 byte right (till end of line)
            if (cursor_x+1 < current_line_length) { 
                remove_cursor(cursor_x, cursor_y);

                file_offset++;
                cursor_x++;
                move_cursor(cursor_x, cursor_y);  // Cursor will be 1 forward from print_char above
            }
            continue;
        }

        if (input_char == UPARROW) {      // Up arrow key
            if (cursor_y == 0)  // On 1st line, can't go up
                continue;

            remove_cursor(cursor_x, cursor_y);

			cursor_y--;  // Move cursor 1 line up

            current_line_length = 0;
            file_offset--;

			// Search for end of previous line above current line (newline 0Ah)
            // TODO: End of line could be at end of screen line, not always a line feed
			while(file_buf[file_offset] != 0x0A) 
                file_offset--;

            file_offset--;          // Move past newline
            current_line_length++;  // Include newline as end of current line

			// Search for either start of file (if 1st line) or end of line above
			//  previous line
            // TODO: End of line could be at end of screen line, not always a line feed
            while (file_buf[file_offset] != 0x0A && file_offset != 0) {
                file_offset--;
                current_line_length++;
            }
            
            if (file_buf[file_offset] == 0x0A) {
                file_offset++;

            } else if (file_offset == 0) {
                current_line_length++;  // Include 1st byte of file
            }

            // If line is shorter than where cursor is, move cursor to end of shorter line
            if (current_line_length < cursor_x + 1)  // Cursor is 0-based
                cursor_x = current_line_length - 1;

            file_offset += cursor_x;
            move_cursor(cursor_x, cursor_y);

            continue;
        }

        if (input_char == DOWNARROW) {    // Down arrow key
            if (cursor_y == file_length_lines)  // On last line of file
                continue;
                
            remove_cursor(cursor_x, cursor_y);

            cursor_y++;  // Move cursor down 1 line

            current_line_length = 0;

			// Search file data forwards for a newline (0Ah)
            // TODO: End of line could be at end of screen line, not always a line feed
			while (file_buf[file_offset] != 0x0A) 
                file_offset++;

			// Found end of current line, move past newline
            file_offset++;

            // Now search for end of next line or end of file
            //   File length is 1-based, offset is 0-based
            // TODO: End of line could be at end of screen line, not always a line feed
			while ((file_buf[file_offset] != 0x0A && file_buf[file_offset] != 0x00) && (file_offset != file_length_bytes - 1)) {
                file_offset++;
                current_line_length++;
			}

            // Include end of file byte
            current_line_length++;

            // If line is shorter than where cursor is, move cursor to end of shorter line
            if (current_line_length < cursor_x + 1)  // Cursor is 0-based
                cursor_x = current_line_length - 1;

            // Move to start of current line
            file_offset -= current_line_length - 1;    

            // Move to cursor position in line
            file_offset += cursor_x;    

            move_cursor(cursor_x, cursor_y);

			continue;
        }

        if (input_char == HOMEKEY) {      // Home key
            remove_cursor(cursor_x, cursor_y);

            // Move to beginning of line
            file_offset -= cursor_x;    // Move file data to start of line

            // Move cursor to start of line
            cursor_x = 0;           
            move_cursor(cursor_x, cursor_y);

            continue;
        }

        if (input_char == ENDKEY) {       // End key
            remove_cursor(cursor_x, cursor_y);

            // Move to end of line
            // Get difference of current_line_length and cursor_x (0-based),
            //   add this difference to cursor_x, and file data
            file_offset += ((current_line_length - 1) - cursor_x);
            cursor_x    += ((current_line_length - 1) - cursor_x);

            move_cursor(cursor_x, cursor_y);
    
            continue;
        }

		// Else print out user input character to screen (assuming "insert" mode)
        current_line_length++;  // Update line length
        file_length_bytes++;    // Update file length
        if (input_char == 0x0D) {
            file_length_lines++;  // Update file length
            input_char = 0x0A;  // Convert CR to LF
        }

        // Move all file data forward 1 byte then fill in current character
        for (uint32_t i = file_length_bytes; i > file_offset; i--)
            file_buf[i] = file_buf[i-1];

        file_buf[file_offset] = input_char;     // Overwrite current character at cursor

        // Reprint file data, starting from cursor 
        save_x = cursor_x;
        save_y = cursor_y;
        save_file_offset = file_offset;

        // Newline, reprint all lines until new end of file
        if (input_char == 0x0A) {
            // Blank out rest of current line
            for (uint8_t i = cursor_x; i < 80; i++)
                print_char(&cursor_x, &cursor_y, SPACE);

            // Blank out rest of file
            while (cursor_y <= file_length_lines) {
                for (uint8_t i = 0; i < 80; i++) {
                    cursor_x = i;
                    print_char(&cursor_x, &cursor_y, SPACE);
                }
            }

            // Reprint file data from current position
            cursor_x = save_x;
            cursor_y = save_y;
            while (file_buf[file_offset] != 0x00) {
                if (file_buf[file_offset] == 0x0A) {
                    // Newline
                    print_char(&cursor_x, &cursor_y, SPACE);
                    cursor_x = 0;
                    cursor_y++;

                } else 
                    print_char(&cursor_x, &cursor_y, file_buf[file_offset]);

                file_offset++;
            }

        } else {
            // Not a newline, only print current line
            while (file_buf[file_offset] != 0x0A && file_buf[file_offset] != 0x00) {
                    print_char(&cursor_x, &cursor_y, file_buf[file_offset]);
                    file_offset++;
            }
        }

        cursor_x = save_x;
        cursor_y = save_y;
        file_offset = save_file_offset;

        file_offset++;  // Inserted new character, move forward 1

        // If inserted newline, go to start of next line
        if (input_char == 0x0A) {
            cursor_x = 0;
            cursor_y++;

            // Get length of new current line
            current_line_length = 0;
            while ((file_buf[file_offset] != 0x0A && file_buf[file_offset] != 0x00)) {
                file_offset++;
                current_line_length++;
            }

            // Include end of line or end of file byte
            current_line_length++;

            // Move file data to start of line
            file_offset -= current_line_length - 1;

        } else
            cursor_x++;

        move_cursor(cursor_x, cursor_y);    // Move cursor on screen
        unsaved = 1;        // Inserted new character, unsaved changes
    }
}

void hex_editor(char *filename, uint8_t *file_buf, FILE *file_ptr)
{
    uint8_t *keybinds_hex_editor = " $ = Run code ? = Return to kernel S = Save file to disk";
    uint32_t file_offset = 0;
    uint8_t hex_count = 0;
    uint8_t hex_byte = 0;   // 1 byte/2 hex digits
    uint16_t cursor_x, cursor_y;

    fill_out_bottom_editor_message(keybinds_hex_editor);  // Write keybinds to screen
    cursor_x = 0;   // Initialize cursor
    cursor_y = 0;
    move_cursor(cursor_x, cursor_y);    // Draw initial cursor

    while (1) {
        input_char = get_key();

        // Check for hex editor keybinds
        if (input_char == RUNINPUT) {
            file_buf[editor_filesize] = 0xCB;       // CB = far return

            ((void (*)(void))file_buf)();           // Jump to and execute input

            hex_count = 0;                          // Reset byte counter
            file_offset = 0;                        // Reset to start of file
            
            // Reset cursor x/y
            cursor_x = 0;
            cursor_y = 0;

            continue;
        } 

        if (input_char == ENDPGM)      // End program, return to caller
            return;

        if (input_char == SAVEPGM) {   // Does user want to save?
            remove_cursor(cursor_x, cursor_y); 

            save_hex_program(filename, file_buf, file_ptr);
            continue;
        }

        // Check backspace
        // TODO: Move all file data back 1 byte after blanking out current byte
        if (input_char == 0x08) {
            if (cursor_x >= 3) {
                // Blank out 1st and 2nd nibbles of hex byte 
                print_char(&cursor_x, &cursor_y, SPACE);     // space ' ' in ascii
                print_char(&cursor_x, &cursor_y, SPACE);     // space ' ' in ascii

                // Move cursor 1 full hex byte left on screen
                cursor_x -= 5;  // Cursor is after hex byte, go to start of previous hex byte
                move_cursor(cursor_x, cursor_y);

                file_buf[file_offset--] = 0;  // Make current byte 0 in file
            }

            continue;
        }

        // Check delete key
        // TODO: Move all file data back 1 byte after blanking out current byte
        if (input_char == DELKEY) {
            // Blank out 1st and 2nd nibbles of hex byte
            print_char(&cursor_x, &cursor_y, SPACE);     // space ' ' in ascii
            print_char(&cursor_x, &cursor_y, SPACE);     // space ' ' in ascii

            file_buf[file_offset] = 0;  // Make current byte 0 in file
            cursor_x -= 2;  // Cursor is after hex byte, move back to 1st nibble of hex byte
            move_cursor(cursor_x, cursor_y);

            continue;
        }
        
        // Check navigation keys (arrows + home/end)
        if (input_char == LEFTARROW) {     // Left arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 byte left (till beginning of line)
            if (cursor_x >= 3) {
                cursor_x -= 3;
                move_cursor(cursor_x, cursor_y);
                file_offset--;  // Move file data to previous byte
            }

            continue;
        }

        if (input_char == RIGHTARROW) {    // Right arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 byte right (till end of line)
            if (cursor_x <= 75) {
                cursor_x += 3;
                move_cursor(cursor_x, cursor_y);
                file_offset++;      // Move file data to next byte
            }

            continue;
        }

        if (input_char == UPARROW) {     // Up arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 line up
            if (cursor_y != 0) {
                cursor_y--;
                file_offset -= 27;  // # of hex bytes (2 nibbles + space) in 1 line
                move_cursor(cursor_x, cursor_y);
            }

            continue;
        }

        if (input_char == DOWNARROW) {   // Down arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 line down
            if (cursor_y != (gfx_mode->y_resolution / 16) - 1)	{   // At bottom row of screen?
                cursor_y++;
                file_offset += 27;      // # of hex bytes (2 nibbles + space) in 1 line
                move_cursor(cursor_x, cursor_y);
            }

            continue;
        }

        if (input_char == HOMEKEY) {     // Home key pressed
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move to beginning of line
            file_offset -= (cursor_x / 3);  // Each hex byte on screen is 2 nibbles + space
            cursor_x = 0;
            
            move_cursor(cursor_x, cursor_y);

            continue;
        }

        if (input_char == ENDKEY) {       // End key pressed
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move to end of line
            file_offset += (79 - cursor_x / 3);     // Each hex byte on screen is 2 nibbles + space
            cursor_x = 78;

            move_cursor(cursor_x, cursor_y);

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
        print_char(&cursor_x, &cursor_y, input_char);
        move_cursor(cursor_x, cursor_y);

        // Convert input_char to hexidecimal 
        if (input_char > '9')
            input_char -= 0x37;     // Ascii 'A'-'F', get hex A-F/decimal 10-15
        else
            input_char -= 0x30;     // Ascii '0'-'9', get hex/decimal 0-9 

        hex_count++;            // Increment byte counter
        if (hex_count == 2) {   // 2 ascii bytes = 1 hex byte
            hex_byte <<= 4;                         // Move digit 4 bits to the left, make room for 2nd digit
            hex_byte |= input_char;                 // Move 2nd ascii byte/hex digit into memory
            input_char = hex_byte;                  // TODO: Is this needed?
            file_buf[file_offset++] = hex_byte;     // Put hex byte(2 hex digits) into file area, and point to next byte
            editor_filesize++;                      // Increment file size byte counter
            hex_count = 0;                          // Reset byte counter

            // Print space to screen if not at start of line
            if (cursor_x != 0) { 
                print_char(&cursor_x, &cursor_y, SPACE);
                move_cursor(cursor_x, cursor_y);
            }

        } else {
            hex_byte = input_char;  // Put input into hex byte variable
        }
    }
}
	
void save_hex_program(char *filename, uint8_t *file_buf, FILE *file_ptr)
{
    uint8_t *keybinds_hex_editor = " $ = Run code ? = Return to kernel S = Save file to disk\0";
    char blank_line[80];
    uint16_t cursor_x, cursor_y;

    memset(blank_line, ' ', sizeof blank_line);
    blank_line[79] = '\0';

	// Fill out rest of sector with data if not already filled out
	// Divide file size by 512 to get # of sectors filled out and remainder of sector not filled out
    if (editor_filesize / 512 == 0) { // Less than 1 sector filled out?
        if (editor_filesize % 512 != 0) 
            memset(file_buf, 0, 512 - (editor_filesize % 512));
        else 
            memset(file_buf, 0, 512); // Otherwise file is empty, fill out 1 whole sector	

    } else if (editor_filesize % 512 != 0) {
        memset(file_buf, 0, 512 - (editor_filesize % 512)); // Fill out rest of sector with 0s
    }

	// Print filename string
    write_bottom_screen_message(blank_line);
    write_bottom_screen_message(filename_string);
    move_cursor(cursor_x, cursor_y);

	// Input file name to save
	input_file_name(filename);

	// Call save_file function
    if (fwrite(file_buf, editor_filesize, 1, file_ptr) != 1) {
        write_bottom_screen_message(save_file_error_msg);   // Error on save_file()

    } else {
        write_bottom_screen_message(keybinds_hex_editor);   // Write keybinds at bottom

        // Init cursor pos
        // TODO: Restore cursor to last user position, not start of file
        cursor_x = 0;
        cursor_y = 0;
        move_cursor(cursor_x, cursor_y);
    }
}

// Have user input a file name
void input_file_name(char *editor_filename)
{
	// Save file name
    for (uint8_t i = 0; (input_char = get_key()) != '\r' && i < 60; i++) {
        editor_filename[i] = input_char;

		// Print character to screen
        putc(input_char);
    }
}

// Subroutine: fill out message at bottom of editor
void fill_out_bottom_editor_message(const uint8_t *msg)
{
    uint8_t bottom_msg[80];

	// Fill string variable with message to write
    strcpy(bottom_msg, msg);
	
	write_bottom_screen_message(bottom_msg);
}

// Write message at bottom of screen
void write_bottom_screen_message(const uint8_t *msg)
{
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;

    printf("\eX%dY%d;%s", 0, (gfx_mode->y_resolution / font_height) - 1, msg);
}
