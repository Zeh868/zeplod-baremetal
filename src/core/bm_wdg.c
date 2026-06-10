/**
 * @file bm_wdg.c
 * @brief 软件看门狗模块注册与喂狗实现
 *
 * 所有已注册模块均喂狗后才向硬件 WDG 提交喂狗信号。
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
#include "bm_wdg.h"
#include "bm_hal_wdg.h"
#include "bm_hal_timer.h"
#include "bm_log.h"

#include <string.h>

/** 已注册模块记录 */
typedef struct {
    const char *name;
    uint32_t    last_feed_ticks;
} bm_wdg_module_t;

static bm_wdg_module_t _wdg_modules[BM_CONFIG_MAX_WDG_MODULES];
static uint32_t        _wdg_module_count = 0;

/**
 * @brief 注册一个需喂狗的软件模块
 *
 * @param name 模块名称字符串
 * @return BM_OK 成功；BM_ERR_NO_MEM 注册表已满
 */
int bm_wdg_register(const char *name) {
    if (_wdg_module_count >= BM_CONFIG_MAX_WDG_MODULES) {
        BM_LOGW("wdg", "register table full");
        return BM_ERR_NO_MEM;
    }
    _wdg_modules[_wdg_module_count].name = name;
    _wdg_modules[_wdg_module_count].last_feed_ticks = 0;
    _wdg_module_count++;
    BM_LOGI("wdg", "module '%s' registered", name ? name : "(null)");
    return BM_OK;
}

/**
 * @brief 记录指定模块的喂狗时间戳
 *
 * @param name 模块名称字符串
 */
void bm_wdg_feed_module(const char *name) {
    for (uint32_t i = 0; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].name && strcmp(_wdg_modules[i].name, name) == 0) {
            _wdg_modules[i].last_feed_ticks = bm_hal_timer_get_ticks();
            BM_LOGT("wdg", "feed module '%s'", name);
            return;
        }
    }
    BM_LOGW("wdg", "feed unknown module '%s'", name ? name : "(null)");
}

/**
 * @brief 检查所有模块均已喂狗后向硬件看门狗提交喂狗信号
 */
void bm_wdg_feed(void) {
    if (_wdg_module_count == 0) {
        bm_hal_wdg_feed();
        BM_LOGT("wdg", "hw feed (no modules)");
        return;
    }
    for (uint32_t i = 0; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].last_feed_ticks == 0) {
            BM_LOGD("wdg", "hw feed blocked: '%s' not fed",
                    _wdg_modules[i].name ? _wdg_modules[i].name : "(null)");
            return;
        }
    }
    bm_hal_wdg_feed();
    BM_LOGT("wdg", "hw feed all modules ok");
}
