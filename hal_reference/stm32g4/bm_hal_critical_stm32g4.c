/**
 * @file bm_hal_critical_stm32g4.c
 * @brief STM32G4 临界区 HAL 实现（PRIMASK + BASEPRI）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#define BM_HAL_HAS_PRIORITY_MASK 1

#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 4
#endif

#include "bm_hal_critical.h"

static inline uint32_t read_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    return primask;
}

static inline void write_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

static inline uint32_t read_basepri(void) {
    uint32_t basepri;
    __asm volatile ("mrs %0, basepri" : "=r" (basepri));
    return basepri;
}

static inline void write_basepri(uint32_t basepri) {
    __asm volatile ("msr basepri, %0" :: "r" (basepri) : "memory");
}

/** 关全局中断（PRIMASK） */
bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = read_primask();
    __asm volatile ("cpsid i" ::: "memory");
    return state;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    write_primask(state);
}

/**
 * 屏蔽优先级不高于 threshold 的中断（BASEPRI）。
 * 返回值打包 basepri（低 8 位）与 primask（高 8 位）。
 */
bm_irq_state_t bm_hal_critical_enter_below(uint8_t threshold) {
    bm_irq_state_t packed = read_basepri();
    packed |= (read_primask() << 8);
    write_basepri((uint32_t)threshold << (8 - __NVIC_PRIO_BITS));
    return packed;
}

void bm_hal_critical_exit_below(bm_irq_state_t previous_state) {
    write_basepri((uint32_t)(previous_state & 0xFFu));
    write_primask((uint32_t)(previous_state >> 8));
}
