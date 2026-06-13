/**
 * @file bm_algo_spectral.c
 * @brief 频谱分析算法实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_spectral.h"

#include <math.h>

#ifndef BM_ALGO_PI_F
#define BM_ALGO_PI_F 3.14159265358979323846f
#endif

int bm_algo_goertzel_init(bm_algo_goertzel_state_t *state,
                          const bm_algo_goertzel_config_t *config) {
    float omega;

    if (state == NULL || config == NULL ||
        config->block_size == 0u || config->sample_hz <= 0.0f) {
        return -1;
    }

    omega = 2.0f * BM_ALGO_PI_F * config->target_freq_hz / config->sample_hz;
    bm_algo_goertzel_reset(state);
    state->coeff = 2.0f * cosf(omega);
    return 0;
}

void bm_algo_goertzel_reset(bm_algo_goertzel_state_t *state) {
    if (state != NULL) {
        state->s_prev = 0.0f;
        state->s_prev2 = 0.0f;
        state->count = 0u;
    }
}

int bm_algo_goertzel_feed(bm_algo_goertzel_state_t *state,
                          const bm_algo_goertzel_config_t *config,
                          float sample) {
    float s;

    if (state == NULL || config == NULL) {
        return -1;
    }

    s = sample + state->coeff * state->s_prev - state->s_prev2;
    state->s_prev2 = state->s_prev;
    state->s_prev = s;
    state->count++;

    return (state->count >= config->block_size) ? 1 : 0;
}

float bm_algo_goertzel_result(bm_algo_goertzel_state_t *state,
                              const bm_algo_goertzel_config_t *config) {
    float real;
    float imag;
    float power;

    if (state == NULL || config == NULL) {
        return 0.0f;
    }

    real = state->s_prev - state->s_prev2 * cosf(
        2.0f * BM_ALGO_PI_F * config->target_freq_hz / config->sample_hz);
    imag = state->s_prev2 * sinf(
        2.0f * BM_ALGO_PI_F * config->target_freq_hz / config->sample_hz);
    power = real * real + imag * imag;
    return sqrtf(power / (float)config->block_size);
}

void bm_algo_psd_from_spectrum(const float *mag,
                               uint32_t bin_count,
                               float scale,
                               float *psd) {
    uint32_t i;

    if (mag == NULL || psd == NULL) {
        return;
    }

    for (i = 0u; i < bin_count; ++i) {
        psd[i] = mag[i] * mag[i] * scale;
    }
}

void bm_algo_envelope_reset(bm_algo_envelope_state_t *state) {
    if (state != NULL) {
        state->prev = 0.0f;
        state->envelope = 0.0f;
        state->alpha = 0.1f;
    }
}

float bm_algo_envelope_step(bm_algo_envelope_state_t *state, float input) {
    float abs_in;

    if (state == NULL) {
        return input;
    }

    abs_in = fabsf(input);
    state->envelope += state->alpha * (abs_in - state->envelope);
    state->prev = input;
    return state->envelope;
}

float bm_algo_correlate(const float *a, const float *b, uint32_t len) {
    float sum = 0.0f;
    uint32_t i;

    if (a == NULL || b == NULL) {
        return 0.0f;
    }

    for (i = 0u; i < len; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

int bm_algo_find_peak_bin(const float *spectrum,
                          uint32_t start_bin,
                          uint32_t end_bin,
                          uint32_t *peak_bin,
                          float *peak_value) {
    uint32_t i;
    float max_v = -1.0f;
    uint32_t max_i = start_bin;

    if (spectrum == NULL || peak_bin == NULL || peak_value == NULL ||
        end_bin <= start_bin) {
        return -1;
    }

    for (i = start_bin; i < end_bin; ++i) {
        if (spectrum[i] > max_v) {
            max_v = spectrum[i];
            max_i = i;
        }
    }

    *peak_bin = max_i;
    *peak_value = max_v;
    return 0;
}
