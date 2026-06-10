/**
 * @file bm_hal_critical_ch32v003.c
 * @brief CH32V003 临界区 HAL 参考实现（RISC-V mstatus.MIE）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_critical.h"

static inline uintptr_t read_mstatus(void) {
    uintptr_t value;
    __asm volatile ("csrr %0, mstatus" : "=r"(value));
    return value;
}

static inline void write_mstatus(uintptr_t value) {
    __asm volatile ("csrw mstatus, %0" :: "r"(value) : "memory");
}

bm_irq_state_t bm_hal_critical_enter(void) {
    uintptr_t state = read_mstatus();
    __asm volatile ("csrc mstatus, 8" ::: "memory");
    return (bm_irq_state_t)state;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    write_mstatus((uintptr_t)state);
}
