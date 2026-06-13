/**
 * @file bm_algo_resample.c
 * @brief 重采样实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_resample.h"

void bm_algo_decimator_reset(bm_algo_decimator_state_t *state) {
    if (state != NULL) {
        state->counter = 0u;
        state->decim = 1u;
    }
}

int bm_algo_decimator_step(bm_algo_decimator_state_t *state,
                           uint32_t decim,
                           float input,
                           float *output) {
    if (state == NULL || decim == 0u) {
        return 0;
    }

    state->decim = decim;
    if (state->counter == 0u) {
        if (output != NULL) {
            *output = input;
        }
        state->counter = decim - 1u;
        return 1;
    }

    if (state->counter > 0u) {
        state->counter--;
    }
    return 0;
}

void bm_algo_linear_resampler_reset(bm_algo_linear_resampler_state_t *state,
                                    float ratio,
                                    float initial) {
    if (state != NULL) {
        state->ratio = ratio;
        state->phase = (ratio > 0.0f) ? (1.0f / ratio) : 0.0f;
        state->prev_sample = initial;
    }
}

int bm_algo_linear_resampler_step(bm_algo_linear_resampler_state_t *state,
                                  float input,
                                  float *outputs,
                                  uint32_t max_outputs,
                                  uint32_t *out_count) {
    uint32_t n = 0u;
    uint32_t required = 0u;
    float phase;
    float step;

    if (state == NULL || outputs == NULL || out_count == NULL ||
        max_outputs == 0u || state->ratio <= 0.0f) {
        if (out_count != NULL) {
            *out_count = 0u;
        }
        return 0;
    }

    step = 1.0f / state->ratio;
    phase = state->phase;
    while (phase <= 1.0f) {
        required++;
        phase += step;
    }
    if (required > max_outputs) {
        *out_count = 0u;
        return -1;
    }

    phase = state->phase;
    while (phase <= 1.0f) {
        outputs[n++] = state->prev_sample * (1.0f - phase)
                       + input * phase;
        phase += step;
    }

    state->phase = phase - 1.0f;
    state->prev_sample = input;
    *out_count = n;
    return (int)n;
}
