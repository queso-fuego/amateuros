/*
 * elf/elf.h: Types, definitions, functions, etc. for loading and running ELF objects
 */
#pragma once

#include "C/stdint.h"
#include "C/string.h"
#include "C/stddef.h"
#include "memory/malloc.h"

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;

#define EI_NIDENT (16)

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half	  e_type;
    Elf32_Half	  e_machine;
    Elf32_Word	  e_version;
    Elf32_Addr	  e_entry;
    Elf32_Off	  e_phoff;
    Elf32_Off	  e_shoff;
    Elf32_Word	  e_flags;
    Elf32_Half	  e_ehsize;
    Elf32_Half	  e_phentsize;
    Elf32_Half	  e_phnum;
    Elf32_Half	  e_shentsize;
    Elf32_Half	  e_shnum;
    Elf32_Half	  e_shstrndx;
} Elf32_Ehdr;

// e_type values
enum {
    ET_NONE = 0x0,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    // ...
};

typedef struct {
    Elf32_Word	p_type;
    Elf32_Off	p_offset;
    Elf32_Addr	p_vaddr;
    Elf32_Addr	p_paddr;
    Elf32_Word	p_filesz;
    Elf32_Word	p_memsz;
    Elf32_Word	p_flags;
    Elf32_Word	p_align;
} Elf32_Phdr;

// p_type values
enum {
    PT_NULL = 0x0,
    PT_LOAD,            // Loadable section
    // ...
};

// Load ELF File
void *load_elf_file(uint8_t *file_address, void *exe_buffer) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)file_address;

    // Print ELF Info
    printf("Info:\r\n"
           "Type: %d\r\n"
           "Machine: %x\r\n"
           "Entry: %x\r\n"
           "Program Header offset: %d\r\n"
           "Elf Header size: %d\r\n"
           "Program Header entry size: %d\r\n"
           "Program Header num: %d\r\n",
           ehdr->e_type,
           ehdr->e_machine,
           ehdr->e_entry,
           ehdr->e_phoff,
           ehdr->e_ehsize,
           ehdr->e_phentsize,
           ehdr->e_phnum);

    // Only allow executables or Dynamic executables (e.g. PIE)
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        printf("\r\nError: Program is not an executable or dynamic executable.\r\n");
        return NULL;
    }

    // Figure out max needed memory to load elf sections into
    uint32_t mem_min = 0xFFFFFFFF, mem_max = 0;

    Elf32_Phdr *phdr = (Elf32_Phdr *)(file_address + ehdr->e_phoff);

    uint32_t alignment = PAGE_SIZE;
    uint32_t align = alignment;

    printf("Program Headers:\r\n");
    for (uint32_t i = 0; i < ehdr->e_phnum; i++) {
        // Only interested in loadable program sections
        if (phdr[i].p_type != PT_LOAD) continue;

        // Print out program header info
        printf("No: %d, Type: %d, Offset: %d, Virt Addr: %x, Phys Addr: %x, "
               "File Size: %d, Mem Size: %d, Flags: %d, Align: %x\r\n",
               i,
               phdr[i].p_type,
               phdr[i].p_offset,
               phdr[i].p_vaddr,
               phdr[i].p_paddr,
               phdr[i].p_filesz,
               phdr[i].p_memsz,
               phdr[i].p_flags,
               phdr[i].p_align);

        // Update max alignment as needed
        if (align < phdr[i].p_align) align = phdr[i].p_align;

        uint32_t mem_begin = phdr[i].p_vaddr;
        uint32_t mem_end = phdr[i].p_vaddr + phdr[i].p_memsz + align-1;

        mem_begin &= ~(align-1);
        mem_end &= ~(align-1);

        // Get new minimum & maximum memory bounds for all program sections
        if (mem_begin < mem_min) mem_min = mem_begin;
        if (mem_end > mem_max) mem_max = mem_end;
    }

    uint32_t buffer_size = mem_max - mem_min;
    printf("\r\nMemory needed for file: %x\r\n", buffer_size);

    // Create buffer for file
    exe_buffer = malloc(buffer_size);

    if (exe_buffer == NULL) {
        printf("\r\nError: Could not malloc() enough memory for program\r\n");
        return NULL;
    }

    printf("\r\nProgram buffer address: %x\r\n", (uint32_t)exe_buffer);

    // Zero init buffer, to ensure 0 padding for all program sections
    memset(exe_buffer, 0, buffer_size);

    // Load program headers into buffer
    for (uint32_t i = 0; i < ehdr->e_phnum; i++) {
        // Only interested in loadable program sections
        if (phdr[i].p_type != PT_LOAD) continue;

        // Use relative position of program section in file, to ensure it still works correctly.
        //   With PIE executables, this means we can use any entry point or addresses, as long as
        //   we use the same relative addresses.
        // In PE files this should be equivalent to the "RVA"
        uint32_t relative_offset = phdr[i].p_vaddr - mem_min;

        // Read in p_memsz amount of data from p_offset into original file buffer,
        //   to p_vaddr (offset by a relative amount) into new buffer
        uint8_t *dst = (uint8_t *)exe_buffer + relative_offset; 
        uint8_t *src = file_address + phdr[i].p_offset;
        uint32_t len = phdr[i].p_memsz;

        printf("MEMCPY dst: %x, src: %x, len: %x\r\n", (uint32_t)dst, (uint32_t)src, len);

        memcpy(dst, src, len);
    }

    // Return entry point which if offset into new buffer, 
    //   also needs to be relatively offset from the start of the original ELF buffer
    void *entry_point = (void *)((uint8_t *)exe_buffer + (ehdr->e_entry - mem_min));

    return entry_point;
}









