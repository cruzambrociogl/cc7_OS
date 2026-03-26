#include "../lib/stdio.h"

void main(void) {
    char c = 'a';
    while (1) {
        PRINT("----From P2: %c\n", c);
        c++;
        if (c > 'z') c = 'a';
        for (volatile int i = 0; i < 200000; i++);
    }
}
