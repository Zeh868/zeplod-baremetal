/**
 * @file bm_hal_wdg.c
 * @brief 看门狗 HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_wdg.h"
#include "bm_hal_weak.h"
#include "bm_types.h"

BM_HAL_WEAK
int bm_hal_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
void bm_hal_wdg_feed(void) {
}
