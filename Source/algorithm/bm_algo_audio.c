/**
 * @file bm_algo_audio.c
 * @brief 音频数学核实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_audio.h"
#include "bm/algorithm/bm_algo_common.h"

#include <math.h>

void bm_algo_audio_gain(const float *in, float *out, uint32_t n, float gain) {
    uint32_t i;

    if (in == NULL || out == NULL) {
        return;
    }
    for (i = 0u; i < n; ++i) {
        out[i] = in[i] * gain;
    }
}

void bm_algo_audio_mix2(const float *a, const float *b, float *out,
                        uint32_t n, float gain_a, float gain_b) {
    uint32_t i;

    if (a == NULL || b == NULL || out == NULL) {
        return;
    }
    for (i = 0u; i < n; ++i) {
        out[i] = a[i] * gain_a + b[i] * gain_b;
    }
}

void bm_algo_limiter_process(const float *in, float *out, uint32_t n,
                             const bm_algo_limiter_config_t *config) {
    uint32_t i;
    float th;
    float knee;

    if (in == NULL || out == NULL || config == NULL) {
        return;
    }

    th = config->threshold;
    knee = config->knee;

    for (i = 0u; i < n; ++i) {
        float x = in[i];
        float ax = fabsf(x);

        if (ax <= th) {
            out[i] = x;
        } else if (ax <= th + knee) {
            float t = (ax - th) / knee;
            float gain = th + knee * t * t * 0.5f;
            out[i] = (x > 0.0f ? 1.0f : -1.0f) * gain;
        } else {
            out[i] = (x > 0.0f ? 1.0f : -1.0f) * (th + knee * 0.5f);
        }
    }
}

void bm_algo_agc_reset(bm_algo_agc_state_t *state, float gain_init) {
    if (state != NULL) {
        state->gain = gain_init;
    }
}

void bm_algo_agc_process(bm_algo_agc_state_t *state,
                         const bm_algo_agc_config_t *config,
                         const float *in,
                         float *out,
                         uint32_t n) {
    uint32_t i;
    float level;
    float err;
    float coeff;

    if (state == NULL || config == NULL || in == NULL || out == NULL) {
        return;
    }

    level = 0.0f;
    for (i = 0u; i < n; ++i) {
        level += fabsf(in[i]);
    }
    level /= (float)n;

    err = config->target_level - level * state->gain;
    coeff = (err > 0.0f) ? config->attack_coeff : config->release_coeff;
    state->gain += coeff * err;
    if (state->gain < 0.01f) {
        state->gain = 0.01f;
    }

    for (i = 0u; i < n; ++i) {
        out[i] = in[i] * state->gain;
    }
}

void bm_algo_vad_reset(bm_algo_vad_state_t *state) {
    if (state != NULL) {
        state->energy = 0.0f;
        state->voice_active = 0;
    }
}

void bm_algo_vad_process(bm_algo_vad_state_t *state,
                         const bm_algo_vad_config_t *config,
                         const float *in,
                         uint32_t n) {
    uint32_t i;
    float e = 0.0f;

    if (state == NULL || config == NULL || in == NULL) {
        return;
    }

    for (i = 0u; i < n; ++i) {
        e += in[i] * in[i];
    }
    e /= (float)n;

    state->energy += config->alpha * (e - state->energy);
    state->voice_active = (state->energy > config->energy_threshold) ? 1 : 0;
}
