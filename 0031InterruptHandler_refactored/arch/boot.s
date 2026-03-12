.section .text
.syntax unified
.code 32
.globl _start
.globl vector_table
.globl hang

// ============================================================================
// Exception Vector Table
// Must be aligned to 32 bytes (0x20)
// ============================================================================
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

_reset_h:    .word reset_handler     @ defined in startup_<target>.s
_undef_h:    .word undefined_handler
_swi_h:      .word swi_handler
_prefetch_h: .word prefetch_handler
_data_h:     .word data_handler
_unused_h:   .word hang
_irq_h:      .word irq_handler
_fiq_h:      .word fiq_handler

// ============================================================================
// Exception Handlers
// ============================================================================

undefined_handler:
    b hang

swi_handler:
    b hang

prefetch_handler:
    b hang

data_handler:
    b hang

irq_handler:
    stmfd sp!, {r0-r12, lr}   @ Save all registers and Link Register
    bl timer_irq_handler       @ Call the C interrupt handler
    ldmfd sp!, {r0-r12, lr}   @ Restore registers
    subs pc, lr, #4            @ Return from interrupt

fiq_handler:
    b hang

hang:
    b hang

// ============================================================================
// Low-level memory access functions
// ============================================================================
.globl PUT32
PUT32:
    str r1, [r0]
    bx lr

.globl GET32
GET32:
    ldr r0, [r0]
    bx lr

// ============================================================================
// Interrupt control
// ============================================================================
.globl enable_irq
enable_irq:
    mrs r0, cpsr        @ Read Current Program Status Register
    bic r0, r0, #0x80   @ Clear bit 7 (I-bit) to enable IRQ
    msr cpsr_c, r0       @ Write back to CPSR
    bx lr

// ============================================================================
// Stack space allocation
// ============================================================================
.section .bss
.align 4
_stack_bottom:
    .skip 0x2000  @ 8KB stack space
_stack_top:
