// 
// clear_screen.h: clears screen by writing to video memory 
//
#pragma once

void clear_screen(void)
{
    // Get 32bit pointer to framebuffer value in VBE mode info block, 
    //   dereference to get the 32bit value,
    //   get 32bit pointer to that value - memory address of framebuffer
    uint32_t *framebuffer = (uint32_t *)*(uint32_t *)0x9028; 

    for (uint32_t i = 0; i < 1920*1080; i++)
        framebuffer[i] = 0x000000FF;    // Write 32bit ARGB pixel to screen
}
