#include "bm_hal_critical.h"

static inline uint32_t get_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    return primask;
}

static inline void set_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = get_primask();
    __asm volatile ("cpsid i" ::: "memory");
    return state;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    set_primask(state);
}
