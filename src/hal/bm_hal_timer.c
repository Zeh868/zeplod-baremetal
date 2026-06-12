/**
 * @file bm_hal_timer.c
 * @brief 定时器 HAL 分发层（契约 → driver API）
 */
#include "bm_drv_timer.h"
#include "bm_hal_timer.h"
#include "bm_types.h"

#ifdef BM_DRV_HAS_BACKEND
extern const struct bm_timer_driver_api bm_drv_timer_api;
#define BM_TIMER_DRV (&bm_drv_timer_api)
#else
static int timer_stub_init(uint32_t freq_hz) {
    (void)freq_hz;
    return BM_ERR_NOT_INIT;
}

static void timer_stub_stop(void) {
}

static uint32_t timer_stub_get_ticks(void) {
    return 0u;
}

static uint32_t timer_stub_get_freq(void) {
    return 0u;
}

static void timer_stub_set_callback(void (*cb)(void)) {
    (void)cb;
}

static const struct bm_timer_driver_api timer_stub = {
    timer_stub_init,
    timer_stub_stop,
    timer_stub_get_ticks,
    timer_stub_get_freq,
    timer_stub_set_callback,
};

#define BM_TIMER_DRV (&timer_stub)
#endif

int bm_hal_timer_init(uint32_t freq_hz) {
    if (!BM_TIMER_DRV->init) {
        return BM_ERR_NOT_INIT;
    }
    return BM_TIMER_DRV->init(freq_hz);
}

void bm_hal_timer_stop(void) {
    if (BM_TIMER_DRV->stop) {
        BM_TIMER_DRV->stop();
    }
}

uint32_t bm_hal_timer_get_ticks(void) {
    if (!BM_TIMER_DRV->get_ticks) {
        return 0u;
    }
    return BM_TIMER_DRV->get_ticks();
}

uint32_t bm_hal_timer_get_freq(void) {
    if (!BM_TIMER_DRV->get_freq) {
        return 0u;
    }
    return BM_TIMER_DRV->get_freq();
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    if (BM_TIMER_DRV->set_callback) {
        BM_TIMER_DRV->set_callback(cb);
    }
}
