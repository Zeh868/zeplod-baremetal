/**
 * @file daq_frontend.h
 * @brief DAQ 前端：触发、预触发与 RMS/波峰因数捕获
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 0.1
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       0.1            zeh            初始骨架
 */
#ifndef BM_DAQ_FRONTEND_H
#define BM_DAQ_FRONTEND_H

#include "bm/algorithm/bm_algo_power.h"
#include "bm/algorithm/bm_algo_statistics.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float trigger_level;
    uint32_t pre_trigger_samples;
    uint32_t capture_samples;
    uint32_t sample_hz;
} bm_daq_frontend_config_t;

typedef struct {
    bm_algo_rms_config_t   rms_cfg;
    bm_algo_rms_state_t    rms;
    float                 *rms_buffer;
    uint32_t               rms_buflen;
    float                  peak;
    float                  crest_factor;
    uint32_t               captured;
    int                    armed;
    int                    triggered;
} bm_daq_frontend_state_t;

typedef struct {
    bm_daq_frontend_config_t config;
    bm_daq_frontend_state_t  state;
} bm_daq_frontend_axis_t;

int bm_daq_frontend_init(bm_daq_frontend_axis_t *axis,
                         float *rms_buffer,
                         uint32_t rms_buflen);
void bm_daq_frontend_reset(bm_daq_frontend_axis_t *axis);
void bm_daq_frontend_arm(bm_daq_frontend_axis_t *axis);
int bm_daq_frontend_feed(bm_daq_frontend_axis_t *axis, float sample);

#ifdef __cplusplus
}
#endif

#endif /* BM_DAQ_FRONTEND_H */
