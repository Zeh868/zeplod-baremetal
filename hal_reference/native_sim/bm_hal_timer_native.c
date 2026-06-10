#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "bm_hal_timer.h"

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static uint32_t tick_freq = 1000;

int bm_hal_timer_init(uint32_t freq_hz) {
    tick_freq = freq_hz ? freq_hz : 1000;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) {
    uint64_t elapsed_ms;

#ifdef _WIN32
    elapsed_ms = (uint64_t)GetTickCount();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    elapsed_ms = (uint64_t)ts.tv_sec * 1000U;
    elapsed_ms += (uint64_t)ts.tv_nsec / 1000000U;
#endif

    return (uint32_t)(elapsed_ms * tick_freq / 1000U);
}

uint32_t bm_hal_timer_get_freq(void) {
    return tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    (void)cb;
}
