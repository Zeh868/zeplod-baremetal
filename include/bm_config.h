/**
 * @file bm_config.h
 * @brief 框架全局编译期配置宏
 *
 * 各子模块容量、可选组件开关及混合域参数均在此集中定义。
 * 应用可通过项目级 bm_config.h 覆盖默认值。
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
#ifndef BM_CONFIG_H
#define BM_CONFIG_H

/* 日志子系统 */
#define BM_CONFIG_ENABLE_LOG                 1
#define BM_CONFIG_LOG_LEVEL                  3   /* BM_LOG_INFO */
#define BM_CONFIG_LOG_BUF_SIZE               128
#define BM_CONFIG_LOG_USE_STDIO              0

/* Core event system */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_EVENT_INLINE_DATA_SIZE     8
#define BM_CONFIG_EVENT_PRIORITY_BURST_MAX   8
/* EVENT_QUEUE_SIZE 须能被 EVENT_PRIORITIES 整除，且商为 2 的幂 */

/* Optional components */
#define BM_CONFIG_MAX_MODULES                8
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_SHELL_MAX_ARGS             4
#define BM_CONFIG_SHELL_MAX_CMDS             8
#define BM_CONFIG_SHELL_MAX_NAME_LEN         16
#define BM_CONFIG_MAX_WDG_MODULES            4
#define BM_CONFIG_WDG_MODULE_TIMEOUT_MS      1000

/* Header-only ultra profile */
#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES      8
#define BM_CONFIG_ULTRA_QUEUE_DEPTH          8
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE  8

/* Hybrid domain (optional) */
#ifndef BM_CONFIG_ENABLE_PRIORITY_MASK
#define BM_CONFIG_ENABLE_PRIORITY_MASK       0
#endif
#define BM_CONFIG_HRT_PRIORITY_THRESHOLD     4
#define BM_CONFIG_HRT_TICK_US                100
#define BM_CONFIG_HRT_MAX_SLOTS              16
#define BM_CONFIG_TICKER_MAX_SLOTS           8
#define BM_CONFIG_TICKER_MAX_CATCHUP         4
#define BM_CONFIG_MAX_CTRL_SLOTS             32
#define BM_CONFIG_MAX_CTRL_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64

#endif /* BM_CONFIG_H */
