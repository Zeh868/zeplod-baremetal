/**
 * @file bm_module.h
 * @brief 模块生命周期管理
 *
 * 通过 bm_module_t 描述模块的 init/start/stop/deinit 回调，
 * 由框架按优先级统一调度。
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

/** 模块描述符（编译期静态注册） */
typedef struct {
    const char        *name;
    uint8_t            priority;
    bm_module_state_t  state;
    int (*init)(void);
    int (*start)(void);
    int (*stop)(void);
    int (*deinit)(void);
} bm_module_t;

/*
 * 编译期注册方式（兼容 SDCC/STM8）：
 *   static const bm_module_t _bm_module_table[] = { ... };
 *   const uint32_t _bm_module_count = N;
 */

/**
 * @brief 按优先级初始化所有已注册模块
 *
 * @return BM_OK 全部成功；否则为首个失败模块的错误码
 */
int bm_module_init_all(void);

/**
 * @brief 按优先级启动所有已初始化模块
 *
 * @return BM_OK 全部成功；否则为首个失败模块的错误码
 */
int bm_module_start_all(void);

/**
 * @brief 按优先级停止所有已启动模块
 *
 * @return BM_OK 全部成功；否则为首个失败模块的错误码
 */
int bm_module_stop_all(void);

/**
 * @brief 按优先级反初始化所有模块
 *
 * @return BM_OK 全部成功；否则为首个失败模块的错误码
 */
int bm_module_deinit_all(void);

#endif /* BM_MODULE_H */
