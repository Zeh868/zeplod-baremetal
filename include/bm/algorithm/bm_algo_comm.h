/**
 * @file bm_algo_comm.h
 * @brief 通信 DSP：CRC、DTMF 检测（Goertzel 组）
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_COMM_H
#define BM_ALGO_COMM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t bm_algo_crc16_ccitt(const uint8_t *data, uint32_t len, uint16_t init);
uint32_t bm_algo_crc32(const uint8_t *data, uint32_t len, uint32_t init);

#define BM_ALGO_DTMF_ROW_COUNT 4u
#define BM_ALGO_DTMF_COL_COUNT 4u

typedef struct {
    float sample_hz;
    uint32_t block_size;
} bm_algo_dtmf_config_t;

typedef struct {
    float row_energy[BM_ALGO_DTMF_ROW_COUNT];
    float col_energy[BM_ALGO_DTMF_COL_COUNT];
} bm_algo_dtmf_state_t;

void bm_algo_dtmf_reset(bm_algo_dtmf_state_t *state);
int bm_algo_dtmf_detect(bm_algo_dtmf_state_t *state,
                        const bm_algo_dtmf_config_t *config,
                        const float *samples,
                        uint32_t n,
                        char *symbol_out);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_COMM_H */
