/**
 * @file bm_hal_regs_stm32g4.h
 * @brief STM32G4 外设寄存器最小映射（参考实现用，可替换为 CMSIS 设备头）
 *
 * 地址与位域依据 RM0440（STM32G474）。应用可定义 BM_STM32G4_USE_CMSIS
 * 并包含厂商 stm32g4xx.h，参考 HAL 将优先使用 TIM1/ADC1 等 CMSIS 符号。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#ifndef BM_HAL_REGS_STM32G4_H
#define BM_HAL_REGS_STM32G4_H

#include <stdint.h>

#ifndef BM_STM32G4_TIM1_BASE
#define BM_STM32G4_TIM1_BASE  0x40012C00u
#endif
#ifndef BM_STM32G4_TIM2_BASE
#define BM_STM32G4_TIM2_BASE  0x40000000u
#endif
#ifndef BM_STM32G4_TIM6_BASE
#define BM_STM32G4_TIM6_BASE  0x40001000u
#endif
#ifndef BM_STM32G4_ADC1_BASE
#define BM_STM32G4_ADC1_BASE  0x50000000u
#endif
#ifndef BM_STM32G4_COMP1_BASE
#define BM_STM32G4_COMP1_BASE 0x40010200u
#endif

#define BM_REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

#define BM_TIM_CR1(tim)     BM_REG32((tim) + 0x00u)
#define BM_TIM_CR2(tim)     BM_REG32((tim) + 0x04u)
#define BM_TIM_DIER(tim)    BM_REG32((tim) + 0x0Cu)
#define BM_TIM_SR(tim)      BM_REG32((tim) + 0x10u)
#define BM_TIM_EGR(tim)     BM_REG32((tim) + 0x14u)
#define BM_TIM_CCMR1(tim)   BM_REG32((tim) + 0x18u)
#define BM_TIM_CCMR2(tim)   BM_REG32((tim) + 0x1Cu)
#define BM_TIM_CCER(tim)    BM_REG32((tim) + 0x20u)
#define BM_TIM_CNT(tim)     BM_REG32((tim) + 0x24u)
#define BM_TIM_PSC(tim)     BM_REG32((tim) + 0x28u)
#define BM_TIM_ARR(tim)     BM_REG32((tim) + 0x2Cu)
#define BM_TIM_CCR1(tim)    BM_REG32((tim) + 0x34u)
#define BM_TIM_CCR2(tim)    BM_REG32((tim) + 0x38u)
#define BM_TIM_CCR3(tim)    BM_REG32((tim) + 0x3Cu)
#define BM_TIM_BDTR(tim)    BM_REG32((tim) + 0x44u)

#define BM_TIM_DIER_UIE     (1u << 0)
#define BM_TIM_SR_UIF       (1u << 0)
#define BM_TIM_BDTR_MOE     (1u << 15)
#define BM_TIM_BDTR_AOE     (1u << 14)
#define BM_TIM_CCER_CC1E    (1u << 0)
#define BM_TIM_CCER_CC2E    (1u << 4)
#define BM_TIM_CCER_CC3E    (1u << 8)
#define BM_TIM_CR1_CEN      (1u << 0)

#define BM_ADC_ISR(tim)     BM_REG32((tim) + 0x00u)
#define BM_ADC_IER(tim)     BM_REG32((tim) + 0x04u)
#define BM_ADC_CR(tim)      BM_REG32((tim) + 0x08u)
#define BM_ADC_JDR1(tim)    BM_REG32((tim) + 0x3Cu)
#define BM_ADC_JDR2(tim)    BM_REG32((tim) + 0x40u)
#define BM_ADC_JDR3(tim)    BM_REG32((tim) + 0x44u)
#define BM_ADC_JDR4(tim)    BM_REG32((tim) + 0x48u)

#define BM_ADC_ISR_JEOC     (1u << 4)
#define BM_ADC_IER_JEOCIE   (1u << 4)

#define BM_COMP_CSR(comp)   BM_REG32((comp) + 0x00u)
#define BM_COMP_CSR_LOCK    (1u << 31)
#define BM_COMP_CSR_VALUE   (1u << 30)

#endif /* BM_HAL_REGS_STM32G4_H */
