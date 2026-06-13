/**
 * @file bm_drv_adc_stm32g4.c
 * @brief STM32G4 ADC 设备驱动（CMSIS 寄存器访问）
 */
#include "bm_hal_adc.h"
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_sdk_stm32g4.h"
#include "bm_types.h"

#include <string.h>

typedef struct {
    ADC_TypeDef *adc;
    uint8_t      injected_rank_count;
} bm_adc_stm32g4_config_t;

static bm_hal_hrt_binding_t g_adc_complete_binding;
static const bm_hal_adc_t  *g_adc_complete_owner;

static const bm_adc_stm32g4_config_t *adc_cfg(const bm_hal_adc_t *adc) {
    if (!adc || !adc->config) {
        return NULL;
    }
    return (const bm_adc_stm32g4_config_t *)adc->config;
}

static volatile uint32_t *jdr_for_rank(ADC_TypeDef *adc, uint32_t rank) {
    switch (rank) {
    case 0u: return &adc->JDR1;
    case 1u: return &adc->JDR2;
    case 2u: return &adc->JDR3;
    case 3u: return &adc->JDR4;
    default: return NULL;
    }
}

void ADC1_2_IRQHandler(void) {
    if ((ADC1->ISR & ADC_ISR_JEOC) == 0u) {
        return;
    }
    ADC1->ISR = ADC_ISR_JEOC;
    if (g_adc_complete_owner && g_adc_complete_binding.callback) {
        g_adc_complete_binding.callback(g_adc_complete_binding.context);
    }
}

static int stm32_adc_read_injected(const struct bm_hal_adc *adc,
                                   uint32_t rank, uint16_t *value) {
    const bm_adc_stm32g4_config_t *cfg = adc_cfg(adc);
    volatile uint32_t *jdr;

    if (!cfg || !value || rank >= cfg->injected_rank_count) {
        return BM_ERR_INVALID;
    }
    jdr = jdr_for_rank(cfg->adc, rank);
    if (!jdr) {
        return BM_ERR_INVALID;
    }
    *value = (uint16_t)(*jdr & 0xFFFFu);
    return BM_OK;
}

static int stm32_adc_bind_complete(const struct bm_hal_adc *adc,
                                   const bm_hal_hrt_binding_t *binding) {
    const bm_adc_stm32g4_config_t *cfg = adc_cfg(adc);

    if (!cfg) {
        return BM_ERR_INVALID;
    }
    if (!binding) {
        cfg->adc->IER &= (uint32_t)~ADC_IER_JEOCIE;
        if (g_adc_complete_owner == adc) {
            memset(&g_adc_complete_binding, 0, sizeof(g_adc_complete_binding));
            g_adc_complete_owner = NULL;
        }
        return BM_OK;
    }
    g_adc_complete_binding = *binding;
    g_adc_complete_owner = adc;
    cfg->adc->IER |= ADC_IER_JEOCIE;
    return BM_OK;
}

static const struct bm_adc_driver_api bm_adc_stm32g4_api = {
    stm32_adc_read_injected,
    stm32_adc_bind_complete,
};

static const bm_adc_stm32g4_config_t bm_adc1_cfg = {
    ADC1,
    4u,
};

const bm_hal_adc_t BM_HAL_ADC1 = { &bm_adc_stm32g4_api, &bm_adc1_cfg };
