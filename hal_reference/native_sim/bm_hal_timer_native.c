/**
 * @file bm_hal_timer_native.c
 * @brief 原生仿真环境软件定时器 HAL 实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_timer.h"
#include "bm_hal_timer_native.h"
#include "bm_log.h"

#include <stdint.h>

#define TAG "hal_timer"

static uint32_t tick_freq = 1000u;
static uint32_t tick_count;
static void (*tick_callback)(void);

/** 初始化软件 tick 频率（Hz），0 时使用默认 1000 Hz */
int bm_hal_timer_init(uint32_t freq_hz) {
    tick_freq = freq_hz ? freq_hz : 1000u;
    BM_LOGI(TAG, "init: freq_hz=%u", tick_freq);
    return 0;
}

/** 停止定时器并清除 tick 回调 */
void bm_hal_timer_stop(void) {
    tick_callback = NULL;
    BM_LOGI(TAG, "stop");
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

/** 测试辅助：手动推进 tick 计数并触发回调 */
void bm_hal_timer_native_advance_ticks(uint32_t delta) {
    uint32_t i;

    for (i = 0u; i < delta; ++i) {
        tick_count++;
        if (tick_callback) {
            tick_callback();
        }
    }
}

/** 测试辅助：清零 tick 计数 */
void bm_hal_timer_native_reset_ticks(void) {
    tick_count = 0u;
}
