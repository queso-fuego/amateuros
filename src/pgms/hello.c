#include "C/stdint.h"
#include "C/stdio.h"
#include "C/stdlib.h"

int32_t main(int32_t argc, char *argv[]) {
    char *test = "Hello, Userspace/Ring 3 World!";
    printf("\r\n%s\r\n", test);

    for (int32_t i = 0; i < argc; i++)
        printf("argc: %d, argv: %s\r\n", i, argv[i]);

    exit(42);
}
