/**
 * @file bm_algo_audio.c
 * @brief 音频数学核实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            正式发布
 * 2026-06-13       1.1            zeh            增加 EQ/compressor/gate/GCC-PHAT
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_audio.h"
#include "bm/algorithm/bm_algo_common.h"
#include "bm/algorithm/bm_algo_filter.h"

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

    if (state == NULL || config == NULL || in == NULL || out == NULL ||
        n == 0u) {
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

    if (state == NULL || config == NULL || in == NULL || n == 0u) {
        return;
    }

    for (i = 0u; i < n; ++i) {
        e += in[i] * in[i];
    }
    e /= (float)n;

    state->energy += config->alpha * (e - state->energy);
    state->voice_active = (state->energy > config->energy_threshold) ? 1 : 0;
}

int bm_algo_eq_peaking_design(bm_algo_eq_peaking_state_t *state,
                              const bm_algo_eq_peaking_config_t *config) {
    bm_algo_biquad_config_t bq;
    bm_algo_biquad_design_t design;

    if (state == NULL || config == NULL ||
        config->sample_hz <= 0.0f || config->freq_hz <= 0.0f ||
        config->q <= 0.0f) {
        return -1;
    }

    design.type = BM_ALGO_BIQUAD_BPF;
    design.sample_hz = config->sample_hz;
    design.freq_hz = config->freq_hz;
    design.q = config->q;
    design.gain_db = config->gain_db;

    if (bm_algo_biquad_design(&bq, &design) != 0) {
        return -1;
    }

    state->b0 = bq.b0;
    state->b1 = bq.b1;
    state->b2 = bq.b2;
    state->a1 = bq.a1;
    state->a2 = bq.a2;
    bm_algo_eq_peaking_reset(state);
    return 0;
}

void bm_algo_eq_peaking_reset(bm_algo_eq_peaking_state_t *state) {
    if (state != NULL) {
        state->z1 = 0.0f;
        state->z2 = 0.0f;
    }
}

void bm_algo_eq_peaking_process(bm_algo_eq_peaking_state_t *state,
                                const bm_algo_eq_peaking_config_t *config,
                                const float *in,
                                float *out,
                                uint32_t n) {
    uint32_t i;
    bm_algo_biquad_state_t bq_state;
    bm_algo_biquad_config_t bq_config;

    if (state == NULL || config == NULL || in == NULL || out == NULL ||
        n == 0u) {
        return;
    }

    if (state->b0 == 0.0f && state->b1 == 0.0f && state->b2 == 0.0f) {
        (void)bm_algo_eq_peaking_design(state, config);
    }

    bq_state.z1 = state->z1;
    bq_state.z2 = state->z2;
    bq_config.b0 = state->b0;
    bq_config.b1 = state->b1;
    bq_config.b2 = state->b2;
    bq_config.a1 = state->a1;
    bq_config.a2 = state->a2;

    for (i = 0u; i < n; ++i) {
        out[i] = bm_algo_biquad_step(&bq_state, &bq_config, in[i]);
    }

    state->z1 = bq_state.z1;
    state->z2 = bq_state.z2;
}

void bm_algo_compressor_reset(bm_algo_compressor_state_t *state) {
    if (state != NULL) {
        state->envelope = 0.0f;
    }
}

void bm_algo_compressor_process(bm_algo_compressor_state_t *state,
                                const bm_algo_compressor_config_t *config,
                                const float *in,
                                float *out,
                                uint32_t n) {
    uint32_t i;
    float th;
    float ratio;
    float atk;
    float rel;

    if (state == NULL || config == NULL || in == NULL || out == NULL ||
        n == 0u) {
        return;
    }

    th = config->threshold;
    ratio = (config->ratio > 1.0f) ? config->ratio : 1.0f;
    atk = bm_algo_clamp_f(config->attack_coeff, 0.0f, 1.0f);
    rel = bm_algo_clamp_f(config->release_coeff, 0.0f, 1.0f);

    for (i = 0u; i < n; ++i) {
        float x = in[i];
        float ax = fabsf(x);
        float gain;
        float coeff;

        coeff = (ax > state->envelope) ? atk : rel;
        state->envelope += coeff * (ax - state->envelope);

        if (state->envelope <= th) {
            gain = config->makeup_gain;
        } else {
            float over = state->envelope - th;
            float compressed = th + over / ratio;
            gain = (state->envelope > 1e-12f)
                       ? (compressed / state->envelope) * config->makeup_gain
                       : config->makeup_gain;
        }

        out[i] = x * gain;
    }
}

void bm_algo_noise_gate_reset(bm_algo_noise_gate_state_t *state) {
    if (state != NULL) {
        state->envelope = 0.0f;
        state->gain = 1.0f;
    }
}

void bm_algo_noise_gate_process(bm_algo_noise_gate_state_t *state,
                                const bm_algo_noise_gate_config_t *config,
                                const float *in,
                                float *out,
                                uint32_t n) {
    uint32_t i;
    float th;
    float atk;
    float rel;
    float floor;

    if (state == NULL || config == NULL || in == NULL || out == NULL ||
        n == 0u) {
        return;
    }

    th = config->threshold;
    atk = bm_algo_clamp_f(config->attack_coeff, 0.0f, 1.0f);
    rel = bm_algo_clamp_f(config->release_coeff, 0.0f, 1.0f);
    floor = bm_algo_clamp_f(config->floor_gain, 0.0f, 1.0f);

    for (i = 0u; i < n; ++i) {
        float x = in[i];
        float ax = fabsf(x);
        float target;
        float coeff;

        coeff = (ax > state->envelope) ? atk : rel;
        state->envelope += coeff * (ax - state->envelope);

        target = (state->envelope >= th) ? 1.0f : floor;
        coeff = (target > state->gain) ? atk : rel;
        state->gain += coeff * (target - state->gain);
        out[i] = x * state->gain;
    }
}

static float gcc_phat_corr_at_lag(const float *ref,
                                  const float *sig,
                                  uint32_t n,
                                  int32_t lag) {
    uint32_t i;
    float num = 0.0f;
    float den_ref = 0.0f;
    float den_sig = 0.0f;
    int32_t j;

    for (i = 0u; i < n; ++i) {
        j = (int32_t)i + lag;
        if (j < 0 || (uint32_t)j >= n) {
            continue;
        }
        num += ref[i] * sig[(uint32_t)j];
        den_ref += ref[i] * ref[i];
        den_sig += sig[(uint32_t)j] * sig[(uint32_t)j];
    }

    if (den_ref <= 1e-12f || den_sig <= 1e-12f) {
        return 0.0f;
    }
    return num / sqrtf(den_ref * den_sig);
}

int32_t bm_algo_gcc_phat_delay(const float *ref,
                               const float *sig,
                               uint32_t n,
                               int32_t max_lag) {
    int32_t lag;
    int32_t best_lag = 0;
    float best = -2.0f;
    float c;

    if (ref == NULL || sig == NULL || n == 0u || max_lag < 0) {
        return 0;
    }

    for (lag = -max_lag; lag <= max_lag; ++lag) {
        c = gcc_phat_corr_at_lag(ref, sig, n, lag);
        if (c > best) {
            best = c;
            best_lag = lag;
        }
    }

    return best_lag;
}
