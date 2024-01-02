// 
// file_ops.h: file operations - delete, rename, save, or load a file
//
#pragma once

#include "C/stdio.h"
#include "C/string.h"
#include "ports/io.h"

#define MAX_FILENAME_LENGTH 10

enum {
    READ_WITH_RETRY = 0x20,
    WRITE_WITH_RETRY = 0x30,
} ata_pio_commands;

//-------------------------------------
// Check for filename in filetable
// Input 1: File name to check
//       2: Length of file name
// Output 1: Error Code 0-success, 1-Fail
//       (2): File pointer will point to start of file table entry if found
//-------------------------------------
// TODO: Remove this function, as it's not used anymore?
uint8_t *check_filename(uint8_t *filename, const uint16_t filename_length)
{
    uint16_t i; 
    uint8_t *ptr = (uint8_t *)0x1000;    // 1000h = location of fileTable in memory

    // Check passed in filename to filetable entry
    while (*ptr != 0) {
        for (i = 0; i < filename_length && filename[i] == ptr[i]; i++) 
            ; 

        if (i == MAX_FILENAME_LENGTH || (i == filename_length && ptr[i] == ' '))  // Found file
            break;

        ptr += 16;     // Go to next file table entry
    }

    return ptr;
}

//-----------------------------------------------
// Read/write sectors to/from disk using ATA PIO
// Inputs:
//   1: File size in sectors from file table entry
//   2: Starting sector from file table entry
//   3: Address to Read/Write data from/to
//   4: Command to perform (Reading or Writing)
//-----------------------------------------------
void rw_sectors(const uint32_t size_in_sectors, uint32_t starting_sector, uint32_t address, const uint8_t command) {
    //uint8_t head = 0;
    //uint16_t cylinder = 0;

    // Convert given starting sector to correct CHS values
    // "Typical" disk limits are Cylinder: 0-1023, Head: 0-15, Sector: 1-63 (sector is 1-based!)
    //if (starting_sector > 63) {
    //    head = starting_sector / 63;
    //    starting_sector %= 63;
    //    if (starting_sector == 0) {
    //        starting_sector = 63;
    //        head--;
    //    }
    //    cylinder = head / 16;
    //    head %= 16;
    //}

    //if (cylinder >= 1024) {
    //    // TODO: Error for disk limit reached
    //}
    
    // TODO: Change to LBA later on? Either 28/48bit LBA
    // CHS -> LBA = ((cylinder * 16 + head) * 63) + sector - 1
    // LBA -> CHS =
    //   cylinder = LBA / (16 * 63);
    //   temp     = LBA % (16 * 63);
    //   head     = temp / 63;
    //   sector   = temp % 63 + 1;
    
    // TODO: If size in sectors > 256, then loop and "chunk" to send 256 sectors at a time until all is read/written
    // Port 1F6h head/drive # bits: 7: always set(1), 6 = CHS(0) or LBA(1), 5 = always set(1), 4 = drive # (0 = primary, 1 = secondary),
    //   3-0: Head # OR LBA bits 24-27
    outb(0x1F6, (0xE0 | ((starting_sector >> 24) & 0x0F))); // Head/drive # port - send head/drive #
    outb(0x1F2, size_in_sectors);                           // Sector count port - # of sectors to read/write
    outb(0x1F3, starting_sector & 0xFF);                    // Sector number port / LBA low bits 0-7
    outb(0x1F4, ((starting_sector >> 8)  & 0xFF));          // Cylinder low port / LBA mid bits 8-15
    outb(0x1F5, ((starting_sector >> 16) & 0xFF));          // Cylinder high port / LBA high bits 16-23
    outb(0x1F7, command);                                   // Command port - send read/write command
                                                            
    uint16_t *address_ptr = (uint16_t *)address;                                                           

    if (command == READ_WITH_RETRY) {
        for (uint8_t i = size_in_sectors; i > 0; i--) {
            // Poll status port after reading 1 sector
            while (inb(0x1F7) & (1 << 7)) // Wait until BSY bit is clear 
                ;

            // Read 256 words from data port into memory
            for (uint32_t j = 0; j < 256; j++)
                *address_ptr++ = inw(0x1F0);

            // 400ns delay - Read alternate status register
            for (uint8_t k = 0; k < 4; k++)
                inb(0x3F6);
        }

    } else if (command == WRITE_WITH_RETRY) {
        for (uint8_t i = size_in_sectors; i > 0; i--) {
            // Poll status port after reading 1 sector
            while (inb(0x1F7) & (1 << 7)) // Wait until BSY bit is clear 
                ;

            // Write 256 words from memory to data port
            for (uint32_t j = 0; j < 256; j++)
                outw(0x1F0, *address_ptr++);

            // 400ns delay - Read alternate status register
            for (uint8_t k = 0; k < 4; k++)
                inb(0x3F6);
        }

        // Send cache flush command after write command is finished
        outb(0x1F7, 0xE7);

        // Wait until BSY bit is clear after cache flush
        while (inb(0x1F7) & (1 << 7)) 
            ;
    }

    // TODO: Handle disk write error for file table here...
    //   Check error ata pio register here...
}

