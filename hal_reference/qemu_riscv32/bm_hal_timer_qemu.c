/**
 * @file bm_hal_timer_qemu.c
 * @brief QEMU RISC-V32 定时器 HAL 桩实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_timer.h"
#include "bm_log.h"

#define TAG "hal_timer"

static volatile uint32_t _ticks = 0;
static void (*_tick_cb)(void) = NULL;
static uint32_t _tick_freq = 1000u;

int bm_hal_timer_init(uint32_t freq_hz) {
    _tick_freq = (freq_hz > 0u) ? freq_hz : 1000u;
    BM_LOGI(TAG, "init: freq_hz=%u (stub)", _tick_freq);
    return 0;
}

void bm_hal_timer_stop(void) {
    _tick_cb = NULL;
    _ticks = 0u;
    BM_LOGI(TAG, "stop");
}

uint32_t bm_hal_timer_get_ticks(void) {
    return _ticks;
}

uint32_t bm_hal_timer_get_freq(void) {
    return _tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    _tick_cb = cb;
}
