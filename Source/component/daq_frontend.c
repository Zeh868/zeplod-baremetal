/**
 * @file daq_frontend.c
 * @brief DAQ 前端组件实现
 * @author zeh (china_qzh@163.com)
 * @version 0.1
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       0.1            zeh            初始骨架
 *
 */
#include "bm/component/daq_frontend.h"

#include <math.h>

int bm_daq_frontend_init(bm_daq_frontend_axis_t *axis,
                         float *rms_buffer,
                         uint32_t rms_buflen) {
    if (axis == NULL || rms_buffer == NULL || rms_buflen == 0u) {
        return -1;
    }
    axis->state.rms_buffer = rms_buffer;
    axis->state.rms_buflen = rms_buflen;
    axis->state.rms_cfg.window_samples = rms_buflen;
    if (bm_algo_rms_init(&axis->state.rms, &axis->state.rms_cfg,
                         rms_buffer, rms_buflen) != 0) {
        return -1;
    }
    bm_daq_frontend_reset(axis);
    return 0;
}

void bm_daq_frontend_reset(bm_daq_frontend_axis_t *axis) {
    if (axis == NULL) {
        return;
    }
    bm_algo_rms_reset(&axis->state.rms);
    axis->state.peak = 0.0f;
    axis->state.crest_factor = 0.0f;
    axis->state.captured = 0u;
    axis->state.armed = 0;
    axis->state.triggered = 0;
}

void bm_daq_frontend_arm(bm_daq_frontend_axis_t *axis) {
    if (axis != NULL) {
        axis->state.armed = 1;
        axis->state.triggered = 0;
        axis->state.captured = 0u;
    }
}

int bm_daq_frontend_feed(bm_daq_frontend_axis_t *axis, float sample) {
    float rms;
    float ax;

    if (axis == NULL || !axis->state.armed) {
        return 0;
    }

    rms = bm_algo_rms_step(&axis->state.rms, &axis->state.rms_cfg, sample);
    ax = fabsf(sample);
    if (ax > axis->state.peak) {
        axis->state.peak = ax;
    }

    if (!axis->state.triggered &&
        fabsf(sample) >= axis->config.trigger_level) {
        axis->state.triggered = 1;
    }

    if (axis->state.triggered) {
        axis->state.captured++;
        if (rms > 1e-9f) {
            axis->state.crest_factor = axis->state.peak / rms;
        }
        if (axis->state.captured >= axis->config.capture_samples) {
            axis->state.armed = 0;
            return 1;
        }
    }
    return 0;
}
