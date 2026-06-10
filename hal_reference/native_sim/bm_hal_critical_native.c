#include "bm_hal_critical.h"

static volatile bm_irq_state_t irq_state;

bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t previous = irq_state;
    irq_state = 1;
    return previous;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    irq_state = state;
}
