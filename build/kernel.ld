SECTIONS
{
    .text 0x50000 :
    {
        *(kernel_entry); 
        *(.text*);
    }
    .rodata :
    {
        *(.rodata*);
    }
    .bss :
    {
        *(.bss*);
    }
}
