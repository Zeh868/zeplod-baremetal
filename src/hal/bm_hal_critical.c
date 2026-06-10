/**
 * @file bm_hal_critical.c
 * @brief 临界区 HAL 默认弱符号桩（无平台实现时的占位）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_critical.h"
#include "bm_hal_weak.h"

BM_HAL_WEAK
bm_irq_state_t bm_hal_critical_enter(void) {
    return 0;
}

BM_HAL_WEAK
void bm_hal_critical_exit(bm_irq_state_t state) {
    (void)state;
}

#if BM_HAL_HAS_PRIORITY_MASK
BM_HAL_WEAK
bm_irq_state_t bm_hal_critical_enter_below(uint8_t threshold) {
    (void)threshold;
    return 0;
}

BM_HAL_WEAK
void bm_hal_critical_exit_below(bm_irq_state_t previous_state) {
    (void)previous_state;
}
#endif
