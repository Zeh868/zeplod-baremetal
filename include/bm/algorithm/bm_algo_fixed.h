/**
 * @file bm_algo_fixed.h
 * @brief 定点算法核：Q31/Q15 显式后缀 API（与 float 并存）
 *
 * Q31：1.0 ≈ 0x7FFFFFFF；Q15：1.0 = 32767。系数与信号均按 ±1.0 归一化。
 * 与 float 核分文件、分符号，不使用全局 typedef 在编译期切换 ABI。
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
#ifndef BM_ALGO_FIXED_H
#define BM_ALGO_FIXED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t bm_algo_q31_t;
typedef int16_t bm_algo_q15_t;

#define BM_ALGO_Q31_ONE   ((bm_algo_q31_t)2147483647)
#define BM_ALGO_Q15_ONE   ((bm_algo_q15_t)32767)

bm_algo_q31_t bm_algo_clamp_q31(bm_algo_q31_t value,
                                bm_algo_q31_t min_v,
                                bm_algo_q31_t max_v);

bm_algo_q15_t bm_algo_clamp_q15(bm_algo_q15_t value,
                                bm_algo_q15_t min_v,
                                bm_algo_q15_t max_v);

/** float → Q31（饱和） */
bm_algo_q31_t bm_algo_float_to_q31(float value);

/** Q31 → float */
float bm_algo_q31_to_float(bm_algo_q31_t value);

/** float → Q15（饱和） */
bm_algo_q15_t bm_algo_float_to_q15(float value);

/** Q15 → float */
float bm_algo_q15_to_float(bm_algo_q15_t value);

typedef struct {
    bm_algo_q31_t kp;
    bm_algo_q31_t ki;
    bm_algo_q31_t out_min;
    bm_algo_q31_t out_max;
    bm_algo_q31_t integrator_min;
    bm_algo_q31_t integrator_max;
} bm_algo_pi_q31_config_t;

typedef struct {
    bm_algo_q31_t integrator;
    bm_algo_q31_t output;
} bm_algo_pi_q31_state_t;

void bm_algo_pi_q31_reset(bm_algo_pi_q31_state_t *state, bm_algo_q31_t output);

bm_algo_q31_t bm_algo_pi_q31_step(bm_algo_pi_q31_state_t *state,
                                  const bm_algo_pi_q31_config_t *config,
                                  bm_algo_q31_t error,
                                  bm_algo_q31_t dt_q31);

typedef struct {
    bm_algo_q15_t alpha_q15;
} bm_algo_lpf1_q15_config_t;

typedef struct {
    bm_algo_q15_t output;
} bm_algo_lpf1_q15_state_t;

void bm_algo_lpf1_q15_reset(bm_algo_lpf1_q15_state_t *state, bm_algo_q15_t output);

bm_algo_q15_t bm_algo_lpf1_q15_step(bm_algo_lpf1_q15_state_t *state,
                                    const bm_algo_lpf1_q15_config_t *config,
                                    bm_algo_q15_t input);

typedef struct {
    bm_algo_q31_t kp;
    bm_algo_q31_t ki;
    bm_algo_q31_t kd;
    bm_algo_q31_t out_min;
    bm_algo_q31_t out_max;
    bm_algo_q31_t integrator_min;
    bm_algo_q31_t integrator_max;
    bm_algo_q31_t d_filter_alpha_q31;
} bm_algo_pid_q31_config_t;

typedef struct {
    bm_algo_q31_t integrator;
    bm_algo_q31_t prev_error;
    bm_algo_q31_t d_filtered;
    bm_algo_q31_t output;
} bm_algo_pid_q31_state_t;

void bm_algo_pid_q31_reset(bm_algo_pid_q31_state_t *state, bm_algo_q31_t output);

bm_algo_q31_t bm_algo_pid_q31_step(bm_algo_pid_q31_state_t *state,
                                   const bm_algo_pid_q31_config_t *config,
                                   bm_algo_q31_t error,
                                   bm_algo_q31_t dt_q31);

typedef struct {
    bm_algo_q15_t b0;
    bm_algo_q15_t b1;
    bm_algo_q15_t b2;
    bm_algo_q15_t a1;
    bm_algo_q15_t a2;
} bm_algo_biquad_q15_config_t;

typedef struct {
    bm_algo_q15_t z1;
    bm_algo_q15_t z2;
} bm_algo_biquad_q15_state_t;

void bm_algo_biquad_q15_reset(bm_algo_biquad_q15_state_t *state);

bm_algo_q15_t bm_algo_biquad_q15_step(bm_algo_biquad_q15_state_t *state,
                                      const bm_algo_biquad_q15_config_t *config,
                                      bm_algo_q15_t input);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_FIXED_H */
