/*
 * syscalls.h: Syscall handler/dispatcher and system call functions
 */
#pragma once

#include "../print/print_types.h"

#define MAX_SYSCALLS 2

// Test syscall 0
void syscall_test0(void)
{
    uint16_t x = 0, y = 20;
    uint32_t color = user_gfx_info->fg_color;   // Save current text color

    user_gfx_info->fg_color = convert_color(0x00FFFF00);    // YELLOW
    print_string(&x, &y, "Test system call 0");

    user_gfx_info->fg_color = color;    // Restore text color
}

// Test syscall 1
void syscall_test1(void)
{
    uint16_t x = 0, y = 21;
    uint32_t color = user_gfx_info->fg_color;   // Save current text color

    user_gfx_info->fg_color = convert_color(0x0000FF00);    // GREEN
    print_string(&x, &y, "Test system call 1");

    user_gfx_info->fg_color = color;    // Restore text color
}

// Syscall table
void *syscalls[MAX_SYSCALLS] = {
    syscall_test0,
    syscall_test1
};

// Syscall dispatcher
// naked attribute means no function prologue/epilogue, and only allows inline asm
__attribute__ ((naked)) void syscall_dispatcher(void)
{
    // "basic" syscall handler, push everything we want to save, call the syscall by
    //   offsetting into syscalls table with value in eax, then pop everything back 
    //   and return using "iret" (d/q), NOT regualar "ret" as this is technically
    //   an interrupt (software interrupt)
    //
    // Already on stack: SS, SP, FLAGS, CS, IP
    // Need to push: AX, GS, FS, ES, DS, BP, DI, SI, DX, CX, BX
    //
    // NOTE: Easier to do call in intel syntax, I'm not sure how to do it in att syntax
    __asm__ __volatile__ (".intel_syntax noprefix\n"

                          ".equ MAX_SYSCALLS, 2\n"  // Have to define again, inline asm does not see the #define

                          "cmp eax, MAX_SYSCALLS-1\n"   // syscalls table is 0-based
                          "ja invalid_syscall\n"        // invalid syscall number, skip and return

                          "push eax\n"
                          "push gs\n"
                          "push fs\n"
                          "push es\n"
                          "push ds\n"
                          "push ebp\n"
                          "push edi\n"
                          "push esi\n"
                          "push edx\n"
                          "push ecx\n"
                          "push ebx\n"
                          "push esp\n"
                          "call [syscalls+eax*4]\n"
                          "add esp, 4\n"
                          "pop ebx\n"
                          "pop ecx\n"
                          "pop edx\n"
                          "pop esi\n"
                          "pop edi\n"
                          "pop ebp\n"
                          "pop ds\n"
                          "pop es\n"
                          "pop fs\n"
                          "pop gs\n"
                          "add esp, 4\n"    // Save eax value in case
                          "iretd\n"         // Need interrupt return here! iret, NOT ret

                          "invalid_syscall:\n"
                          "iretd\n"

                          ".att_syntax");
}
