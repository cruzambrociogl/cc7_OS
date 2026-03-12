.section .text
.syntax unified
.code 32
.globl reset_handler

reset_handler:
    @ Set up stack pointer
    ldr sp, =_stack_top

    @ Copy vector table (16 words = 64 bytes) to address 0x0
    @ QEMU VersatilePB expects vectors at 0x0
    mov r0, #0x0
    ldr r1, =vector_table
    mov r2, #16
copy_vectors:
    ldr r3, [r1], #4
    str r3, [r0], #4
    subs r2, r2, #1
    bne copy_vectors

    @ Jump to C main
    bl main
    b hang
