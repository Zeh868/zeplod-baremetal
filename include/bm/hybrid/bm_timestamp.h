/**
 * @file bm_timestamp.h
 * @brief 单调时钟域时间戳（采样/块流用）
 *
 * 为块流、融合与多媒体算法提供可比较的采样时刻；支持单调回绕比较辅助函数。
 * 时钟域标识与质量字段供多 ADC、音频与相机时间对齐使用。
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-12
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-12       1.0            zeh            正式发布
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_TIMESTAMP_H
#define BM_TIMESTAMP_H

#include <stdint.h>

/** HRT 定时器时钟域（与 bm_hal_timer 对齐） */
#define BM_TIMESTAMP_CLOCK_HRT  0u

typedef struct {
    uint16_t clock_id;
    uint16_t quality;
    uint32_t ticks;
    uint32_t rate_hz;
} bm_timestamp_t;

static inline int bm_timestamp_before(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) < 0;
}

static inline int bm_timestamp_after(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) > 0;
}

#endif /* BM_TIMESTAMP_H */
