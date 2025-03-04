//
// keyboard.h: Returns an ascii character from a global memory location
//
#pragma once

#include "C/stdbool.h"
#include "C/stdint.h"
#include "C/time.h"

typedef struct {
    uint8_t key;   
    bool    shift;
    bool    ctrl; 
} key_info_t;

key_info_t get_key(void) {
    key_info_t key_info = {0}, null_key = {0};
    read(stdin, &key_info, sizeof key_info);
    while (!key_info.key) {
        seek(stdin, -sizeof key_info, SEEK_CUR);
        sleep_milliseconds(10);
        read(stdin, &key_info, sizeof key_info);
    }

    // Erase key data in file, to prevent infinite duplicate reads 
    seek(stdin, -sizeof key_info, SEEK_CUR);
    write(stdin, &null_key, sizeof key_info);

    return key_info;
}
