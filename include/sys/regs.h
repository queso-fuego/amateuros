/*
 * regs.h
 */
#pragma once

#include "C/stdint.h"

// Registers pushed onto stack when a syscall function is called
//  from the syscall dispatcher.
// NOTE: Ordering here is reverse from push order, last value pushed is
//    the first value in memory in the struct
typedef struct {
    int32_t ds;
    int32_t es;
    int32_t fs;
    int32_t gs;

    int32_t edi;
    int32_t esi;
    int32_t ebp;
    int32_t esp;
    int32_t ebx;
    int32_t edx;
    int32_t ecx;
    int32_t eax;

    int32_t syscall_num;

    // Interrupt frame pushed by cpu
    int32_t eip;   
    int32_t cs;
    int32_t eflags;
    int32_t useresp;
    int32_t ss;
} syscall_regs_t;

