/**
 * @file bm_hal_encoder_stm32g4.c
 * @brief STM32G4 编码器 HAL 参考实现（TIM2/TIM3 正交模式读 CNT）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_encoder.h"
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

struct bm_hal_encoder {
    uint32_t tim_base;
};

const bm_hal_encoder_t BM_HAL_ENC_TIM2 = {
    BM_STM32G4_TIM2_BASE,
};

static const bm_hal_encoder_t *enc_valid(const bm_hal_encoder_t *enc) {
    if (!enc || enc->tim_base == 0u) {
        return NULL;
    }
    return enc;
}

int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value) {
    const bm_hal_encoder_t *inst = enc_valid(enc);

    if (!inst || !value) {
        return BM_ERR_INVALID;
    }
    *value = (int32_t)BM_TIM_CNT(inst->tim_base);
    return BM_OK;
}
