/**
 * @file bm_algo_motion.h
 * @brief 运动辅助：编码器展开、速度估算与 DDA 插补
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_MOTION_H
#define BM_ALGO_MOTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- 编码器计数展开 ---------- */
typedef struct {
    uint32_t counts_per_rev;
    int32_t  prev_count;
    int32_t  turns;
    float    position_rad;
} bm_algo_encoder_config_t;

typedef struct {
    int32_t  prev_count;
    int32_t  turns;
    float    position_rad;
    float    velocity_rad_s;
} bm_algo_encoder_state_t;

void bm_algo_encoder_reset(bm_algo_encoder_state_t *state,
                           const bm_algo_encoder_config_t *config,
                           int32_t raw_count);
float bm_algo_encoder_update(bm_algo_encoder_state_t *state,
                             const bm_algo_encoder_config_t *config,
                             int32_t raw_count,
                             float dt_s);

/* ---------- DDA 直线插补（二维） ---------- */
typedef struct {
    float x0;
    float y0;
    float x1;
    float y1;
    float step_size;
} bm_algo_dda_config_t;

typedef struct {
    float x;
    float y;
    float err;
    int   done;
    int   step_x;
    int   step_y;
    float dx;
    float dy;
    float steps;
    float step_count;
} bm_algo_dda_state_t;

void bm_algo_dda_reset(bm_algo_dda_state_t *state,
                       const bm_algo_dda_config_t *config);
int bm_algo_dda_step(bm_algo_dda_state_t *state,
                     const bm_algo_dda_config_t *config,
                     float *x_out,
                     float *y_out);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_MOTION_H */
