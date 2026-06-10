/**
 * @file bm_hal_pwm.c
 * @brief PWM HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_pwm.h"
#include "bm_hal_weak.h"
#include "bm_types.h"

BM_HAL_WEAK
int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm, uint32_t phase, uint16_t duty) {
    (void)pwm;
    (void)phase;
    (void)duty;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable) {
    (void)pwm;
    (void)enable;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm) {
    (void)pwm;
}

BM_HAL_WEAK
int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding) {
    (void)pwm;
    (void)binding;
    return BM_ERR_NOT_INIT;
}
