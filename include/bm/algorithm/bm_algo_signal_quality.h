/**
 * @file bm_algo_signal_quality.h
 * @brief 信号质量：去抖、范围监控、变化率与冻结值检测
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_SIGNAL_QUALITY_H
#define BM_ALGO_SIGNAL_QUALITY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t stable_count_required;
    float    tolerance;
} bm_algo_debounce_analog_config_t;

typedef struct {
    float    candidate;
    uint32_t stable_count;
    float    latched;
    int      valid;
} bm_algo_debounce_analog_state_t;

void bm_algo_debounce_analog_reset(bm_algo_debounce_analog_state_t *state,
                                   float initial);
int bm_algo_debounce_analog_step(bm_algo_debounce_analog_state_t *state,
                                 const bm_algo_debounce_analog_config_t *config,
                                 float sample);

typedef struct {
    float min_v;
    float max_v;
    float max_rate_per_s;
} bm_algo_range_monitor_config_t;

typedef struct {
    float prev;
    uint32_t fault_flags;
} bm_algo_range_monitor_state_t;

#define BM_ALGO_FAULT_UNDER_RANGE  (1u << 0)
#define BM_ALGO_FAULT_OVER_RANGE   (1u << 1)
#define BM_ALGO_FAULT_RATE         (1u << 2)
#define BM_ALGO_FAULT_FROZEN       (1u << 3)

void bm_algo_range_monitor_reset(bm_algo_range_monitor_state_t *state, float v);
uint32_t bm_algo_range_monitor_step(bm_algo_range_monitor_state_t *state,
                                    const bm_algo_range_monitor_config_t *config,
                                    float sample,
                                    float dt_s);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_SIGNAL_QUALITY_H */
