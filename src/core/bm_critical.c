#include "bm_hal_critical.h"
#include "bm_atomic.h"

/* Default implementation for native_sim: use a global flag */
static volatile bm_irq_state_t _irq_state = 0;

#if defined(_MSC_VER) || defined(_WIN32)
#define BM_WEAK
#else
#define BM_WEAK __attribute__((weak))
#endif

BM_WEAK bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t prev = _irq_state;
    _irq_state = 1;
    return prev;
}

BM_WEAK void bm_hal_critical_exit(bm_irq_state_t state) {
    _irq_state = state;
}

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
