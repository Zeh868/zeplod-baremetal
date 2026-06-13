/**
 * @file motor_current_sense.h
 * @brief 电机分流电阻电流采样重构（2/3 分流）
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
#ifndef BM_MOTOR_CURRENT_SENSE_H
#define BM_MOTOR_CURRENT_SENSE_H

#include "bm/algorithm/bm_algo_motor.h"
#include "hal/bm_hal_adc.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BM_MOTOR_CS_2SHUNT = 0,
    BM_MOTOR_CS_3SHUNT
} bm_motor_current_sense_topology_t;

typedef struct {
    float *ia_a;
    float *ib_a;
    float *ic_a;
} bm_motor_current_sense_sim_fb_t;

typedef struct {
    bm_hal_adc_t *adc;
    uint32_t      rank_ia;
    uint32_t      rank_ib;
    uint32_t      rank_ic;
    float         adc_scale;
    bm_motor_current_sense_sim_fb_t sim_fb;
} bm_motor_current_sense_resources_t;

typedef struct {
    bm_motor_current_sense_topology_t topology;
    float                             offset_a;
} bm_motor_current_sense_config_t;

typedef struct {
    bm_algo_abc_t      abc;
    bm_algo_alphabeta_t alphabeta;
    int                valid;
} bm_motor_current_sense_state_t;

typedef struct {
    bm_motor_current_sense_config_t    config;
    bm_motor_current_sense_resources_t resources;
    bm_motor_current_sense_state_t     state;
} bm_motor_current_sense_axis_t;

int  bm_motor_current_sense_validate_config(
    const bm_motor_current_sense_config_t *config);
int  bm_motor_current_sense_init(bm_motor_current_sense_axis_t *axis);
void bm_motor_current_sense_reset(bm_motor_current_sense_axis_t *axis);
int  bm_motor_current_sense_step(bm_motor_current_sense_axis_t *axis);

#ifdef __cplusplus
}
#endif

#endif /* BM_MOTOR_CURRENT_SENSE_H */
