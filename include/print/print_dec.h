/*
 * print_dec.h: Print a base 10 integer as a string of characters (maybe an itoa or itostr?)
 */
#pragma once

void print_dec(uint16_t *cursor_x, uint16_t *cursor_y, int32_t number)
{
    uint8_t dec_string[80];
    uint8_t i = 0, j, temp;
    uint8_t negative = 0;       // Is number negative?

    if (number == 0) dec_string[i++] = '0'; // If passed in 0, print a 0
    else if (number < 0)  {
        negative = 1;       // Number is negative
        number = -number;   // Easier to work with positive values
    }

    while (number > 0) {
        dec_string[i] = (number % 10) + '0';    // Store next digit as ascii
        number /= 10;                           // Remove last digit
        i++;
    }

    if (negative) dec_string[i++] = '-';    // Add negative sign to front

    dec_string[i] = '\0';   // Null terminate

    // Number stored backwards in dec_string, reverse the string by swapping each end
    //   until they meet in the middle
    i--;    // Skip the null byte
    for (j = 0; j < i; j++, i--) {
        temp          = dec_string[j];
        dec_string[j] = dec_string[i];
        dec_string[i] = temp;
    }

    // Print out result
    print_string(cursor_x, cursor_y, dec_string);
}
