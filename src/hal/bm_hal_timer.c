/**
 * @file bm_hal_timer.c
 * @brief 定时器 HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_timer.h"
#include "bm_hal_weak.h"
#include "bm_types.h"

BM_HAL_WEAK
int bm_hal_timer_init(uint32_t freq_hz) {
    (void)freq_hz;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
void bm_hal_timer_stop(void) {
}

BM_HAL_WEAK
uint32_t bm_hal_timer_get_ticks(void) {
    return 0u;
}

BM_HAL_WEAK
uint32_t bm_hal_timer_get_freq(void) {
    return 0u;
}

BM_HAL_WEAK
void bm_hal_timer_set_callback(void (*cb)(void)) {
    (void)cb;
}
