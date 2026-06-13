/**
 * @file bm_algo_audio.h
 * @brief 音频数学核：增益、混音、均衡、限幅、AGC 与简易 VAD
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_AUDIO_H
#define BM_ALGO_AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bm_algo_audio_gain(const float *in, float *out, uint32_t n, float gain);
void bm_algo_audio_mix2(const float *a, const float *b, float *out,
                        uint32_t n, float gain_a, float gain_b);

typedef struct {
    float threshold;
    float knee;
} bm_algo_limiter_config_t;

void bm_algo_limiter_process(const float *in, float *out, uint32_t n,
                             const bm_algo_limiter_config_t *config);

typedef struct {
    float target_level;
    float attack_coeff;
    float release_coeff;
    float gain;
} bm_algo_agc_config_t;

typedef struct {
    float gain;
} bm_algo_agc_state_t;

void bm_algo_agc_reset(bm_algo_agc_state_t *state, float gain_init);
void bm_algo_agc_process(bm_algo_agc_state_t *state,
                         const bm_algo_agc_config_t *config,
                         const float *in,
                         float *out,
                         uint32_t n);

typedef struct {
    float energy_threshold;
    float alpha;
} bm_algo_vad_config_t;

typedef struct {
    float energy;
    int   voice_active;
} bm_algo_vad_state_t;

void bm_algo_vad_reset(bm_algo_vad_state_t *state);
void bm_algo_vad_process(bm_algo_vad_state_t *state,
                         const bm_algo_vad_config_t *config,
                         const float *in,
                         uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_AUDIO_H */
