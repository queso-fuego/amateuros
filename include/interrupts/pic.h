/*
 * pic.h: 8259 Programmable Interrupt Controller functions
 */
#pragma once

#include "../C/stdbool.h"
#include "../global/global_addresses.h"
#include "../ports/io.h"
#include "../interrupts/idt.h"
#include "../print/print_types.h"
#include "../keyboard/keyboard.h"

#define PIC_1_CMD  0x20
#define PIC_1_DATA 0x21

#define PIC_2_CMD  0xA0
#define PIC_2_DATA 0xA1

#define NEW_IRQ_0  0x20 // IRQ 0-7  will be mapped to interrupts 0x20-0x27 (32-39)
#define NEW_IRQ_8  0x28 // IRQ 8-15 will be mapped to interrupts 0x28-0x2F (40-47)

#define PIC_EOI    0x20 // "End of interrupt" command

#define PS2_DATA_PORT 0x60

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

// Keyboard IRQ1 handler
__attribute__ ((interrupt)) void keyboard_irq1_handler(int_frame_32_t *frame)
{
    enum {
        LSHIFT_MAKE  = 0x2A,
        LSHIFT_BREAK = 0xAA,
        RSHIFT_MAKE  = 0x36,
        RSHIFT_BREAK = 0xB6,
        LCTRL_MAKE   = 0x1D,
        LCTRL_BREAK  = 0x9D,
    };

    uint8_t key;
    static bool e0 = false; 
    static bool e1 = false;

    // TODO: Add keyboard initialization & scancode functions, 
    //   do not assume scancode set 1
    
    // Scancode set 1 -> Ascii lookup table
    const uint8_t *scancode_to_ascii = "\x00\x1B" "1234567890-=" "\x08"
    "\x00" "qwertyuiop[]" "\x0D\x1D" "asdfghjkl;'`" "\x00" "\\"
    "zxcvbnm,./" "\x00\x00\x00" " ";

    // Shift key pressed on number row lookup table (0-9 keys)
    const uint8_t *num_row_shifts = ")!@#$%^&*(";

    // Set current key to null
    key_info->key = 0;

    key = inb(PS2_DATA_PORT);   // Read in new key

    if (key) {
        if      (key == LSHIFT_MAKE  || key == RSHIFT_MAKE) key_info->shift = true; 
        else if (key == LSHIFT_BREAK || key == RSHIFT_BREAK) key_info->shift = false; 
        else if (key == LCTRL_MAKE)  key_info->ctrl = true;
        else if (key == LCTRL_BREAK) key_info->ctrl = false;
        else if (key == 0xE0) e0 = true;
        else {
            if (!(key & 0x80)) {
                // Don't translate escaped scancodes, only return them
                if (!e0) { 
                    key = scancode_to_ascii[key]; 

                    // If pressed shift, translate key to shifted key
                    if (key_info->shift) {
                        if      (key >= 'a' && key <= 'z') key -= 0x20;  // Convert lowercase into uppercase
                        else if (key >= '0' && key <= '9') key = num_row_shifts[key-0x30];  // Get int value of character, offset into shifted nums
                        else {
                            if      (key == '=')  key = '+';
                            else if (key == '\\') key = '|'; 
                            else if (key == '`')  key = '~';
                            else if (key == '[')  key = '{';
                            else if (key == ']')  key = '}';
                            else if (key == '\'') key = '\"';
                            else if (key == ';')  key = ':';
                            else if (key == ',')  key = '<';
                            else if (key == '.')  key = '>';
                            else if (key == '/')  key = '?';
                            // TODO: Add more shifted keys here...
                        }
                    }
                }
                key_info->key = key;    // Set ascii key value in struct
            }
            if (e0) e0 = false;
        }
    }
    send_pic_eoi(1);
}

// Show date time info or not
static bool show_datetime = false;

// Date time info
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} __attribute__ ((packed)) datetime_t;

datetime_t *datetime = (datetime_t *)RTC_DATETIME_AREA;

// CMOS registers
enum {
    cmos_address = 0x70,
    cmos_data = 0x71 
};

// Read CMOS update in progress flag/bit in status register A
bool cmos_update_in_progress(void)
{
    outb(cmos_address, 0x8A);       // Will read from status register A, disable NMI
    io_wait();                      // Small delay
    return (inb(cmos_data) & 0x80); // If register A top bit is set, CMOS update is in progress
}

// Get an RTC register value
uint8_t get_rtc_register(uint8_t reg)
{
    outb(cmos_address, reg | 0x80);     // Disable NMI when sending register to read
    io_wait();                          // Small delay
    return inb(cmos_data);              // Return data at that register
}

// Enable RTC
void enable_rtc(void)
{
    uint8_t prev_regB_value = get_rtc_register(0x0B);

    outb(cmos_address, 0x8B);                // Select register B again, because reading a CMOS register resets to register D
    io_wait();                               // Small delay
    outb(cmos_data, prev_regB_value | 0x40); // Set bit 6 to enable periodic interrupts at default rate of 1024hz

    get_rtc_register(0x0C);                  // Read status register C to clear out any pending IRQ8 interrupts
}

