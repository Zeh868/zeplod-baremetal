/**
 * @file bm_module.c
 * @brief 模块生命周期管理实现
 *
 * 从链接器段表加载模块，按优先级排序后依次 init/start/stop/deinit。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-10       1.1            zeh            失败回滚与状态机加固
 *
 */
#include "bm_module.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"

#include <string.h>

extern const bm_module_t _bm_module_table[];
extern const uint32_t    _bm_module_count;

static bm_module_t _modules[BM_CONFIG_MAX_MODULES];
static uint32_t    _module_count = 0;
static int         _modules_initialized = 0;

/**
 * @brief 按 priority 升序对模块表冒泡排序
 */
static void _sort_modules(void) {
    for (uint32_t i = 0; i < _module_count; i++) {
        for (uint32_t j = i + 1; j < _module_count; j++) {
            if (_modules[i].priority > _modules[j].priority) {
                bm_module_t tmp = _modules[i];
                _modules[i] = _modules[j];
                _modules[j] = tmp;
            }
        }
    }
}

/**
 * @brief init 失败时逆序回滚已初始化模块
 */
static void _rollback_inits(uint32_t through_index) {
    while (through_index > 0u) {
        through_index--;
        if (_modules[through_index].state == BM_MODULE_STATE_INITED) {
            if (_modules[through_index].deinit) {
                (void)_modules[through_index].deinit();
            }
            _modules[through_index].state = BM_MODULE_STATE_UNINIT;
        }
    }
}

/**
 * @brief start 失败时逆序停止已启动模块
 */
static void _rollback_starts(uint32_t through_index) {
    while (through_index > 0u) {
        through_index--;
        if (_modules[through_index].state == BM_MODULE_STATE_STARTED) {
            if (_modules[through_index].stop) {
                (void)_modules[through_index].stop();
            }
            _modules[through_index].state = BM_MODULE_STATE_INITED;
        }
    }
}

/**
 * @brief 从链接器段表加载模块并依次调用 init
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
int bm_module_init_all(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    if (_modules_initialized) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("module", "init_all already done");
        return BM_ERR_ALREADY;
    }
    BM_CRITICAL_EXIT(s);

    if (_bm_module_count > BM_CONFIG_MAX_MODULES) {
        BM_LOGE("module", "module table truncated: %u > %u",
                (unsigned)_bm_module_count, (unsigned)BM_CONFIG_MAX_MODULES);
        return BM_ERR_OVERFLOW;
    }

    _module_count = _bm_module_count;
    memcpy(_modules, _bm_module_table, _module_count * sizeof(bm_module_t));
    for (uint32_t i = 0u; i < _module_count; i++) {
        _modules[i].state = BM_MODULE_STATE_UNINIT;
    }
    _sort_modules();

    BM_LOGI("module", "init_all count=%u", (unsigned)_module_count);
    for (uint32_t i = 0u; i < _module_count; i++) {
        if (_modules[i].init) {
            int r = _modules[i].init();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_INITED;
                BM_LOGD("module", "'%s' inited",
                        _modules[i].name ? _modules[i].name : "(null)");
            } else {
                BM_LOGE("module", "'%s' init failed rc=%d",
                        _modules[i].name ? _modules[i].name : "(null)", r);
                _rollback_inits(i);
                return r;
            }
        }
    }
    s = BM_CRITICAL_ENTER();
    _modules_initialized = 1;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

/**
 * @brief 依次启动已初始化的模块
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
int bm_module_start_all(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    int initialized = _modules_initialized;

    BM_CRITICAL_EXIT(s);
    if (!initialized) {
        return BM_ERR_NOT_INIT;
    }

    for (uint32_t i = 0u; i < _module_count; i++) {
        if (_modules[i].state == BM_MODULE_STATE_INITED && _modules[i].start) {
            int r = _modules[i].start();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STARTED;
                BM_LOGD("module", "'%s' started",
                        _modules[i].name ? _modules[i].name : "(null)");
            } else {
                BM_LOGE("module", "'%s' start failed rc=%d",
                        _modules[i].name ? _modules[i].name : "(null)", r);
                _rollback_starts(i);
                return r;
            }
        }
    }
    return BM_OK;
}

/**
 * @brief 逆序停止已启动的模块
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
int bm_module_stop_all(void) {
    int rc = BM_OK;
    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].state == BM_MODULE_STATE_STARTED && _modules[i].stop) {
            int r = _modules[i].stop();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STOPPED;
            } else {
                BM_LOGE("module", "stop failed idx=%d rc=%d", i, r);
                rc = r;
            }
        }
    }
    BM_LOGI("module", "stop_all done");
    return rc;
}

/**
 * @brief 逆序反初始化所有模块
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
int bm_module_deinit_all(void) {
    bm_irq_state_t s;
    int rc = BM_OK;

    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].deinit) {
            int r = _modules[i].deinit();
            if (r != BM_OK) {
                rc = r;
            }
        }
        _modules[i].state = BM_MODULE_STATE_UNINIT;
    }
    s = BM_CRITICAL_ENTER();
    _modules_initialized = 0;
    _module_count = 0u;
    BM_CRITICAL_EXIT(s);
    BM_LOGI("module", "deinit_all done");
    return rc;
}
