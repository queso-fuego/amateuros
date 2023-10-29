//
// keyboard.h: Returns an ascii character from a global memory location
//
#pragma once

#include "../C/stdbool.h"
#include "../global/global_addresses.h"

typedef struct {
    uint8_t key;   
    bool    shift;
    bool    ctrl; 
} __attribute__ ((packed)) key_info_t;

uint8_t get_key(void) {
    key_info_t *key_info = (key_info_t *)KEY_INFO_ADDRESS;
    while (!key_info->key) __asm__ __volatile__("hlt");

    uint8_t output = key_info->key;
    key_info->key = 0;

    return output;
}
