// 
// file_ops.h: file operations - delete, rename, save, or load a file
//
#pragma once

#include "C/string.h"

enum {
    READ_WITH_RETRY = 0x20,
    WRITE_WITH_RETRY = 0x30,
} ata_pio_commands;

//-----------------------------------------------
// Read/write sectors to/from disk using ATA PIO
// Inputs:
//   1: File size in sectors from file table entry
//   2: Starting sector from file table entry
//   3: Address to Read/Write data from/to
//   4: Command to perform (Reading or Writing)
//-----------------------------------------------
void rw_sectors(const uint32_t size_in_sectors, uint32_t starting_sector, const uint32_t address, const uint8_t command)
{
    uint8_t test_byte = 0;

    // To convert CHS <-> LBA:
    // CHS -> LBA = ((cylinder * 16 + head) * 63) + sector - 1
    // LBA -> CHS =
    //   cylinder = LBA / (16 * 63);
    //   temp     = LBA % (16 * 63);
    //   head     = temp / 63;
    //   sector   = temp % 63 + 1;
    
    // TODO: If size_in_sectors > 256, loop and send 0 and size -= 256 sectors until size < 256, then send remaining size
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0xE0 | (starting_sector >> 24) & 0x0F),  "d"(0x1F6) );  // Head/drive # port, flags - send head/drive # & top 4 bits of LBA
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(size_in_sectors),               "d"(0x1F2) );  // Sector count port - # of sectors to read/write
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(starting_sector & 0xFF),        "d"(0x1F3) );  // Sector number / LBA low port 
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((starting_sector >> 8) & 0xFF), "d"(0x1F4) );  // Cylinder low  / LBA mid port 
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"((starting_sector >> 16) & 0xFF),"d"(0x1F5) );  // Cylinder high / LBA high port
    __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(command),                       "d"(0x1F7) );  // Command port - send read/write command

    if (command == READ_WITH_RETRY) {
        for (uint8_t i = size_in_sectors; i > 0; i--) {
            while (1) {
                // Poll status port after reading 1 sector
                __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );  // Read status register (port 1F7h)
                if (!(test_byte & (1 << 7)))    // Wait until BSY bit is clear
                    break;
            }

            // Read words from DX port # into EDI, CX # of times
            // DI = address to read into, cx = # of words to read for 1 sector,
            // DX = data port
            __asm__ __volatile__ ("rep insw" : : "D"(address), "c"(256), "d"(0x1F0) ); 

            // 400ns delay - Read alternate status register
            for (uint8_t i = 0; i < 4; i++)
                __asm__ __volatile__ ("inb %%dx, %%al" : : "d"(0x3F6), "a"(0) );   // Use any value for "a", it's just to keep the al
        }

    } else if (command == WRITE_WITH_RETRY) {
        for (uint8_t i = size_in_sectors; i > 0; i--) {
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
                                  // SI = address to write sectors from, CX = # of words to write,
                                  // DX = data port
                                  : : "S"(address), "c"(256), "d"(0x1F0) );

            // 400ns delay - Read alternate status register
            for (uint8_t i = 0; i < 4; i++)
                __asm__ __volatile__ ("inb %%dx, %%al" : : "d"(0x3F6), "a"(0) );   // Use any value for "a", it's just to keep the al
        }

        // Send cache flush command after write command is finished
        __asm__ __volatile__ ("outb %%al, %%dx" : : "a"(0xE7), "d"(0x1F7) );  // Command port - cache flush

        // Wait until BSY bit is clear after cache flush
        while (1) {
            __asm__ __volatile__ ("inb %%dx, %%al" : "=a"(test_byte) : "d"(0x1F7) );
            if (!(test_byte & (1 << 7)))
                break;
        }
    }

    // TODO: Handle disk write error for file table here...
    //   Check error ata pio register here...
}

