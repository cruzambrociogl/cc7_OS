#include "timer.h"
#include "uart.h"

void timer_irq_handler(void) {
    timer_irq_clear();
    os_write("Tick\n");
}
