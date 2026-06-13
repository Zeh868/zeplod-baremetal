/**
 * @file bm_algo_power_quality.c
 * @brief 电能质量：THD 与 P/Q/S 计量实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            初始版本
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_power_quality.h"

#include <math.h>

float bm_algo_thd_percent(const float *harmonics, uint32_t count) {
    float fundamental;
    float harm_sum_sq = 0.0f;
    uint32_t i;

    if (harmonics == NULL || count < 2u) {
        return 0.0f;
    }

    fundamental = harmonics[0];
    if (fundamental <= 1e-12f) {
        return 0.0f;
    }

    for (i = 1u; i < count; ++i) {
        harm_sum_sq += harmonics[i] * harmonics[i];
    }

    return sqrtf(harm_sum_sq) / fundamental * 100.0f;
}

void bm_algo_power_quality_pq(float v_rms,
                              float i_rms,
                              float phase_rad,
                              float *p_active,
                              float *q_reactive,
                              float *s_apparent) {
    float s;
    float cos_phi;
    float sin_phi;

    s = v_rms * i_rms;
    cos_phi = cosf(phase_rad);
    sin_phi = sinf(phase_rad);

    if (p_active != NULL) {
        *p_active = s * cos_phi;
    }
    if (q_reactive != NULL) {
        *q_reactive = s * sin_phi;
    }
    if (s_apparent != NULL) {
        *s_apparent = s;
    }
}
