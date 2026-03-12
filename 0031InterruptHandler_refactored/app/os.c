#include "../hal/uart.h"
#include "../hal/timer.h"
#include "../hal/interrupt.h"
#include "../lib/stdio.h"

// Simple random number generator (Linear Congruential Generator)
static unsigned int seed = 12345;

unsigned int rand(void) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

int main(void) {
    os_write("Initializing BeagleBone Black Timer Lab...\n");

    timer_init();
    enable_irq();

    os_write("Interrupts enabled. Starting main loop.\n");

    while (1) {
        unsigned int random_num = rand() % 1000;
        uart_putnum(random_num);

        // Small delay to prevent overwhelming UART
        for (volatile int i = 0; i < 1000000; i++);
    }

    return 0;
}
