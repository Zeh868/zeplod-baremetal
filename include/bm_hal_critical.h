#ifndef BM_HAL_CRITICAL_H
#define BM_HAL_CRITICAL_H

#include "bm_types.h"

/* Weak symbols: platforms can override */
bm_irq_state_t bm_hal_critical_enter(void);
void bm_hal_critical_exit(bm_irq_state_t state);

#endif
