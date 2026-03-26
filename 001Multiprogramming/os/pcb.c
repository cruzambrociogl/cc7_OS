#include "pcb.h"
#include "board.h"

PCB pcb[NUM_PROCESSES];
int current_process = 0;

void pcb_init(void) {
    // OS (pid 0) — not scheduled, only used during boot
    pcb[0].pid = 0;
    pcb[0].state = RUNNING;

    // P1 (pid 1)
    pcb[1].pid = 1;
    pcb[1].state = READY;
    pcb[1].pc = P1_ENTRY;
    pcb[1].sp = P1_STACK;
    pcb[1].lr = P1_ENTRY;
    pcb[1].cpsr = 0x13;         // SVC mode, IRQs enabled
    for (int i = 0; i < 13; i++)
        pcb[1].regs[i] = 0;

    // P2 (pid 2)
    pcb[2].pid = 2;
    pcb[2].state = READY;
    pcb[2].pc = P2_ENTRY;
    pcb[2].sp = P2_STACK;
    pcb[2].lr = P2_ENTRY;
    pcb[2].cpsr = 0x13;
    for (int i = 0; i < 13; i++)
        pcb[2].regs[i] = 0;
}
