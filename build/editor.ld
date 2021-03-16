SECTIONS
{
    .text 0x10000 :
    {
        *(editor_entry); 
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
