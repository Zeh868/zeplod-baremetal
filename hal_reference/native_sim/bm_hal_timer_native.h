#ifndef BM_HAL_TIMER_NATIVE_H
#define BM_HAL_TIMER_NATIVE_H

#include <stdint.h>

void bm_hal_timer_native_advance_ticks(uint32_t delta);
void bm_hal_timer_native_reset_ticks(void);

#endif /* BM_HAL_TIMER_NATIVE_H */
