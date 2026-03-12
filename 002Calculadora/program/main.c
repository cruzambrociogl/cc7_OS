#include "../lib/stdio.h"

void main(void) {
    int a100, b100, sum100;

    PRINT("Program: Add Two Numbers\n");
    PRINT("(You can type decimals like 20.25)\n\n");

    while (1) {
        PRINT("Enter first number: ");
        READ("%f", &a100);

        PRINT("Enter second number: ");
        READ("%f", &b100);

        sum100 = a100 + b100;

        PRINT("Sum: %f\n\n", sum100);
    }
}
