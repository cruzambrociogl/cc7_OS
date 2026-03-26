.section .text
.syntax unified
.code 32
.globl reset_handler

.extern main
.extern vector_table
.extern __bss_start__
.extern __bss_end__

reset_handler:
    @ ---- Set SVC mode, IRQ + FIQ disabled ----
    mov r0, #0xD3
    msr cpsr_c, r0

    @ ---- Set SVC mode stack (OS stack) ----
    ldr sp, =0x00022000

    @ ---- Set IRQ mode stack ----
    mov r0, #0xD2               @ IRQ mode, IRQ+FIQ disabled
    msr cpsr_c, r0
    ldr sp, =0x00024000         @ Separate 8KB IRQ stack

    @ ---- Back to SVC mode ----
    mov r0, #0xD3
    msr cpsr_c, r0

    @ ---- Copy vector table to 0x0 ----
    @ QEMU VersatilePB expects exception vectors at address 0x0
    mov r0, #0x0
    ldr r1, =vector_table
    mov r2, #16                 @ 16 words = 64 bytes (8 vectors + 8 addresses)
copy_vectors:
    ldr r3, [r1], #4
    str r3, [r0], #4
    subs r2, r2, #1
    bne copy_vectors

    @ ---- Clear .bss section ----
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0
clear_bss:
    cmp r0, r1
    strlt r2, [r0], #4
    blt clear_bss
    mcr p15, 0, r0, c7, c10, 4  @ DSB equivalent for ARMv5
    mcr p15, 0, r0, c7, c5, 4   @ ISB equivalent for ARMv5

    @ ---- Jump to C main ----
    bl main

    @ Should never return
    b .
