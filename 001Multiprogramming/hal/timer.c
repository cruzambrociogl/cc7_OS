#include "timer.h"
#include "../os/pcb.h"

// Global buffers shared with boot.s (IRQ handler)
extern unsigned int saved_regs[13];
extern unsigned int saved_lr;
extern unsigned int saved_svc_sp;
extern unsigned int saved_svc_lr;

void timer_irq_handler(void) {
    // 1. Clear the timer interrupt
    timer_irq_clear();

    // 2. Save current process context from global buffers into its PCB
    for (int i = 0; i < 13; i++)
        pcb[current_process].regs[i] = saved_regs[i];
    pcb[current_process].pc = saved_lr;
    pcb[current_process].sp = saved_svc_sp;
    pcb[current_process].lr = saved_svc_lr;
    pcb[current_process].state = READY;

    // 3. Round-robin: switch to the other process
    if (current_process == 1)
        current_process = 2;
    else
        current_process = 1;

    // 4. Load next process context from its PCB into global buffers
    for (int i = 0; i < 13; i++)
        saved_regs[i] = pcb[current_process].regs[i];
    saved_lr = pcb[current_process].pc;
    saved_svc_sp = pcb[current_process].sp;
    saved_svc_lr = pcb[current_process].lr;
    pcb[current_process].state = RUNNING;
}
