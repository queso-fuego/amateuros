SECTIONS
{
    .text 0x10000 :
    {
        *(calc_entry); 
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
