#ifndef INTERRUPT_TIMER_H
#define INTERRUPT_TIMER_H

typedef void (*interrupt_timer_callback_t)(void);

void interrupt_timer_init(interrupt_timer_callback_t callback);

#endif /* INTERRUPT_TIMER_H */
