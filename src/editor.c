//
// editor.c: text editor with "modes", from hexidecimal monitor to ascii editing
//
#include "../include/C/stdint.h"
#include "../include/C/string.h"
#include "../include/gfx/2d_gfx.h"
#include "../include/print/print_types.h"
#include "../include/disk/file_ops.h"
#include "../include/print/print_fileTable.h"
#include "../include/screen/cursor.h"
#include "../include/screen/clear_screen.h"
#include "../include/keyboard/get_key.h"
#include "../include/type_conversions/hex_to_ascii.h"

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
 
void editor_load_file(void);    // Function declarations
void text_editor(void);
void hex_editor(void);
void save_hex_program(void);
void input_file_name(void);
void fill_out_bottom_editor_message(const uint8_t *msg);
void write_bottom_screen_message(const uint8_t *msg);

uint8_t editor_filetype[3];     // Global variables
uint8_t editor_filename[10];
uint16_t editor_filesize = 0;
uint8_t input_char;
uint16_t cursor_x = 0;
uint16_t cursor_y = 0;
uint8_t *file_ptr;
uint16_t file_offset;
uint16_t current_line_length;
uint16_t file_length_lines;
uint16_t file_length_bytes;
static const uint8_t *filename_string = "Enter file name: \0";
uint8_t scancode;
uint8_t ctrl_key;
uint8_t hex_count = 0;
uint8_t blank_line[80];
uint8_t *extBin = "bin";
static const uint8_t *choose_file_msg = "File to load: \0";
static const uint8_t *load_file_error_msg = "Load file error occurred";
static const uint8_t *save_file_error_msg = "Save file error occurred";
uint8_t hex_byte = 0;   // 1 byte/2 hex digits
uint8_t bottom_msg[80];
uint8_t file_mode;     

__attribute__ ((section ("editor_entry"))) void editor_main(void)
{
    uint8_t *new_or_current_string = "(C)reate new file or (L)oad existing file?\0";
    uint8_t *choose_filetype_string = "(B)inary/hex file or (O)ther file type (.txt, etc)?\0";
    uint8_t *keybinds_text_editor = " Ctrl-R = Return to kernel Ctrl-S = Save file to disk\0";

    // Fill out blank line variable
    memset(blank_line, ' ', 79);
    blank_line[79] = '\0';

	clear_screen(user_gfx_info->bg_color); // Initial screen clear

	// Print options string
    print_string(&cursor_x, &cursor_y, new_or_current_string);
    move_cursor(cursor_x, cursor_y); 

    // Choose option
    input_char = get_key();
    while (input_char != CREATENEW && input_char != LOADCURR)
        input_char = get_key();

    if (input_char == CREATENEW) {
        clear_screen(user_gfx_info->bg_color);

        file_mode = NEW;    // Creating a new file

        // Initialize cursor values
        cursor_x = 0;
        cursor_y = 0;

        // Choose filetype string
        print_string(&cursor_x, &cursor_y, choose_filetype_string);
        move_cursor(cursor_x, cursor_y); 

        editor_filesize = 0;        // Reset file size byte counter

        input_char = get_key();     // Get user input
        while (input_char != BINFILE && input_char != OTHERFILE)
            input_char = get_key();

        clear_screen(user_gfx_info->bg_color);

        // Reset cursor position
        cursor_x = 0;
        cursor_y = 0;

        file_ptr = (uint8_t *)0x20000;  // New file starts at 20,000h
        file_offset = 0;

        if (input_char == BINFILE) {
            // Fill out filetype (.bin)
            strncpy(editor_filetype, "bin", 3);

            hex_editor();

        } else {
            // Fill out filetype (.txt)
            strncpy(editor_filetype, "txt", 3);
            fill_out_bottom_editor_message(keybinds_text_editor);   // Write keybinds & filetype at bottom of screen

            // Fill out 1 blank sector for new file
            memset(file_ptr, 0, 512);

            // Initialize cursor and file variables for new file
            cursor_x = 0;
            cursor_y = 0;
            current_line_length = 0;
            file_length_lines = 0;
            file_length_bytes = 0;

            text_editor();
        }
    
    } else {
        // Otherwise load file
        file_mode = UPDATE;   // Updating an existing file
        editor_load_file();
    }
}

