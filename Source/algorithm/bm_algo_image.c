/**
 * @file bm_algo_image.c
 * @brief 低分辨率图像算子实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_image.h"

#include <string.h>

void bm_algo_image_threshold_u8(const uint8_t *src, uint8_t *dst,
                              uint32_t width, uint32_t height,
                              uint8_t thresh, uint8_t max_val) {
    uint32_t i;
    uint32_t n;

    if (src == NULL || dst == NULL) {
        return;
    }

    n = width * height;
    for (i = 0u; i < n; ++i) {
        dst[i] = (src[i] >= thresh) ? max_val : 0u;
    }
}

void bm_algo_image_erode_u8(const uint8_t *src, uint8_t *dst,
                            uint32_t width, uint32_t height) {
    uint32_t x;
    uint32_t y;

    if (src == NULL || dst == NULL || width < 3u || height < 3u) {
        return;
    }

    for (y = 1u; y + 1u < height; ++y) {
        for (x = 1u; x + 1u < width; ++x) {
            uint32_t idx = y * width + x;
            uint8_t min_v = 255u;
            int32_t dy;
            int32_t dx;

            for (dy = -1; dy <= 1; ++dy) {
                for (dx = -1; dx <= 1; ++dx) {
                    uint8_t v = src[(y + (uint32_t)dy) * width + (x + (uint32_t)dx)];
                    if (v < min_v) {
                        min_v = v;
                    }
                }
            }
            dst[idx] = min_v;
        }
    }
}

void bm_algo_image_dilate_u8(const uint8_t *src, uint8_t *dst,
                             uint32_t width, uint32_t height) {
    uint32_t x;
    uint32_t y;

    if (src == NULL || dst == NULL || width < 3u || height < 3u) {
        return;
    }

    for (y = 1u; y + 1u < height; ++y) {
        for (x = 1u; x + 1u < width; ++x) {
            uint32_t idx = y * width + x;
            uint8_t max_v = 0u;
            int32_t dy;
            int32_t dx;

            for (dy = -1; dy <= 1; ++dy) {
                for (dx = -1; dx <= 1; ++dx) {
                    uint8_t v = src[(y + (uint32_t)dy) * width + (x + (uint32_t)dx)];
                    if (v > max_v) {
                        max_v = v;
                    }
                }
            }
            dst[idx] = max_v;
        }
    }
}

int bm_algo_image_label_u8(const uint8_t *binary,
                           uint16_t *labels,
                           uint32_t width,
                           uint32_t height,
                           bm_algo_blob_info_t *blobs,
                           uint32_t max_blobs) {
    uint32_t x;
    uint32_t y;
    uint16_t next_label = 1u;
    uint32_t blob_count = 0u;

    if (binary == NULL || labels == NULL || width == 0u || height == 0u ||
        (blobs == NULL && max_blobs != 0u)) {
        return -1;
    }

    memset(labels, 0, width * height * sizeof(uint16_t));

    for (y = 0u; y < height; ++y) {
        for (x = 0u; x < width; ++x) {
            uint32_t idx = y * width + x;

            if (binary[idx] == 0u || labels[idx] != 0u) {
                continue;
            }

            if (next_label == 0u) {
                return -1;
            }

            labels[idx] = next_label;
            {
                uint32_t area = 1u;
                uint64_t sum_x = x;
                uint64_t sum_y = y;
                int changed;

                /*
                 * labels[] doubles as the visited map. Repeated scans avoid a
                 * caller-supplied queue while still producing true 4-connected
                 * components.
                 */
                do {
                    uint32_t scan_x;
                    uint32_t scan_y;

                    changed = 0;
                    for (scan_y = 0u; scan_y < height; ++scan_y) {
                        for (scan_x = 0u; scan_x < width; ++scan_x) {
                            uint32_t current = scan_y * width + scan_x;
                            int connected = 0;

                            if (binary[current] == 0u || labels[current] != 0u) {
                                continue;
                            }
                            if (scan_x > 0u &&
                                labels[current - 1u] == next_label) {
                                connected = 1;
                            } else if (scan_x + 1u < width &&
                                       labels[current + 1u] == next_label) {
                                connected = 1;
                            } else if (scan_y > 0u &&
                                       labels[current - width] == next_label) {
                                connected = 1;
                            } else if (scan_y + 1u < height &&
                                       labels[current + width] == next_label) {
                                connected = 1;
                            }

                            if (connected) {
                                labels[current] = next_label;
                                area++;
                                sum_x += scan_x;
                                sum_y += scan_y;
                                changed = 1;
                            }
                        }
                    }
                } while (changed);

                if (blobs != NULL && blob_count < max_blobs) {
                    blobs[blob_count].label = next_label;
                    blobs[blob_count].area = area;
                    blobs[blob_count].cx = (int32_t)(sum_x / area);
                    blobs[blob_count].cy = (int32_t)(sum_y / area);
                }
            }

            if (blobs == NULL || blob_count < max_blobs) {
                blob_count++;
            }
            next_label++;
        }
    }

    return (int)blob_count;
}

void bm_algo_image_frame_diff_u8(const uint8_t *prev,
                                 const uint8_t *curr,
                                 uint8_t *diff,
                                 uint32_t width,
                                 uint32_t height,
                                 uint8_t thresh) {
    uint32_t i;
    uint32_t n;

    if (prev == NULL || curr == NULL || diff == NULL) {
        return;
    }

    n = width * height;
    for (i = 0u; i < n; ++i) {
        int16_t d = (int16_t)curr[i] - (int16_t)prev[i];
        if (d < 0) {
            d = -d;
        }
        diff[i] = ((uint8_t)d >= thresh) ? 255u : 0u;
    }
}
