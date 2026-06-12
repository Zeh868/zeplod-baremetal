/**
 * @file bm_hal_timer.h
 * @brief 系统定时器 HAL 接口
 *
 * 提供自由运行计数器、频率查询及周期性 tick 回调注册。
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
#ifndef BM_HAL_TIMER_H
#define BM_HAL_TIMER_H

#include <stdint.h>

/**
 * @brief 初始化系统定时器
 *
 * 驱动须按 `freq_hz` 产生周期性 tick；在 `set_callback` 注册有效回调之前
 * 不得触发回调（可先 `set_callback(NULL)` 再 init）。
 *
 * @param freq_hz 定时器计数频率（Hz）
 * @return BM_OK 成功；否则为平台错误码
 */
int bm_hal_timer_init(uint32_t freq_hz);

/**
 * @brief 停止系统定时器
 */
void bm_hal_timer_stop(void);

/**
 * @brief 读取当前 tick 计数值
 *
 * @return 自由运行计数器当前值
 */
uint32_t bm_hal_timer_get_ticks(void);

/**
 * @brief 查询定时器计数频率
 *
 * @return 计数频率（Hz）
 */
uint32_t bm_hal_timer_get_freq(void);

/**
 * @brief 注册定时器周期性 tick 回调
 *
 * 按 `init` 所设 `freq_hz` 每个计数节拍在 ISR 上下文调用一次；NULL 取消注册。
 *
 * @param cb tick 回调函数；NULL 表示取消注册
 */
void bm_hal_timer_set_callback(void (*cb)(void));

#endif /* BM_HAL_TIMER_H */
