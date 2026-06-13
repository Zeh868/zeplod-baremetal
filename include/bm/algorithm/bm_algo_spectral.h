/**
 * @file bm_algo_spectral.h
 * @brief 频谱分析：Goertzel、PSD、包络、相关与谱峰
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_SPECTRAL_H
#define BM_ALGO_SPECTRAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Goertzel 单频检测 ---------- */
typedef struct {
    float target_freq_hz;
    float sample_hz;
    uint32_t block_size;
    float coeff;
} bm_algo_goertzel_config_t;

typedef struct {
    float s_prev;
    float s_prev2;
    float coeff;
    uint32_t count;
} bm_algo_goertzel_state_t;

int bm_algo_goertzel_init(bm_algo_goertzel_state_t *state,
                          const bm_algo_goertzel_config_t *config);
void bm_algo_goertzel_reset(bm_algo_goertzel_state_t *state);
int bm_algo_goertzel_feed(bm_algo_goertzel_state_t *state,
                          const bm_algo_goertzel_config_t *config,
                          float sample);
float bm_algo_goertzel_result(bm_algo_goertzel_state_t *state,
                              const bm_algo_goertzel_config_t *config);

/* ---------- PSD（周期图，输入为幅度谱平方） ---------- */
void bm_algo_psd_from_spectrum(const float *mag,
                               uint32_t bin_count,
                               float scale,
                               float *psd);

/* ---------- Hilbert 包络（FIR 近似 + 解析信号幅值） ---------- */
typedef struct {
    float prev;
    float envelope;
    float alpha;
} bm_algo_envelope_state_t;

void bm_algo_envelope_reset(bm_algo_envelope_state_t *state);
float bm_algo_envelope_step(bm_algo_envelope_state_t *state, float input);

/* ---------- 互相关（有限长度） ---------- */
float bm_algo_correlate(const float *a,
                        const float *b,
                        uint32_t len);

/* ---------- 谱峰搜索 ---------- */
int bm_algo_find_peak_bin(const float *spectrum,
                          uint32_t start_bin,
                          uint32_t end_bin,
                          uint32_t *peak_bin,
                          float *peak_value);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_SPECTRAL_H */
