#include "../../hal/timer.h"
#include "../../hal/types.h"

// ============================================================================
// Watchdog Timer (WDT1)
// Must be disabled or the board resets after ~60 seconds
// ============================================================================
#define WDT1_BASE   0x44E35000
#define WDT_WSPR    (WDT1_BASE + 0x48)  // Start/Stop Register
#define WDT_WWPS    (WDT1_BASE + 0x34)  // Write Posting Bits Register

static void wdt_disable(void) {
    // Sequence to disable: write 0xAAAA, wait, write 0x5555, wait
    PUT32(WDT_WSPR, 0xAAAA);
    while (GET32(WDT_WWPS) & (1 << 4));  // Wait for write to complete
    PUT32(WDT_WSPR, 0x5555);
    while (GET32(WDT_WWPS) & (1 << 4));
}

// ============================================================================
// DMTimer2 (IRQ 68)
// ============================================================================
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

// ============================================================================
// Quantum Configuration
// Change QUANTUM_MS to set the time slice for each process (in milliseconds)
// Formula: TIMER_LOAD = 0xFFFFFFFF - (QUANTUM_MS * TICKS_PER_MS) + 1
// ============================================================================
#define TIMER_FREQ_HZ    24000000
#define TICKS_PER_MS     (TIMER_FREQ_HZ / 1000)
#define QUANTUM_MS       1000
#define TIMER_LOAD_VALUE (0xFFFFFFFF - (QUANTUM_MS * TICKS_PER_MS) + 1)

void timer_init(void) {
    // Disable watchdog first
    wdt_disable();

    // Enable the timer clock
    PUT32(CM_PER_TIMER2_CLKCTRL, 0x2);

    // Unmask IRQ 68 (bit 4 of MIR_CLEAR2)
    PUT32(INTC_MIR_CLEAR2, (1 << 4));

    // Configure interrupt priority
    PUT32(INTC_ILR68, 0x0);

    // Stop the timer
    PUT32(TCLR, 0);

    // Clear any pending interrupts
    PUT32(TISR, 0x7);

    // Set load value (QUANTUM_MS ms at 24MHz)
    PUT32(TLDR, TIMER_LOAD_VALUE);

    // Set counter to same value
    PUT32(TCRR, TIMER_LOAD_VALUE);

    // Enable overflow interrupt
    PUT32(TIER, 0x2);

    // Start timer in auto-reload mode
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
