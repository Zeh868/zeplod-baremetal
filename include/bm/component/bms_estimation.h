/**
 * @file bms_estimation.h
 * @brief BMS Pack 估算领域组件（库仑 + OCV 融合 + 温度补偿）
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
 *
 * 对齐路线图 §5.3 pack_estimator：低频 SOC 估算，无 HAL 硬编码。
 */
#ifndef BM_BMS_ESTIMATION_H
#define BM_BMS_ESTIMATION_H

#include "bm/algorithm/bm_algo_battery.h"
#include "bm/hybrid/bm_exec.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BM_BMS_EST_TEL_VALID  (1u << 0u)
#define BM_BMS_EST_TEL_STALE  (1u << 1u)

typedef struct {
    uint32_t sequence;
    uint32_t status;
    float    soc;
    float    pack_voltage_v;
    float    pack_current_a;
    float    temp_c;
} bm_bms_est_telemetry_t;

typedef int (*bm_bms_est_read_sample_fn)(void *user,
                                         float *pack_current_a,
                                         float *pack_voltage_v,
                                         float *temp_c);

typedef void (*bm_bms_est_publish_telemetry_fn)(
    void *user,
    const bm_bms_est_telemetry_t *telemetry);

typedef struct {
    bm_bms_est_read_sample_fn read_sample;
    void                     *read_sample_user;
    bm_bms_est_publish_telemetry_fn publish_telemetry;
    void                     *publish_telemetry_user;
} bm_bms_estimation_resources_t;

typedef struct {
    bm_algo_coulomb_config_t      coulomb;
    bm_algo_battery_temp_config_t temp;
    bm_algo_soc_fusion_config_t   fusion;
    const bm_algo_ocv_table_t    *ocv_table;
    float                         resting_current_a;
    float                         resting_time_s;
    float                         dt_s;
    float                         soc_init;
} bm_bms_estimation_config_t;

typedef struct {
    bm_algo_coulomb_state_t coulomb;
    float                   soc_fused;
    float                   resting_elapsed_s;
    uint32_t                step_count;
    bm_bms_est_telemetry_t  telemetry;
} bm_bms_estimation_state_t;

typedef struct {
    bm_bms_estimation_config_t    config;
    bm_bms_estimation_resources_t resources;
    bm_bms_estimation_state_t     state;
} bm_bms_estimation_axis_t;

int bm_bms_estimation_validate_config(const bm_bms_estimation_config_t *config);

void bm_bms_estimation_reset(bm_bms_estimation_axis_t *axis, float soc_init);

void bm_bms_estimation_step(bm_bms_estimation_axis_t *axis);

void bm_bms_estimation_exec_step(const bm_exec_t *instance);

int bm_bms_estimation_exec_init(const bm_exec_t *instance);

int bm_bms_estimation_exec_start(const bm_exec_t *instance);

void bm_bms_estimation_exec_safe_stop(const bm_exec_t *instance);

extern const bm_exec_ops_t bm_bms_estimation_exec_ops;

#ifdef __cplusplus
}
#endif

#endif /* BM_BMS_ESTIMATION_H */
