/**
 * @file bm_drv_comp_stm32g4.c
 * @brief STM32G4 比较器设备驱动（CMSIS 寄存器访问）
 */
#include "bm_hal_comp.h"
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_sdk_stm32g4.h"
#include "bm_types.h"

typedef struct {
    COMP_TypeDef *comp;
} bm_comp_stm32g4_config_t;

static const bm_comp_stm32g4_config_t *comp_cfg(const bm_hal_comp_t *comp) {
    if (!comp || !comp->config) {
        return NULL;
    }
    return (const bm_comp_stm32g4_config_t *)comp->config;
}

static int stm32_comp_clear_latch(const struct bm_hal_comp *comp) {
    const bm_comp_stm32g4_config_t *cfg = comp_cfg(comp);
    uint32_t csr;

    if (!cfg) {
        return BM_ERR_INVALID;
    }
    csr = cfg->comp->CSR;
    if ((csr & COMP_CSR_COMP1LOCK) != 0u) {
        return BM_ERR_BUSY;
    }
    csr &= (uint32_t)~COMP_CSR_COMP1VALUE;
    cfg->comp->CSR = csr;
    return BM_OK;
}

static const struct bm_comp_driver_api bm_comp_stm32g4_api = {
    stm32_comp_clear_latch,
};

static const bm_comp_stm32g4_config_t bm_comp1_cfg = {
    COMP1,
};

const bm_hal_comp_t BM_HAL_COMP1 = { &bm_comp_stm32g4_api, &bm_comp1_cfg };
