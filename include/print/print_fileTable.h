//
// print_fileTable.h: prints file table entries to screen
//
// input 1: cursor Y position (address, not value)
// input 2: cursor X position (address, not value)
//
#pragma once

#include "C/stdio.h"

void print_fileTable()
{
    uint8_t *fileTable_name = "fileTable ";
    uint8_t *filetable_ptr;
    uint8_t *file_ext = "   ";
    uint16_t hex_num = 0;
    uint8_t *heading = "\x0A\x0D"
        "---------   ---------   -------   ------------   --------------" "\x0A\x0D"
        "File Name   Extension   Entry #   Start Sector   Size (sectors)" "\x0A\x0D" 
        "---------   ---------   -------   ------------   --------------" "\x0A\x0D"
        "\0";

    // Print file table heading
    printf(heading); 

    // Load file table string from its memory location (1000h), print file
    //   and program names & sector numbers to screen
    // --------------------------------------------------------------------
    // Load file table sector from disk to memory, to get any updates
    load_file(fileTable_name, (uint16_t)10, (uint32_t)0x1000, file_ext); 

    filetable_ptr = (uint8_t *)0x1000;

    // Filename loop (0 = end of filetable)
    while (*filetable_ptr != 0) {
        // Print file name
        for (uint8_t i = 0; i < 10; i++) {
            putc(*filetable_ptr);
            filetable_ptr++;             // Get next byte at file table
        }

        // 2 blanks before file extension
        printf("  ");

        // Print file ext bytes
        for (uint8_t i = 0; i < 3; i++) {
            putc(*filetable_ptr);
            filetable_ptr++;
        }

        // Directory entry # section
        // 9 blanks before entry #
        printf("         ");
        
        // Print hex value
        hex_num = (uint16_t)*filetable_ptr;
        printf("%x", hex_num);
        filetable_ptr++;

        // Starting sector section
        // 6 blanks before starting sector
        printf("      ");
        
        // Print hex value
        hex_num = (uint16_t)*filetable_ptr;
        printf("%x", hex_num);
        filetable_ptr++;

        // File size section
        // 11 blanks before file size
        printf("           ");
        
        // Print hex value
        hex_num = (uint16_t)*filetable_ptr;
        printf("%x", hex_num);
        filetable_ptr++;

        // Print newline bewtween file table entries
        printf("\r\n");
    }
}
