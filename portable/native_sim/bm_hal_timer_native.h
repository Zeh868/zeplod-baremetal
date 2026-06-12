/**
 * @file bm_hal_timer_native.h
 * @brief 原生仿真定时器测试辅助接口
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#ifndef BM_HAL_TIMER_NATIVE_H
#define BM_HAL_TIMER_NATIVE_H

#include <stdint.h>

/** 手动推进 delta 个 tick */
void bm_hal_timer_native_advance_ticks(uint32_t delta);
/** 跳跃 delta 个 tick 后仅触发一次回调（模拟 deadline 错过） */
void bm_hal_timer_native_jump_ticks(uint32_t delta);
/** 重置 tick 计数为 0 */
void bm_hal_timer_native_reset_ticks(void);
/** 测试辅助：复位定时器为未初始化状态（freq=0） */
void bm_hal_timer_native_deinit(void);
/** 测试辅助：设置后续 init 的返回值，BM_OK 恢复成功路径 */
void bm_hal_timer_native_set_init_result(int result);

#endif /* BM_HAL_TIMER_NATIVE_H */
