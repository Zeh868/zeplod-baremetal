/**
 * @file bm_hal_critical_stm32f0.c
 * @brief STM32F0 临界区 HAL 实现（CMSIS PRIMASK）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_critical.h"

/*
 * 若厂商包使用不同 CMSIS 设备头文件名，可通过 BM_STM32F0_DEVICE_HEADER 覆盖。
 */
#ifndef BM_STM32F0_DEVICE_HEADER
#define BM_STM32F0_DEVICE_HEADER "stm32f0xx.h"
#endif

#include BM_STM32F0_DEVICE_HEADER

/** 关全局中断并返回先前 PRIMASK */
bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

/** 恢复 PRIMASK */
void bm_hal_critical_exit(bm_irq_state_t state) {
    __set_PRIMASK(state);
}
