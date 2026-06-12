/**
 * @file bm_hal_comp.c
 * @brief 比较器 HAL 分发层（契约 → driver API）
 */
#include "bm_hal_comp.h"
#include "bm_types.h"

int bm_hal_comp_clear_latch(const bm_hal_comp_t *comp) {
    if (!comp || !comp->api || !comp->api->clear_latch) {
        return BM_ERR_NOT_INIT;
    }
    return comp->api->clear_latch(comp);
}
