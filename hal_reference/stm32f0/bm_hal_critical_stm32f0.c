#include "bm_hal_critical.h"
#include <stdint.h>

/* CMSIS-style PRIMASK save/restore for Cortex-M */
static inline uint32_t _get_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    return primask;
}

static inline void _set_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r" (primask));
}

bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = _get_primask();
    _set_primask(1);
    return state;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    _set_primask(state);
}
