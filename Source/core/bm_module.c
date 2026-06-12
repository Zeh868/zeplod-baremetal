/**
 * @file bm_module.c
 * @brief 模块生命周期管理实现
 *
 * 从应用提供的 _bm_module_table 加载模块，按优先级排序后依次 init/start/stop/deinit。
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
#include "bm_event.h"
#include "bm_log.h"

#include <stdbool.h>
#include <string.h>

extern const bm_module_t *_bm_module_table[];
extern const uint32_t     _bm_module_count;

static bm_module_t _modules[BM_CONFIG_MAX_MODULES];
static uint32_t    _module_count = 0;

enum {
    BM_MODULES_UNINITIALIZED = 0,
    BM_MODULES_INITIALIZING,
    BM_MODULES_READY,
    BM_MODULES_TRANSITIONING,
    BM_MODULES_CLEANUP_PENDING
};

static int _modules_initialized = BM_MODULES_UNINITIALIZED;

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
static int _rollback_inits(uint32_t through_index) {
    int rc = BM_OK;

    while (through_index > 0u) {
        through_index--;
        if (_modules[through_index].state == BM_MODULE_STATE_INITED) {
            if (_modules[through_index].deinit) {
                int r = _modules[through_index].deinit();

                if (r != BM_OK) {
                    BM_LOGE("module", "init rollback failed idx=%u rc=%d",
                            (unsigned)through_index, r);
                    if (rc == BM_OK) {
                        rc = r;
                    }
                    continue;
                }
            }
            _modules[through_index].state = BM_MODULE_STATE_UNINIT;
        }
    }
    return rc;
}

/**
 * @brief start 失败时逆序停止已启动模块
 */
static int _rollback_starts(uint32_t through_index) {
    int rc = BM_OK;

    while (through_index > 0u) {
        through_index--;
        if (_modules[through_index].state == BM_MODULE_STATE_STARTED) {
            if (_modules[through_index].stop) {
                int r = _modules[through_index].stop();

                if (r != BM_OK) {
                    BM_LOGE("module", "start rollback failed idx=%u rc=%d",
                            (unsigned)through_index, r);
                    if (rc == BM_OK) {
                        rc = r;
                    }
                    continue;
                }
            }
            _modules[through_index].state = BM_MODULE_STATE_INITED;
        }
    }
    return rc;
}

/**
 * @brief 从模块表加载并依次调用 init
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
static int _module_init_all(bool reset_event_bus) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    if (_modules_initialized != BM_MODULES_UNINITIALIZED) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("module", "init_all already done");
        return BM_ERR_ALREADY;
    }
    _modules_initialized = BM_MODULES_INITIALIZING;
    BM_CRITICAL_EXIT(s);

    if (reset_event_bus) {
        bm_event_reset();
    }

    if (_bm_module_count > BM_CONFIG_MAX_MODULES) {
        BM_LOGE("module", "module table truncated: %u > %u",
                (unsigned)_bm_module_count, (unsigned)BM_CONFIG_MAX_MODULES);
        s = BM_CRITICAL_ENTER();
        _modules_initialized = BM_MODULES_UNINITIALIZED;
        BM_CRITICAL_EXIT(s);
        return BM_ERR_OVERFLOW;
    }

    _module_count = _bm_module_count;
    for (uint32_t i = 0u; i < _module_count; i++) {
        if (_bm_module_table[i] == NULL) {
            BM_LOGE("module", "module table contains null entry idx=%u",
                    (unsigned)i);
            s = BM_CRITICAL_ENTER();
            _module_count = 0u;
            _modules_initialized = BM_MODULES_UNINITIALIZED;
            BM_CRITICAL_EXIT(s);
            return BM_ERR_INVALID;
        }
        memcpy(&_modules[i], _bm_module_table[i], sizeof(bm_module_t));
    }
    for (uint32_t i = 0u; i < _module_count; i++) {
        _modules[i].state = BM_MODULE_STATE_UNINIT;
    }
    _sort_modules();

    BM_LOGI("module", "init_all count=%u", (unsigned)_module_count);
    for (uint32_t i = 0u; i < _module_count; i++) {
        int r = _modules[i].init ? _modules[i].init() : BM_OK;

        if (r == BM_OK) {
            _modules[i].state = BM_MODULE_STATE_INITED;
            BM_LOGD("module", "'%s' inited",
                    _modules[i].name ? _modules[i].name : "(null)");
        } else {
            int rollback_rc;

            BM_LOGE("module", "'%s' init failed rc=%d",
                    _modules[i].name ? _modules[i].name : "(null)", r);
            rollback_rc = _rollback_inits(i);
            if (rollback_rc != BM_OK) {
                s = BM_CRITICAL_ENTER();
                _modules_initialized = BM_MODULES_CLEANUP_PENDING;
                BM_CRITICAL_EXIT(s);
            } else {
                s = BM_CRITICAL_ENTER();
                _module_count = 0u;
                _modules_initialized = BM_MODULES_UNINITIALIZED;
                BM_CRITICAL_EXIT(s);
            }
            return r;
        }
    }
    s = BM_CRITICAL_ENTER();
    _modules_initialized = BM_MODULES_READY;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_module_boot(void) {
    int r = _module_init_all(true);

    if (r != BM_OK) {
        return r;
    }
    return bm_module_start_all();
}

int bm_module_init_all(void) {
    return _module_init_all(false);
}

/**
 * @brief 依次启动已初始化的模块
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
int bm_module_start_all(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    int initialized = _modules_initialized;

    if (initialized == BM_MODULES_INITIALIZING ||
        initialized == BM_MODULES_TRANSITIONING ||
        initialized == BM_MODULES_CLEANUP_PENDING) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_BUSY;
    }
    if (initialized != BM_MODULES_READY) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_NOT_INIT;
    }
    _modules_initialized = BM_MODULES_TRANSITIONING;
    BM_CRITICAL_EXIT(s);

    for (uint32_t i = 0u; i < _module_count; i++) {
        if (_modules[i].state == BM_MODULE_STATE_INITED ||
            _modules[i].state == BM_MODULE_STATE_STOPPED) {
            int r = _modules[i].start ? _modules[i].start() : BM_OK;

            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STARTED;
                BM_LOGD("module", "'%s' started",
                        _modules[i].name ? _modules[i].name : "(null)");
            } else {
                BM_LOGE("module", "'%s' start failed rc=%d",
                        _modules[i].name ? _modules[i].name : "(null)", r);
                if (_rollback_starts(i) != BM_OK) {
                    s = BM_CRITICAL_ENTER();
                    _modules_initialized = BM_MODULES_CLEANUP_PENDING;
                    BM_CRITICAL_EXIT(s);
                } else {
                    s = BM_CRITICAL_ENTER();
                    _modules_initialized = BM_MODULES_READY;
                    BM_CRITICAL_EXIT(s);
                }
                return r;
            }
        }
    }
    s = BM_CRITICAL_ENTER();
    _modules_initialized = BM_MODULES_READY;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

/**
 * @brief 逆序停止已启动的模块
 *
 * @return BM_OK 全部成功；负值为首个失败模块的错误码
 */
