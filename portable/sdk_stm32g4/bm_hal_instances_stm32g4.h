/**
 * @file bm_hal_instances_stm32g4.h
 * @brief STM32G4 预定义外设实例句柄（NUCLEO-G474RE 等板卡参考映射）
 */
#ifndef BM_HAL_INSTANCES_STM32G4_H
#define BM_HAL_INSTANCES_STM32G4_H

#include "bm_hal_adc.h"
#include "bm_hal_comp.h"
#include "bm_hal_encoder.h"
#include "bm_hal_pwm.h"

#include "bm_hal_sdk_stm32g4.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const bm_hal_pwm_t BM_HAL_PWM_TIM1;
extern const bm_hal_adc_t BM_HAL_ADC1;
extern const bm_hal_comp_t BM_HAL_COMP1;
extern const bm_hal_encoder_t BM_HAL_ENC_TIM2;

#ifdef __cplusplus
}
#endif

#endif /* BM_HAL_INSTANCES_STM32G4_H */
