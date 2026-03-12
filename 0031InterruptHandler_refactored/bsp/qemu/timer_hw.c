#include "../../hal/timer.h"
#include "../../hal/types.h"

// VersatilePB Timer (SP804) and VIC (PL190)
#define TIMER0_BASE      0x101E2000
#define TIMER0_LOAD      (TIMER0_BASE + 0x00)
#define TIMER0_CTRL      (TIMER0_BASE + 0x08)
#define TIMER0_INTCLR    (TIMER0_BASE + 0x0C)
#define VIC_BASE         0x10140000
#define VIC_INTENABLE    (VIC_BASE + 0x10)

void timer_init(void) {
    // Set Load Value (approx 1 sec)
    PUT32(TIMER0_LOAD, 0x100000);

    // Control: Enable(7), Periodic(6), IntEn(5), 32bit(1) -> 0xE2
    PUT32(TIMER0_CTRL, 0xE2);

    // Enable Timer0 interrupt (Bit 4) in VIC
    PUT32(VIC_INTENABLE, (1 << 4));
}

void timer_irq_clear(void) {
    // Clear the timer interrupt
    PUT32(TIMER0_INTCLR, 1);
}
