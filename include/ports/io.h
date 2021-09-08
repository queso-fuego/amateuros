/*
 * io.h: C abstractions for asm in/out instructions
 */
#pragma once

// Write AL to port DX
void outb(uint16_t port, uint8_t value)
{
    // Out imm8, al will be used if applicable ("N" constraint will be converted to immediate constant),
    //   otherwise Out DX, and AL will be used
    __asm__ __volatile__ ("outb %0, %1" : : "a"(value), "Nd"(port) );
}

// Read in value from port DX to AL & return AL
uint8_t inb(uint16_t port)
{
    uint8_t ret_val;

    // In AL, imm8 will be used if applicable, otherwise in AL, DX will be used
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret_val) : "Nd"(port) );

    return ret_val;
}

// Wait 1 I/O cycle for I/O operations to complete
void io_wait(void)
{
    // Port 0x80 is used for checkpoints during POST
    //  Linux uses this, so we "should" be OK to use it also
    __asm__ __volatile__ ("outb %%al, $0x80" : : "a"(0) );
}
