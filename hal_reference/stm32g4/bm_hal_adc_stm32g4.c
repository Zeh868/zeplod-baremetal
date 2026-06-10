/**
 * @file bm_hal_adc_stm32g4.c
 * @brief STM32G4 ADC HAL 参考实现（注入组 + JEOC 中断绑定）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_adc.h"
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

#include <stddef.h>
#include <string.h>

struct bm_hal_adc {
    uint32_t adc_base;
    uint8_t injected_rank_count;
};

static bm_hal_hrt_binding_t g_adc_complete_binding;
static const bm_hal_adc_t *g_adc_complete_owner;

const bm_hal_adc_t BM_HAL_ADC1 = {
    BM_STM32G4_ADC1_BASE,
    4u,
};

static const bm_hal_adc_t *adc_valid(const bm_hal_adc_t *adc) {
    if (!adc || adc->adc_base == 0u) {
        return NULL;
    }
    return adc;
}

static volatile uint32_t *jdr_for_rank(uint32_t adc_base, uint32_t rank) {
    switch (rank) {
    case 0u:
        return &BM_ADC_JDR1(adc_base);
    case 1u:
        return &BM_ADC_JDR2(adc_base);
    case 2u:
        return &BM_ADC_JDR3(adc_base);
    case 3u:
        return &BM_ADC_JDR4(adc_base);
    default:
        return NULL;
    }
}

/** ADC1/2 注入组转换完成中断（由向量表挂接） */
void ADC1_2_IRQHandler(void) {
    if ((BM_ADC_ISR(BM_STM32G4_ADC1_BASE) & BM_ADC_ISR_JEOC) == 0u) {
        return;
    }
    BM_ADC_ISR(BM_STM32G4_ADC1_BASE) &= ~BM_ADC_ISR_JEOC;
    if (g_adc_complete_owner && g_adc_complete_binding.callback) {
        g_adc_complete_binding.callback(g_adc_complete_binding.context);
    }
}

int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value) {
    const bm_hal_adc_t *inst = adc_valid(adc);
    volatile uint32_t *jdr;

    if (!inst || !value || rank >= inst->injected_rank_count) {
        return BM_ERR_INVALID;
    }
    jdr = jdr_for_rank(inst->adc_base, rank);
    if (!jdr) {
        return BM_ERR_INVALID;
    }
    *value = (uint16_t)(*jdr & 0xFFFFu);
    return BM_OK;
}

int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding) {
    const bm_hal_adc_t *inst = adc_valid(adc);

    if (!inst) {
        return BM_ERR_INVALID;
    }

    if (!binding) {
        BM_ADC_IER(inst->adc_base) &= ~BM_ADC_IER_JEOCIE;
        if (g_adc_complete_owner == inst) {
            memset(&g_adc_complete_binding, 0, sizeof(g_adc_complete_binding));
            g_adc_complete_owner = NULL;
        }
        return BM_OK;
    }

    g_adc_complete_binding = *binding;
    g_adc_complete_owner = inst;
    BM_ADC_IER(inst->adc_base) |= BM_ADC_IER_JEOCIE;
    return BM_OK;
}
