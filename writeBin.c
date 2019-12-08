#include <stdio.h>

void main(void)
{
    int i;

    FILE *bootsect = fopen("bootsect.bin", "wb+");
    for (i = 0; i < 510; i++) {
        putc(0x00, bootsect);
    }
    putc(0x55, bootsect);
    putc(0xaa, bootsect);

    fclose(bootsect);
}
