ENTRY(kernel_main)

PHDRS {
    text   PT_LOAD FLAGS(0x5);    /* RX */
    data   PT_LOAD FLAGS(0x6);    /* RW */
    rodata PT_LOAD FLAGS(0x4);    /* R */
}

SECTIONS
{
    /* Higher half kernel - will run from 3GB+ addresses */
    .text 0xC0000000 : {
        KEEP(*(kernel_entry)); 
        *(.text*);
    } :text

    .data : { *(.data*); }          :data
    .rodata : { *(.rodata*); }      :rodata
    .bss : { *(.bss*); *(COMMON); } :data

    /DISCARD/ : {
        *(.note*);
        *(.comment*);
    }
}
