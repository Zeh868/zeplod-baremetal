#include "bm_hal_timer.h"
#include <time.h>

static uint32_t tick_freq = 1000;

int bm_hal_timer_init(uint32_t freq_hz) {
    tick_freq = freq_hz ? freq_hz : 1000;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ms = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return (uint32_t)(ms * tick_freq / 1000);
}

uint32_t bm_hal_timer_get_freq(void) {
    return tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    (void)cb;
}