int bm_module_stop_all(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    int rc = BM_OK;

    if (_modules_initialized == BM_MODULES_INITIALIZING ||
        _modules_initialized == BM_MODULES_TRANSITIONING ||
        _modules_initialized == BM_MODULES_CLEANUP_PENDING) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_BUSY;
    }
    if (_modules_initialized == BM_MODULES_UNINITIALIZED) {
        BM_CRITICAL_EXIT(s);
        return BM_OK;
    }
    _modules_initialized = BM_MODULES_TRANSITIONING;
    BM_CRITICAL_EXIT(s);

    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].state == BM_MODULE_STATE_STARTED) {
            int r = _modules[i].stop ? _modules[i].stop() : BM_OK;

            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STOPPED;
            } else {
                BM_LOGE("module", "stop failed idx=%d rc=%d", i, r);
                if (rc == BM_OK) {
                    rc = r;
                }
            }
        }
    }
    s = BM_CRITICAL_ENTER();
    _modules_initialized = BM_MODULES_READY;
    BM_CRITICAL_EXIT(s);
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

    s = BM_CRITICAL_ENTER();
    if (_modules_initialized == BM_MODULES_INITIALIZING ||
        _modules_initialized == BM_MODULES_TRANSITIONING) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_BUSY;
    }
    _modules_initialized = BM_MODULES_CLEANUP_PENDING;
    BM_CRITICAL_EXIT(s);

    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].state == BM_MODULE_STATE_STARTED) {
            int r = _modules[i].stop ? _modules[i].stop() : BM_OK;

            if (r != BM_OK) {
                BM_LOGE("module", "deinit stop failed idx=%d rc=%d", i, r);
                if (rc == BM_OK) {
                    rc = r;
                }
                continue;
            }
            _modules[i].state = BM_MODULE_STATE_STOPPED;
        }
        if (_modules[i].state != BM_MODULE_STATE_UNINIT &&
            _modules[i].deinit) {
            int r = _modules[i].deinit();
            if (r != BM_OK) {
                if (rc == BM_OK) {
                    rc = r;
                }
                continue;
            }
        }
        _modules[i].state = BM_MODULE_STATE_UNINIT;
    }
    if (rc != BM_OK) {
        return rc;
    }
    s = BM_CRITICAL_ENTER();
    _modules_initialized = BM_MODULES_UNINITIALIZED;
    _module_count = 0u;
    BM_CRITICAL_EXIT(s);
    BM_LOGI("module", "deinit_all done");
    return rc;
}
