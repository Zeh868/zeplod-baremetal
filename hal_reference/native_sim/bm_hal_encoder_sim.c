/**
 * @file bm_hal_encoder_sim.c
 * @brief 原生仿真环境编码器 HAL 实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_encoder.h"
#include "bm_log.h"

#define TAG "hal_encoder"

struct bm_hal_encoder {
    uint32_t id;
};

static int32_t g_encoder_count[2];

const bm_hal_encoder_t BM_HAL_ENC_SIM0 = { 0u };

/** 仿真注入：设置编码器计数值 */
void bm_hal_encoder_sim_set_count(const bm_hal_encoder_t *enc, int32_t value) {
    if (!enc || enc->id >= 2u) {
        BM_LOGW(TAG, "set_count: invalid enc=%p", (const void *)enc);
        return;
    }
    g_encoder_count[enc->id] = value;
}

/** 读取编码器当前计数值 */
int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value) {
    if (!enc || !value || enc->id >= 2u) {
        BM_LOGE(TAG, "read: invalid enc=%p value=%p", (const void *)enc, (const void *)value);
        return BM_ERR_INVALID;
    }
    *value = g_encoder_count[enc->id];
    return BM_OK;
}
