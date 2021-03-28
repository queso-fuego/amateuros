// 
// file_ops.h: file operations - delete, rename, save, or load a file
//
#pragma once

uint8_t *check_filename(uint8_t *filename, uint16_t filename_length);

// delete_file: Delete a file from the disk
//
// Input 1 - File name to delete
//	     2 - Length of file name
// Output 1 - Return code
uint16_t delete_file(uint8_t *filename, uint16_t filename_length)
{
    uint8_t drive_num       = *(uint8_t *)0x1500;  // Get drive #
    uint16_t error_code     = 0;
    uint8_t file_size       = 0;
    uint8_t starting_sector = 0;
    uint8_t ft_size         = 0;
    uint8_t ft_start_sector = 0;
    uint8_t *free_space     = "FREE SPACE";
    uint8_t *filetable      = "fileTable ";
    uint8_t *file_ptr;
    uint8_t test_byte;

    // Get file size and starting sector for file table
    file_ptr = check_filename(filetable, 10);  

    ft_start_sector = file_ptr[14];  // Starting sector
    ft_size         = file_ptr[15];  // File size

    // Check for filename in file table, if successful file_ptr points to file table entry
    file_ptr = check_filename(filename, filename_length);  

    if (*file_ptr == 0)  // Didn't find file
        return 1;

    starting_sector = file_ptr[14];  // Starting sector
    file_size       = file_ptr[15];  // File size

    // Write changed file table entry first
    for (uint8_t i = 0; i < 10; i++)
        *file_ptr++ = free_space[i];    // File name

    for (uint8_t i = 0; i < 3; i++)
        *file_ptr++ = ' ';              // File type
    
    for (uint8_t i = 0; i < 3; i++)
        *file_ptr++ = 0;              // Directory entry #, starting sector, file size

    // TODO: Assuming file table is 1 sector, change later 
    
    // Write changed filetable to disk 
    // drive number is and-ed with head # in low nibble (0Fh), and or-ed with primary drive A0h (drive 1). Drive 2 is 0Bh
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((drive_num & 0x0F) | 0xA0), "d"(0x1F6) );   // Head/drive # port - send head/drive #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(ft_size),         "d"(0x1F2) );  // Sector count port - # of sectors to read
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(ft_start_sector), "d"(0x1F3) );  // Sector number port - sector to start writing at (1-based!)
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F4) );  // Cylinder low port - cylinder low #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F5) );  // Cylinder high port - cylinder high #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0x30), "d"(0x1F7) );  // Command port - write with retry

    // Keep trying until sector buffer is ready
    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
        if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
            break;
    }

    __asm__ __volatile__ ("1:\n"
                          "outsw\n"               // Write words from (DS:)SI into DX port
                          "jmp .+2\n"             // Small delay after each word written to port
                          "loop 1b"               // Write words until CX = 0
                          // SI = address to write sector from, CX = # of words for 1 sector,
                          // DX = data port
                          : : "S"(0x1000), "c"(256), "d"(0x1F0) );   

    // Send cache flush command after write command is finished
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0xE7), "d"(0x1F7) );  // Command port - cache flush

    // TODO: Handle disk write error for file table here...
    //   Check error ata pio register here...

    // Memory location to write NULL data to
    file_ptr = (uint8_t *)0x20000;

	// Write nulls to memory location
    for (uint16_t i = 0; i < file_size*512; i++) 
        file_ptr[i] = 0;
		
	// Write zeroed-out data to disk
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((drive_num & 0x0F) | 0xA0), "d"(0x1F6) );   // Head/drive # port - head/drive #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(file_size),       "d"(0x1F2) );  // Sector count port - # of sectors to read
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(starting_sector), "d"(0x1F3) );  // Sector number port - sector to start writing at (1-based!)
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F4) );  // Cylinder low port - cylinder low #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F5) );  // Cylinder high port - cylinder high #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0x30), "d"(0x1F7) );  // Command port - write with retry

    // Poll status port after reading 1 sector
    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
        if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
            break;
    }

    __asm__ __volatile__ ("1:\n"
                          "outsw\n"             // Write bytes from (DS:)SI into DX port
                          "jmp .+2\n"           // Small delay after each word written to port
                          "loop 1b\n"           // Write words until CX = 0
                          // SI = address to write sector from, CX = file size in hex sectors in words,
                          // DX = Data port
                          : : "S"(0x20000), "c"(file_size*256), "d"(0x1F0) );  

    // Send cache flush command after write command is finished
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0xE7), "d"(0x1F7) );  // Command port - cache flush

    // TODO: Check error register here

    return 0;   // Success, normal return
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
    uint8_t drive_num       = *(uint8_t *)0x1500;  // Get drive #
    uint16_t error_code     = 0;
    uint16_t return_code    = 0;
    uint8_t file_size       = 0;
    uint8_t starting_sector = 0;
    uint8_t *file_ptr;
    uint8_t test_byte;

    // Check for filename in file table, if successful file_ptr points to file table entry
    file_ptr = check_filename(filename, filename_length);  

    if (*file_ptr == 0)  // File not found
        return 1;

	// Get file type into variable to pass back
    for (uint8_t i = 0; i < 3; i++)
        file_ext[i] = file_ptr[10+i];

    return_code = 0;    // Init return code to 'success'

    file_size       = file_ptr[15];  // # of sectors to read
    starting_sector = file_ptr[14];  // Sector # to start reading at

    // Read sectors using ATA PIO ports
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((drive_num & 0x0F) | 0xA0), "d"(0x1F6) );   // Head/drive # port - head/drive #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(file_size),       "d"(0x1F2) );        // Sector count port - # of sectors to read
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(starting_sector), "d"(0x1F3) );  // Sector number port - sector to start writing at (1-based!)
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F4) );  // Cylinder low port - cylinder low #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F5) );  // Cylinder high port - cylinder high #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0x20), "d"(0x1F7) );  // Command port - read with retry

    for (int8_t i = file_size; i > 0; i--) {
        while (1) {
            // Poll status port after reading 1 sector
            __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
            if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
                break;
        }

        // Read dbl words from DX port # into EDI, CX # of times
        // DI = address to read into, cx = # of double words to read for 1 sector,
        // DX = data port
        __asm__ __volatile__ ("rep insl" : : "D"(address), "c"(128), "d"(0x1F0) ); 

        // 400ns delay - Read alternate status register
        for (uint8_t i = 0; i < 4; i++)
            __asm__ __volatile__ ("inb %%dx, %%al" : : "d"(0x3F6), "a"(0) );   // Use any value for "a", it's just to keep the al
    }

    // TODO: Check PIO error register here

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
    uint8_t drive_num       = *(uint8_t *)0x1500;  // Get drive #
    uint16_t error_code     = 0;
    uint16_t return_code    = 0;
    uint8_t ft_size         = 0;
    uint8_t ft_start_sector = 0;
    uint8_t *filetable      = "fileTable ";
    uint8_t *file_ptr;
    uint8_t test_byte;

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
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((drive_num & 0x0F) | 0xA0), "d"(0x1F6) );   // Head/drive # port - head/drive #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(ft_size),         "d"(0x1F2) );  // Sector count port - # of sectors to read
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(ft_start_sector), "d"(0x1F3) );  // Sector number port - sector to start writing at (1-based!)
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F4) );  // Cylinder low port - cylinder low #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F5) );  // Cylinder high port - cylinder high #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0x30), "d"(0x1F7) );  // Command port - write with retry

    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
        if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
            break;
    }

    __asm__ __volatile__ ("1:\n"
                          "outsw\n"           // Write bytes from (DS:)SI into DX port
                          "jmp .+2\n"         // Small delay after each word written to port
                          "loop 1b"           // Write words until CX = 0
                          // SI = address to write from, CX = # of words for 1 sector,
                          // DX = data port
                          : : "S"(0x1000), "c"(256), "d"(0x1F0) );

    // Send cache flush command after write command is finished
    __asm__ __volatile__ ("outb %%al, %%dx": : "a"(0xE7), "d"(0x1F7) );

    // TODO: Check error registers here

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
    uint8_t drive_num         = *(uint8_t *)0x1500;  // Get drive #
    uint16_t error_code       = 0;
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
    uint8_t test_byte;

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

    // TODO: Assuming file table is 1 sector, change later
    // Write changed filetable to disk
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((drive_num & 0x0F) | 0xA0), "d"(0x1F6) );   // Head/drive # port - head/drive #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(ft_size),         "d"(0x1F2) );  // Sector count port - # of sectors to read
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(ft_start_sector), "d"(0x1F3) );  // Sector number port - sector to start writing at (1-based!)
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F4) );  // Cylinder low port - cylinder low #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F5) );  // Cylinder high port - cylinder high #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0x30), "d"(0x1F7) );  // Command port - write with retry

    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
        if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
            break;
    }

    __asm__ __volatile__ ("1:\n"
                          "outsw\n"           // Write bytes from (DS:)SI into DX port
                          "jmp .+2\n"         // Small delay after each word written to port
                          "loop 1b"           // Write words until CX = 0
                          // SI = address to write from, CX = # of words for 1 sector,
                          // DX = data port
                          : : "S"(0x1000), "c"(256), "d"(0x1F0) );

    // Send cache flush command after write command is finished
    __asm__ __volatile__ ("outb %%al, %%dx": : "a"(0xE7), "d"(0x1F7) );

    // Wait until BSY bit is clear after cache flush
    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );
        if (!(test_byte & (1 << 7)))
            break;
    }

    // TODO: Handle disk write error for file table here...

    // Write file data to disk
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((drive_num & 0x0F) | 0xA0), "d"(0x1F6) );   // Head/drive # port - head/drive #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(in_file_size),      "d"(0x1F2) );  // Sector count port - # of sectors to read
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(last_saved_sector), "d"(0x1F3) );  // Sector number port - sector to start writing at (1-based!)
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F4) );  // Cylinder low port - cylinder low #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0),    "d"(0x1F5) );  // Cylinder high port - cylinder high #
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0x30), "d"(0x1F7) );  // Command port - write with retry

    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
        if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
            break;
    }

    __asm__ __volatile__ ("1:\n"
                          "outsw\n"           // Write bytes from (DS:)SI into DX port
                          "jmp .+2\n"         // Small delay after each word written to port
                          "loop 1b"           // Write words until CX = 0
                          // SI = address to write from, CX = # of words for 1 sector * # of sectors,
                          // DX = data port
                          : : "S"(address), "c"(in_file_size*256), "d"(0x1F0) );

    // Send cache flush command
    __asm__ __volatile__ ("outb %%al, %%dx": : "a"(0xE7), "d"(0x1F7) );

    // Wait until BSY bit is clear
    while (1) {
        __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );
        if (!(test_byte & (1 << 7)))
            break;
    }

    // TODO: Handle disk write error for file data here...
    // return_code = 1;    // Error occurred

    return 0;
}

//-------------------------------------
// Check for filename in filetable
// Input 1: File name to check
//       2: Length of file name
//       3: Pointer to file table
// Output 1: Error Code 0-success, 1-Fail
//       (2): File pointer will point to start of file table entry if found
//-------------------------------------
uint8_t *check_filename(uint8_t *filename, uint16_t filename_length)
{
    uint16_t i; 
    uint8_t *file_ptr = (uint8_t *)0x1000;    // 1000h = location of fileTable in memory

    // Check passed in filename to filetable entry
    while (*file_ptr != 0) {
        for (i = 0; i < filename_length && filename[i] == file_ptr[i]; i++) ; 

        if (i == filename_length)  // Found file
            break;

        file_ptr += 16;     // Go to next file table entry
    }

    return file_ptr;
}
