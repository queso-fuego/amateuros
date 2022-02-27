/*
 * malloctst.c: Basic test for malloc()/free() C functions/syscalls
 */
#include "C/stdint.h"
#include "C/stdlib.h"
#include "print/print_types.h"
#include "screen/clear_screen.h"
#include "keyboard/keyboard.h"

__attribute__ ((section ("test_entry"))) void test_main(void)
{
    uint16_t x = 0, y = 5;

    static void *buf;
    static void *buf2;
    static void *buf3;
    static void *buf4;
    static void *buf5;

    clear_screen(user_gfx_info->bg_color);

    print_string(&x, &y, "Malloc test:\r\n------------\r\n");

    buf = malloc(100);

    print_string(&x, &y, "Malloc-ed ");
    print_dec(&x, &y, 100);
    print_string(&x, &y, " bytes to address: ");
    print_hex(&x, &y, (uint32_t)buf);

    print_string(&x, &y, "\r\nFree-ing bytes...\r\n");
    free(buf);

    buf2 = malloc(42);

    print_string(&x, &y, "Malloc-ed ");
    print_dec(&x, &y, 42);
    print_string(&x, &y, " bytes to address: ");
    print_hex(&x, &y, (uint32_t)buf2);

    print_string(&x, &y, "\r\nFree-ing bytes...\r\n");
    free(buf2);

    print_string(&x, &y, "Multiple malloc/free tests:\r\n");

    buf3 = malloc(250);
    print_string(&x, &y, "250 bytes to address: ");
    print_hex(&x, &y, (uint32_t)buf3);

    buf4 = malloc(6000);
    print_string(&x, &y, "\r\n6000 bytes to address: ");
    print_hex(&x, &y, (uint32_t)buf4);

    buf5 = malloc(333);
    print_string(&x, &y, "\r\n333 bytes to address: ");
    print_hex(&x, &y, (uint32_t)buf5);

    free(buf4);
    free(buf5);
    free(buf3);

    // Get a key before returning
    print_string(&x, &y, "\r\nTest successful, press any key to return...");
    get_key();
}
