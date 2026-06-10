/**
 * @file bm_hal_pwm_stm32g4.c
 * @brief STM32G4 PWM HAL 参考实现（TIM1/TIM8 高级定时器）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_pwm.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

#include <stddef.h>
#include <string.h>

struct bm_hal_pwm {
    uint32_t tim_base;
    uint8_t phase_count;
};

static bm_hal_hrt_binding_t g_pwm_update_binding;
static const bm_hal_pwm_t *g_pwm_update_owner;

const bm_hal_pwm_t BM_HAL_PWM_TIM1 = {
    BM_STM32G4_TIM1_BASE,
    3u,
};

static const bm_hal_pwm_t *pwm_valid(const bm_hal_pwm_t *pwm) {
    if (!pwm || pwm->tim_base == 0u) {
        return NULL;
    }
    return pwm;
}

static volatile uint32_t *ccr_for_phase(uint32_t tim_base, uint32_t phase) {
    switch (phase) {
    case 0u:
        return &BM_TIM_CCR1(tim_base);
    case 1u:
        return &BM_TIM_CCR2(tim_base);
    case 2u:
        return &BM_TIM_CCR3(tim_base);
    default:
        return NULL;
    }
}

/** TIM1 更新中断服务例程（由启动文件/向量表挂接） */
void TIM1_UP_TIM16_IRQHandler(void) {
    if ((BM_TIM_SR(BM_STM32G4_TIM1_BASE) & BM_TIM_SR_UIF) == 0u) {
        return;
    }
    BM_TIM_SR(BM_STM32G4_TIM1_BASE) &= ~BM_TIM_SR_UIF;
    if (g_pwm_update_owner && g_pwm_update_binding.callback) {
        g_pwm_update_binding.callback(g_pwm_update_binding.context);
    }
}

int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm, uint32_t phase, uint16_t duty) {
    const bm_hal_pwm_t *inst = pwm_valid(pwm);
    volatile uint32_t *ccr;

    if (!inst || phase >= inst->phase_count) {
        return BM_ERR_INVALID;
    }
    ccr = ccr_for_phase(inst->tim_base, phase);
    if (!ccr) {
        return BM_ERR_INVALID;
    }
    *ccr = duty;
    return BM_OK;
}

int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable) {
    const bm_hal_pwm_t *inst = pwm_valid(pwm);
    uint32_t cc_mask;
    uint32_t bdtr;

    if (!inst) {
        return BM_ERR_INVALID;
    }

    cc_mask = BM_TIM_CCER_CC1E | BM_TIM_CCER_CC2E | BM_TIM_CCER_CC3E;
    if (enable) {
        BM_TIM_CCER(inst->tim_base) |= cc_mask;
        bdtr = BM_TIM_BDTR(inst->tim_base);
        bdtr |= BM_TIM_BDTR_AOE | BM_TIM_BDTR_MOE;
        BM_TIM_BDTR(inst->tim_base) = bdtr;
        BM_TIM_CR1(inst->tim_base) |= BM_TIM_CR1_CEN;
    } else {
        bdtr = BM_TIM_BDTR(inst->tim_base);
        bdtr &= ~BM_TIM_BDTR_MOE;
        BM_TIM_BDTR(inst->tim_base) = bdtr;
        BM_TIM_CCER(inst->tim_base) &= ~cc_mask;
    }
    return BM_OK;
}

void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm) {
    const bm_hal_pwm_t *inst = pwm_valid(pwm);
    uint32_t phase;

    if (!inst) {
        return;
    }
    (void)bm_hal_pwm_enable_outputs(inst, 0);
    for (phase = 0u; phase < inst->phase_count; ++phase) {
        volatile uint32_t *ccr = ccr_for_phase(inst->tim_base, phase);
        if (ccr) {
            *ccr = 0u;
        }
    }
}

int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding) {
    const bm_hal_pwm_t *inst = pwm_valid(pwm);

    if (!inst) {
        return BM_ERR_INVALID;
    }

    if (!binding) {
        BM_TIM_DIER(inst->tim_base) &= ~BM_TIM_DIER_UIE;
        if (g_pwm_update_owner == inst) {
            memset(&g_pwm_update_binding, 0, sizeof(g_pwm_update_binding));
            g_pwm_update_owner = NULL;
        }
        return BM_OK;
    }

    g_pwm_update_binding = *binding;
    g_pwm_update_owner = inst;
    BM_TIM_DIER(inst->tim_base) |= BM_TIM_DIER_UIE;
    return BM_OK;
}