void editor_load_file(void)
{
    uint8_t idx;
    uint8_t *keybinds_text_editor = " Ctrl-R = Return to kernel Ctrl-S = Save file to disk\0";

    // Choose file to load
    while (1) {
        print_fileTable(&cursor_x, &cursor_y);

        // Choose file message
        print_string(&cursor_x, &cursor_y, choose_file_msg);
        move_cursor(cursor_x, cursor_y); 

        // Have user input file name to load
        input_file_name();

        // Load file from input file name
        // filename, filename length, memory to load file to, file extension
        if (load_file(editor_filename, 10, 0x20000, editor_filetype) == 0)
            break;  // Success

        // Loading file error
        write_bottom_screen_message(load_file_error_msg);

        input_char = get_key();

        clear_screen(user_gfx_info->bg_color);

        // Initialize cursor pos
        cursor_x = 0;
        cursor_y = 0;
    }

    // Load file success
	// Go to editor depending on file type
    if (strncmp(editor_filetype, extBin, 3) == 0) {
        // Load hex file
        clear_screen(user_gfx_info->bg_color);

        // Reset cursor position
        cursor_x = 0;
        cursor_y = 0;
     
        // Load file bytes to screen
        file_ptr = (uint8_t *)0x20000;  // File location

        // TODO: Change to actual file size, not 1 sector/512 bytes
        for (uint16_t i = 0; i < 512; i++) {
            input_char = (*file_ptr >> 4) & 0x0F;   // Read hex byte from file location - 2 nibbles!
            input_char = hex_to_ascii(input_char);  // 1st nibble as ascii

            // Print char
            print_char(&cursor_x, &cursor_y, input_char);

            input_char = *file_ptr & 0x0F;          // 2nd nibble
            input_char = hex_to_ascii(input_char);  // as ascii

            // Print char in AL
            print_char(&cursor_x, &cursor_y, input_char);

            if (cursor_x)    // At start of line? Print space between hex bytes
                print_char(&cursor_x, &cursor_y, SPACE);

            file_ptr++;
            editor_filesize++;
        }

        hex_count = 0;      // Reset byte counter
        file_ptr = (uint8_t *)0x20000;  // File location
        file_offset = 0;
        
        hex_editor();

    } else {
        // Load text file
        clear_screen(user_gfx_info->bg_color);

        cursor_x = 0;
        cursor_y = 0;
        current_line_length = 0;
        file_length_lines = 0;
        file_length_bytes = 0;
        file_ptr = (uint8_t *)0x20000;  // File location

        // Load file bytes to screen - stop at EOF if not 512 bytes
        // TODO: Change to actual file size, not just 1 sector
        for (uint16_t i = 0; i < 512 && *file_ptr != 0x00; i++) { 
            if (*file_ptr == 0x0A) {
                print_char(&cursor_x, &cursor_y, SPACE);  // Newline = space visually

                // Go down 1 row
                cursor_x = 0;
                cursor_y++;
                file_length_lines++;

            } else if (*file_ptr <= 0x0F) {
                input_char = hex_to_ascii(*file_ptr);
                print_char(&cursor_x, &cursor_y, input_char);

            } else {
                print_char(&cursor_x, &cursor_y, *file_ptr);
            }

            file_ptr++;
            file_offset++;
            file_length_bytes++;
        }

        // Write keybinds at bottom of screen
        fill_out_bottom_editor_message(keybinds_text_editor);

        file_ptr   -= file_offset;  // Reset file location
        file_offset = 0;
        cursor_x = 0;                   // Reset cursor position
        cursor_y = 0;

        move_cursor(cursor_x, cursor_y);

        // Get length of first line
        current_line_length = 0;
        while (*file_ptr != 0x0A && file_offset != file_length_bytes) {
            file_ptr++;
            file_offset++;
            current_line_length++;
        }

        current_line_length++;  // Include newline or last byte in file

        file_ptr   -= file_offset;  // Reset file location
        file_offset = 0;

        text_editor();
    }
}

