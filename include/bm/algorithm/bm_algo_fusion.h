/**
 * @file bm_algo_fusion.h
 * @brief 姿态融合：互补滤波、Mahony 与 Madgwick
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_FUSION_H
#define BM_ALGO_FUSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float roll_rad;
    float pitch_rad;
    float yaw_rad;
} bm_algo_euler_t;

typedef struct {
    float w;
    float x;
    float y;
    float z;
} bm_algo_quat_t;

/* ---------- 互补滤波（仅 roll/pitch） ---------- */
typedef struct {
    float alpha;  /**< 陀螺权重 */
} bm_algo_complementary_config_t;

typedef struct {
    float roll_rad;
    float pitch_rad;
} bm_algo_complementary_state_t;

void bm_algo_complementary_reset(bm_algo_complementary_state_t *state);
void bm_algo_complementary_step(bm_algo_complementary_state_t *state,
                                const bm_algo_complementary_config_t *config,
                                float gx, float gy, float gz,
                                float ax, float ay, float az,
                                float dt_s);

/* ---------- Mahony AHRS ---------- */
typedef struct {
    float kp;
    float ki;
} bm_algo_mahony_config_t;

typedef struct {
    bm_algo_quat_t q;
    float integral_x;
    float integral_y;
    float integral_z;
} bm_algo_mahony_state_t;

void bm_algo_mahony_reset(bm_algo_mahony_state_t *state);
void bm_algo_mahony_step(bm_algo_mahony_state_t *state,
                         const bm_algo_mahony_config_t *config,
                         float gx, float gy, float gz,
                         float ax, float ay, float az,
                         float dt_s);
void bm_algo_quat_to_euler(const bm_algo_quat_t *q, bm_algo_euler_t *euler);

/* ---------- Madgwick AHRS ---------- */
typedef struct {
    float beta;
} bm_algo_madgwick_config_t;

typedef struct {
    bm_algo_quat_t q;
} bm_algo_madgwick_state_t;

void bm_algo_madgwick_reset(bm_algo_madgwick_state_t *state);
void bm_algo_madgwick_step(bm_algo_madgwick_state_t *state,
                           const bm_algo_madgwick_config_t *config,
                           float gx, float gy, float gz,
                           float ax, float ay, float az,
                           float dt_s);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_FUSION_H */
