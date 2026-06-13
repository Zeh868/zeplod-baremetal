/**
 * @file bm_algo_compensation.h
 * @brief 执行器非线性补偿：死区逆映射与摩擦前馈
 *
 * 纯数学核，供液压、门机与机械臂控制环前馈使用。
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            初始版本
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#ifndef BM_ALGO_COMPENSATION_H
#define BM_ALGO_COMPENSATION_H

#ifdef __cplusplus
extern "C" {
#endif

/** 死区逆映射：将死区内指令置零，区外按 gain 放大 */
float bm_algo_deadzone_inverse(float command, float deadband, float gain);

/** 库仑 + 粘性摩擦补偿量（叠加到控制输出） */
float bm_algo_friction_comp(float velocity,
                            float coulomb,
                            float viscous,
                            float v_deadband);

#ifdef __cplusplus
}
#endif

#endif /* BM_ALGO_COMPENSATION_H */
