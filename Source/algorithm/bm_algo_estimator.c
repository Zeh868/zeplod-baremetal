/**
 * @file bm_algo_estimator.c
 * @brief 固定维度状态估算实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_estimator.h"

void bm_algo_kalman1d_reset(bm_algo_kalman1d_state_t *state,
                            float x0,
                            float p0) {
    if (state != NULL) {
        state->x = x0;
        state->p = p0;
    }
}

float bm_algo_kalman1d_predict(bm_algo_kalman1d_state_t *state,
                               const bm_algo_kalman1d_config_t *config) {
    if (state == NULL || config == NULL) {
        return 0.0f;
    }
    state->p += config->q;
    return state->x;
}

float bm_algo_kalman1d_update(bm_algo_kalman1d_state_t *state,
                              const bm_algo_kalman1d_config_t *config,
                              float measurement) {
    float k;

    if (state == NULL || config == NULL) {
        return measurement;
    }

    k = state->p / (state->p + config->r);
    state->x += k * (measurement - state->x);
    state->p *= (1.0f - k);
    return state->x;
}

void bm_algo_ekf_cv_reset(bm_algo_ekf_cv_state_t *state, float pos, float vel) {
    if (state != NULL) {
        state->pos = pos;
        state->vel = vel;
        state->p00 = 1.0f;
        state->p01 = 0.0f;
        state->p10 = 0.0f;
        state->p11 = 1.0f;
    }
}

void bm_algo_ekf_cv_predict(bm_algo_ekf_cv_state_t *state,
                            const bm_algo_ekf_cv_config_t *config,
                            float dt_s) {
    float p00;
    float p01;
    float p10;
    float p11;

    if (state == NULL || config == NULL || dt_s <= 0.0f) {
        return;
    }

    state->pos += state->vel * dt_s;

    p00 = state->p00 + dt_s * (state->p10 + state->p01) + dt_s * dt_s * state->p11;
    p01 = state->p01 + dt_s * state->p11;
    p10 = state->p10 + dt_s * state->p11;
    p11 = state->p11;

    state->p00 = p00 + config->q_pos;
    state->p01 = p01;
    state->p10 = p10;
    state->p11 = p11 + config->q_vel;
}

void bm_algo_ekf_cv_update(bm_algo_ekf_cv_state_t *state,
                           const bm_algo_ekf_cv_config_t *config,
                           float pos_meas) {
    float s;
    float k0;
    float k1;
    float y;
    float p00;
    float p01;
    float p10;
    float p11;

    if (state == NULL || config == NULL) {
        return;
    }

    s = state->p00 + config->r_pos;
    if (s <= 0.0f) {
        return;
    }
    k0 = state->p00 / s;
    k1 = state->p10 / s;
    y = pos_meas - state->pos;
    p00 = state->p00;
    p01 = state->p01;
    p10 = state->p10;
    p11 = state->p11;

    state->pos += k0 * y;
    state->vel += k1 * y;

    state->p00 = p00 - k0 * p00;
    state->p01 = p01 - k0 * p01;
    state->p10 = p10 - k1 * p00;
    state->p11 = p11 - k1 * p01;
    state->p01 = 0.5f * (state->p01 + state->p10);
    state->p10 = state->p01;
}
