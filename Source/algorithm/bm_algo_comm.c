/**
 * @file bm_algo_comm.c
 * @brief 通信 DSP：CRC 与 DTMF 检测实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_comm.h"
#include "bm/algorithm/bm_algo_spectral.h"

#include <math.h>

static const float s_dtmf_rows[BM_ALGO_DTMF_ROW_COUNT] = {
    697.0f, 770.0f, 852.0f, 941.0f
};
static const float s_dtmf_cols[BM_ALGO_DTMF_COL_COUNT] = {
    1209.0f, 1336.0f, 1477.0f, 1633.0f
};
static const char s_dtmf_map[BM_ALGO_DTMF_ROW_COUNT][BM_ALGO_DTMF_COL_COUNT] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

uint16_t bm_algo_crc16_ccitt(const uint8_t *data, uint32_t len, uint16_t init) {
    uint32_t i;
    uint32_t crc = init;

    if (data == NULL) {
        return (uint16_t)crc;
    }

    for (i = 0u; i < len; ++i) {
        uint32_t b;
        crc ^= (uint32_t)data[i] << 8;
        for (b = 0u; b < 8u; ++b) {
            if (crc & 0x8000u) {
                crc = ((crc << 1) ^ 0x1021u) & 0xffffu;
            } else {
                crc = (crc << 1) & 0xffffu;
            }
        }
    }
    return (uint16_t)crc;
}

uint32_t bm_algo_crc32(const uint8_t *data, uint32_t len, uint32_t init) {
    uint32_t i;
    uint32_t crc = init;
    static const uint32_t poly = 0xEDB88320u;

    if (data == NULL) {
        return crc;
    }

    for (i = 0u; i < len; ++i) {
        uint32_t b;
        crc ^= data[i];
        for (b = 0u; b < 8u; ++b) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void bm_algo_dtmf_reset(bm_algo_dtmf_state_t *state) {
    uint32_t i;

    if (state == NULL) {
        return;
    }
    for (i = 0u; i < BM_ALGO_DTMF_ROW_COUNT; ++i) {
        state->row_energy[i] = 0.0f;
    }
    for (i = 0u; i < BM_ALGO_DTMF_COL_COUNT; ++i) {
        state->col_energy[i] = 0.0f;
    }
}

static float goertzel_energy(const float *samples, uint32_t n,
                             float freq_hz, float sample_hz) {
    bm_algo_goertzel_config_t cfg;
    bm_algo_goertzel_state_t st;
    uint32_t i;

    cfg.target_freq_hz = freq_hz;
    cfg.sample_hz = sample_hz;
    cfg.block_size = n;
    bm_algo_goertzel_init(&st, &cfg);

    for (i = 0u; i < n; ++i) {
        bm_algo_goertzel_feed(&st, &cfg, samples[i]);
    }
    return bm_algo_goertzel_result(&st, &cfg);
}

int bm_algo_dtmf_detect(bm_algo_dtmf_state_t *state,
                        const bm_algo_dtmf_config_t *config,
                        const float *samples,
                        uint32_t n,
                        char *symbol_out) {
    uint32_t ri;
    uint32_t ci;
    uint32_t best_r = 0u;
    uint32_t best_c = 0u;
    float max_r = 0.0f;
    float max_c = 0.0f;

    if (state == NULL || config == NULL || samples == NULL ||
        symbol_out == NULL || n == 0u) {
        return -1;
    }

    for (ri = 0u; ri < BM_ALGO_DTMF_ROW_COUNT; ++ri) {
        state->row_energy[ri] = goertzel_energy(samples, n,
                                                s_dtmf_rows[ri],
                                                config->sample_hz);
        if (state->row_energy[ri] > max_r) {
            max_r = state->row_energy[ri];
            best_r = ri;
        }
    }

    for (ci = 0u; ci < BM_ALGO_DTMF_COL_COUNT; ++ci) {
        state->col_energy[ci] = goertzel_energy(samples, n,
                                                s_dtmf_cols[ci],
                                                config->sample_hz);
        if (state->col_energy[ci] > max_c) {
            max_c = state->col_energy[ci];
            best_c = ci;
        }
    }

    if (max_r < 1e-4f || max_c < 1e-4f) {
        *symbol_out = '\0';
        return 0;
    }

    *symbol_out = s_dtmf_map[best_r][best_c];
    return 1;
}
