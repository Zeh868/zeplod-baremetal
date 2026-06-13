/**
 * @file bm_algo_features.c
 * @brief TinyML 前后处理实现
 *
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
#include "bm/algorithm/bm_algo_features.h"
#include "bm/algorithm/bm_algo_common.h"

#include <math.h>

int8_t bm_algo_quantize_f32_to_i8(float value, float scale, int32_t zero_point) {
    double transformed;
    long q;

    if (scale <= 0.0f || !bm_algo_is_finite_f(scale)) {
        return 0;
    }
    if (!bm_algo_is_finite_f(value)) {
        return (value > 0.0f) ? INT8_MAX :
               (value < 0.0f) ? INT8_MIN : 0;
    }

    transformed = (double)value / (double)scale + (double)zero_point;
    if (transformed >= (double)INT8_MAX) {
        return INT8_MAX;
    }
    if (transformed <= (double)INT8_MIN) {
        return INT8_MIN;
    }
    q = lround(transformed);
    if (q < -128) {
        q = -128;
    }
    if (q > 127) {
        q = 127;
    }
    return (int8_t)q;
}

float bm_algo_dequantize_i8_to_f32(int8_t value, float scale, int32_t zero_point) {
    return ((float)value - (float)zero_point) * scale;
}

void bm_algo_quantize_buffer_f32_i8(const float *in,
                                    int8_t *out,
                                    uint32_t n,
                                    float scale,
                                    int32_t zero_point) {
    uint32_t i;

    if (in == NULL || out == NULL) {
        return;
    }

    for (i = 0u; i < n; ++i) {
        out[i] = bm_algo_quantize_f32_to_i8(in[i], scale, zero_point);
    }
}

void bm_algo_log_mel_energy(const float *power_spectrum,
                            const float *mel_weights,
                            uint32_t mel_bins,
                            uint32_t fft_bins,
                            float *mel_out) {
    uint32_t m;
    uint32_t k;
    float e;

    if (power_spectrum == NULL || mel_weights == NULL || mel_out == NULL) {
        return;
    }

    for (m = 0u; m < mel_bins; ++m) {
        e = 0.0f;
        for (k = 0u; k < fft_bins; ++k) {
            e += power_spectrum[k] * mel_weights[m * fft_bins + k];
        }
        mel_out[m] = logf(e + 1e-10f);
    }
}
