#include "../../hal/timer.h"
#include "../../hal/types.h"

// BeagleBone Black DMTIMER2
#define DMTIMER2_BASE    0x48040000
#define TCLR             (DMTIMER2_BASE + 0x38)  // Timer Control Register
#define TCRR             (DMTIMER2_BASE + 0x3C)  // Timer Counter Register
#define TISR             (DMTIMER2_BASE + 0x28)  // Timer Interrupt Status Register
#define TIER             (DMTIMER2_BASE + 0x2C)  // Timer Interrupt Enable Register
#define TLDR             (DMTIMER2_BASE + 0x40)  // Timer Load Register

// Interrupt Controller (INTCPS)
#define INTCPS_BASE      0x48200000
#define INTC_MIR_CLEAR2  (INTCPS_BASE + 0xC8)
#define INTC_CONTROL     (INTCPS_BASE + 0x48)
#define INTC_ILR68       (INTCPS_BASE + 0x110)

// Clock Manager
#define CM_PER_BASE      0x44E00000
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)

void timer_init(void) {
    // 1. Enable the timer clock
    PUT32(CM_PER_TIMER2_CLKCTRL, 0x2);

    // 2. Unmask IRQ 68 (bit 4 of MIR_CLEAR2)
    PUT32(INTC_MIR_CLEAR2, (1 << 4));

    // 3. Configure interrupt priority
    PUT32(INTC_ILR68, 0x0);

    // 4. Stop the timer
    PUT32(TCLR, 0);

    // 5. Clear any pending interrupts
    PUT32(TISR, 0x7);

    // 6. Set the load value for 2 seconds
    PUT32(TLDR, 0xFE91CA00);

    // 7. Set the counter to the same value
    PUT32(TCRR, 0xFE91CA00);

    // 8. Enable overflow interrupt
    PUT32(TIER, 0x2);

    // 9. Start timer in auto-reload mode
    PUT32(TCLR, 0x3);
}

void timer_irq_clear(void) {
    // Clear the timer interrupt flag
    PUT32(TISR, 0x2);
    // Read back to ensure write has completed (flush posted write)
    (void)GET32(TISR);

    // Acknowledge the interrupt to the controller
    PUT32(INTC_CONTROL, 0x1);
}
