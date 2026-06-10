/**
 * @file bm_hal_critical_native.c
 * @brief 原生仿真环境临界区 HAL 实现（软件模拟 PRIMASK）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_critical.h"

static volatile bm_irq_state_t irq_state;

/** 进入临界区，返回先前中断状态 */
bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t previous = irq_state;
    irq_state = 1;
    return previous;
}

/** 退出临界区，恢复先前中断状态 */
void bm_hal_critical_exit(bm_irq_state_t state) {
    irq_state = state;
}
