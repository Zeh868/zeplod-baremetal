/**
 * @file bm_algo_compensation.c
 * @brief 执行器非线性补偿实现
 *
 * @author zeh (china_qzh@163.com)
 * @version 0.1
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       0.1            zeh            初始骨架
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm/algorithm/bm_algo_compensation.h"

#include <math.h>

float bm_algo_deadzone_inverse(float command, float deadband, float gain) {
    float a;

    if (deadband <= 0.0f || gain <= 0.0f) {
        return command;
    }

    a = fabsf(command);
    if (a <= deadband) {
        return 0.0f;
    }
    if (command > 0.0f) {
        return gain * (command - deadband);
    }
    return gain * (command + deadband);
}

float bm_algo_friction_comp(float velocity,
                            float coulomb,
                            float viscous,
                            float v_deadband) {
    float sign_v;

    if (fabsf(velocity) < v_deadband) {
        return 0.0f;
    }
    sign_v = (velocity > 0.0f) ? 1.0f : -1.0f;
    return sign_v * coulomb + viscous * velocity;
}
