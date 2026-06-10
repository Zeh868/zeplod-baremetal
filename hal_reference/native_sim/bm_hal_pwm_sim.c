/**
 * @file bm_hal_pwm_sim.c
 * @brief 原生仿真环境 PWM HAL 实现（占空比与安全态）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_pwm.h"
#include "bm_log.h"

#include <string.h>

#define BM_SIM_PWM_MAX_PHASES 3u
#define BM_SIM_PWM_INSTANCES  4u
#define TAG                   "hal_pwm"

struct bm_hal_pwm {
    uint32_t id;
};

typedef struct {
    uint16_t duty[BM_SIM_PWM_MAX_PHASES];
    int outputs_enabled;
    bm_hal_hrt_binding_t update_binding;
} pwm_sim_state_t;

static pwm_sim_state_t g_pwm_state[BM_SIM_PWM_INSTANCES];

static pwm_sim_state_t *pwm_state_for(const bm_hal_pwm_t *pwm) {
    if (!pwm || pwm->id >= BM_SIM_PWM_INSTANCES) {
        return NULL;
    }
    return &g_pwm_state[pwm->id];
}

const bm_hal_pwm_t BM_HAL_PWM_SIM0 = { 0u };
const bm_hal_pwm_t BM_HAL_PWM_SIM1 = { 1u };
const bm_hal_pwm_t BM_HAL_PWM_SIM2 = { 2u };

/** 设置指定相位的占空比 */
int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm, uint32_t phase, uint16_t duty) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state || phase >= BM_SIM_PWM_MAX_PHASES) {
        BM_LOGE(TAG, "set_duty: invalid pwm=%p phase=%u", (const void *)pwm, phase);
        return BM_ERR_INVALID;
    }
    state->duty[phase] = duty;
    return BM_OK;
}

/** 使能或关闭 PWM 输出 */
int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state) {
        BM_LOGE(TAG, "enable_outputs: invalid pwm=%p", (const void *)pwm);
        return BM_ERR_INVALID;
    }
    state->outputs_enabled = enable ? 1 : 0;
    BM_LOGI(TAG, "enable_outputs: pwm id=%u enable=%d", pwm->id, enable);
    return BM_OK;
}

/** 请求安全态：关闭输出并清零占空比 */
void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state) {
        BM_LOGW(TAG, "request_safe_state: invalid pwm=%p", (const void *)pwm);
        return;
    }
    state->outputs_enabled = 0;
    memset(state->duty, 0, sizeof(state->duty));
    BM_LOGI(TAG, "request_safe_state: pwm id=%u", pwm->id);
}

/** 绑定 PWM 更新周期 HRT 回调 */
int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state) {
        BM_LOGE(TAG, "bind_update: invalid pwm=%p", (const void *)pwm);
        return BM_ERR_INVALID;
    }
    if (!binding) {
        memset(&state->update_binding, 0, sizeof(state->update_binding));
        BM_LOGI(TAG, "bind_update: unbound pwm id=%u", pwm->id);
        return BM_OK;
    }
    state->update_binding = *binding;
    BM_LOGI(TAG, "bind_update: bound pwm id=%u", pwm->id);
    return BM_OK;
}

/** 仿真触发：调用已绑定的更新回调 */
void bm_hal_pwm_sim_fire_update(const bm_hal_pwm_t *pwm) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state || !state->update_binding.callback) {
        return;
    }
    state->update_binding.callback(state->update_binding.context);
}

/** 查询指定相位占空比（仿真辅助） */
uint16_t bm_hal_pwm_sim_get_duty(const bm_hal_pwm_t *pwm, uint32_t phase) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state || phase >= BM_SIM_PWM_MAX_PHASES) {
        return 0u;
    }
    return state->duty[phase];
}

/** 查询输出是否使能（仿真辅助） */
int bm_hal_pwm_sim_outputs_enabled(const bm_hal_pwm_t *pwm) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    return state ? state->outputs_enabled : 0;
}
