.section .text
.syntax unified
.code 32
.globl _start
.globl vector_table
.globl hang
.globl PUT32
.globl GET32
.globl enable_irq
.globl disable_irq
.globl start_first_process

.extern timer_irq_handler
.extern saved_regs
.extern saved_lr
.extern saved_svc_sp
.extern saved_svc_lr

// ============================================================================
// Exception Vector Table
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

_reset_h:    .word reset_handler
_undef_h:    .word hang
_swi_h:      .word hang
_prefetch_h: .word hang
_data_h:     .word hang
_unused_h:   .word hang
_irq_h:      .word irq_handler
_fiq_h:      .word hang

// ============================================================================
// IRQ Handler — Context Switch
//
// When a timer IRQ fires, the CPU automatically:
//   - Copies CPSR → SPSR_irq
//   - Sets CPSR to IRQ mode (banked sp_irq, lr_irq)
//   - Sets lr_irq = PC of interrupted instruction + 4
//   - Jumps here
//
// We need to:
//   1. Save the interrupted process's full context into global buffers
//   2. Call the C handler (clears IRQ + does round-robin swap in PCB)
//   3. Restore the next process's context from the global buffers
//   4. Return to the next process
// ============================================================================
irq_handler:
    // ---- Step 1: Save R0-R12 to global buffer ----
    // We need R0 as a pointer, so first push it to IRQ stack temporarily
    sub lr, lr, #4              // Adjust return address (standard ARM IRQ fix)
    stmfd sp!, {r0-r1}         // Temp save R0-R1 on IRQ stack

    ldr r0, =saved_regs
    // Save R2-R12 directly (R0-R1 are on IRQ stack, we'll get them after)
    add r1, r0, #8             // &saved_regs[2]
    stmia r1, {r2-r12}         // Save R2-R12

    // Now recover original R0-R1 from IRQ stack and save them
    ldmfd sp!, {r1-r2}         // Pop original R0→r1, original R1→r2
    str r1, [r0, #0]           // saved_regs[0] = original R0
    str r2, [r0, #4]           // saved_regs[1] = original R1

    // ---- Step 2: Save return address (PC of interrupted process) ----
    ldr r0, =saved_lr
    str lr, [r0]               // saved_lr = lr_irq - 4 (adjusted above)

    // ---- Step 3: Save SVC mode SP and LR ----
    // The interrupted process was running in SVC mode.
    // SP and LR are banked per mode, so we must switch to SVC to read them.
    mrs r4, cpsr               // Save current CPSR (IRQ mode)

    // Switch to SVC mode, IRQs disabled
    bic r5, r4, #0x1F
    orr r5, r5, #0x93          // SVC mode (0x13) + IRQ disabled (0x80)
    msr cpsr_c, r5

    // Now in SVC mode — read its banked SP and LR
    mov r6, sp                 // SVC SP
    mov r7, lr                 // SVC LR

    // Switch back to IRQ mode
    msr cpsr_c, r4

    // Store SVC SP and LR to global buffers
    ldr r0, =saved_svc_sp
    str r6, [r0]
    ldr r0, =saved_svc_lr
    str r7, [r0]

    // ---- Step 4: Call C handler ----
    // timer_irq_handler() will:
    //   - Clear the timer IRQ
    //   - Save globals → pcb[current]
    //   - Round-robin: switch current_process
    //   - Load pcb[next] → globals
    bl timer_irq_handler

    // ---- Step 5: Restore next process's SVC SP and LR ----
    ldr r0, =saved_svc_sp
    ldr r6, [r0]
    ldr r0, =saved_svc_lr
    ldr r7, [r0]

    mrs r4, cpsr               // Save CPSR (IRQ mode)
    bic r5, r4, #0x1F
    orr r5, r5, #0x93
    msr cpsr_c, r5             // Switch to SVC mode

    mov sp, r6                 // Restore SVC SP
    mov lr, r7                 // Restore SVC LR

    msr cpsr_c, r4             // Back to IRQ mode

    // ---- Step 6: Restore return address ----
    ldr r0, =saved_lr
    ldr lr, [r0]               // lr_irq = next process's PC

    // ---- Step 7: Restore R0-R12 ----
    ldr r0, =saved_regs
    // Restore R1-R12 first, then R0 last (since we're using R0 as pointer)
    ldr r1, [r0, #4]           // R1
    ldr r2, [r0, #8]           // R2
    ldr r3, [r0, #12]          // R3
    ldr r4, [r0, #16]          // R4
    ldr r5, [r0, #20]          // R5
    ldr r6, [r0, #24]          // R6
    ldr r7, [r0, #28]          // R7
    ldr r8, [r0, #32]          // R8
    ldr r9, [r0, #36]          // R9
    ldr r10, [r0, #40]         // R10
    ldr r11, [r0, #44]         // R11
    ldr r12, [r0, #48]         // R12
    ldr r0, [r0, #0]           // R0 (last, since we were using it as base)

    // ---- Step 8: Return to next process ----
    // movs pc, lr copies SPSR_irq → CPSR (restores mode + IRQ flag)
    // and jumps to lr (the next process's PC)
    movs pc, lr

// ============================================================================
// start_first_process
// Called once from OS main() to jump into P1 for the first time.
// R0 = pointer to PCB of the first process
// ============================================================================
start_first_process:
    // PCB layout: pid(4) + state(4) + regs[13](52) + sp(4) + lr(4) + pc(4) + cpsr(4)
    // sp is at offset: 4 + 4 + 52 = 60
    // pc is at offset: 68
    ldr sp, [r0, #60]          // pcb.sp
    ldr lr, [r0, #68]          // pcb.pc (we'll jump here via lr)

    // Enable IRQs before jumping (so timer can fire)
    mrs r1, cpsr
    bic r1, r1, #0x80
    msr cpsr_c, r1

    // Jump to the process entry point
    mov pc, lr

// ============================================================================
// Low-level memory access
// ============================================================================
PUT32:
    str r1, [r0]
    bx lr

GET32:
    ldr r0, [r0]
    bx lr

// ============================================================================
// Interrupt control
// ============================================================================
enable_irq:
    mrs r0, cpsr
    bic r0, r0, #0x80          // Clear I-bit to enable IRQ
    msr cpsr_c, r0
    bx lr

disable_irq:
    mrs r0, cpsr
    orr r0, r0, #0x80          // Set I-bit to disable IRQ
    msr cpsr_c, r0
    bx lr

hang:
    b hang

// ============================================================================
// Global buffers for context switch
// The C handler (timer.c) copies these to/from the PCB arrays
// ============================================================================
.data
.align 2                        // 4-byte alignment (2^2) — required for str/ldr
.globl saved_regs
.globl saved_lr
.globl saved_svc_sp
.globl saved_svc_lr

saved_regs:   .space 52        // 13 registers × 4 bytes (R0-R12)
saved_lr:     .space 4         // Interrupted process's return address
saved_svc_sp: .space 4         // SVC mode stack pointer
saved_svc_lr: .space 4         // SVC mode link register
