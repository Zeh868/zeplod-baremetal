/**
 * @file bm_algo_image.h
 * @brief 低分辨率图像算子：阈值、形态学、连通域与帧差
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_IMAGE_H
#define BM_ALGO_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bm_algo_image_threshold_u8(const uint8_t *src, uint8_t *dst,
                              uint32_t width, uint32_t height,
                              uint8_t thresh, uint8_t max_val);

void bm_algo_image_erode_u8(const uint8_t *src, uint8_t *dst,
                            uint32_t width, uint32_t height);
void bm_algo_image_dilate_u8(const uint8_t *src, uint8_t *dst,
                             uint32_t width, uint32_t height);

typedef struct {
    uint32_t label;
    uint32_t area;
    int32_t  cx;
    int32_t  cy;
} bm_algo_blob_info_t;

#define BM_ALGO_MAX_BLOBS 32u

int bm_algo_image_label_u8(const uint8_t *binary,
                           uint16_t *labels,
                           uint32_t width,
                           uint32_t height,
                           bm_algo_blob_info_t *blobs,
                           uint32_t max_blobs);

void bm_algo_image_frame_diff_u8(const uint8_t *prev,
                                 const uint8_t *curr,
                                 uint8_t *diff,
                                 uint32_t width,
                                 uint32_t height,
                                 uint8_t thresh);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_IMAGE_H */
