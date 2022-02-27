/*
 * syscalls.h: Syscall handler/dispatcher and system call functions
 */
#pragma once

#include "print/print_types.h"
#include "interrupts/pic.h"
#include "memory/malloc.h"

#define MAX_SYSCALLS 5

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

// Sleep for a given number of milliseconds
// INPUTS:
//  EBX = # of milliseconds
void syscall_sleep(void)
{
    //__asm__ __volatile__ ("movl %%ebx, %0" : "=r"(*sleep_timer_ticks) );
    __asm__ __volatile__ ("mov %%ebx, %0" : "=r"(*sleep_timer_ticks) );

    // Wait ("Sleep") until # of ticks is 0
    while (*sleep_timer_ticks > 0) __asm__ __volatile__ ("sti;hlt;cli");
}

// Allocate uninitialized memory
// INPUT:
//   EBX = size in bytes to allocate
void syscall_malloc(void)
{
    uint32_t bytes = 0;

    __asm__ __volatile__ ("mov %%EBX, %0" : "=b"(bytes) );

    // First malloc() from the calling program?
    if (!malloc_list_head)
        malloc_init(bytes); // Yes, set up initial memory/linked list

    void *ptr = malloc_next_block(bytes);

    merge_free_blocks();    // Combine consecutive free blocks of memory

    // Return pointer to malloc-ed memory
    __asm__ __volatile__ ("mov %0, %%EAX" : : "r"(ptr) );
}

// Free allocated memory at a pointer
// INPUT:
//   EBX = pointer to malloc-ed bytes
void syscall_free(void)
{
    void *ptr = 0;

    __asm__ __volatile__ ("mov %%EBX, %0" : "=b"(ptr) );

    malloc_free(ptr);
}

// Syscall table
void *syscalls[MAX_SYSCALLS] = {
    syscall_test0,
    syscall_test1,
    syscall_sleep,
    syscall_malloc,
    syscall_free,
};

// Syscall dispatcher
// naked attribute means no function prologue/epilogue, and only allows inline asm
__attribute__ ((naked)) __attribute__ ((interrupt)) void syscall_dispatcher(int_frame_32_t *frame)
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

                          ".equ MAX_SYSCALLS, 5\n"  // Have to define again, inline asm does not see the #define

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
                          "mov eax, -1\n"   // Error will be -1
                          "iretd\n"

                          ".att_syntax");
}
