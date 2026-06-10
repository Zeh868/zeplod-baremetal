/**
 * @file bm_wdg.h
 * @brief 软件看门狗模块注册与喂狗
 *
 * 允许多个逻辑模块分别注册，任一模块未在 BM_CONFIG_WDG_MODULE_TIMEOUT_MS
 * 内调用 feed_module 则本次不喂硬件狗。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-10       1.1            zeh            对齐超时语义与返回值说明
 *
 */
#ifndef BM_WDG_H
#define BM_WDG_H

#include "bm_types.h"

#ifndef BM_CONFIG_MAX_WDG_MODULES
#define BM_CONFIG_MAX_WDG_MODULES 4
#endif

#ifndef BM_CONFIG_WDG_MODULE_TIMEOUT_MS
#define BM_CONFIG_WDG_MODULE_TIMEOUT_MS 1000
#endif

/**
 * @brief 注册一个需喂狗的逻辑模块
 *
 * @param name 模块名称（非 NULL）
 * @return BM_OK 成功；BM_ERR_NO_MEM 注册表已满；BM_ERR_INVALID 参数无效
 */
int bm_wdg_register(const char *name);

/**
 * @brief 检查所有模块已按时喂狗后喂硬件看门狗
 */
void bm_wdg_feed(void);

/**
 * @brief 按名称记录模块喂狗时间戳
 *
 * @param name 模块名称（非 NULL）
 */
void bm_wdg_feed_module(const char *name);

/**
 * @brief 清空软件看门狗注册表（仅用于测试或停机）
 */
void bm_wdg_reset(void);

#endif /* BM_WDG_H */
