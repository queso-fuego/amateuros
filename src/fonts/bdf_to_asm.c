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

    char buffer[256] = {0};

    // Write font file "title" 
    fputs(";;;\n;;; ", output_file);
    fputs(output_name, output_file);
    fputs("\n;;;\n", output_file);

    // Write font width/height: "db <font_width>, <font_height>\n"
    fputs("\n;; Font width, height\ndb ", output_file);

    // Get font size line: "FONTBOUNDINGBOX <width> <height> <something> <something>"
    read_until(font_file, buffer, sizeof buffer, "FONTBOUNDINGBOX"); 
    char *font_num = strtok(buffer, " \n");

    font_num = strtok(NULL, " \n");   // Width
    fputs(font_num, output_file);
    fputs(", ", output_file);

    const long font_width = strtol(font_num, NULL, 10);

    font_num = strtok(NULL, " \n");   // Height
    fputs(font_num, output_file);
    fputs("\n\n", output_file);

    const long font_height = strtol(font_num, NULL, 10);

    // Write initial padding: "times 31*<font_height> - 2 db 0\n"
    fputs(";; Initial padding\n", output_file);
    fputs("times 31*", output_file);

    if (font_width <= 8)
        fprintf(output_file, "%lu", font_height);
    else if (font_width <= 16)
        fprintf(output_file, "%lu", font_height*2);

    fputs(" - 2 db 0\n", output_file);
    fputc('\n', output_file);

    // Start adding characters at space/ascii 32
    read_until(font_file, buffer, sizeof buffer, "STARTCHAR space");  
    fseek(font_file, -strlen("STARTCHAR space\n"), SEEK_CUR);    // Move back before line
                                                                          
    // Loop for next character
    while (true) {
        read_until(font_file, buffer, sizeof buffer, "STARTCHAR");  // STARTCHAR <name> line

        // Get next character name
        char *name_pos = strtok(buffer, " \n");   // "STARTCHAR"
        name_pos = strtok(NULL, " \n");           // <name>
                                                  
        // Name and number string: ";; <name> <num>\n"             
        char ascii_name_num[25]; 
        strcpy(ascii_name_num, name_pos);
                                                  
        fgets(buffer, sizeof buffer, font_file);    // Get ENCODING <num> line
        char *num_pos = strtok(buffer, " \n");      // "ENCODING"
        num_pos = strtok(NULL, " \n");              // <num>
        if (strtol(num_pos, NULL, 10) > 126)
            break;  // Stop reading characters when past ascii range 0-127
                    
        strcat(ascii_name_num, " ");
        strcat(ascii_name_num, num_pos);

        // Write character name/number 
        fputs(";; ", output_file);
        fwrite(ascii_name_num, strlen(ascii_name_num), 1, output_file);
        fputc('\n', output_file);

        // Write bitmap lines
        read_until(font_file, buffer, sizeof buffer, "BITMAP");

        while (fgets(buffer, sizeof buffer, font_file))
            if (!strncmp(buffer, "ENDCHAR", strlen("ENDCHAR")))
                break;  // Reached end of bitmap
            else {
                // Add next bitmap bytes: "db(/dw/etc.) 0x<bitmap>\n"
                if (font_width <= 8)
                    fputs("db 0x", output_file);
                else if (font_width <= 16)
                    fputs("dw 0x", output_file);
                
                fwrite(buffer, strlen(buffer), 1, output_file); 
            }
    }

    // Add final character for cursor line
    fputs(";; Cursor Line 127\n",output_file);
    for (long i = 0; i < font_height-1; i++)
        if (font_width <= 8)
            fputs("db 0x00\n", output_file);
        else if (font_width <= 16)
            fputs("dw 0x0000\n", output_file);

    if (font_width <= 8)
        fputs("db 0xFF\n", output_file);  // Full line at bottom of character
    else if (font_width <= 16)
        fputs("dw 0xFFFF\n", output_file);  // Full line at bottom of character

    // Add sector padding to next 512 byte boundary
    fputs("\n;; Sector padding\n", output_file);
    const long current_size = ftell(output_file);
    const long next_sector_size = current_size - (current_size % 512) + 512;
    fputs("times ",output_file);
    fprintf(output_file, "%lu", next_sector_size);
    fputs("-($-$$) db 0\n", output_file);
    
    // File cleanup
    fclose(font_file);
    fclose(output_file);

    // End program
    return EXIT_SUCCESS;
}
