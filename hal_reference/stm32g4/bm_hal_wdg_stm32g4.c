/**
 * @file bm_hal_wdg_stm32g4.c
 * @brief STM32G4 独立看门狗 HAL 参考桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_wdg.h"
#include "bm_log.h"
#include "bm_types.h"

#define TAG "hal_wdg"

int bm_hal_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    BM_LOGI(TAG, "init: IWDG stub (configure prescaler/reload)");
    return BM_OK;
}

void bm_hal_wdg_feed(void) {
}
