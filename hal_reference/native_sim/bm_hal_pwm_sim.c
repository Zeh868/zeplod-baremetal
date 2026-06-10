#include "bm_hal_pwm.h"

#include <string.h>

#define BM_SIM_PWM_MAX_PHASES 3u
#define BM_SIM_PWM_INSTANCES  4u

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

int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm, uint32_t phase, uint16_t duty) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state || phase >= BM_SIM_PWM_MAX_PHASES) {
        return BM_ERR_INVALID;
    }
    state->duty[phase] = duty;
    return BM_OK;
}

int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state) {
        return BM_ERR_INVALID;
    }
    state->outputs_enabled = enable ? 1 : 0;
    return BM_OK;
}

void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state) {
        return;
    }
    state->outputs_enabled = 0;
    memset(state->duty, 0, sizeof(state->duty));
}

int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state) {
        return BM_ERR_INVALID;
    }
    if (!binding) {
        memset(&state->update_binding, 0, sizeof(state->update_binding));
        return BM_OK;
    }
    state->update_binding = *binding;
    return BM_OK;
}

void bm_hal_pwm_sim_fire_update(const bm_hal_pwm_t *pwm) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state || !state->update_binding.callback) {
        return;
    }
    state->update_binding.callback(state->update_binding.context);
}

uint16_t bm_hal_pwm_sim_get_duty(const bm_hal_pwm_t *pwm, uint32_t phase) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    if (!state || phase >= BM_SIM_PWM_MAX_PHASES) {
        return 0u;
    }
    return state->duty[phase];
}

int bm_hal_pwm_sim_outputs_enabled(const bm_hal_pwm_t *pwm) {
    pwm_sim_state_t *state = pwm_state_for(pwm);
    return state ? state->outputs_enabled : 0;
}
