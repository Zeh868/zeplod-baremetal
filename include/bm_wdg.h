/**
 * @file bm_wdg.h
 * @brief 软件看门狗模块注册与喂狗
 *
 * 允许多个逻辑模块分别注册，任一模块未按时喂狗则触发超时。
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
#ifndef BM_WDG_H
#define BM_WDG_H

#include "bm_types.h"

#ifndef BM_CONFIG_MAX_WDG_MODULES
#define BM_CONFIG_MAX_WDG_MODULES 4
#endif

/**
 * @brief 注册一个需喂狗的逻辑模块
 *
 * @param name 模块名称
 * @return 模块 ID（>= 0）；BM_ERR_NO_MEM 注册表已满；BM_ERR_INVALID 参数无效
 */
int bm_wdg_register(const char *name);

/**
 * @brief 喂全部已注册模块
 */
void bm_wdg_feed(void);

/**
 * @brief 按名称喂指定模块
 *
 * @param name 模块名称
 */
void bm_wdg_feed_module(const char *name);

#endif
