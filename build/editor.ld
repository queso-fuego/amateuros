SECTIONS
{
    .text :
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
