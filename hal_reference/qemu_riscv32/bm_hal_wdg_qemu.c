/**
 * @file bm_hal_wdg_qemu.c
 * @brief QEMU RISC-V32 看门狗 HAL 桩实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_wdg.h"
#include "bm_log.h"

#define TAG "hal_wdg"

int bm_hal_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    BM_LOGI(TAG, "init: timeout_ms=%u (stub)", timeout_ms);
    return 0;
}

void bm_hal_wdg_feed(void) {
}
