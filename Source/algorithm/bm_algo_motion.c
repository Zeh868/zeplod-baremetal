/**
 * @file bm_algo_motion.c
 * @brief 运动辅助：编码器与 DDA 实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            正式发布
 * 2026-06-13       1.1            zeh            增加步进脉冲生成
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_motion.h"

#include <math.h>

#ifndef BM_ALGO_PI_F
#define BM_ALGO_PI_F 3.14159265358979323846f
#endif

void bm_algo_encoder_reset(bm_algo_encoder_state_t *state,
                           const bm_algo_encoder_config_t *config,
                           int32_t raw_count) {
    if (state == NULL || config == NULL) {
        return;
    }
    state->prev_count = raw_count;
    state->turns = 0;
    state->position_rad = 0.0f;
    state->velocity_rad_s = 0.0f;
    (void)config;
}

float bm_algo_encoder_update(bm_algo_encoder_state_t *state,
                             const bm_algo_encoder_config_t *config,
                             int32_t raw_count,
                             float dt_s) {
    int32_t delta;
    float counts_to_rad;
    float pos_counts;

    if (state == NULL || config == NULL || config->counts_per_rev == 0u ||
        dt_s <= 0.0f) {
        return 0.0f;
    }

    delta = raw_count - state->prev_count;
    if (delta > (int32_t)(config->counts_per_rev / 2u)) {
        state->turns--;
    } else if (delta < -(int32_t)(config->counts_per_rev / 2u)) {
        state->turns++;
    }

    state->prev_count = raw_count;
    pos_counts = (float)state->turns * (float)config->counts_per_rev
                 + (float)raw_count;
    counts_to_rad = 2.0f * BM_ALGO_PI_F / (float)config->counts_per_rev;

    state->velocity_rad_s =
        (pos_counts * counts_to_rad - state->position_rad) / dt_s;
    state->position_rad = pos_counts * counts_to_rad;
    return state->position_rad;
}

void bm_algo_dda_reset(bm_algo_dda_state_t *state,
                       const bm_algo_dda_config_t *config) {
    if (state == NULL || config == NULL) {
        return;
    }

    state->x = config->x0;
    state->y = config->y0;
    state->dx = config->x1 - config->x0;
    state->dy = config->y1 - config->y0;
    state->steps = fabsf(state->dx);
    if (fabsf(state->dy) > state->steps) {
        state->steps = fabsf(state->dy);
    }
    state->step_count = 0.0f;
    state->err = 0.0f;
    state->step_x = (state->dx >= 0.0f) ? 1 : -1;
    state->step_y = (state->dy >= 0.0f) ? 1 : -1;
    state->done = (state->steps < 1e-6f) ? 1 : 0;
}

int bm_algo_dda_step(bm_algo_dda_state_t *state,
                     const bm_algo_dda_config_t *config,
                     float *x_out,
                     float *y_out) {
    float inc_x;
    float inc_y;

    (void)config;

    if (state == NULL || state->done) {
        return 0;
    }

    if (state->steps < 1e-6f) {
        state->done = 1;
        return 0;
    }

    inc_x = state->dx / state->steps;
    inc_y = state->dy / state->steps;
    state->x += inc_x;
    state->y += inc_y;
    state->step_count += 1.0f;

    if (state->step_count >= state->steps) {
        state->done = 1;
    }

    if (x_out != NULL) {
        *x_out = state->x;
    }
    if (y_out != NULL) {
        *y_out = state->y;
    }
    return 1;
}

void bm_algo_stepper_reset(bm_algo_stepper_state_t *state, int32_t position) {
    if (state != NULL) {
        state->phase = 0.0f;
        state->position_steps = position;
    }
}

uint32_t bm_algo_stepper_process(bm_algo_stepper_state_t *state,
                                 const bm_algo_stepper_config_t *config,
                                 float velocity_steps_s,
                                 float dt_s,
                                 int8_t *pulses,
                                 uint32_t max_pulses) {
    float max_vel;
    int8_t dir;
    uint32_t count = 0u;

    if (state == NULL || config == NULL || dt_s <= 0.0f) {
        return 0u;
    }

    max_vel = config->max_velocity_steps_s;
    if (max_vel > 0.0f) {
        if (velocity_steps_s > max_vel) {
            velocity_steps_s = max_vel;
        } else if (velocity_steps_s < -max_vel) {
            velocity_steps_s = -max_vel;
        }
    }

    if (velocity_steps_s == 0.0f) {
        return 0u;
    }

    dir = (velocity_steps_s > 0.0f) ? 1 : -1;
    state->phase += fabsf(velocity_steps_s) * dt_s;

    while (state->phase >= 1.0f) {
        state->phase -= 1.0f;
        state->position_steps += dir;
        if (pulses != NULL) {
            if (count >= max_pulses) {
                break;
            }
            pulses[count++] = dir;
        } else {
            count++;
        }
    }

    return count;
}
