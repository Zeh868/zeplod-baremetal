/**
 * @file bm_hal_encoder.c
 * @brief 编码器 HAL 分发层（契约 → driver API）
 */
#include "bm_hal_encoder.h"
#include "bm_types.h"

int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value) {
    if (!enc || !enc->api || !enc->api->read) {
        return BM_ERR_NOT_INIT;
    }
    return enc->api->read(enc, value);
}
