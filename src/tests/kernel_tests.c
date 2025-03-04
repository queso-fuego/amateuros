//
// kernel_tests.c
//
bool test_open_close(void); // Test functions...
bool test_seek(void);
bool test_write(void);
bool test_read(void);
bool test_malloc(void);

// Run test functions and present results
bool cmd_runtests(int32_t argc, char *argv[]) {
    (void)argc, (void)argv;

    typedef struct {
        char *name;
        bool (*function)(void);
    } test_function_t;

    test_function_t tests[] = {
        { "Open() & Close() Syscalls",        test_open_close },
        { "Seek() Syscall on New/Empty file", test_seek },
        { "Write() Syscall for New file",     test_write },
        { "Read() Syscall on file",           test_read },
        { "Malloc() & Free() tests",          test_malloc },
        // TODO: { "Seek() Syscall on file with data", test_seek },
    };

    const uint32_t fg = user_gfx_info->fg_color, bg = user_gfx_info->bg_color;
    const uint32_t num_tests = sizeof tests / sizeof tests[0];  
    uint32_t num_passed = 0;

    printf("\r\nRunning tests...\r\n----------------");

    for (uint32_t i = 0; i < num_tests; i++) {
        printf("\r\n%s: ", tests[i].name);
        if (tests[i].function()) {
            printf("\033FG%#xBG%#x;OK", GREEN, bg);
            num_passed++;
        } else {
            printf("\033FG%#xBG%#x;FAIL", RED, bg);
        }

        // Reset default colors
        printf("\033FG%#xBG%#x;", fg, bg);
    }

    printf("\r\nTests passed: %d/%d\r\n", num_passed, num_tests);
    return true;
}

// Test open() & close() syscalls
bool test_open_close(void) {
    char *file = "openclosetest.txt";
    int32_t fd = open(file, O_CREAT);
    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    // Test close() syscall
    if (close(fd) != 0) {
        printf("Error: could not close file %s\r\n", file);
        return false;
    }
    return true;
}

// Test seek() syscall
bool test_seek(void) {
    // TODO: Add more to this after read() and write() are filled out,
    //   to be able to actually write data to a file

    char *file = "seektest.txt";
    int32_t fd = open(file, O_CREAT);
    int32_t seek_val = 0;

    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    // Test seeking on a new/empty file
    if (0 != seek(fd, 0, SEEK_SET)) {
        printf("\r\nError: could not SEEK_SET to 0 for %s\r\n", file);
        return false;
    }

    if (100 != seek(fd, 100, SEEK_SET)) {
        printf("\r\nError: could not SEEK_SET to 100 for %s\r\n", file);
        return false;
    }

    if (-1 != seek(fd, -100, SEEK_SET)) {
        printf("\r\nError: could not SEEK_SET to -100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, 0, SEEK_CUR)) {
        printf("Error: could not SEEK_CUR to 0 for %s\r\n", file);
        printf("seek result: %d\r\n", seek_val);
        return false;
    }

    if (100 != seek(fd, 100, SEEK_CUR)) {
        printf("\r\nError: could not SEEK_CUR to 100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, -100, SEEK_CUR)) {
        printf("\r\nError: could not SEEK_CUR to -100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, 0, SEEK_END)) {
        printf("\r\nError: could not SEEK_END to 0 for %s\r\n", file);
        return false;
    }

    if (100 != seek(fd, 100, SEEK_END)) {
        printf("\r\nError: could not SEEK_END to 100 for %s\r\n", file);
        return false;
    }

    if (0 != seek(fd, -100, SEEK_END)) {
        printf("\r\nError: could not SEEK_END to -100 for %s\r\n", file);
        return false;
    }

    close(fd);
    return true;
}

// Test write() syscall
bool test_write(void) {
    char *file = "writetest.txt";
    int32_t fd = open(file, O_CREAT | O_WRONLY); 

    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    char buf[] = "Hello, World!";

    if (14 != write(fd, buf, sizeof buf)) {
        printf("\r\nError: could not write \"%s\" to file %s\r\n", 
               buf, file);
        return false;
    }

    // TODO: Test O_APPEND

    close(fd);
    return true;
}

// Test read() syscall
bool test_read(void) {
    char *file = "readtest.txt";
    int32_t fd = open(file, O_CREAT | O_RDWR);

    if (fd < 0) {
        printf("\r\nError: could not create file %s\r\n", file);
        return false;
    }

    char str_buf[] = "Hello, World!";

    if (14 != write(fd, str_buf, sizeof str_buf)) {
        printf("\r\nError: could not write \"%s\" to file %s\r\n", 
               str_buf, file);
        return false;
    }

    char read_buf[14];

    // read() from end of file
    if (read(fd, read_buf, sizeof read_buf) > 0) {
        printf("\r\nError: Read from end of file %s should return 0\r\n", file);
        return false;
    }

    // Move back to start of file
    seek(fd, 0, SEEK_SET);

    // read() from start of file
    if (read(fd, read_buf, sizeof read_buf) < 1) {
        printf("\r\nError: could not read from file %s into buffer\r\n", file);
        return false;
    }

    if (strncmp(read_buf, "Hello, World!", strlen(read_buf)) != 0) {
        printf("\r\nError: could not read file %s into buffer correctly\r\n", file);
        return false;
    }

    close(fd);

    return true;
}

// Test malloc() & free() function calls
bool test_malloc(void) {
    void *buf;
    void *buf2;
    void *buf3;
    void *buf4;
    void *buf5;

    buf = malloc(100);
    printf("\r\nMalloc-ed 100 bytes to address %x\r\n", (uint32_t)buf);
    printf("Free-ing bytes...\r\n");
    free(buf);

    buf2 = malloc(42);
    printf("Malloc-ed 42 bytes to address %x\r\n", (uint32_t)buf2);
    printf("Free-ing bytes...\r\n");
    free(buf2);

    buf3 = malloc(250);
    printf("Malloc-ed 250 bytes to address %x\r\n", (uint32_t)buf3);

    buf4 = malloc(6000);
    printf("Malloc-ed 6000 bytes to address %x\r\n", (uint32_t)buf4);

    buf5 = malloc(333);
    printf("Malloc-ed 333 bytes to address %x\r\n", (uint32_t)buf5);

    free(buf4);
    free(buf5);
    free(buf3);

    return true;
}

