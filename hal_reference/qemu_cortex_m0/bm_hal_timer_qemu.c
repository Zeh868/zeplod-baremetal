#include "bm_hal_timer.h"
#include <stddef.h>

#define SYSTICK_BASE    0xE000E010
#define SYSTICK_CTRL    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD    (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL     (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

static volatile uint32_t _ticks = 0;
static void (*_tick_cb)(void) = NULL;

void SysTick_Handler(void) {
    _ticks++;
    if (_tick_cb) {
        _tick_cb();
    }
}

int bm_hal_timer_init(uint32_t freq_hz) {
    uint32_t reload = (freq_hz > 0) ? (1000 / freq_hz) : 1000;
    SYSTICK_LOAD = reload - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return 1000; }
void bm_hal_timer_set_callback(void (*cb)(void)) { _tick_cb = cb; }
