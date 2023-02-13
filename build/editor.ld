SECTIONS
{
    /* Link programs at fixed entry point */
    .text 0x400000 :
    {
        *(editor_entry); 
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
