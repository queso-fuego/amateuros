//
// calculator.c: basic 4 function calculator (+,-,*,/), with negative numbers 
//               and nesting ()
//
//  Sample BNF grammar:
//	sum		:= product | sum '+' product | sum '-' product ;
//	product := term | product '*' term | product '/' term ;
//	term	:= '-' term | '(' sum ')' | number ;
//
#include "C/stdint.h"
#include "C/string.h" 
#include "C/stdio.h"
#include "C/stdbool.h"
#include "C/stdlib.h"
#include "sys/syscall_wrappers.h" 
#include "gfx/2d_gfx.h"
#include "keyboard/keyboard.h"

void parse_buffer(void);    // Function declarations
int32_t parse_sum(void);
int32_t parse_product(void);
int32_t parse_term(void);
void skip_blanks(void);
int32_t parse_number(void);
int8_t is_digit(int32_t *num);
int8_t match_char(uint8_t in_char);

// TODO: Change how ERRORs are handled and percolated back up through function calls
//   because getting a -1 result will print Syntax Error regardless - BUG
const int8_t ERROR = -1;

char buffer[80];
uint32_t scan;
int32_t parse_num = 0;
bool interactive = false;

int32_t main(int32_t argc, char *argv[]) {
    const char valid_input[] = "0123456789+-*/()" "\x20\x0D\x1B" "r";

    // Check if user passed in expressions to evaluate; if so, do not run interactively
    if (argc > 1) {
        interactive = false;    

        for (uint8_t i = 1; i < argc; i++) {
            // Set calc input buffer to expression in arg,
            // skip initial dbl quote, and stop before ending dbl quote (length - 2)
            memset(buffer, 0, sizeof buffer); // Initialize buffer

            memcpy(buffer, &argv[i][1], strlen(argv[i]) - 2);

            parse_buffer();
        }

        printf("\r\n"); // End newline to fix spacing 
        exit(0);   // Return to caller with SUCCESS
    }
    
    // Initial newline to not be at end of last user input from shell prompt
    printf("\r\n");

    interactive = true; 

    // Get line of input
    scan = 0;
    while (1) {
        printf("\033CSRON;");
        key_info_t key_info = get_key();

        // TODO: Use "strcspn" to check against valid_input string instead?
        uint8_t idx = 0;
        for (; idx < 20 && valid_input[idx] != key_info.key; idx++) 
            ;

        if (idx == 20) continue;    // Invalid input character

        // Enter key with some input or buffer is full
        if ((key_info.key == '\r' && scan > 0) || scan == 80) {    
            printf("\033CSROFF; "); // Print newline for result
            parse_buffer();
            scan = 0;

            // Clear buffer for next line of input
            memset(buffer, 0, sizeof buffer); 
            continue;
        }

        if (key_info.key == 0x1B || (key_info.ctrl && key_info.key == 'r'))    // Escape key or ctrl-R
           exit(0);  // Return to caller 

        buffer[scan] = key_info.key;
        scan++;

        // Print char to screen
        putchar(key_info.key);
    }
    exit(0);
}

void parse_buffer(void) {
    scan = 0;

	// Print error msg or result
    int32_t num = 0;
	if ((num = parse_sum()) == ERROR) {
        printf("\r\nSyntax Error");
    } else {
        printf("\r\n%d", num);
    }

    if (interactive) printf("\r\n");
}

int32_t parse_sum(void) {
    int32_t num, num2;

	if ((num = parse_product()) == ERROR) return ERROR;

	while (true) {
		skip_blanks();

		if (match_char('+')) {
            if ((num2 = parse_product()) == ERROR) return ERROR;
            num += num2;

        } else if (match_char('-')) {
            if ((num2 = parse_product()) == ERROR) return ERROR;
            num -= num2;

        } else return num;
    }

    return ERROR;   // Should be unreachable?
}

int32_t parse_product(void) {
    int32_t num, num2;

    if ((num = parse_term()) == ERROR) return ERROR;

	while (true) {
		skip_blanks();

		if (match_char('*')) {
            if ((num2 = parse_term()) == ERROR) return ERROR;
            num *= num2;

        } else if (match_char('/')) {
            if ((num2 = parse_term()) == ERROR) return ERROR;
            num /= num2;

        } else return num;
    }

    return ERROR;   // Should be unreachable?
}

int32_t parse_term(void) {
    int32_t num;

	skip_blanks();

    // New expression
	if (match_char('(')) {
        // Overall expression recursion
        if ((num = parse_sum()) == ERROR) return ERROR; 
        skip_blanks();
        if (!match_char(')')) return ERROR;  // Did not balance parantheses

    // Negative number
    } else if (match_char('-')) {
        num = -parse_term();    // Recursion for negative number

    } else if ((num = parse_number()) == ERROR) return ERROR;

    return num;
}

void skip_blanks(void) {
    while (match_char(' ')) ;
}

int32_t parse_number(void) {
    parse_num = 0;

	if (is_digit(&parse_num)) {
        while (is_digit(&parse_num)) ;
        return parse_num;
    } 
    return ERROR;
}

int8_t is_digit(int32_t *num) {
    if (buffer[scan] >= '0' && buffer[scan] <= '9') {
        *num = (*num * 10) + (buffer[scan] - '0');
        scan++;
        return true;

    } else return false;
}

int8_t match_char(uint8_t in_char) {
    if (in_char == buffer[scan]) {
        scan++;
        return true;

    } else return false;
}