// delete_file: Delete a file from the disk
//
// Input 1 - File name to delete
//	     2 - Length of file name
// Output 1 - Return code
uint16_t delete_file(uint8_t *filename, uint16_t filename_length)
{
    uint8_t ft_size         = 0;
    uint8_t ft_start_sector = 0;
    uint8_t *free_space     = "FREE SPACE";
    uint8_t *filetable      = "fileTable ";
    uint8_t *file_ptr;

    // Get file size and starting sector for file table
    file_ptr = check_filename(filetable, 10);  

    if (*file_ptr == 0)  // Didn't find file
        return 0;

    ft_start_sector = file_ptr[14];  // Starting sector
    ft_size         = file_ptr[15];  // File size

    // Check for filename in file table, if successful file_ptr points to file table entry
    file_ptr = check_filename(filename, filename_length);  

    if (*file_ptr == 0)  // Didn't find file
        return 0;

    // Write changed file table entry first
    for (uint8_t i = 0; i < 10; i++)
        *file_ptr++ = free_space[i];    // File name

    for (uint8_t i = 0; i < 3; i++)
        *file_ptr++ = ' ';              // File type

    // Write changed filetable to disk 
    rw_sectors(ft_size, ft_start_sector, 0x1000, WRITE_WITH_RETRY);

    return 1;   // Success, normal return
}

// load_file: Read a file from filetable and its sectors into a memory location
//
// input 1: File name (address)
//       2: File name length   
//       3: Memory offset to load data to
//       4: File extension
// output 1: Return code
uint16_t load_file(uint8_t *filename, uint16_t filename_length, uint32_t address, uint8_t *file_ext)
{
    uint16_t return_code = 0;
    uint8_t *file_ptr;

    // Check for filename in file table, if successful file_ptr points to file table entry
    file_ptr = check_filename(filename, filename_length);  

    if (*file_ptr == 0)  // File not found
        return 0;

	// Get file type into variable to pass back
    for (uint8_t i = 0; i < 3; i++)
        file_ext[i] = file_ptr[10+i];

    return_code = 1;    // Init return code to 'success'

    // Read sectors using ATA PIO
    rw_sectors(file_ptr[15], file_ptr[14], address, READ_WITH_RETRY);

    return return_code;	
}

// rename_file.inc: rename a file in the file table!!
//
// Input 1: File name  (address)
//		 2: File name length
//		 3: New file name  (address)
//		 4: New file name length
// Output 1: Return code
uint16_t rename_file(uint8_t *file1, uint16_t file1_length, uint8_t *file2, uint16_t file2_length)
{
    uint16_t return_code    = 0;
    uint8_t ft_size         = 0;
    uint8_t ft_start_sector = 0;
    uint8_t *filetable      = "fileTable ";
    uint8_t *file_ptr;

    // Get file size and starting sector for file table
    file_ptr = check_filename(filetable, 10);  

    ft_start_sector = file_ptr[14];  // Starting sector
    ft_size         = file_ptr[15];  // File size

    // Check for filename in file table, if successful file_ptr points to file table entry
    file_ptr = check_filename(file1, file1_length);  

    if (*file_ptr == 0)  // File not found
        return 1;

	// Found, replace with new file name - overwrite with spaces first
    for (uint8_t i = 0; i < 10; i++)
        file_ptr[i] = ' ';

    for (uint8_t i = 0; i < file2_length; i++)
        file_ptr[i] = file2[i];

	// Write changed file table data to disk
    return_code = 0;    // Init return code to success

    // Write changed filetable to disk 
    rw_sectors(ft_size, ft_start_sector, 0x1000, WRITE_WITH_RETRY);

    return return_code;
}

// save_file.inc: Save file data to disk
//
// Input 1: File name (address)
//       2: File ext  (.bin,.txt,etc)
//       3: File size (in hex sectors)
//       4: Memory offset to save from
// Output 1: Return code
uint16_t save_file(uint8_t *filename, uint8_t *file_ext, uint8_t in_file_size, uint32_t address)
{
    uint16_t return_code      = 0;
    uint16_t filename_length  = 10;
    uint8_t file_size         = 0;
    uint8_t starting_sector   = 0;
    uint8_t ft_size           = 0;
    uint8_t ft_start_sector   = 0;
    uint8_t last_saved_sector = 0;
    uint8_t *free_space       = "FREE SPACE";
    uint8_t *filetable        = "fileTable ";
    uint8_t *file_ptr;

    // Get file size and starting sector for file table
    file_ptr = check_filename(filetable, filename_length);  

    ft_start_sector = file_ptr[14];  // Starting sector
    ft_size         = file_ptr[15];  // File size

    // Check for filename in file table, if successful file_ptr points to file table entry
    file_ptr = check_filename(filename, filename_length);  

    if (*file_ptr == 0) {
        // Check for free space in file table and write file at that entry
        // If FREE SPACE not found, file_ptr will point to end of file table
        file_ptr = check_filename(free_space, filename_length);  

    } else {
        // Otherwise file exists in filetable and on disk
        // TODO: Ask user to overwrite current file or copy to new file

        // TODO: Overwrite current file

        // TODO: Create copy and save to new file

        // TODO: Otherwise handle write sectors error here...
        
        // Return here?
        // return return_code;
    }

    // Grab previous file table entry's starting sector + file size
    //  for new starting sector
    file_ptr -= 16; 
    starting_sector   = file_ptr[14];                   // Starting sector
    file_size         = file_ptr[15];                   // File size
    last_saved_sector = starting_sector + file_size;    // Sector for new file
    file_ptr += 16; 

    // Create new file table entry at end of current entries
    for (uint8_t i = 0; i < 10; i++)
        file_ptr[i] = filename[i];

    file_ptr += 10;

    for (uint8_t i = 0; i < 3; i++)
        file_ptr[i] = file_ext[i];

    file_ptr   += 3;
    *file_ptr++ = 0;                  // Directory entry # (0 = first entry)
    *file_ptr++ = last_saved_sector;  // Starting sector for new file
    *file_ptr++ = in_file_size;       // File size

    // Write changed filetable to disk
    rw_sectors(ft_size, ft_start_sector, 0x1000, WRITE_WITH_RETRY); 
    
    // Write file data to disk
    rw_sectors(in_file_size, last_saved_sector, address, WRITE_WITH_RETRY);
   
    return_code = 1;
    return return_code;
}
