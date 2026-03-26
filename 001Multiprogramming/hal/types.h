#ifndef TYPES_H
#define TYPES_H

// Low-level memory access functions (implemented in arch/boot.s)
void PUT32(unsigned int addr, unsigned int value);
unsigned int GET32(unsigned int addr);

// Interrupt control (implemented in arch/boot.s)
void enable_irq(void);
void disable_irq(void);

#endif
