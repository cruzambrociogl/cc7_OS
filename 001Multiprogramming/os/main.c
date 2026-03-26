#include "../hal/uart.h"
#include "../hal/timer.h"
#include "../hal/types.h"
#include "pcb.h"

extern void start_first_process(PCB *p);

void main(void) {
    timer_init();
    os_write("OS: Booting...\r\n");

    pcb_init();
    os_write("OS: PCBs OK\r\n");

    enable_irq();
    os_write("OS: IRQs enabled\r\n");

    os_write("OS: Starting P1...\r\n");
    current_process = 1;
    pcb[1].state = RUNNING;
    start_first_process(&pcb[1]);

    while (1);
}
