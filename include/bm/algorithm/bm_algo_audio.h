/**
 * @file bm_algo_audio.h
 * @brief 音频数学核：增益、混音、均衡、限幅、AGC 与简易 VAD
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            正式发布
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

/* ---------- Peaking EQ（Biquad 封装） ---------- */
typedef struct {
    float sample_hz;
    float freq_hz;
    float q;
    float gain_db;
} bm_algo_eq_peaking_config_t;

typedef struct {
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
    float z1;
    float z2;
} bm_algo_eq_peaking_state_t;

int bm_algo_eq_peaking_design(bm_algo_eq_peaking_state_t *state,
                              const bm_algo_eq_peaking_config_t *config);
void bm_algo_eq_peaking_reset(bm_algo_eq_peaking_state_t *state);
void bm_algo_eq_peaking_process(bm_algo_eq_peaking_state_t *state,
                                const bm_algo_eq_peaking_config_t *config,
                                const float *in,
                                float *out,
                                uint32_t n);

/* ---------- 动态压缩 ---------- */
typedef struct {
    float threshold;
    float ratio;
    float attack_coeff;
    float release_coeff;
    float makeup_gain;
} bm_algo_compressor_config_t;

typedef struct {
    float envelope;
} bm_algo_compressor_state_t;

void bm_algo_compressor_reset(bm_algo_compressor_state_t *state);
void bm_algo_compressor_process(bm_algo_compressor_state_t *state,
                                const bm_algo_compressor_config_t *config,
                                const float *in,
                                float *out,
                                uint32_t n);

/* ---------- 噪声门 ---------- */
typedef struct {
    float threshold;
    float attack_coeff;
    float release_coeff;
    float floor_gain;
} bm_algo_noise_gate_config_t;

typedef struct {
    float envelope;
    float gain;
} bm_algo_noise_gate_state_t;

void bm_algo_noise_gate_reset(bm_algo_noise_gate_state_t *state);
void bm_algo_noise_gate_process(bm_algo_noise_gate_state_t *state,
                                const bm_algo_noise_gate_config_t *config,
                                const float *in,
                                float *out,
                                uint32_t n);

/* ---------- GCC-PHAT 时延估计 ---------- */
/** 正滞后：sig 相对 ref 延迟的采样数；失败返回本常量 */
#define BM_ALGO_GCC_PHAT_DELAY_INVALID  INT32_MIN

/** 工作区 float 个数：2 路复数谱，长度 4 * fft_n */
uint32_t bm_algo_gcc_phat_work_count(uint32_t n, int32_t max_lag);

int32_t bm_algo_gcc_phat_delay(const float *ref,
                               const float *sig,
                               uint32_t n,
                               int32_t max_lag,
                               float *work,
                               uint32_t work_count);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_AUDIO_H */
