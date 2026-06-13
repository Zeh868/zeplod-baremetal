/**
 * @file bm_algo_motor.c
 * @brief 电机数学核：Clarke/Park 与 SVPWM 实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_motor.h"
#include "bm/algorithm/bm_algo_common.h"

#include <math.h>

#ifndef BM_ALGO_SQRT3_F
#define BM_ALGO_SQRT3_F 1.7320508075688772f
#endif

void bm_algo_clarke(const bm_algo_abc_t *abc, bm_algo_alphabeta_t *ab) {
    if (abc == NULL || ab == NULL) {
        return;
    }
    ab->i_alpha = abc->ia;
    ab->i_beta = (abc->ia + 2.0f * abc->ib) / BM_ALGO_SQRT3_F;
}

void bm_algo_clarke_2shunt(float ia, float ib, bm_algo_alphabeta_t *ab) {
    if (ab == NULL) {
        return;
    }
    ab->i_alpha = ia;
    ab->i_beta = (ia + 2.0f * ib) / BM_ALGO_SQRT3_F;
}

void bm_algo_park(const bm_algo_alphabeta_t *ab,
                  float theta_rad,
                  bm_algo_dq_t *dq) {
    float c;
    float s;

    if (ab == NULL || dq == NULL) {
        return;
    }

    c = cosf(theta_rad);
    s = sinf(theta_rad);
    dq->id =  c * ab->i_alpha + s * ab->i_beta;
    dq->iq = -s * ab->i_alpha + c * ab->i_beta;
}

void bm_algo_inv_park(const bm_algo_dq_t *dq,
                      float theta_rad,
                      bm_algo_alphabeta_t *ab) {
    float c;
    float s;

    if (dq == NULL || ab == NULL) {
        return;
    }

    c = cosf(theta_rad);
    s = sinf(theta_rad);
    ab->i_alpha = c * dq->id - s * dq->iq;
    ab->i_beta  = s * dq->id + c * dq->iq;
}

void bm_algo_inv_clarke(const bm_algo_alphabeta_t *ab, bm_algo_abc_t *abc) {
    if (ab == NULL || abc == NULL) {
        return;
    }

    abc->ia = ab->i_alpha;
    abc->ib = -0.5f * ab->i_alpha + 0.5f * BM_ALGO_SQRT3_F * ab->i_beta;
    abc->ic = -0.5f * ab->i_alpha - 0.5f * BM_ALGO_SQRT3_F * ab->i_beta;
}

void bm_algo_voltage_limit(float *vd, float *vq, float v_max) {
    float mag;

    if (vd == NULL || vq == NULL || v_max <= 0.0f) {
        return;
    }

    mag = sqrtf((*vd) * (*vd) + (*vq) * (*vq));
    if (mag > v_max) {
        float scale = v_max / mag;
        *vd *= scale;
        *vq *= scale;
    }
}

void bm_algo_svpwm(float v_alpha,
                   float v_beta,
                   float vbus_v,
                   bm_algo_svpwm_out_t *out) {
    float va;
    float vb;
    float vc;
    float v_offset;
    float v_max;
    float v_min;
    float inv_vbus;

    if (out == NULL) {
        return;
    }

    if (vbus_v <= 0.0f) {
        vbus_v = 1.0f;
    }

    inv_vbus = 1.0f / vbus_v;

    /* 逆 Clarke 得相电压 */
    va = v_alpha;
    vb = -0.5f * v_alpha + 0.5f * BM_ALGO_SQRT3_F * v_beta;
    vc = -0.5f * v_alpha - 0.5f * BM_ALGO_SQRT3_F * v_beta;

    /* 中点注入（min-max 法） */
    v_max = va;
    if (vb > v_max) {
        v_max = vb;
    }
    if (vc > v_max) {
        v_max = vc;
    }

    v_min = va;
    if (vb < v_min) {
        v_min = vb;
    }
    if (vc < v_min) {
        v_min = vc;
    }

    v_offset = -0.5f * (v_max + v_min);
    va += v_offset;
    vb += v_offset;
    vc += v_offset;

    out->duty_a = bm_algo_clamp_f(0.5f + 0.5f * va * inv_vbus, 0.0f, 1.0f);
    out->duty_b = bm_algo_clamp_f(0.5f + 0.5f * vb * inv_vbus, 0.0f, 1.0f);
    out->duty_c = bm_algo_clamp_f(0.5f + 0.5f * vc * inv_vbus, 0.0f, 1.0f);
}
