#include "../lib/helpers.h"
#include <stdio.h>

#define A_MAY 0x41 // Capital
#define Z_MAY 0x5A
#define A_MIN 0x61 // Small
#define Z_MIN 0x7A

void rot13(const char* source, char* dest, int len ) {
    int i = 0;
    char c = 0;

    while (i < len && source[i] != '\0') {
        c = source[i];
        dest[i] = source[i] + 13;

        if (c >= A_MAY && c <= Z_MAY) {
            if (dest[i] > Z_MAY)
                dest[i] -= Z_MAY;
        }
        else {
            if (dest[i] > Z_MIN)
                dest[i] -= Z_MIN;
        }

        i++;
    }
}

int getUserInput(char* buffer, int buffer_size) {

    char c = 0;
    int index = 0;

    while( (c = getchar()) != '\n' && index < buffer_size - 1) {
        buffer[index] = c;
        index++;
    }
    buffer[index++] = '\0';

    return index;
}
