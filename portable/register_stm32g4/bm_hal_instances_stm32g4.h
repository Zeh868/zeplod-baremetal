/**
 * @file bm_hal_instances_stm32g4.h
 * @brief STM32G4 预定义外设实例句柄（NUCLEO-G474RE 等板卡参考映射）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#ifndef BM_HAL_INSTANCES_STM32G4_H
#define BM_HAL_INSTANCES_STM32G4_H

#include "bm_hal_adc.h"
#include "bm_hal_comp.h"
#include "bm_hal_encoder.h"
#include "bm_hal_pwm.h"

#include "bm_hal_regs_stm32g4.h"

#ifdef __cplusplus
extern "C" {
#endif

/** PWM：TIM1 三相高级定时器 */
extern const bm_hal_pwm_t BM_HAL_PWM_TIM1;
/** ADC：ADC1 注入组 */
extern const bm_hal_adc_t BM_HAL_ADC1;
/** 比较器：COMP1（可路由至 TIM1 Break） */
extern const bm_hal_comp_t BM_HAL_COMP1;
/** 编码器：TIM2 正交模式 */
extern const bm_hal_encoder_t BM_HAL_ENC_TIM2;

#ifdef __cplusplus
}
#endif

#endif /* BM_HAL_INSTANCES_STM32G4_H */
