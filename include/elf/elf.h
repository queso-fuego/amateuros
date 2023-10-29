/*
 * elf/elf.h: Types, definitions, functions, etc. for loading and running ELF objects
 */
#pragma once

#include "C/stdint.h"

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











