#include "bm_hal_timer.h"

/* STM32F0 SysTick stub */
static volatile uint32_t _ticks = 0;
static void (*_tick_cb)(void) = NULL;
static uint32_t _tick_freq = 1000u;

int bm_hal_timer_init(uint32_t freq_hz) {
    _tick_freq = (freq_hz > 0u) ? freq_hz : 1000u;
    return 0;
}

void bm_hal_timer_stop(void) {
    _tick_cb = NULL;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return _tick_freq; }
void bm_hal_timer_set_callback(void (*cb)(void)) { _tick_cb = cb; }
