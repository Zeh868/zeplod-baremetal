/**
 * @file bm_algo_estimator.h
 * @brief 固定维度状态估算：一维卡尔曼与简易 EKF 接口
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_ESTIMATOR_H
#define BM_ALGO_ESTIMATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- 一维卡尔曼滤波 ---------- */
typedef struct {
    float q;  /**< 过程噪声 */
    float r;  /**< 测量噪声 */
} bm_algo_kalman1d_config_t;

typedef struct {
    float x;
    float p;
} bm_algo_kalman1d_state_t;

void bm_algo_kalman1d_reset(bm_algo_kalman1d_state_t *state,
                            float x0,
                            float p0);
float bm_algo_kalman1d_predict(bm_algo_kalman1d_state_t *state,
                               const bm_algo_kalman1d_config_t *config);
float bm_algo_kalman1d_update(bm_algo_kalman1d_state_t *state,
                              const bm_algo_kalman1d_config_t *config,
                              float measurement);

/* ---------- 二维 EKF（位置-速度常速度模型） ---------- */
typedef struct {
    float q_pos;
    float q_vel;
    float r_pos;
} bm_algo_ekf_cv_config_t;

typedef struct {
    float pos;
    float vel;
    float p00, p01, p10, p11;
} bm_algo_ekf_cv_state_t;

void bm_algo_ekf_cv_reset(bm_algo_ekf_cv_state_t *state, float pos, float vel);
void bm_algo_ekf_cv_predict(bm_algo_ekf_cv_state_t *state,
                            const bm_algo_ekf_cv_config_t *config,
                            float dt_s);
void bm_algo_ekf_cv_update(bm_algo_ekf_cv_state_t *state,
                           const bm_algo_ekf_cv_config_t *config,
                           float pos_meas);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_ESTIMATOR_H */
