ENTRY(prekernel_main)

PHDRS {
    text   PT_LOAD FLAGS(0x5);    /* RX */
    data   PT_LOAD FLAGS(0x6);    /* RW */
    rodata PT_LOAD FLAGS(0x4);    /* R */
}

SECTIONS
{
    .text 0x50000 : {
        KEEP(*(prekernel_entry)); 
        *(.text*);
    }

    .data : { *(.data*); }          :data
    .rodata : { *(.rodata*); }      :rodata
    .bss : { *(.bss*); *(COMMON); } :data

    /DISCARD/ : {
        *(.note*);
        *(.comment*);
    }
}
