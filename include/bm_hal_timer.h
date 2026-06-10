/**
 * @file bm_hal_timer.h
 * @brief 系统定时器 HAL 接口
 *
 * 提供自由运行计数器、频率查询及溢出回调注册。
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
 * @brief 注册定时器溢出回调
 *
 * @param cb 溢出时调用的回调函数；NULL 表示取消注册
 */
void bm_hal_timer_set_callback(void (*cb)(void));

#endif /* BM_HAL_TIMER_H */
