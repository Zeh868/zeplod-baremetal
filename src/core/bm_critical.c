#include "bm_hal_critical.h"

/* Default implementation for native_sim: use a global flag */
static volatile bm_irq_state_t _irq_state = 0;

__attribute__((weak)) bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t prev = _irq_state;
    _irq_state = 1;
    return prev;
}

__attribute__((weak)) void bm_hal_critical_exit(bm_irq_state_t state) {
    _irq_state = state;
}
