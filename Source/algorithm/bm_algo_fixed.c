/**
 * @file bm_algo_fixed.c
 * @brief Q31/Q15 定点算法实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            正式发布
 * 2026-06-13       1.1            zeh            增加 PID Q31 与 Biquad Q15
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_fixed.h"

#include <limits.h>
#include <math.h>

bm_algo_q31_t bm_algo_clamp_q31(bm_algo_q31_t value,
                                bm_algo_q31_t min_v,
                                bm_algo_q31_t max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

bm_algo_q15_t bm_algo_clamp_q15(bm_algo_q15_t value,
                                bm_algo_q15_t min_v,
                                bm_algo_q15_t max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

bm_algo_q31_t bm_algo_float_to_q31(float value) {
    float scaled;

    if (isnan(value)) {
        return 0;
    }
    if (value >= 1.0f) {
        return BM_ALGO_Q31_ONE;
    }
    if (value <= -1.0f) {
        return (bm_algo_q31_t)INT32_MIN;
    }
    scaled = value * 2147483648.0f;
    if (scaled > 2147483647.0f) {
        return BM_ALGO_Q31_ONE;
    }
    if (scaled < -2147483648.0f) {
        return (bm_algo_q31_t)INT32_MIN;
    }
    return (bm_algo_q31_t)scaled;
}

float bm_algo_q31_to_float(bm_algo_q31_t value) {
    return (float)value / 2147483648.0f;
}

bm_algo_q15_t bm_algo_float_to_q15(float value) {
    float scaled;

    if (isnan(value)) {
        return 0;
    }
    if (value >= 1.0f) {
        return BM_ALGO_Q15_ONE;
    }
    if (value <= -1.0f) {
        return (bm_algo_q15_t)(-32768);
    }
    scaled = value * 32768.0f;
    if (scaled > 32767.0f) {
        return BM_ALGO_Q15_ONE;
    }
    if (scaled < -32768.0f) {
        return (bm_algo_q15_t)(-32768);
    }
    return (bm_algo_q15_t)scaled;
}

float bm_algo_q15_to_float(bm_algo_q15_t value) {
    return (float)value / 32768.0f;
}

void bm_algo_pi_q31_reset(bm_algo_pi_q31_state_t *state, bm_algo_q31_t output) {
    if (state != NULL) {
        state->integrator = 0;
        state->output = output;
    }
}

static bm_algo_q31_t saturate_q31_i64(int64_t value);
static bm_algo_q15_t saturate_q15_i32(int32_t value);

static bm_algo_q31_t mul_q31(bm_algo_q31_t a, bm_algo_q31_t b) {
    int64_t prod = (int64_t)a * (int64_t)b;
    int64_t scaled = prod >> 31;

    return saturate_q31_i64(scaled);
}

static bm_algo_q31_t saturate_q31_i64(int64_t value) {
    if (value > (int64_t)INT32_MAX) {
        return (bm_algo_q31_t)INT32_MAX;
    }
    if (value < (int64_t)INT32_MIN) {
        return (bm_algo_q31_t)INT32_MIN;
    }
    return (bm_algo_q31_t)value;
}

static bm_algo_q15_t saturate_q15_i32(int32_t value) {
    if (value > 32767) {
        return (bm_algo_q15_t)32767;
    }
    if (value < -32768) {
        return (bm_algo_q15_t)-32768;
    }
    return (bm_algo_q15_t)value;
}

static uint64_t magnitude_i64(int64_t value) {
    return (value < 0) ? (uint64_t)(-(value + 1)) + 1u
                       : (uint64_t)value;
}

static uint64_t magnitude_i32(int32_t value) {
    return (value < 0) ? (uint64_t)(-(int64_t)value)
                       : (uint64_t)value;
}

static bm_algo_q31_t div_q31(int64_t num, bm_algo_q31_t den) {
    uint64_t num_mag;
    uint64_t den_mag;
    uint64_t scaled;
    int negative;

    if (den == 0) {
        return 0;
    }

    negative = ((num < 0) != (den < 0));
    num_mag = magnitude_i64(num);
    den_mag = magnitude_i32(den);
    if (num_mag >= den_mag) {
        return negative ? (bm_algo_q31_t)INT32_MIN
                        : (bm_algo_q31_t)INT32_MAX;
    }

    scaled = (num_mag << 31u) / den_mag;
    if (negative) {
        return (scaled >= 0x80000000ull)
                   ? (bm_algo_q31_t)INT32_MIN
                   : (bm_algo_q31_t)(-(int64_t)scaled);
    }
    return (scaled > (uint64_t)INT32_MAX)
               ? (bm_algo_q31_t)INT32_MAX
               : (bm_algo_q31_t)scaled;
}

bm_algo_q31_t bm_algo_pi_q31_step(bm_algo_pi_q31_state_t *state,
                                  const bm_algo_pi_q31_config_t *config,
                                  bm_algo_q31_t error,
                                  bm_algo_q31_t dt_q31) {
    bm_algo_q31_t p_term;
    bm_algo_q31_t u_unsat;
    bm_algo_q31_t u_sat;

    if (state == NULL || config == NULL || dt_q31 <= 0) {
        return 0;
    }

    p_term = mul_q31(config->kp, error);
    state->integrator = saturate_q31_i64(
        (int64_t)state->integrator + (int64_t)mul_q31(error, dt_q31));
    state->integrator = bm_algo_clamp_q31(state->integrator,
                                          config->integrator_min,
                                          config->integrator_max);

    {
        int64_t u_sum = (int64_t)p_term +
                        (int64_t)mul_q31(config->ki, state->integrator);

        if (u_sum > (int64_t)INT32_MAX) {
            u_unsat = (bm_algo_q31_t)INT32_MAX;
        } else if (u_sum < (int64_t)INT32_MIN) {
            u_unsat = (bm_algo_q31_t)INT32_MIN;
        } else {
            u_unsat = (bm_algo_q31_t)u_sum;
        }
    }
    u_sat = bm_algo_clamp_q31(u_unsat, config->out_min, config->out_max);

    if (config->ki != 0 && u_sat != u_unsat) {
        state->integrator = div_q31(
            (int64_t)u_sat - (int64_t)p_term, config->ki);
        state->integrator = bm_algo_clamp_q31(state->integrator,
                                              config->integrator_min,
                                              config->integrator_max);
    }

    state->output = u_sat;
    return state->output;
}

void bm_algo_lpf1_q15_reset(bm_algo_lpf1_q15_state_t *state, bm_algo_q15_t output) {
    if (state != NULL) {
        state->output = output;
    }
}

bm_algo_q15_t bm_algo_lpf1_q15_step(bm_algo_lpf1_q15_state_t *state,
                                    const bm_algo_lpf1_q15_config_t *config,
                                    bm_algo_q15_t input) {
    int32_t delta;
    int32_t y;

    if (state == NULL || config == NULL) {
        return input;
    }

    delta = ((int32_t)input - (int32_t)state->output) *
            (int32_t)config->alpha_q15;
    y = (int32_t)state->output + (delta >> 15);
    state->output = saturate_q15_i32(y);
    return state->output;
}

void bm_algo_pid_q31_reset(bm_algo_pid_q31_state_t *state, bm_algo_q31_t output) {
    if (state != NULL) {
        state->integrator = 0;
        state->prev_error = 0;
        state->d_filtered = 0;
        state->output = output;
    }
}

bm_algo_q31_t bm_algo_pid_q31_step(bm_algo_pid_q31_state_t *state,
                                   const bm_algo_pid_q31_config_t *config,
                                   bm_algo_q31_t error,
                                   bm_algo_q31_t dt_q31) {
    bm_algo_q31_t p_term;
    bm_algo_q31_t d_raw;
    bm_algo_q31_t d_term;
    bm_algo_q31_t u_unsat;
    bm_algo_q31_t u_sat;
    bm_algo_q31_t alpha;

    if (state == NULL || config == NULL || dt_q31 <= 0) {
        return 0;
    }

    p_term = mul_q31(config->kp, error);

    state->integrator = saturate_q31_i64(
        (int64_t)state->integrator + (int64_t)mul_q31(error, dt_q31));
    state->integrator = bm_algo_clamp_q31(state->integrator,
                                          config->integrator_min,
                                          config->integrator_max);

    d_raw = div_q31(
        (int64_t)error - (int64_t)state->prev_error, dt_q31);
    state->prev_error = error;

    alpha = bm_algo_clamp_q31(config->d_filter_alpha_q31, 0, BM_ALGO_Q31_ONE);
    state->d_filtered = saturate_q31_i64(
        (int64_t)state->d_filtered +
        (int64_t)mul_q31(
            alpha,
            saturate_q31_i64(
                (int64_t)d_raw - (int64_t)state->d_filtered)));
    d_term = mul_q31(config->kd, state->d_filtered);

    {
        int64_t u_sum = (int64_t)p_term +
                        (int64_t)mul_q31(config->ki, state->integrator) +
                        (int64_t)d_term;

        if (u_sum > (int64_t)INT32_MAX) {
            u_unsat = (bm_algo_q31_t)INT32_MAX;
        } else if (u_sum < (int64_t)INT32_MIN) {
            u_unsat = (bm_algo_q31_t)INT32_MIN;
        } else {
            u_unsat = (bm_algo_q31_t)u_sum;
        }
    }
    u_sat = bm_algo_clamp_q31(u_unsat, config->out_min, config->out_max);

    if (config->ki != 0 && u_sat != u_unsat) {
        state->integrator = div_q31(
            (int64_t)u_sat - (int64_t)p_term - (int64_t)d_term,
            config->ki);
        state->integrator = bm_algo_clamp_q31(state->integrator,
                                              config->integrator_min,
                                              config->integrator_max);
    }

    state->output = u_sat;
    return state->output;
}

void bm_algo_biquad_q15_reset(bm_algo_biquad_q15_state_t *state) {
    if (state != NULL) {
        state->z1 = 0;
        state->z2 = 0;
    }
}

static bm_algo_q15_t mul_q15(bm_algo_q15_t a, bm_algo_q15_t b) {
    int32_t prod = (int32_t)a * (int32_t)b;

    return saturate_q15_i32(prod >> 15);
}

bm_algo_q15_t bm_algo_biquad_q15_step(bm_algo_biquad_q15_state_t *state,
                                      const bm_algo_biquad_q15_config_t *config,
                                      bm_algo_q15_t input) {
    bm_algo_q15_t output;
    int32_t acc;

    if (state == NULL || config == NULL) {
        return input;
    }

    acc = (int32_t)mul_q15(config->b0, input) + (int32_t)state->z1;
    output = saturate_q15_i32(acc);

    acc = (int32_t)mul_q15(config->b1, input) -
          (int32_t)mul_q15(config->a1, output) + (int32_t)state->z2;
    state->z1 = saturate_q15_i32(acc);

    acc = (int32_t)mul_q15(config->b2, input) -
          (int32_t)mul_q15(config->a2, output);
    state->z2 = saturate_q15_i32(acc);

    return output;
}
