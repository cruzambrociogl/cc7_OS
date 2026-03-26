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
    ldr sp, =0x82012000         @ 8KB above OS base (0x82000000 + 0x10000 + 0x2000)

    @ ---- Set IRQ mode stack ----
    mov r0, #0xD2               @ IRQ mode, IRQ+FIQ disabled
    msr cpsr_c, r0
    ldr sp, =0x82014000         @ Separate 8KB IRQ stack

    @ ---- Back to SVC mode ----
    mov r0, #0xD3
    msr cpsr_c, r0

    @ ---- Set VBAR (Vector Base Address Register) ----
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0

    @ ---- Clear .bss section ----
    @ All uninitialized globals (like the PCB array) must start at zero.
    @ The linker exports __bss_start__ and __bss_end__ so we know the range.
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0
clear_bss:
    cmp r0, r1                  @ Are we done?
    strlt r2, [r0], #4          @ If not, store 0 and advance pointer by 4 bytes
    blt clear_bss               @ Loop until r0 >= r1
    dsb                         @ Data Synchronization Barrier — ensure all stores complete
    isb                         @ Instruction Synchronization Barrier — flush pipeline

    @ ---- Jump to C main ----
    bl main

    @ Should never return
    b .
