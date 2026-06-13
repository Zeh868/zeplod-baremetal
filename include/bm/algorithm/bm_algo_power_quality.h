/**
 * @file bm_algo_power_quality.h
 * @brief 电能质量：THD 与 P/Q/S 计量
 *
 * @maturity E1
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
#ifndef BM_ALGO_POWER_QUALITY_H
#define BM_ALGO_POWER_QUALITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 由谐波幅值数组计算 THD（%），harmonics[0] 为基波 */
float bm_algo_thd_percent(const float *harmonics, uint32_t count);

/** 由 V/I RMS 与相位差（rad）计算有功/无功/视在功率 */
void bm_algo_power_quality_pq(float v_rms,
                              float i_rms,
                              float phase_rad,
                              float *p_active,
                              float *q_reactive,
                              float *s_apparent);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_POWER_QUALITY_H */
