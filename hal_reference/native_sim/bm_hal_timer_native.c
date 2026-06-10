#include "bm_hal_timer.h"
#include "bm_hal_timer_native.h"

#include <stdint.h>

static uint32_t tick_freq = 1000u;
static uint32_t tick_count;
static void (*tick_callback)(void);

int bm_hal_timer_init(uint32_t freq_hz) {
    tick_freq = freq_hz ? freq_hz : 1000u;
    return 0;
}

void bm_hal_timer_stop(void) {
    tick_callback = NULL;
}

uint32_t bm_hal_timer_get_ticks(void) {
    return tick_count;
}

uint32_t bm_hal_timer_get_freq(void) {
    return tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    tick_callback = cb;
}

void bm_hal_timer_native_advance_ticks(uint32_t delta) {
    uint32_t i;

    for (i = 0u; i < delta; ++i) {
        tick_count++;
        if (tick_callback) {
            tick_callback();
        }
    }
}

void bm_hal_timer_native_reset_ticks(void) {
    tick_count = 0u;
}
