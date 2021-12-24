/*
 * pc_speaker.h: Definitions and functions for emulated (maybe hardware?) pc speaker
 */
#pragma once

#include "../C/time.h"
#include "../interrupts/pic.h"
#include "../ports/io.h"

#define PC_SPEAKER_PORT 0x61

// TODO: Mess with PWM and get it to work for pc speaker sound?
// ...

// Frequency rates to divide PIT default rate of 1193182mhz by to get a given note
typedef enum {
    A4 = 2711
} note_freq_t;

// Enable PC Speaker
void enable_pc_speaker() 
{
    uint8_t temp = inb(PC_SPEAKER_PORT);
    outb(PC_SPEAKER_PORT, temp | 3);  // Set first 2 bits to turn on speaker
}

// Disable PC Speaker
void disable_pc_speaker() 
{
    uint8_t temp = inb(PC_SPEAKER_PORT);
    outb(PC_SPEAKER_PORT, temp & 0xFC);  // Clear first 2 bits to turn off speaker
}

// Play a note for a given duration
void play_note(const note_freq_t note, const uint32_t ms_duration)
{
    set_pit_channel_mode_frequency(2, 3, note);

    // Note: Assuming PIT channel 0 is set to 1000hz or 1ms rate
    sleep_milliseconds(ms_duration);
}

// Rest for a given duration
void rest(const uint32_t ms_duration)
{
    set_pit_channel_mode_frequency(2, 3, 40);

    // Note: Assuming PIT channel 0 is set to 1000hz or 1ms rate
    sleep_milliseconds(ms_duration);
}
