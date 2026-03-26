#include "../lib/stdio.h"

void main(void) {
    int n = 0;
    while (1) {
        PRINT("----From P1: %d\n", n);
        n = (n + 1) % 10;
        for (volatile int i = 0; i < 200000; i++);
    }
}
