/**
 * @file bm_drv_pwm_stm32g4.c
 * @brief STM32G4 PWM 设备驱动（CMSIS 寄存器访问）
 */
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_pwm.h"
#include "bm_hal_sdk_stm32g4.h"
#include "bm_types.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    TIM_TypeDef *tim;
    uint8_t      phase_count;
} bm_pwm_stm32g4_config_t;

static bm_hal_hrt_binding_t g_pwm_update_binding;
static const bm_hal_pwm_t  *g_pwm_update_owner;

static const bm_pwm_stm32g4_config_t *pwm_cfg(const bm_hal_pwm_t *pwm) {
    if (!pwm || !pwm->config) {
        return NULL;
    }
    return (const bm_pwm_stm32g4_config_t *)pwm->config;
}

static volatile uint32_t *ccr_for_phase(TIM_TypeDef *tim, uint32_t phase) {
    switch (phase) {
    case 0u: return &tim->CCR1;
    case 1u: return &tim->CCR2;
    case 2u: return &tim->CCR3;
    default: return NULL;
    }
}

void TIM1_UP_TIM16_IRQHandler(void) {
    if ((TIM1->SR & TIM_SR_UIF) == 0u) {
        return;
    }
    TIM1->SR &= (uint32_t)~TIM_SR_UIF;
    if (g_pwm_update_owner && g_pwm_update_binding.callback) {
        g_pwm_update_binding.callback(g_pwm_update_binding.context);
    }
}

static int stm32_pwm_set_duty(const struct bm_hal_pwm *pwm, uint32_t phase, uint16_t duty) {
    const bm_pwm_stm32g4_config_t *cfg = pwm_cfg(pwm);
    volatile uint32_t *ccr;

    if (!cfg || phase >= cfg->phase_count) {
        return BM_ERR_INVALID;
    }
    ccr = ccr_for_phase(cfg->tim, phase);
    if (!ccr) {
        return BM_ERR_INVALID;
    }
    *ccr = duty;
    return BM_OK;
}

static int stm32_pwm_enable_outputs(const struct bm_hal_pwm *pwm, int enable) {
    const bm_pwm_stm32g4_config_t *cfg = pwm_cfg(pwm);
    uint32_t cc_mask;
    uint32_t bdtr;

    if (!cfg) {
        return BM_ERR_INVALID;
    }
    cc_mask = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;
    if (enable) {
        cfg->tim->CCER |= cc_mask;
        bdtr = cfg->tim->BDTR;
        bdtr |= TIM_BDTR_AOE | TIM_BDTR_MOE;
        cfg->tim->BDTR = bdtr;
        cfg->tim->CR1 |= TIM_CR1_CEN;
    } else {
        bdtr = cfg->tim->BDTR;
        bdtr &= (uint32_t)~TIM_BDTR_MOE;
        cfg->tim->BDTR = bdtr;
        cfg->tim->CCER &= (uint32_t)~cc_mask;
    }
    return BM_OK;
}

static void stm32_pwm_request_safe_state(const struct bm_hal_pwm *pwm) {
    const bm_pwm_stm32g4_config_t *cfg = pwm_cfg(pwm);
    uint32_t phase;

    if (!cfg) {
        return;
    }
    (void)stm32_pwm_enable_outputs(pwm, 0);
    for (phase = 0u; phase < cfg->phase_count; ++phase) {
        volatile uint32_t *ccr = ccr_for_phase(cfg->tim, phase);
        if (ccr) {
            *ccr = 0u;
        }
    }
}

static int stm32_pwm_bind_update(const struct bm_hal_pwm *pwm,
                                 const bm_hal_hrt_binding_t *binding) {
    const bm_pwm_stm32g4_config_t *cfg = pwm_cfg(pwm);

    if (!cfg) {
        return BM_ERR_INVALID;
    }
    if (!binding) {
        cfg->tim->DIER &= (uint32_t)~TIM_DIER_UIE;
        if (g_pwm_update_owner == pwm) {
            memset(&g_pwm_update_binding, 0, sizeof(g_pwm_update_binding));
            g_pwm_update_owner = NULL;
        }
        return BM_OK;
    }
    g_pwm_update_binding = *binding;
    g_pwm_update_owner = pwm;
    cfg->tim->DIER |= TIM_DIER_UIE;
    return BM_OK;
}

static const struct bm_pwm_driver_api bm_pwm_stm32g4_api = {
    stm32_pwm_set_duty,
    stm32_pwm_enable_outputs,
    stm32_pwm_request_safe_state,
    stm32_pwm_bind_update,
};

static const bm_pwm_stm32g4_config_t bm_pwm_tim1_cfg = {
    TIM1,
    3u,
};

const bm_hal_pwm_t BM_HAL_PWM_TIM1 = { &bm_pwm_stm32g4_api, &bm_pwm_tim1_cfg };
