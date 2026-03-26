#include "../../hal/timer.h"
#include "../../hal/types.h"

// ============================================================================
// VersatilePB SP804 Timer + PL190 VIC
// ============================================================================
#define TIMER0_BASE      0x101E2000
#define TIMER0_LOAD      (TIMER0_BASE + 0x00)
#define TIMER0_CTRL      (TIMER0_BASE + 0x08)
#define TIMER0_INTCLR    (TIMER0_BASE + 0x0C)

#define VIC_BASE         0x10140000
#define VIC_INTENABLE    (VIC_BASE + 0x10)

// ============================================================================
// Quantum Configuration
// Change QUANTUM_MS to set the time slice for each process (in milliseconds)
// SP804 on VersatilePB runs at 1 MHz (1,000,000 Hz)
// Note: QEMU virtual time runs slower than wall clock, so use smaller values
// Formula: TIMER_LOAD = QUANTUM_MS * TICKS_PER_MS
// ============================================================================
#define TIMER_FREQ_HZ    1000000
#define TICKS_PER_MS     (TIMER_FREQ_HZ / 1000)
#define QUANTUM_MS       500
#define TIMER_LOAD_VALUE (QUANTUM_MS * TICKS_PER_MS)

void timer_init(void) {
    // Set Load Value
    PUT32(TIMER0_LOAD, TIMER_LOAD_VALUE);

    // Control: Enable(7), Periodic(6), IntEn(5), 32bit(1) -> 0xE2
    PUT32(TIMER0_CTRL, 0xE2);

    // Enable Timer0 interrupt (Bit 4) in VIC
    PUT32(VIC_INTENABLE, (1 << 4));
}

void timer_irq_clear(void) {
    PUT32(TIMER0_INTCLR, 1);
}
