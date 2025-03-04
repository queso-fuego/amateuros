/*
 * exceptions.h: Functions/ISRs for exceptions (interrupts 0-31 in the IDT)
 */
#pragma once

#include "C/stdio.h"
#include "interrupts/idt.h"
#include "gfx/2d_gfx.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"

__attribute__ ((interrupt)) void div_by_0_handler(int_frame_32_t *frame) {
    uint32_t color = user_gfx_info->fg_color;   // Save current text color

    user_gfx_info->fg_color = convert_color(0x00FF0000);          // Red
    printf("\033X0Y0;DIVIDE BY 0 ERROR - EXCEPTION HANDLED");

    user_gfx_info->fg_color = color;

    // Attempt to move on after the offending DIV/IDIV instruction by 
    //   incrementing instruction pointer
    frame->eip++;
}

__attribute__ ((interrupt)) void protection_fault_handler(int_frame_32_t *frame, uint32_t error_code) {
    (void)frame;    // Silence compiler warnings

    user_gfx_info->fg_color = convert_color(0x00FF0000);    // Red text
    printf("\033CSROFF;\033X0Y0;"
           "PROTECTION FAULT EXCEPTION (#GP)\r\nERROR CODE: %#x\r\n", 
           error_code);

    int32_t ext = error_code & 1;               // bit 0; External yes/no
    int32_t tbl = (error_code >> 1) & 0x02;     // bits 2-1; Table descriptor type
    int32_t idx = (error_code >> 3) & 0x01FFF;  // bits 15-3; Index in table
    char *tbl_type[4] = {
        "GDT",
        "IDT",
        "LDT",
        "IDT",
    };

    if (error_code != 0) {
        printf("Ext: %d\r\n"
               "Tbl: %d (%s)\r\n"
               "Idx: %#x\r\n",
               ext,
               tbl, tbl_type[tbl],
               idx);
    }

    __asm__ __volatile__("cli;hlt");
}

__attribute__ ((interrupt)) void page_fault_handler(int_frame_32_t *frame, uint32_t error_code) {
    (void)frame, (void)error_code;    // Silence compiler warnings
                    
    uint32_t color = user_gfx_info->fg_color;   // Save current text color
    uint32_t bad_address = 0;

    user_gfx_info->fg_color = convert_color(0x00FF0000);          // Red

    // CR2 contains bad address that caused page fault
    __asm__ __volatile__("movl %%CR2, %0" : "=r"(bad_address) );

    // Map in bad page, and set present/read/write flags
    void *phys_address = allocate_blocks(1);
    if (!phys_address) {
        printf("\033X0Y0;PAGE FAULT EXCEPTION (#PF)\r\nERROR CODE: %#x", error_code);
        printf("\r\nADDRESS: %#x", bad_address);
        printf("\r\nCOULD NOT ALLOCATE PAGES");
        __asm__ __volatile__("cli;hlt");
    }

    if (!map_address(current_page_directory, (uint32_t)phys_address, bad_address, 
                     PTE_PRESENT | PTE_READ_WRITE)) {

        printf("\033X0Y0;PAGE FAULT EXCEPTION (#PF)\r\nERROR CODE: %#x", error_code);
        printf("\r\nADDRESS: %#x", bad_address);
        printf("\r\nCOULD NOT MAP PAGES");
        __asm__ __volatile__("cli;hlt");
    }

    //printf("\r\nEXCEPTION HANDLED; NEW PHYS ADDR: %#x, VIRT ADDR: %#x",
    //       (uint32_t)phys_address, bad_address);

    user_gfx_info->fg_color = color;    // Restore text color
}




