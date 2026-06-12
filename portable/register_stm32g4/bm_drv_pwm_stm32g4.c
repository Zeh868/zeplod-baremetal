/**
 * @file bm_drv_pwm_stm32g4.c
 * @brief STM32G4 PWM 设备驱动（寄存器参考实现，可替换为 Cube LL 后端）
 */
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_pwm.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    uint32_t tim_base;
    uint8_t  phase_count;
} bm_pwm_stm32g4_config_t;

static bm_hal_hrt_binding_t g_pwm_update_binding;
static const bm_hal_pwm_t  *g_pwm_update_owner;

static const bm_pwm_stm32g4_config_t *pwm_cfg(const bm_hal_pwm_t *pwm) {
    if (!pwm || !pwm->config) {
        return NULL;
    }
    return (const bm_pwm_stm32g4_config_t *)pwm->config;
}

static volatile uint32_t *ccr_for_phase(uint32_t tim_base, uint32_t phase) {
    switch (phase) {
    case 0u: return &BM_TIM_CCR1(tim_base);
    case 1u: return &BM_TIM_CCR2(tim_base);
    case 2u: return &BM_TIM_CCR3(tim_base);
    default: return NULL;
    }
}

void TIM1_UP_TIM16_IRQHandler(void) {
    if ((BM_TIM_SR(BM_STM32G4_TIM1_BASE) & BM_TIM_SR_UIF) == 0u) {
        return;
    }
    BM_TIM_SR(BM_STM32G4_TIM1_BASE) &= ~BM_TIM_SR_UIF;
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
    ccr = ccr_for_phase(cfg->tim_base, phase);
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
    cc_mask = BM_TIM_CCER_CC1E | BM_TIM_CCER_CC2E | BM_TIM_CCER_CC3E;
    if (enable) {
        BM_TIM_CCER(cfg->tim_base) |= cc_mask;
        bdtr = BM_TIM_BDTR(cfg->tim_base);
        bdtr |= BM_TIM_BDTR_AOE | BM_TIM_BDTR_MOE;
        BM_TIM_BDTR(cfg->tim_base) = bdtr;
        BM_TIM_CR1(cfg->tim_base) |= BM_TIM_CR1_CEN;
    } else {
        bdtr = BM_TIM_BDTR(cfg->tim_base);
        bdtr &= ~BM_TIM_BDTR_MOE;
        BM_TIM_BDTR(cfg->tim_base) = bdtr;
        BM_TIM_CCER(cfg->tim_base) &= ~cc_mask;
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
        volatile uint32_t *ccr = ccr_for_phase(cfg->tim_base, phase);
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
        BM_TIM_DIER(cfg->tim_base) &= ~BM_TIM_DIER_UIE;
        if (g_pwm_update_owner == pwm) {
            memset(&g_pwm_update_binding, 0, sizeof(g_pwm_update_binding));
            g_pwm_update_owner = NULL;
        }
        return BM_OK;
    }
    g_pwm_update_binding = *binding;
    g_pwm_update_owner = pwm;
    BM_TIM_DIER(cfg->tim_base) |= BM_TIM_DIER_UIE;
    return BM_OK;
}

static const struct bm_pwm_driver_api bm_pwm_stm32g4_api = {
    stm32_pwm_set_duty,
    stm32_pwm_enable_outputs,
    stm32_pwm_request_safe_state,
    stm32_pwm_bind_update,
};

static const bm_pwm_stm32g4_config_t bm_pwm_tim1_cfg = {
    BM_STM32G4_TIM1_BASE,
    3u,
};

const bm_hal_pwm_t BM_HAL_PWM_TIM1 = { &bm_pwm_stm32g4_api, &bm_pwm_tim1_cfg };
