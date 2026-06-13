/**
 * @file bm_algo_motor.h
 * @brief 电机纯数学核：Clarke/Park 变换与 SVPWM
 *
 * 采用幅值不变 Clarke 变换；角度为电角度（弧度）。
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            正式发布
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_MOTOR_H
#define BM_ALGO_MOTOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ia;
    float ib;
    float ic;
} bm_algo_abc_t;

typedef struct {
    float i_alpha;
    float i_beta;
} bm_algo_alphabeta_t;

typedef struct {
    float id;
    float iq;
} bm_algo_dq_t;

typedef struct {
    float duty_a;
    float duty_b;
    float duty_c;
} bm_algo_svpwm_out_t;

/** Clarke：三相 → αβ（幅值不变） */
void bm_algo_clarke(const bm_algo_abc_t *abc, bm_algo_alphabeta_t *ab);

/** 两相电流 Clarke（假定 ia+ib+ic=0，仅用 ia/ib） */
void bm_algo_clarke_2shunt(float ia, float ib, bm_algo_alphabeta_t *ab);

/** Park：αβ → dq */
void bm_algo_park(const bm_algo_alphabeta_t *ab,
                  float theta_rad,
                  bm_algo_dq_t *dq);

/** 逆 Park：dq → αβ */
void bm_algo_inv_park(const bm_algo_dq_t *dq,
                      float theta_rad,
                      bm_algo_alphabeta_t *ab);

/** 逆 Clarke：αβ → 三相 */
void bm_algo_inv_clarke(const bm_algo_alphabeta_t *ab, bm_algo_abc_t *abc);

/**
 * SVPWM：αβ 电压参考 → 三相占空比 [0,1]
 * @param v_alpha,v_beta  per-unit 电压（相对直流母线）
 * @param vbus_v          母线电压（V），用于归一化；若已 per-unit 可传 1
 */
void bm_algo_svpwm(float v_alpha,
                   float v_beta,
                   float vbus_v,
                   bm_algo_svpwm_out_t *out);

/** 限制 dq 电压矢量幅值（圆限幅） */
void bm_algo_voltage_limit(float *vd, float *vq, float v_max);

/** 双电阻采样重构三相电流（ic = -ia - ib） */
void bm_algo_current_from_2shunt(float ia, float ib, bm_algo_abc_t *abc);

/**
 * 死区压降补偿：按相电流方向补偿测量电压（V）
 * @param deadtime_s  死区时间（s）
 */
float bm_algo_deadtime_comp_v(float phase_v,
                              float phase_current_a,
                              float deadtime_s,
                              float vbus_v);

/* ---------- 无感 FOC 辅助（K0） ---------- */
typedef struct {
    float rs_ohm;
    float ls_h;
    float pll_kp;
    float pll_ki;
} bm_algo_flux_observer_config_t;

typedef struct {
    float theta_rad;
    float omega_rad_s;
    float flux_alpha;
    float flux_beta;
} bm_algo_flux_observer_state_t;

void bm_algo_flux_observer_reset(bm_algo_flux_observer_state_t *state,
                                 float theta_rad);

/**
 * 磁链观测 + PLL，返回电角度（rad）
 * 定子磁链：ψ = ∫(V - Rs·I)dt - Ls·I
 */
float bm_algo_flux_observer_step(bm_algo_flux_observer_state_t *state,
                                 const bm_algo_flux_observer_config_t *config,
                                 float v_alpha,
                                 float v_beta,
                                 float i_alpha,
                                 float i_beta,
                                 float dt_s);

/** MTPA：由 iq 参考求 id 参考（简化 IPM 模型） */
float bm_algo_mtpa_id_ref(float iq_ref_a,
                          float ld_h,
                          float lq_h,
                          float psi_f_wb);

/** 弱磁：电压饱和时下调 id 参考 */
float bm_algo_fw_id_adjust(float id_ref_a, float vd, float vq, float v_max_pu);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_MOTOR_H */
