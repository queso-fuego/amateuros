/*
 * pic.h: 8259 Programmable Interrupt Controller functions
 */
#pragma once

#include "../ports/io.h"
#include "../interrupts/idt.h"

#define PIC_1_CMD  0x20
#define PIC_1_DATA 0x21

#define PIC_2_CMD  0xA0
#define PIC_2_DATA 0xA1

#define NEW_IRQ_0  0x20 // IRQ 0-7  will be mapped to interrupts 0x20-0x27 (32-39)
#define NEW_IRQ_8  0x28 // IRQ 8-15 will be mapped to interrupts 0x28-0x2F (40-47)

#define PIC_EOI    0x20 // "End of interrupt" command

#define IRQ0_SLEEP_TIMER_TICKS_AREA 0x1700

uint32_t *sleep_timer_ticks = (uint32_t *)IRQ0_SLEEP_TIMER_TICKS_AREA;

// Send end of interrupt command to signal IRQ has been handled
void send_pic_eoi(uint8_t irq)
{
    if (irq >= 8) outb(PIC_2_CMD, PIC_EOI);

    outb(PIC_1_CMD, PIC_EOI);
}

// Disable PIC, if using APIC or IOAPIC
void disable_pic(void)
{
    outb(PIC_2_DATA, 0xFF);
    outb(PIC_1_DATA, 0xFF);
}

// Set IRQ mask by setting the bit in the IMR (interrupt mask register)
//   This will ignore the IRQ
void set_irq_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) port = PIC_1_DATA;
    else {
        irq -= 8;
        port = PIC_2_DATA;
    }

    // Get current IMR value, set on the IRQ bit to mask it, then write new value
    //   to IMR
    // NOTE: Masking IRQ2 will stop 2nd PIC from raising IRQs due to 2nd PIC being
    //   mapped to IRQ2 in PIC1
    value = inb(port) | (1 << irq);
    outb(port, value);
}

// Clear IRQ mask by clearing the bit in the IMR (interrupt mask register)
//   This will enable recognizing the IRQ
void clear_irq_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) port = PIC_1_DATA;
    else {
        irq -= 8;
        port = PIC_2_DATA;
    }

    // Get current IMR value, clear the IRQ bit to unmask it, then write new value
    //   to IMR
    // NOTE: Clearing IRQ2 will enable the 2nd PIC to raise IRQs due to 2nd PIC being
    //   mapped to IRQ2 in PIC1
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// Remap PIC to use interrupts above first 15, to not interfere with exceptions (ISRs 0-31)
void remap_pic(void)
{
    uint8_t pic_1_mask, pic_2_mask;

    // Save current masks
    pic_1_mask = inb(PIC_1_DATA);
    pic_2_mask = inb(PIC_2_DATA);

    // ICW 1 (Initialization control word) - bit 0 = send up to ICW 4, bit 4 = initialize PIC
    outb(PIC_1_CMD, 0x11);
    io_wait();
    outb(PIC_2_CMD, 0x11);
    io_wait();

    // ICW 2 - Where to map the base interrupt in the IDT
    outb(PIC_1_DATA, NEW_IRQ_0);
    io_wait();
    outb(PIC_2_DATA, NEW_IRQ_8);
    io_wait();
    
    // ICW 3 - Where to map PIC2 to the IRQ line in PIC1; Mapping PIC2 to IRQ 2
    outb(PIC_1_DATA, 0x4);  // Bit # (0-based) - 0100 = bit 2 = IRQ2
    io_wait();
    outb(PIC_2_DATA, 0x2);  // Binary # for IRQ in PIC1, 0010 = 2
    io_wait();
   
    // ICW 4 - Set x86 mode
    outb(PIC_1_DATA, 0x1); 
    io_wait();
    outb(PIC_2_DATA, 0x1); 
    io_wait();

    // Save current masks
    outb(PIC_1_DATA, pic_1_mask);
    outb(PIC_2_DATA, pic_2_mask);
}

// PIT Timer Channel 0 PIC IRQ0 interrupt handler
__attribute__ ((interrupt)) void timer_irq0_handler(int_frame_32_t *frame)
{
    if (*sleep_timer_ticks > 0) (*sleep_timer_ticks)--;

    send_pic_eoi(0);
}

// Change PIT Channel frequency
void set_pit_channel_mode_frequency(const uint8_t channel, const uint8_t operating_mode, const uint16_t frequency)
{
    // Invalid input
    if (channel > 2) return;

    __asm__ __volatile__ ("cli");

    /* PIT I/O Ports:
     * 0x40 - channel 0     (read/write) 
     * 0x41 - channel 1     (read/write) 
     * 0x42 - channel 2     (read/write) 
     * 0x43 - Mode/Command  (write only) 
     *
     * 0x43 Command register value bits (1 byte):
     * 7-6 select channel:
     *      00 = channel 0
     *      01 = channel 1
     *      10 = channel 2
     *      11 = read-back command
     * 5-4 access mode:
     *      00 = latch count value
     *      01 = lobyte only
     *      10 = hibyte only
     *      11 = lobyte & hibyte
     * 3-1 operating mode:
     *      000 = mode 0 (interrupt on terminal count)
     *      001 = mode 1 (hardware re-triggerable one-shot)
     *      010 = mode 2 (rate generator)
     *      011 = mode 3 (square wave generator)
     *      100 = mode 4 (software triggered strobe)
     *      101 = mode 5 (hardware triggered strobe)
     *      110 = mode 6 (rate generator, same as 010)
     *      111 = mode 7 (square wave generator, same as 011)
     * 0  BCD/Binary mode:
     *      0 = 16bit binary
     *      1 = 4-digit BCD (x86 does not use this!)
     */

    // Send the command byte, always sending lobyte/hibyte for access mode
    outb(0x43, (channel << 6) | (0x3 << 4) | (operating_mode << 1));

    // Send the frequency divider to the input channel
    outb(0x40 + channel, (uint8_t)frequency);           // Low byte
    outb(0x40 + channel, (uint8_t)(frequency >> 8));    // High byte

    __asm__ __volatile__ ("sti");
}









