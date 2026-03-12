#ifndef TIMER_H
#define TIMER_H

// Platform-specific (implemented in bsp/<target>/timer_hw.c)
void timer_init(void);
void timer_irq_clear(void);

// Generic API (implemented in hal/timer.c)
void timer_irq_handler(void);

#endif // TIMER_H
