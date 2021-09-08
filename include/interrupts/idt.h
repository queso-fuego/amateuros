/*
 * idt.h: Data types and setup for the interrupt descriptor table/IDT
 */
#pragma once

#include "../print/print_types.h"

#define TRAP_GATE_FLAGS     0x8F    // P = 1, DPL = 00, S = 0, Type = 1111 (32bit trap gate)
#define INT_GATE_FLAGS      0x8E    // P = 1, DPL = 00, S = 0, Type = 1110 (32bit interrupt gate)
#define INT_GATE_USER_FLAGS 0xEE    // P = 1, DPL = 11, S = 0, Type = 1110 (32bit interrupt gate, called from PL 3)

// IDT entry
typedef struct {
    uint16_t isr_address_low;   // Lower 16bits of isr address
    uint16_t kernel_cs;         // Code segment for this ISR
    uint8_t  reserved;          // Set to 0, reserved by intel
    uint8_t  attributes;        // Type and attributes; Flags
    uint16_t isr_address_high;  // Upper 16bits of isr address
} __attribute__ ((packed)) idt_entry_32_t;

// IDTR layout
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__ ((packed)) idtr_32_t;

idt_entry_32_t idt32[256];  // This is the actual IDT
idtr_32_t idtr32;           // Interrupt descriptor table register instance

// Interrupt "frame" to pass to interrupt handlers/ISRs
typedef struct {
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t sp;
    uint32_t ss;
} __attribute__ ((packed)) int_frame_32_t;

// Default exception handler (no error code)
__attribute__ ((interrupt)) void default_excp_handler(int_frame_32_t *int_frame_32)
{
    uint16_t x = 0, y = 0; 
    print_string(&x, &y, "DEFAULT EXCEPTION HANDLER - NO ERROR CODE");
}

// Default exception handler (includes error code)
__attribute__ ((interrupt)) void default_excp_handler_err_code(int_frame_32_t *int_frame_32, uint32_t error_code)
{
    uint16_t x = 0, y = 0; 

    print_string(&x, &y, "DEFAULT EXCEPTION HANDLER - ERROR CODE: ");
    print_hex(&x, &y, error_code);
}

// Default interrupt handler
__attribute__ ((interrupt)) void default_int_handler(int_frame_32_t *int_frame_32)
{
    uint16_t x = 0, y = 0; 
    print_string(&x, &y, "DEFAULT INTERRUPT HANDLER");
}

// Add an ISR to the IDT
void set_idt_descriptor_32(uint8_t entry_number, void *isr, uint8_t flags)
{
    idt_entry_32_t *descriptor = &idt32[entry_number];

    descriptor->isr_address_low  = (uint32_t)isr & 0xFFFF;  // Low 16 bits of ISR address 
    descriptor->kernel_cs        = 0x08;                    // Kernel code segment containing this isr
    descriptor->reserved         = 0;                       // Reserved, must be set to 0
    descriptor->attributes       = flags;                   // Type & attributes (INT_GATE, TRAP_GATE, etc.)
    descriptor->isr_address_high = ((uint32_t)isr >> 16) & 0xFFFF;  // High 16 bits of ISR address 
}

// Setup/Initiize the IDT
void init_idt_32(void)
{
    idtr32.limit = (uint16_t)(sizeof idt32); // Size should be 8 bytes * 256 descriptors (0-255) 
    idtr32.base  = (uint32_t)&idt32; 

    // Set up exception handlers first (ISRs 0-31)
    for (uint8_t entry = 0; entry < 32; entry++) {
        if (entry == 8  || entry == 10 || entry == 11 || entry == 12 || 
            entry == 13 || entry == 14 || entry == 17 || entry == 21) {
            // Exception takes an error code
            set_idt_descriptor_32(entry, default_excp_handler_err_code, TRAP_GATE_FLAGS);
        } else {
            // Exception does not take an error code
            set_idt_descriptor_32(entry, default_excp_handler, TRAP_GATE_FLAGS);
        }
    }

    // Set up regular interrupts (ISRs 32-255)
    for (uint16_t entry = 32; entry < 256; entry++) {
        set_idt_descriptor_32(entry, default_int_handler, INT_GATE_FLAGS);
    }

    __asm__ __volatile__ ("lidt %0" : : "memory"(idtr32)); // Load IDT to IDT register
}
