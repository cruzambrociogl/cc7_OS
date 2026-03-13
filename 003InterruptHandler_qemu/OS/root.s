.section .text
.syntax unified
.code 32
.globl _start

// Exception Vector Table
// Must be aligned to 32 bytes (0x20)
.align 5
_start:
vector_table:
    ldr pc, _reset_h
    ldr pc, _undef_h
    ldr pc, _swi_h
    ldr pc, _prefetch_h
    ldr pc, _data_h
    ldr pc, _unused_h
    ldr pc, _irq_h
    ldr pc, _fiq_h

_reset_h:    .word reset_handler
_undef_h:    .word undefined_handler
_swi_h:      .word swi_handler
_prefetch_h: .word prefetch_handler
_data_h:     .word data_handler
_unused_h:   .word hang
_irq_h:      .word irq_handler
_fiq_h:      .word fiq_handler

reset_handler:
    // Set up stack pointer
    ldr sp, =_stack_top

#ifdef QEMU
    // Copy vector table (16 words = 64 bytes) to 0x0
    mov r0, #0x0
    ldr r1, =vector_table
    mov r2, #16
copy_vectors:
    ldr r3, [r1], #4
    str r3, [r0], #4
    subs r2, r2, #1
    bne copy_vectors
#else
    // Set up exception vector table base address (VBAR - Vector Base Address Register)
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0
#endif

    // Call main function
    bl main

    // If main returns, loop forever
hang:
    b hang

undefined_handler:
    b hang

swi_handler:
    b hang

prefetch_handler:
    b hang

data_handler:
    b hang

// TODO: Implement IRQ handler
// This handler should:
// 1. Save all registers (r0-r12, lr)
// 2. Call the timer_irq_handler() function in C
// 3. Restore all registers
// 4. Return from interrupt using: subs pc, lr, #4
irq_handler:
    stmfd sp!, {r0-r12, lr}   @ 1. Save all registers and Link Register
    bl timer_irq_handler      @ 2. Call the C interrupt handler
    ldmfd sp!, {r0-r12, lr}   @ 3. Restore registers
    subs pc, lr, #4           @ 4. Return from interrupt

fiq_handler:
    b hang

// Low-level memory access functions
.globl PUT32
PUT32:
    str r1, [r0]
    bx lr

.globl GET32
GET32:
    ldr r0, [r0]
    bx lr

// TODO: Implement enable_irq function
// This function should:
// 1. Read the current CPSR
// 2. Clear the I-bit (bit 7) to enable IRQ interrupts
// 3. Write back to CPSR
.globl enable_irq
enable_irq:
    mrs r0, cpsr        @ 1. Read Current Program Status Register
    bic r0, r0, #0x80   @ 2. Clear bit 7 (I-bit) to enable IRQ
    msr cpsr_c, r0      @ 3. Write back to CPSR
    bx lr

// Stack space allocation
.section .bss
.align 4
_stack_bottom:
    .skip 0x2000  @ 8KB stack space
_stack_top:
