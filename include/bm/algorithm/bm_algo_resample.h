/**
 * @file bm_algo_resample.h
 * @brief 重采样：抽取、线性插值
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_RESAMPLE_H
#define BM_ALGO_RESAMPLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 整数抽取（每 decim 取 1 点） */
typedef struct {
    uint32_t decim;
    uint32_t counter;
} bm_algo_decimator_state_t;

void bm_algo_decimator_reset(bm_algo_decimator_state_t *state);
int bm_algo_decimator_step(bm_algo_decimator_state_t *state,
                           uint32_t decim,
                           float input,
                           float *output);

/** 线性插值重采样（固定比率） */
typedef struct {
    float ratio;       /**< 输出/输入采样率 */
    float phase;
    float prev_sample;
} bm_algo_linear_resampler_state_t;

void bm_algo_linear_resampler_reset(bm_algo_linear_resampler_state_t *state,
                                    float ratio,
                                    float initial);
int bm_algo_linear_resampler_step(bm_algo_linear_resampler_state_t *state,
                                  float input,
                                  float *outputs,
                                  uint32_t max_outputs,
                                  uint32_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_RESAMPLE_H */
