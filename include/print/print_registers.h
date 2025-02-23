//
// print_registers.asm: prints registers and memory addresses to screen
//
// input 1: cursor Y position (address, not value)
// input 2: cursor X position (address, not value)
#pragma once

#include "C/stdio.h"

void print_registers(void)
{
    uint8_t *printRegHeading = "\x0A\x0D" "--------  ------------" 
                               "\x0A\x0D" "Register  Mem Location" 
                               "\x0A\x0D" "--------  ------------\0";
    uint32_t hex_num = 0;

    // TODO: Save all registers (& flags?) at start of file,
    //  then use them to print their original values before
    //  this file was called
    
    printf(printRegHeading);

    // TODO: Change to print out DX after CX, so all are in order

	// Print string for DX
    __asm__ ("movl %%edx, %0" : "=r" (hex_num) );
    printf("\r\nedx        %x", hex_num);

	// Print string for AX
    __asm__ ("movl %%eax, %0" : "=r" (hex_num) );
    printf("\r\neax        %x", hex_num);

	// Print string for BX
    __asm__ ("movl %%ebx, %0" : "=r" (hex_num) );
    printf("\r\nebx        %x", hex_num);

	// Print string for CX
    __asm__ ("movl %%ecx, %0" : "=r" (hex_num) );
    printf("\r\necx        %x", hex_num);

	// Print string for SI
    __asm__ ("movl %%esi, %0" : "=r" (hex_num) );
    printf("\r\nesi        %x", hex_num);

	// Print string for DI
    __asm__ ("movl %%edi, %0" : "=r" (hex_num) );
    printf("\r\nedi        %x", hex_num);

	// Print string for CS
    __asm__ ("movl %%cs, %0" : "=r" (hex_num) );
    printf("\r\ncs         %x", hex_num);

	// Print string for DS
    __asm__ ("movl %%ds, %0" : "=r" (hex_num) );
    printf("\r\nds         %x", hex_num);

	// Print string for ES
    __asm__ ("movl %%es, %0" : "=r" (hex_num) );
    printf("\r\nes         %x", hex_num);

	// Print string for SS
    __asm__ ("movl %%ss, %0" : "=r" (hex_num) );
    printf("\r\nss         %x", hex_num);

	// Print newline when done
    printf("\r\n");
}
