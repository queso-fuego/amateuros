SECTIONS
{
    .text 0x2000 :
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