// Disable RTC
void disable_rtc(void)
{
    uint8_t prev_regB_value;

    __asm__ __volatile__ ("cli");   

    prev_regB_value = get_rtc_register(0x0B);

    outb(cmos_address, 0x8B);                // Select register B again, because reading a CMOS register resets to register D
    io_wait();                               // Small delay
    outb(cmos_data, prev_regB_value & 0xBF); // Clear bit 6 to disable periodic interrupts

    __asm__ __volatile__ ("sti");   
}

// CMOS RTC IRQ8 Handler
__attribute__ ((interrupt)) void cmos_rtc_irq8_handler (int_frame_32_t *frame)
{
    datetime_t new_datetime, old_datetime; 
    static uint16_t rtc_ticks = 0;
    uint8_t regB_value;

    __asm__ __volatile__ ("cli");

    rtc_ticks++;

    // If one second passed, get new date/time values
    if (rtc_ticks % 1024 == 0) {
        while (cmos_update_in_progress()) ; // Wait until CMOS is done updating

        new_datetime.second = get_rtc_register(0x00);
        new_datetime.minute = get_rtc_register(0x02);
        new_datetime.hour   = get_rtc_register(0x04);
        new_datetime.day    = get_rtc_register(0x07);
        new_datetime.month  = get_rtc_register(0x08);
        new_datetime.year   = get_rtc_register(0x09);

        do {
            old_datetime = new_datetime;

            while (cmos_update_in_progress()) ; // Wait until CMOS is done updating

            new_datetime.second = get_rtc_register(0x00);
            new_datetime.minute = get_rtc_register(0x02);
            new_datetime.hour   = get_rtc_register(0x04);
            new_datetime.day    = get_rtc_register(0x07);
            new_datetime.month  = get_rtc_register(0x08);
            new_datetime.year   = get_rtc_register(0x09);

        } while (
            (new_datetime.second != old_datetime.second) || 
            (new_datetime.minute != old_datetime.minute) || 
            (new_datetime.hour   != old_datetime.hour)   || 
            (new_datetime.day    != old_datetime.day)    || 
            (new_datetime.month  != old_datetime.month)  || 
            (new_datetime.year   != old_datetime.year)
          );

        regB_value = get_rtc_register(0x0B);

        // Convert BCD values to binary if needed (bit 2 is clear)
        if (!(regB_value & 0x04)) {
            new_datetime.second = (new_datetime.second & 0x0F) + ((new_datetime.second / 16) * 10);
            new_datetime.minute = (new_datetime.minute & 0x0F) + ((new_datetime.minute / 16) * 10);
            new_datetime.hour = ((new_datetime.hour & 0x0F) + (((new_datetime.hour & 0x70) / 16) * 10)) | (new_datetime.hour & 0x80);
            new_datetime.day = (new_datetime.day & 0x0F) + ((new_datetime.day / 16) * 10);
            new_datetime.month = (new_datetime.month & 0x0F) + ((new_datetime.month / 16) * 10);
            new_datetime.year = (new_datetime.year & 0x0F) + ((new_datetime.year / 16) * 10);
        }

        // Convert 12hr to 24hr if needed (bit 1 is clear in register B and top bit of hour is set)
        if (!(regB_value & 0x02) && (new_datetime.hour & 0x80)) {
            new_datetime.hour = ((new_datetime.hour & 0x7F) + 12) % 24;
        }

        // Get year 
        new_datetime.year += 2000;

        // Set datetime values in memory
        *datetime = new_datetime;

        // Print date/time on screen
        if (show_datetime) {
            uint16_t x = 50, y = 30; // TODO: Change later to fit exactly in lower right corner of screen

            // ISO Formatted date/time - YYYY-MM-DD HH:MM:SS
            print_dec(&x, &y, datetime->year);
            print_char(&x, &y, '-');

            if (datetime->month < 10) print_char(&x, &y, '0');
            print_dec(&x, &y, datetime->month);
            print_char(&x, &y, '-');

            if (datetime->day < 10) print_char(&x, &y, '0');
            print_dec(&x, &y, datetime->day);

            print_char(&x, &y, ' ');

            if (datetime->hour < 10) print_char(&x, &y, '0');
            print_dec(&x, &y, datetime->hour);
            print_char(&x, &y, ':');

            if (datetime->minute < 10) print_char(&x, &y, '0');
            print_dec(&x, &y, datetime->minute);
            print_char(&x, &y, ':');

            if (datetime->second < 10) print_char(&x, &y, '0');
            print_dec(&x, &y, datetime->second);
        }
    }

    // Read register C so that future IRQ8s can occur
    get_rtc_register(0x0C);

    send_pic_eoi(8);

    __asm__ __volatile__ ("sti");
}

