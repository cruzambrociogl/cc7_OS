.section .text
.syntax unified
.code 32
.globl reset_handler

reset_handler:
    @ Set up stack pointer
    ldr sp, =_stack_top

    @ Set VBAR (Vector Base Address Register) to our vector table
    @ Cortex-A8 uses CP15 c12 for this
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0

    @ Jump to C main
    bl main
    b hang