void text_editor(void)
{
    uint8_t *keybinds_text_editor = " Ctrl-R: Return | Ctrl-S: Save | Ctrl-C: Chg name/ext | Ctrl-D: Del line\0";
    uint16_t save_x, save_y;
    uint8_t *x = "X:\0";
    uint8_t *y = "Y:\0";
    uint8_t *length = "LEN:\0";
    uint8_t *filesize = "SIZE:\0";
    uint8_t *file_ext_string = "Enter file extension: \0";  // User input file extension
    uint8_t *save_file_ptr;
    uint8_t save_file_offset;
    uint8_t changed_filename[10];
    uint8_t unsaved = 0;
    uint8_t *new = "NEW \0";
    uint8_t *upd = "UPD \0";
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;

    // Write keybinds at bottom of screen
    fill_out_bottom_editor_message(keybinds_text_editor);
    cursor_x = 0;
    cursor_y = 0;
    move_cursor(cursor_x, cursor_y);

    while (1) {
        // Draw cursor X/Y position and current line length
        save_x = cursor_x;
        save_y = cursor_y;
        cursor_x = 1;
        cursor_y = (gfx_mode->y_resolution / font_height) - 2;  // Above last line
        print_string(&cursor_x, &cursor_y, x); 
        print_hex(&cursor_x, &cursor_y, save_x);    // Cursor X

        cursor_x++;
        print_string(&cursor_x, &cursor_y, y); 
        print_hex(&cursor_x, &cursor_y, save_y);    // Cursor Y

        cursor_x++;
        print_string(&cursor_x, &cursor_y, length); 
        print_hex(&cursor_x, &cursor_y, current_line_length); // Current line length

        cursor_x++;
        print_string(&cursor_x, &cursor_y, filesize);
        print_hex(&cursor_x, &cursor_y, file_length_bytes); // Current file size

        cursor_x++;
        print_char(&cursor_x, &cursor_y, '[');
        for (uint8_t i = 0; i < 10; i++)
            print_char(&cursor_x, &cursor_y, editor_filename[i]);   // Filename

        print_char(&cursor_x, &cursor_y, '.');
        for (uint8_t i = 0; i < 3; i++)
            print_char(&cursor_x, &cursor_y, editor_filetype[i]);   // File extension

        print_char(&cursor_x, &cursor_y, ']');
        print_char(&cursor_x, &cursor_y, ' ');
        print_char(&cursor_x, &cursor_y, ' ');

        // If unsaved changes, put asterisk
        if (unsaved) print_char(&cursor_x, &cursor_y, '*');

        // Print current file mode
        if (file_mode == NEW)
            print_string(&cursor_x, &cursor_y, new);
        else if (file_mode == UPDATE)
            print_string(&cursor_x, &cursor_y, upd);

        cursor_x = save_x;  // Restore cursor position
        cursor_y = save_y;
        move_cursor(cursor_x, cursor_y);

        input_char = get_key();

        scancode = *(uint8_t *)0x1601;  // Scancode for key stored from get_key
        ctrl_key = *(uint8_t *)0x1603;  // Ctrl key status stored from get_key

		// Check for text editor keybinds
        if (ctrl_key == 1) {    // CTRL Key pressed
            if (input_char == 'r')  // CTRLR Return to kernel
                return; 

            if (input_char == 's') {    // CTRLS Save file to disk
                // Save cursor position
                save_x = cursor_x;
                save_y = cursor_y;

                // If new file, input file name and extension
                if (file_mode == NEW) {
                    remove_cursor(cursor_x, cursor_y);  // Erase cursor first

                    write_bottom_screen_message(blank_line);
                    write_bottom_screen_message(filename_string); // Enter file name at bottom of screen
                    move_cursor(cursor_x, cursor_y);

                    input_file_name();  // Enter file name

                    // Input file extension
                    write_bottom_screen_message(blank_line);
                    write_bottom_screen_message(file_ext_string);   // Enter file extension
                    move_cursor(cursor_x, cursor_y);

                    for (uint8_t i = 0; i < 3; i++) {
                        editor_filetype[i] = get_key();
                        print_char(&cursor_x, &cursor_y, editor_filetype[i]);
                        move_cursor(cursor_x, cursor_y);
                    }

                    file_mode = UPDATE;
                    fill_out_bottom_editor_message(keybinds_text_editor);
                }

                // Call save file
                // file name, file type, file size, address to save from
                if (save_file(editor_filename, editor_filetype, 0x0001, 0x20000) != 0) {
                    write_bottom_screen_message(save_file_error_msg);   // Errored on save_file()

                } else {
                    write_bottom_screen_message(keybinds_text_editor);  // Write keybinds at bottom

                    cursor_x = save_x;
                    cursor_y = save_y;
                    move_cursor(cursor_x, cursor_y);
                }

                unsaved = 0;    // User saved file, no more unsaved changes

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
                input_file_name();

                // Input new file extension
                write_bottom_screen_message(blank_line);
                write_bottom_screen_message(file_ext_string);
                move_cursor(cursor_x, cursor_y);

                for (uint8_t i = 0; i < 3; i++) {
                    editor_filetype[i] = get_key();
                    print_char(&cursor_x, &cursor_y, editor_filetype[i]);
                    move_cursor(cursor_x, cursor_y);
                }

                // Call rename_file() with new file name/ext to overwrite filetable
                rename_file(changed_filename, 10, editor_filename, 10);

                // Call save_file() to update file ext and data on disk
                if (save_file(editor_filename, editor_filetype, 1, 0x20000) == 0) {
                    fill_out_bottom_editor_message(keybinds_text_editor);

                    cursor_x = save_x;
                    cursor_y = save_y;
                    move_cursor(cursor_x, cursor_y);

                } else {
                    // TODO: handle save errors here
                }

                unsaved = 0;  // Saved file, no unsaved changes now

                continue;
            }

            // Ctrl-D: Delete current line
            if (input_char == 'd') {
                // If on last line of file, skip TODO: Put in deleting last line
                if (cursor_y == file_length_lines) continue;

                // Move all lines after this one up 1 line
                file_ptr    -= cursor_x;
                file_offset -= cursor_x;
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
                        print_char(&cursor_x, &cursor_y, SPACE);

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
                        print_char(&cursor_x, &cursor_y, SPACE); // Print space visually
                        cursor_x = 0;
                        cursor_y++;

                    } else
                        print_char(&cursor_x, &cursor_y, *file_ptr);

                    file_ptr++;
                }

                // Restore cursor position to start of new line
                cursor_x = 0;
                cursor_y = save_y;
                move_cursor(cursor_x, cursor_y);

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
        if (input_char == 0x08 || scancode == DELKEY) {
            if (input_char == 0x08 && cursor_x == 0) continue;  // Skip backspace at start of line

            // TODO: Handle newline deletion
            if (*file_ptr == 0x0A) continue;

            // At end of file? Move cursor back
            if (*file_ptr == 0x00 && file_offset != 0) {
                remove_cursor(cursor_x, cursor_y);

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
            for (uint16_t i = 0; i < (file_length_bytes - file_offset); i++)
                file_ptr[i] = file_ptr[i+1];

            file_length_bytes--;    // Deleted a char/byte from file data

            // Rewrite this line
            save_file_ptr = file_ptr;
            save_x = cursor_x;

            file_ptr -= cursor_x;   // Start of line
            cursor_x = 0;
            // Redraw line until end of line or end of file
            while (*file_ptr != 0x0A && *file_ptr != 0x00) {
                print_char(&cursor_x, &cursor_y, *file_ptr);
                file_ptr++;
            }
            print_char(&cursor_x, &cursor_y, SPACE);    // Previous end of line now = space
            current_line_length--;

            // Restore cursor_file position
            cursor_x = save_x;
            file_ptr = save_file_ptr;
            move_cursor(cursor_x, cursor_y);

            unsaved = 1;    // File now has unsaved changes

            continue;
        }

        if (scancode == LEFTARROW) {    // Left arrow key
            // Move 1 byte left (till beginning of line)
            if (cursor_x != 0) {
                remove_cursor(cursor_x, cursor_y);

                file_ptr--;     // Move file data to previous byte
                file_offset--;
                cursor_x--;     // Move cursor to previous character
                move_cursor(cursor_x, cursor_y);
            }
            continue;
        }

        if (scancode == RIGHTARROW) {    // Right arrow key
            // Move 1 byte right (till end of line)
            if (cursor_x+1 < current_line_length) { 
                remove_cursor(cursor_x, cursor_y);

                file_ptr++;
                file_offset++;
                cursor_x++;
                move_cursor(cursor_x, cursor_y);  // Cursor will be 1 forward from print_char above
            }
            continue;
        }

        if (scancode == UPARROW) {      // Up arrow key
            if (cursor_y == 0)  // On 1st line, can't go up
                continue;

            remove_cursor(cursor_x, cursor_y);

			cursor_y--;  // Move cursor 1 line up

            current_line_length = 0;
            file_ptr--;
            file_offset--;

			// Search for end of previous line above current line (newline 0Ah)
            // TODO: End of line could be char position 80, not always a line feed
			while(*file_ptr != 0x0A) {
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
            if (current_line_length < cursor_x + 1)  // Cursor is 0-based
                cursor_x = current_line_length - 1;

            file_ptr    += cursor_x;      // offset into line
            file_offset += cursor_x;
            move_cursor(cursor_x, cursor_y);

            continue;
        }

        if (scancode == DOWNARROW) {    // Down arrow key
            if (cursor_y == file_length_lines)  // On last line of file
                continue;
                
            remove_cursor(cursor_x, cursor_y);

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
            if (current_line_length < cursor_x + 1)  // Cursor is 0-based
                cursor_x = current_line_length - 1;

            // Move to start of current line
            file_ptr    -= current_line_length - 1;    
            file_offset -= current_line_length - 1;    

            file_ptr    += cursor_x;   // Move to cursor position in line
            file_offset += cursor_x;

            move_cursor(cursor_x, cursor_y);

			continue;
        }

        if (scancode == HOMEKEY) {      // Home key
            remove_cursor(cursor_x, cursor_y);

            // Move to beginning of line
            file_ptr    -= cursor_x;   // Move file data to start of line
            file_offset -= cursor_x;

            // Move cursor to start of line
            cursor_x = 0;           
            move_cursor(cursor_x, cursor_y);

            continue;
        }

        if (scancode == ENDKEY) {       // End key
            remove_cursor(cursor_x, cursor_y);

            // Move to end of line
            // Get difference of current_line_length and cursor_x (0-based),
            //   add this difference to cursor_x, and file data
            file_ptr    += ((current_line_length - 1) - cursor_x);
            file_offset += ((current_line_length - 1) - cursor_x);
            cursor_x    += ((current_line_length - 1) - cursor_x);

            move_cursor(cursor_x, cursor_y);
    
            continue;
        }

		// Else print out user input character to screen (assuming "insert" mode)
        current_line_length++;  // Update line length
        file_length_bytes++;    // Update file length
        if (input_char == 0x0D) file_length_lines++;  // Update file length

        // Move all file data forward 1 byte then fill in current character
        for (uint16_t i = (file_length_bytes - file_offset); i > 0; i--)
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
            while (*file_ptr != 0x00) {
                if (*file_ptr == 0x0A) {
                    // Newline
                    print_char(&cursor_x, &cursor_y, SPACE);
                    cursor_x = 0;
                    cursor_y++;

                } else 
                    print_char(&cursor_x, &cursor_y, *file_ptr);

                file_ptr++;
            }

        } else {
            // Not a newline, only print current line
            while (*file_ptr != 0x0A && *file_ptr != 0x00) {
                    print_char(&cursor_x, &cursor_y, *file_ptr);
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

        move_cursor(cursor_x, cursor_y);    // Move cursor on screen
        unsaved = 1;        // Inserted new character, unsaved changes
    }
}

void hex_editor(void)
{
    uint8_t *keybinds_hex_editor = " $ = Run code ? = Return to kernel S = Save file to disk\0";
    fill_out_bottom_editor_message(keybinds_hex_editor);  // Write keybinds to screen
    cursor_x = 0;   // Initialize cursor
    cursor_y = 0;
    move_cursor(cursor_x, cursor_y);    // Draw initial cursor

    while (1) {
        input_char = get_key();

        scancode = *(uint8_t *)0x1601;  // Scancode for key stored from get_key
        ctrl_key = *(uint8_t *)0x1603;  // Ctrl key status stored from get_key

        // Check for hex editor keybinds
        if (input_char == RUNINPUT) {
            *(uint8_t *)(0x20000 + editor_filesize) = 0xCB; // CB = far return

            ((void (*)(void))0x20000)();       // Jump to and execute input

            hex_count = 0;                     // Reset byte counter
            file_ptr = (uint8_t *)0x20000;     // Reset to hex memory location 20,000h
            file_offset = 0;
            
            // Reset cursor x/y
            cursor_x = 0;
            cursor_y = 0;

            continue;
        } 

        if (input_char == ENDPGM)      // End program, return to caller
            return;

        if (input_char == SAVEPGM) {   // Does user want to save?
            remove_cursor(cursor_x, cursor_y); 

            save_hex_program();
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

                *file_ptr = 0;  // Make current byte 0 in file
                file_ptr--;     // Move file data to previous byte
                file_offset--;
            }

            continue;
        }

        // Check delete key
        // TODO: Move all file data back 1 byte after blanking out current byte
        if (scancode == DELKEY) {
            // Blank out 1st and 2nd nibbles of hex byte
            print_char(&cursor_x, &cursor_y, SPACE);     // space ' ' in ascii
            print_char(&cursor_x, &cursor_y, SPACE);     // space ' ' in ascii

            *file_ptr = 0;  // Make current byte 0 in file
            cursor_x -= 2;  // Cursor is after hex byte, move back to 1st nibble of hex byte
            move_cursor(cursor_x, cursor_y);

            continue;
        }
        
        // Check navigation keys (arrows + home/end)
        if (scancode == LEFTARROW) {     // Left arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 byte left (till beginning of line)
            if (cursor_x >= 3) {
                cursor_x -= 3;
                move_cursor(cursor_x, cursor_y);
                file_ptr--;     // Move file data to previous byte
                file_offset--;
            }

            continue;
        }

        if (scancode == RIGHTARROW) {    // Right arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 byte right (till end of line)
            if (cursor_x <= 75) {
                cursor_x += 3;
                move_cursor(cursor_x, cursor_y);
                file_ptr++;             // Move file data to next byte
                file_offset++;
            }

            continue;
        }

        if (scancode == UPARROW) {     // Up arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 line up
            if (cursor_y != 0) {
                cursor_y--;
                file_ptr -= 27;  // # of hex bytes (2 nibbles + space) in 1 line
                file_offset -= 27;
                move_cursor(cursor_x, cursor_y);
            }

            continue;
        }

        if (scancode == DOWNARROW) {   // Down arrow key
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move 1 line down
            if (cursor_y != (gfx_mode->y_resolution / 16) - 1)	{   // At bottom row of screen?
                cursor_y++;
                file_ptr += 27;		// # of hex bytes (2 nibbles + space) in 1 line
                file_offset += 27;
                move_cursor(cursor_x, cursor_y);
            }

            continue;
        }

        if (scancode == HOMEKEY) {     // Home key pressed
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move to beginning of line
            file_ptr -= (cursor_x / 3);       // Each hex byte on screen is 2 nibbles + space
            file_offset -= (cursor_x / 3);
            cursor_x = 0;
            
            move_cursor(cursor_x, cursor_y);

            continue;
        }

        if (scancode == ENDKEY) {       // End key pressed
            remove_cursor(cursor_x, cursor_y);  // Blank out cursor line first

            // Move to end of line
            file_ptr += (79 - cursor_x / 3);  // Each hex byte on screen is 2 nibbles + space
            file_offset += (79 - cursor_x / 3);
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
            hex_byte <<= 4;             // Move digit 4 bits to the left, make room for 2nd digit
            hex_byte |= input_char;     // Move 2nd ascii byte/hex digit into memory
            input_char = hex_byte; // TODO: Is this needed?
            *file_ptr++ = hex_byte;   // Put hex byte(2 hex digits) into 10000h memory area, and inc di/point to next byte
            file_offset++;
            editor_filesize++;          // Increment file size byte counter
            hex_count = 0;              // Reset byte counter

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
	
void save_hex_program(void)
{
    uint8_t *keybinds_hex_editor = " $ = Run code ? = Return to kernel S = Save file to disk\0";

	// Fill out rest of sector with data if not already filled out
	// Divide file size by 512 to get # of sectors filled out and remainder of sector not filled out
    if (editor_filesize / 512 == 0) { // Less than 1 sector filled out?
        if (editor_filesize % 512 != 0) {
            memset(file_ptr, 0, 512 - (editor_filesize % 512));

        } else {
            // Otherwise file is empty, fill out 1 whole sector	
            memset(file_ptr, 0, 512);
        }

    } else if (editor_filesize % 512 != 0) {
        // Fill out rest of sector with 0s
        memset(file_ptr, 0, 512 - (editor_filesize % 512));
    }

	// Print filename string
    write_bottom_screen_message(blank_line);
    write_bottom_screen_message(filename_string);
    move_cursor(cursor_x, cursor_y);

	// Save file type - 'bin'
    strncpy(editor_filetype, "bin", 3);

	// Input file name to save
	input_file_name();

	// Call save_file function
    // file name, file ext, file size (hex sectors), address to save from
    // TODO: Allow file size >1
    if (save_file(editor_filename, editor_filetype, 0x0001, 0x20000) != 0) {
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
void input_file_name(void)
{
	// Save file name
    for (uint8_t i = 0; i < 10; i++) {
        input_char = get_key();
        editor_filename[i] = input_char;

		// Print character to screen
        print_char(&cursor_x, &cursor_y, input_char);
        move_cursor(cursor_x, cursor_y);
    }
}

// Subroutine: fill out message at bottom of editor
void fill_out_bottom_editor_message(const uint8_t *msg)
{
	// Fill string variable with message to write
    strcpy(bottom_msg, msg);
    bottom_msg[79] = '\0';
	
	write_bottom_screen_message(bottom_msg);
}

// Write message at bottom of screen
void write_bottom_screen_message(const uint8_t *msg)
{
    uint8_t font_height = *(uint8_t *)FONT_HEIGHT;

    cursor_x = 0;
    cursor_y = (gfx_mode->y_resolution / font_height) - 1;

    print_string(&cursor_x, &cursor_y, msg);
}
