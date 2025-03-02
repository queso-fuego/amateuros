// =========================================================================== 
// process.h: processes/threads
// ===========================================================================
#pragma once

#include "C/stdint.h"
#include "C/string.h"
#include "memory/virtual_memory_manager.h"
#include "fs/fs.h"
#include "elf/elf.h"
#include "interrupts/idt.h"
#include "global/global_addresses.h"
#include "sys/regs.h"

typedef enum {
    INVALID,
    SLEEPING,
    ACTIVE,
} Proc_State;

typedef struct Process Process; 
typedef struct Thread Thread;

typedef struct Thread {
    Process        *parent;
    void           *stack;
    void           *stack_limit;
    void           *kernel_stack;
    uint32_t       priority;
    Proc_State     state;
    uint32_t       pgm_buf;     // Base address of loaded program (entry point is separate)
    uint32_t       pgm_size;    // Size of loaded program
    syscall_regs_t regs;
} Thread;

typedef struct Process {
    uint32_t       id;
    uint32_t       priority;
    page_directory *page_dir;
    Proc_State     state;
    Thread         threads[5];  // TODO: Change for runtime multitasking/using dynamic memory
    uint32_t       thread_count;
} Process;

static Process _proc = {0};

extern open_file_table_t *open_file_table;

Process *get_current_process(void) {
    return &_proc;
}

uint32_t create_process(int32_t argc, char **argv) {
    // Open file
    int32_t fd = open(argv[0], O_RDWR);
    if (fd < 0) return 0;

    // Get virtual address space
    //page_directory *address_space = create_address_space();
    page_directory *address_space = get_page_directory();
    if (!address_space) { close(fd); return 0; }

    // map_kernel_space(address_space); // If creating new address space

    // Create process 
    Process *proc = get_current_process();
    proc->id           = 1;
    proc->page_dir     = address_space;
    proc->priority     = 1;
    proc->state        = ACTIVE;
    proc->thread_count = 1;

    // Create thread
    Thread *main_thread = &proc->threads[0];
    main_thread->kernel_stack = NULL;
    main_thread->parent       = proc;
    main_thread->priority     = 1;
    main_thread->state        = ACTIVE;
    main_thread->stack        = NULL;
    main_thread->stack_limit  = (void *)((uint32_t)main_thread->stack + PAGE_SIZE);

    // Load program into memory; The open file FD addresses and Program Buffer
    //   addresses are virtual and mapped to valid physical memory from malloc(),
    //   syscall_open(), etc.
    void *entry_point = NULL; 
    uint8_t *pgm_buf = NULL;
    uint32_t pgm_size = 0;
    entry_point = load_elf_file(open_file_table[fd].address, (void **)&pgm_buf, &pgm_size); 

    main_thread->pgm_buf  = (uint32_t)pgm_buf;
    main_thread->pgm_size = pgm_size;

    memset(&main_thread->regs, 0, sizeof main_thread->regs);
    main_thread->regs.eip    = (int32_t)entry_point;
    main_thread->regs.eflags = 0x200;

    // Create userspace stack
    void *stack = (void *)((uint8_t *)main_thread->pgm_buf + main_thread->pgm_size + PAGE_SIZE);
    void *stack_phys = allocate_blocks(1);  // Only 4KB for now

    // Map user process stack
    map_address(address_space, (uint32_t)stack_phys, (uint32_t)stack, 
                PTE_PRESENT | PTE_READ_WRITE | PTE_USER);

    // Create userspace storage for arguments, above stack memory
    void *args = (void *)((uint8_t *)stack + PAGE_SIZE);
    void *args_phys = allocate_blocks(1); 

    // Map argument memory
    map_address(address_space, (uint32_t)args_phys, (uint32_t)args, 
                PTE_PRESENT | PTE_READ_WRITE | PTE_USER);

    // Copy argv into args memory
    char **user_argv = args;
    char *argp = (char *)(user_argv+10);    // Assuming max of 10 argv values, start after pointers
    for (int32_t i = 0; i < argc; i++) {
        user_argv[i] = argp;            // Next pointer to argv string
        argp = stpcpy(argp, argv[i])+1; // argv string
    }

    // Set thread stack to created user space stack
    main_thread->stack = (uint8_t *)stack-8;    // Make room for argc/argv

    // Add argc/argv inputs to stack
    *(int32_t *)((uint8_t *)main_thread->stack + 4) = argc;
    *(uint32_t *)((uint8_t *)main_thread->stack + 8) = (uint32_t)user_argv;

    main_thread->regs.esp = (uint32_t)main_thread->stack;
    main_thread->regs.ebp = main_thread->regs.esp;

    // Close file & return new process id
    close(fd);
    return proc->id;
}

void execute_process(void) {
    // Get running process
    Process *proc = get_current_process();
    if (!proc->id || !proc->page_dir) return; 

    // Get EIP/ESP of main thread
    int32_t entry_point = proc->threads[0].regs.eip;
    uint32_t proc_stack = proc->threads[0].regs.esp;

    // Switch to process address space
    set_page_directory(proc->page_dir);

    // Update TSS esp0 with current kernel stack pointer
    // !! NOTE: If you don't do this, stack can be garbage or unable to push correct
    //   values from user interrupts/exceptions e.g. syscalls from int 0x80!!
    int32_t stack = 0;
    __asm__ __volatile__ ("mov %%esp, %0" : "=a"(stack)); 

    // Pointer to 32 bit address
    uint8_t *gdt = (uint8_t *)*(uint32_t *)GDT_ADDRESS;     

    // Pointer to tss address in base 0 of TSS descriptor at offset 0x28 in GDT
    //  Base 0 of descriptor is 2 bytes after start of 0x28 offset, is a 2 byte value, and 
    //  holds the address of the TSS from src/2ndstage.asm
    uint32_t tss_addr = *(uint16_t *)(gdt + 0x2A);
    uint32_t *tss = (uint32_t *)tss_addr;

    // 4 bytes into the TSS is the esp0 value that the kernel gets from user mode interrupts
    *(tss + 1) = stack;

    // Execute process in user mode
    __asm__ __volatile__ ("cli\n"
                          "mov $0x23, %%eax\n"          // User mode data selector (0x20) ORed with priv lvl 3/user
                          "mov %%ax, %%ds\n"
                          "mov %%ax, %%es\n"
                          "mov %%ax, %%fs\n"
                          "mov %%ax, %%gs\n"

                          // Create stack frame
                          "pushl %%eax\n"                // Stack Segment/SS, same value as above segments
                          "movl %[proc_stack], %%eax\n"  // Stack/ESP
                          "pushl %%eax\n"            
                          "movl $0x200, %%eax\n"
                          "pushl %%eax\n"                // EFLAGS
                          "movl $0x1B, %%eax\n"          // User mode CS (0x18) ORed with priv lvl 3/user
                          "pushl %%eax\n"                // CS
                          "movl %[entry_point], %%eax\n" // EIP
                          "pushl %%eax\n"
                          "iretl\n"                      // Iret will pop & use last 5 values from stack
                          :
                          : [proc_stack]"m"(proc_stack), [entry_point]"m"(entry_point)
                          : "memory");
}

