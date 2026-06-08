#ifndef BM_HAL_WDG_H
#define BM_HAL_WDG_H

#include <stdint.h>

int bm_hal_wdg_init(uint32_t timeout_ms);
void bm_hal_wdg_feed(void);

#endif
