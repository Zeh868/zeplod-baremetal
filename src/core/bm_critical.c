#include "bm_hal_critical.h"
#include "bm_atomic.h"

uint32_t bm_atomic_load(bm_atomic_t *v) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t val = *v;
    bm_hal_critical_exit(s);
    return val;
}

void bm_atomic_store(bm_atomic_t *v, uint32_t val) {
    bm_irq_state_t s = bm_hal_critical_enter();
    *v = val;
    bm_hal_critical_exit(s);
}

uint32_t bm_atomic_inc(bm_atomic_t *v) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t val = (*v)++;
    bm_hal_critical_exit(s);
    return val;
}
