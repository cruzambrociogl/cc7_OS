#ifndef PCB_H
#define PCB_H

#define NUM_PROCESSES 3
#define READY   0
#define RUNNING 1

typedef struct {
    int pid;
    int state;
    unsigned int regs[13];  // R0-R12
    unsigned int sp;        // Stack Pointer (SVC mode)
    unsigned int lr;        // Link Register (SVC mode)
    unsigned int pc;        // Program Counter (return address)
    unsigned int cpsr;      // Saved Program Status Register
} PCB;

extern PCB pcb[NUM_PROCESSES];
extern int current_process;

void pcb_init(void);

#endif
