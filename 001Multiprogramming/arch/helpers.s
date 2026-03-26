.section .text
.syntax unified
.code 32

// Minimal PUT32/GET32 for user processes (P1, P2)
// These are needed by uart_hw.c

.globl PUT32
PUT32:
    str r1, [r0]
    bx lr

.globl GET32
GET32:
    ldr r0, [r0]
    bx lr
