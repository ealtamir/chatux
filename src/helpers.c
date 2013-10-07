#include "../lib/helpers.h"
#include <stdio.h>
#include <fcntl.h>

#define A_MAY 0x41 // Capital
#define Z_MAY 0x5A
#define A_MIN 0x61 // Small
#define Z_MIN 0x7A

void rot13(const char* source, char* dest, int len ) {
    int i = 0;
    int c = 0;

    while (i < len && source[i] != '\0') {
        c = source[i] + 13;

        if (source[i] >= A_MAY && source[i] <= Z_MAY) {
            if (c > Z_MAY)
                dest[i] = c - (Z_MAY - A_MAY) + 1;
            else
                dest[i] = c;
        }
        else {
            if (c > Z_MIN)
                dest[i] = c - (Z_MIN - A_MIN) + 1;
            else
                dest[i] = c;
        }

        i++;
    }
    dest[i] = '\0';
}

int getUserInput(char* buffer, int buffer_size) {

    char c = 0;
    int index = 0;

    printf("Client => ");

    while( (c = getchar()) != '\n' && index < buffer_size - 1) {
        buffer[index] = c;
        index++;
    }
    buffer[index++] = '\0';

    return index;
}

void setBlocking(int fd) {

    int flags = 0;

    flags = fcntl(fd, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

}
