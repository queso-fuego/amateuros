/*
 * exceptions.h: Functions/ISRs for exceptions (interrupts 0-31 in the IDT)
 */
#pragma once

#include "../interrupts/idt.h"
#include "../print/print_types.h"
#include "../gfx/2d_gfx.h"

__attribute__ ((interrupt)) void div_by_0_handler(int_frame_32_t *frame)
{
    uint16_t x = 0, y = 0;
    uint32_t color = user_gfx_info->fg_color;   // Save current text color

    user_gfx_info->fg_color = convert_color(0x00FF0000);          // Red
    print_string(&x, &y, "DIVIDE BY 0 ERROR - EXCEPTION HANDLED");

    user_gfx_info->fg_color = color;

    // Attempt to move on after the offending DIV/IDIV instruction by 
    //   incrementing instruction pointer
    frame->eip++;
}
