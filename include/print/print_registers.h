//
// print_registers.asm: prints registers and memory addresses to screen
//
// input 1: cursor Y position (address, not value)
// input 2: cursor X position (address, not value)
#pragma once

void print_registers(uint16_t *cursor_x, uint16_t *cursor_y)
{
    uint8_t *printRegHeading = "\x0A\x0D" "--------  ------------" 
                               "\x0A\x0D" "Register  Mem Location" 
                               "\x0A\x0D" "--------  ------------\0";

    uint8_t *regString  = "\x0A\x0D" "dx        \0";     // Hold string of current register name and memory address
    uint32_t csr_x_addr = 0;
    uint32_t csr_y_addr = 0;
    uint16_t hex_num;

    // TODO: Change to print out 32 bit registers instead of 16 bit 
    
    // TODO: Save all registers (& flags?) at start of file,
    //  then use them to print their original values before
    //  this file was called
    
	// Print string
    print_string(cursor_x, cursor_y, printRegHeading); 

	// Print string for DX
    // TODO: Change to print out DX after CX, so all are in order
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for DX
    __asm ("movw %%dx, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for AX
    regString[2] = 'a';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for AX
    __asm ("movw %%ax, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for BX
    regString[2] = 'b';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for BX
    __asm ("movw %%bx, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for CX
    regString[2] = 'c';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for CX
    __asm ("movw %%cx, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for SI
    regString[2] = 's';
    regString[3] = 'i';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for SI
    __asm ("movw %%si, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for DI
    regString[2] = 'd';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for DI
    __asm ("movw %%di, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for CS
    regString[2] = 'c';
    regString[3] = 's';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for CS
    __asm ("movw %%cs, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for DS
    regString[2] = 'd';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for DS
    __asm ("movw %%ds, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for ES
    regString[2] = 'e';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for ES
    __asm ("movw %%es, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

	// Print string for SS
    regString[2] = 's';
    print_string(cursor_x, cursor_y, regString);

	// Print hex value for SS
    __asm ("movw %%ss, %0" : "=r" (hex_num) );
    print_hex(cursor_x, cursor_y, hex_num);

    // Reset regString to 'dx' for next time
    regString[2] = 'd';
    regString[3] = 'x';

	// Print newline when done
    print_char(cursor_x, cursor_y, 0x0A); 
    print_char(cursor_x, cursor_y, 0x0D); 
}
