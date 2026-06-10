#include "bm_hal_timer.h"
#include <stddef.h>

#define SYSTICK_BASE    0xE000E010
#define SYSTICK_CTRL    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD    (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL     (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

static volatile uint32_t _ticks = 0;
static void (*_tick_cb)(void) = NULL;
static uint32_t _tick_freq = 1000u;

void SysTick_Handler(void) {
    _ticks++;
    if (_tick_cb) {
        _tick_cb();
    }
}

int bm_hal_timer_init(uint32_t freq_hz) {
    uint32_t reload;
    _tick_freq = (freq_hz > 0u) ? freq_hz : 1000u;
    reload = (_tick_freq > 0u) ? (1000u / _tick_freq) : 1000u;
    if (reload == 0u) {
        reload = 1u;
    }
    SYSTICK_LOAD = reload - 1u;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7u;
    return 0;
}

void bm_hal_timer_stop(void) {
    SYSTICK_CTRL = 0u;
    _tick_cb = NULL;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return _tick_freq; }
void bm_hal_timer_set_callback(void (*cb)(void)) { _tick_cb = cb; }
