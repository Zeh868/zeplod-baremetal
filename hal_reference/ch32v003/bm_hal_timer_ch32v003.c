/**
 * @file bm_hal_timer_ch32v003.c
 * @brief CH32V003 定时器 HAL 参考桩（SysTick / TIM1 由应用补充）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_timer.h"
#include "bm_log.h"
#include "bm_types.h"

#define TAG "hal_timer"

static volatile uint32_t g_ticks;
static void (*g_tick_cb)(void);
static uint32_t g_tick_freq = 1000u;

int bm_hal_timer_init(uint32_t freq_hz) {
    g_tick_freq = (freq_hz > 0u) ? freq_hz : 1000u;
    BM_LOGI(TAG, "init: freq_hz=%u (stub)", g_tick_freq);
    return BM_OK;
}

void bm_hal_timer_stop(void) {
    g_tick_cb = NULL;
    g_ticks = 0u;
}

uint32_t bm_hal_timer_get_ticks(void) {
    return g_ticks;
}

uint32_t bm_hal_timer_get_freq(void) {
    return g_tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    g_tick_cb = cb;
}
