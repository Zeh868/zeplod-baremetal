/**
 * @file bm_wdg.c
 * @brief 软件看门狗模块注册与喂狗实现
 *
 * 所有已注册模块均按时喂狗后才向硬件 WDG 提交喂狗信号。
 * @author zeh (china_qzh@163.com)
 * @version 1.2
 * @date 2026-06-11
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-10       1.1            zeh            模块超时窗口与空指针防护
 * 2026-06-11       1.2            zeh            独立 fed 标志、临界区、防重名注册
 *
 */
#include "bm_wdg.h"
#include "bm_critical_wrap.h"
#include "bm_hal_timer.h"
#include "bm_hal_wdg.h"
#include "bm_log.h"
#include "bm_time.h"

#include <stdbool.h>
#include <string.h>

#ifndef BM_CONFIG_WDG_MODULE_TIMEOUT_MS
#define BM_CONFIG_WDG_MODULE_TIMEOUT_MS 1000
#endif

/** 已注册模块记录 */
typedef struct {
    const char *name;
    uint32_t    last_feed_ticks;
    bool        fed;
} bm_wdg_module_t;

static bm_wdg_module_t _wdg_modules[BM_CONFIG_MAX_WDG_MODULES];
static uint32_t        _wdg_module_count = 0;

/**
 * @brief 将毫秒超时转换为定时器 tick（溢出安全）
 */
static int wdg_timeout_ticks(uint32_t *ticks_out) {
    int rc = bm_time_ms_to_ticks(BM_CONFIG_WDG_MODULE_TIMEOUT_MS, ticks_out);

    if (rc == BM_ERR_INVALID) {
        return BM_ERR_NOT_INIT;
    }
    if (rc != BM_OK) {
        return rc;
    }
    if (*ticks_out == 0u) {
        *ticks_out = 1u;
    }
    return BM_OK;
}

/**
 * @brief 判断模块在本周期内是否已按时喂狗
 */
static int wdg_module_fresh(const bm_wdg_module_t *mod, uint32_t now,
                            uint32_t timeout_ticks) {
    uint32_t elapsed;

    if (!mod->fed) {
        return 0;
    }
    elapsed = now - mod->last_feed_ticks;
    return elapsed <= timeout_ticks;
}

/**
 * @brief 注册一个需喂狗的软件模块
 *
 * @param name 模块名称字符串
 * @return BM_OK 成功；BM_ERR_NO_MEM 注册表已满；BM_ERR_INVALID 参数无效；
 *         BM_ERR_ALREADY 同名模块已注册
 */
int bm_wdg_register(const char *name) {
    bm_irq_state_t s;

    if (!name) {
        return BM_ERR_INVALID;
    }

    s = BM_CRITICAL_ENTER();
    if (_wdg_module_count >= BM_CONFIG_MAX_WDG_MODULES) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("wdg", "register table full");
        return BM_ERR_NO_MEM;
    }
    for (uint32_t i = 0u; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].name && strcmp(_wdg_modules[i].name, name) == 0) {
            BM_CRITICAL_EXIT(s);
            BM_LOGW("wdg", "module '%s' already registered", name);
            return BM_ERR_ALREADY;
        }
    }

    _wdg_modules[_wdg_module_count].name = name;
    _wdg_modules[_wdg_module_count].last_feed_ticks = 0u;
    _wdg_modules[_wdg_module_count].fed = false;
    _wdg_module_count++;
    BM_CRITICAL_EXIT(s);

    BM_LOGI("wdg", "module '%s' registered", name);
    return BM_OK;
}

/**
 * @brief 记录指定模块的喂狗时间戳
 *
 * @param name 模块名称字符串
 */
void bm_wdg_feed_module(const char *name) {
    bm_irq_state_t s;

    if (!name) {
        BM_LOGW("wdg", "feed_module null name");
        return;
    }

    s = BM_CRITICAL_ENTER();
    for (uint32_t i = 0u; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].name && strcmp(_wdg_modules[i].name, name) == 0) {
            _wdg_modules[i].last_feed_ticks = bm_hal_timer_get_ticks();
            _wdg_modules[i].fed = true;
            BM_CRITICAL_EXIT(s);
            BM_LOGT("wdg", "feed module '%s'", name);
            return;
        }
    }
    BM_CRITICAL_EXIT(s);
    BM_LOGW("wdg", "feed unknown module '%s'", name);
}

/**
 * @brief 检查所有模块均已按时喂狗后向硬件看门狗提交喂狗信号
 */
void bm_wdg_feed(void) {
    uint32_t timeout_ticks = 0u;
    int rc;
    uint32_t now;
    uint32_t i;
    bm_irq_state_t s;

    s = BM_CRITICAL_ENTER();
    if (_wdg_module_count == 0u) {
        BM_CRITICAL_EXIT(s);
        bm_hal_wdg_feed();
        BM_LOGT("wdg", "hw feed (no modules)");
        return;
    }

    rc = wdg_timeout_ticks(&timeout_ticks);
    if (rc != BM_OK) {
        BM_CRITICAL_EXIT(s);
        BM_LOGD("wdg", "hw feed blocked: timer not ready rc=%d", rc);
        return;
    }

    now = bm_hal_timer_get_ticks();
    for (i = 0u; i < _wdg_module_count; i++) {
        if (!wdg_module_fresh(&_wdg_modules[i], now, timeout_ticks)) {
            BM_CRITICAL_EXIT(s);
            BM_LOGD("wdg", "hw feed blocked: '%s' stale",
                    _wdg_modules[i].name ? _wdg_modules[i].name : "(null)");
            return;
        }
    }

    for (i = 0u; i < _wdg_module_count; i++) {
        _wdg_modules[i].fed = false;
        _wdg_modules[i].last_feed_ticks = 0u;
    }
    BM_CRITICAL_EXIT(s);

    bm_hal_wdg_feed();
    BM_LOGT("wdg", "hw feed all modules ok");
}

/**
 * @brief 清空软件看门狗注册表（仅用于测试或停机）
 */
void bm_wdg_reset(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    _wdg_module_count = 0u;
    BM_CRITICAL_EXIT(s);
}
