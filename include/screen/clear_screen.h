// 
// clear_screen.h: clears screen by writing to video memory 
//
#pragma once

void clear_screen(uint32_t color)
{
    // Get 32bit pointer to framebuffer value in VBE mode info block, 
    //   dereference to get the 32bit value,
    //   get 32bit pointer to that value - memory address of framebuffer
    uint8_t *framebuffer = (uint8_t *)gfx_mode->physical_base_pointer; 
    uint8_t bytes_per_pixel = (gfx_mode->bits_per_pixel+1) / 8;

    for (uint32_t i = 0; i < gfx_mode->x_resolution * gfx_mode->y_resolution; i++) {
        *((uint32_t *)framebuffer) = color;

        framebuffer += bytes_per_pixel;
    }
}
