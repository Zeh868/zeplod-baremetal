#ifndef BM_HAL_CRITICAL_H
#define BM_HAL_CRITICAL_H

#include "bm_types.h"

#include <stdint.h>

#ifndef BM_HAL_HAS_PRIORITY_MASK
#define BM_HAL_HAS_PRIORITY_MASK 0
#endif

bm_irq_state_t bm_hal_critical_enter(void);
void bm_hal_critical_exit(bm_irq_state_t state);

#if BM_HAL_HAS_PRIORITY_MASK
bm_irq_state_t bm_hal_critical_enter_below(uint8_t threshold);
void bm_hal_critical_exit_below(bm_irq_state_t previous_state);
#endif

#endif /* BM_HAL_CRITICAL_H */
