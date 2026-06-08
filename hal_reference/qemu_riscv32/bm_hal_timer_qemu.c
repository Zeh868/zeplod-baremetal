#include "bm_hal_timer.h"

static volatile uint32_t _ticks = 0;

int bm_hal_timer_init(uint32_t freq_hz) {
    (void)freq_hz;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return 1000; }
void bm_hal_timer_set_callback(void (*cb)(void)) { (void)cb; }
