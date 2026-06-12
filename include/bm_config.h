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

/*
 * 组件开关（与 CMake BM_ENABLE_* / PROFILE 对齐）。
 * 应用 bm_config.h 可用 #define 覆盖；CMake 集成时由 bm_config 目标注入 -D。
 * Ultra 剖面设 BM_CONFIG_ENABLE_ULTRA=1，其余组件开关忽略。
 */
#ifndef BM_CONFIG_ENABLE_ULTRA
#define BM_CONFIG_ENABLE_ULTRA               0
#endif
#ifndef BM_CONFIG_ENABLE_MODULE
#define BM_CONFIG_ENABLE_MODULE              1
#endif
#ifndef BM_CONFIG_ENABLE_CHANNEL
#define BM_CONFIG_ENABLE_CHANNEL             0
#endif
#ifndef BM_CONFIG_ENABLE_SHELL
#define BM_CONFIG_ENABLE_SHELL               0
#endif
#ifndef BM_CONFIG_ENABLE_WDG
#define BM_CONFIG_ENABLE_WDG                 1
#endif
#ifndef BM_CONFIG_ENABLE_HRT
#define BM_CONFIG_ENABLE_HRT                 0
#endif
#ifndef BM_CONFIG_ENABLE_TICKER
#define BM_CONFIG_ENABLE_TICKER              0
#endif
#ifndef BM_CONFIG_ENABLE_EXEC
#define BM_CONFIG_ENABLE_EXEC           0
#endif
#ifndef BM_CONFIG_ENABLE_SYNC
#define BM_CONFIG_ENABLE_SYNC                0
#endif

/* 日志子系统 */
#define BM_CONFIG_ENABLE_LOG                 1
#define BM_CONFIG_LOG_LEVEL                  3   /* BM_LOG_INFO */
#define BM_CONFIG_LOG_BUF_SIZE               128
#define BM_CONFIG_LOG_USE_STDIO              0

/* 核心事件子系统 */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_EVENT_INLINE_DATA_SIZE     8
#define BM_CONFIG_EVENT_PRIORITY_BURST_MAX   8
/* EVENT_QUEUE_SIZE 须能被 EVENT_PRIORITIES 整除，且商为 2 的幂 */

/* 可选组件 */
#define BM_CONFIG_MAX_MODULES                8
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_SHELL_MAX_ARGS             4
#define BM_CONFIG_SHELL_MAX_CMDS             8
#define BM_CONFIG_SHELL_MAX_NAME_LEN         16
#define BM_CONFIG_MAX_WDG_MODULES            4
#define BM_CONFIG_WDG_MODULE_TIMEOUT_MS      1000
#define BM_CONFIG_WDG_MAX_NAME_LEN           32

/* Ultra 超轻量剖面（header-only） */
#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES      8
#define BM_CONFIG_ULTRA_QUEUE_DEPTH          8
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE  8

/* 混合域（可选） */
#ifndef BM_CONFIG_ENABLE_PRIORITY_MASK
#define BM_CONFIG_ENABLE_PRIORITY_MASK       0
#endif
#define BM_CONFIG_HRT_PRIORITY_THRESHOLD     4
#define BM_CONFIG_HRT_TICK_US                100
#define BM_CONFIG_HRT_MAX_SLOTS              16
#define BM_CONFIG_TICKER_MAX_SLOTS           8
#define BM_CONFIG_TICKER_MAX_CATCHUP         4
#define BM_CONFIG_MAX_EXEC_SLOTS             32
#define BM_CONFIG_MAX_EXEC_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64
#define BM_CONFIG_MAX_SYNC_MEMBERS           BM_CONFIG_MAX_EXEC_INSTANCES
#define BM_CONFIG_SYNC_MAX_PHASE_TICKS       1000000000u

#endif /* BM_CONFIG_H */
