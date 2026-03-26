#ifndef TIMER_H
#define TIMER_H

void timer_init(void);
void timer_irq_clear(void);
void timer_irq_handler(void);

#endif
