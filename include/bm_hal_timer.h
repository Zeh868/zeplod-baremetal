#ifndef BM_HAL_TIMER_H
#define BM_HAL_TIMER_H

#include <stdint.h>

int bm_hal_timer_init(uint32_t freq_hz);
void bm_hal_timer_stop(void);
uint32_t bm_hal_timer_get_ticks(void);
uint32_t bm_hal_timer_get_freq(void);
void bm_hal_timer_set_callback(void (*cb)(void));

#endif /* BM_HAL_TIMER_H */
