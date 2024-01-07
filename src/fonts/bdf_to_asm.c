/*
 * bdf_to_asm.c: Convert a .bdf font to a nasm compatible asm font
 * for the OS, for ascii characters ~32-127
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// Read file until line starting with string is reached
void read_until(FILE *file, char *buffer, size_t buf_size, const char *string) {
    while (fgets(buffer, buf_size, file))
        if (!strncmp(buffer, string, strlen(string)))
            return;
}

// MAINLINE
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <font.bdf>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *font_file = fopen(argv[1], "r");
    if (!font_file) {
        fprintf(stderr, "Error opening file %s for reading: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }

    char output_name[256] = {0};
    strncpy(output_name, argv[1], strlen(argv[1]) - 3);
    strcat(output_name, "asm");

    FILE *output_file = fopen(output_name, "w");
    if (!output_file) {
        fprintf(stderr, "Error opening file %s for writing: %s\n", output_name, strerror(errno));
        return EXIT_FAILURE;
    }

    // Write font file "title" 
    fprintf(output_file, 
            ";;;\n"
            ";;; %s\n"
            ";;;\n\n",
            output_name);

    // Get font size line: "FONTBOUNDINGBOX <width> <height> <something> <something>"
    char buffer[256] = {0};
    read_until(font_file, buffer, sizeof buffer, "FONTBOUNDINGBOX"); 

    // Write font width/height: "db <font_width>, <font_height>\n"
    char *font_w = strtok(buffer, " \n");
    font_w = strtok(NULL, " \n");           // Width
    char *font_h = strtok(NULL, " \n");     // Height

    fprintf(output_file,
            ";; Font width, height\n"
            "db %s, %s\n\n", 
            font_w, font_h);

    // Write initial padding: "times 31*<font_height> - 2 db 0\n"
    const int font_width  = (int)strtol(font_w, NULL, 10);
    const int font_height = (int)strtol(font_h, NULL, 10);

    // Set db/dw/dd/etc. from font width;
    //   1-8 = 1 byte, 9-16 = 2 bytes (word), etc.
    char *define_width = NULL;
    switch (((font_width-1) / 8) + 1) {
        case 1: define_width = "db"; break;
        case 2: define_width = "dw"; break;
        case 4: define_width = "dd"; break;
        default: 
            fprintf(stderr, "Error: Unsupported font width: %s\n", font_w);
            goto cleanup;
            break;
    }

    fprintf(output_file,
            ";; Initial padding\n"
            "times 31*%u - 2 db 0\n\n", 
            font_height * (((font_width-1) / 8) + 1));  // Font height in width bytes

    // Start adding characters at space/ascii 32
    read_until(font_file, buffer, sizeof buffer, "STARTCHAR space");  
    fseek(font_file, -16, SEEK_CUR);    // Move back before line
                                                                          
    // Loop for next character
    while (true) {
        // Write name and number string: ";; <name> <num>\n"  
        read_until(font_file, buffer, sizeof buffer, "STARTCHAR");  // STARTCHAR <name> line

        char ascii_name[32];
        char *name_pos = strtok(buffer, " \n");   // "STARTCHAR"
        name_pos = strtok(NULL, " \n");           // <name>
        strcpy(ascii_name, name_pos);
                                                  
        fgets(buffer, sizeof buffer, font_file);    // Get ENCODING <num> line
        char *num_pos = strtok(buffer, " \n");      // "ENCODING"
        num_pos = strtok(NULL, " \n");              // <num>

        // Stop reading characters after ascii range 0-127
        if (strtol(num_pos, NULL, 10) > 126) break;  
                    
        fprintf(output_file,
                ";; %s %s\n",
                ascii_name, num_pos);

        // Write bitmap lines: "db/dw/etc. 0x<bitmap>\n"
        read_until(font_file, buffer, sizeof buffer, "BITMAP");

        while (fgets(buffer, sizeof buffer, font_file)) {
            if (!strncmp(buffer, "ENDCHAR", strlen("ENDCHAR"))) 
                break;  // Reached end of bitmap
            else 
                fprintf(output_file, "%s 0x%s", define_width, buffer);
        }
    }

    // Add cursor line as final character 
    fputs(";; Cursor Line 127\n",output_file);
    for (int i = 0; i < font_height; i++) {
        fprintf(output_file, "%s 0x%.*s\n", 
                define_width, 
                (((font_width-1) / 8) + 1) * 2,                 // 2 nibbles per byte
                i < font_height-1 ? "00000000" : "FFFFFFFF");   // Up to width 32, expand as needed
    }

    // Add sector padding to next 512 byte boundary
    const long current_size     = ftell(output_file);
    const long next_sector_size = current_size - (current_size % 512) + 512;
    fprintf(output_file,
            "\n;; Sector padding\n"
            "times %lu-($-$$) db 0\n",
            next_sector_size);

    // File cleanup
    cleanup:
    fclose(font_file);
    fclose(output_file);

    // End program
    return EXIT_SUCCESS;
}
