/**
 * @file bm_hal_critical_qemu.c
 * @brief QEMU Cortex-M0 临界区 HAL 实现（PRIMASK）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_critical.h"

static inline uint32_t get_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    return primask;
}

static inline void set_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

/** 关中断并保存 PRIMASK */
bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = get_primask();
    __asm volatile ("cpsid i" ::: "memory");
    return state;
}

/** 恢复 PRIMASK */
void bm_hal_critical_exit(bm_irq_state_t state) {
    set_primask(state);
}
