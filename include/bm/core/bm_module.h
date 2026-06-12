/**
 * @file bm_module.h
 * @brief 模块生命周期管理
 *
 * 通过 bm_module_t 描述模块的 init/start/stop/deinit 回调，
 * 由框架按优先级统一调度。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-12
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-12       1.1            zeh            模块标志位与看门狗集成
 *
 */
#ifndef BM_MODULE_H
#define BM_MODULE_H

#include "bm_types.h"

#ifndef BM_CONFIG_MAX_MODULES
#define BM_CONFIG_MAX_MODULES 8
#endif

/** 模块运行状态 */
typedef enum {
    BM_MODULE_STATE_UNINIT = 0,
    BM_MODULE_STATE_INITED,
    BM_MODULE_STATE_STARTED,
    BM_MODULE_STATE_STOPPED
} bm_module_state_t;

/** 模块标志：start 时由框架注册软件看门狗（名称同模块 name） */
#define BM_MODULE_FLAG_NONE 0x00u
#define BM_MODULE_FLAG_WDG  0x01u

/** 模块描述符（编译期静态注册） */
typedef struct {
    const char        *name;
    uint8_t            priority;
    bm_module_state_t  state;
    uint8_t            flags;
    int (*init)(void);
    int (*start)(void);
    int (*stop)(void);
    int (*deinit)(void);
} bm_module_t;

/**
 * 单模块描述符（每模块一个 .c）。未使用的回调传 NULL。
 *
 * @code
 * BM_MODULE_DEFINE(sensor, 2,
 *     sensor_init, sensor_start, sensor_stop, sensor_deinit);
 * @endcode
 */
#define BM_MODULE_DEFINE(name, priority, init_fn, start_fn, stop_fn, deinit_fn) \
    BM_MODULE_DEFINE_EX(name, priority, BM_MODULE_FLAG_NONE, \
        init_fn, start_fn, stop_fn, deinit_fn)

/**
 * 带标志位的模块描述符。
 *
 * @code
 * BM_MODULE_DEFINE_EX(comm, 1, BM_MODULE_FLAG_WDG,
 *     comm_init, comm_start, comm_stop, comm_deinit);
 * @endcode
 */
#define BM_MODULE_DEFINE_EX(name, priority, flags, init_fn, start_fn, stop_fn, deinit_fn) \
    const bm_module_t _bm_mod_##name = { \
        #name, \
        (uint8_t)(priority), \
        BM_MODULE_STATE_UNINIT, \
        (uint8_t)(flags), \
        (init_fn), \
        (start_fn), \
        (stop_fn), \
        (deinit_fn) \
    }

/** 在 module_table.c 中前置声明各模块条目 */
#define BM_MODULE_DECLARE(name) \
    extern const bm_module_t _bm_mod_##name

/** 聚合表中的单条引用（指针，兼容 MSVC 静态初始化） */
#define BM_MODULE_ENTRY(name) \
    (&_bm_mod_##name)

/**
 * 应用侧模块表（通常单独 module_table.c）。
 */
#define BM_MODULE_TABLE(...) \
    const bm_module_t *_bm_module_table[] = { __VA_ARGS__ }; \
    const uint32_t _bm_module_count = \
        (uint32_t)(sizeof(_bm_module_table) / sizeof(_bm_module_table[0]))

/**
 * @brief 应用启动：event_reset → init_all → start_all
 */
int bm_module_boot(void);

/**
 * @brief 按优先级初始化所有已注册模块
 */
int bm_module_init_all(void);

/**
 * @brief 按优先级启动所有已初始化模块（含 BM_MODULE_FLAG_WDG 注册）
 */
int bm_module_start_all(void);

/**
 * @brief 按优先级停止所有已启动模块
 */
int bm_module_stop_all(void);

/**
 * @brief 按优先级反初始化所有模块
 */
int bm_module_deinit_all(void);

/**
 * @brief 喂所有带 BM_MODULE_FLAG_WDG 的模块，再调用 bm_wdg_feed()
 */
void bm_module_feed_wdg_all(void);

#endif /* BM_MODULE_H */
