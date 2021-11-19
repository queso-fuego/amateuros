/*
 * time.h: Time related functions
 */
#pragma once

// Sleep for a given number of seconds
void sleep_seconds(const uint16_t seconds) 
{
    __asm__ __volatile__("int $0x80" : : "a"(2), "b"(seconds*1000) );
}

// Sleep for a given number of milliseconds
void sleep_milliseconds(const uint32_t milliseconds)
{
    __asm__ __volatile__("int $0x80" : : "a"(2), "b"(milliseconds) );
}
