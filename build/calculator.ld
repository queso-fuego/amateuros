SECTIONS
{
    /* Link programs at fixed entry point */
    .text 0x400000 :
    {
        *(calc_entry); 
        *(.text*);
    }
    .data :
    {
        *(.data*);
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
