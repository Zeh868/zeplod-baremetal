/**
 * @file bm_drv_encoder_stm32g4.c
 * @brief STM32G4 编码器设备驱动
 */
#include "bm_hal_encoder.h"
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

typedef struct {
    uint32_t tim_base;
} bm_encoder_stm32g4_config_t;

static const bm_encoder_stm32g4_config_t *enc_cfg(const bm_hal_encoder_t *enc) {
    if (!enc || !enc->config) {
        return NULL;
    }
    return (const bm_encoder_stm32g4_config_t *)enc->config;
}

static int stm32_encoder_read(const struct bm_hal_encoder *enc, int32_t *value) {
    const bm_encoder_stm32g4_config_t *cfg = enc_cfg(enc);

    if (!cfg || !value) {
        return BM_ERR_INVALID;
    }
    *value = (int32_t)BM_TIM_CNT(cfg->tim_base);
    return BM_OK;
}

static const struct bm_encoder_driver_api bm_encoder_stm32g4_api = {
    stm32_encoder_read,
};

static const bm_encoder_stm32g4_config_t bm_enc_tim2_cfg = {
    BM_STM32G4_TIM2_BASE,
};

const bm_hal_encoder_t BM_HAL_ENC_TIM2 = { &bm_encoder_stm32g4_api, &bm_enc_tim2_cfg };
