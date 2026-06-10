/**
 * @file bm_sync_hal_native.c
 * @brief 原生仿真环境同步域 HAL 实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_sync.h"
#include "bm_log.h"

#define TAG "hal_sync"

static int g_configured;
static int g_armed;
static int g_triggered;

/** 配置同步域并重置状态 */
int bm_sync_hal_configure(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 1;
    g_armed = 0;
    g_triggered = 0;
    BM_LOGI(TAG, "configure");
    return BM_OK;
}

/** 武装同步域，等待 trigger */
int bm_sync_hal_arm(const bm_sync_domain_t *domain) {
    (void)domain;
    if (!g_configured) {
        BM_LOGE(TAG, "arm: not configured");
        return BM_ERR_NOT_INIT;
    }
    g_armed = 1;
    BM_LOGI(TAG, "arm");
    return BM_OK;
}

/** 触发同步域（须先 arm） */
int bm_sync_hal_trigger(const bm_sync_domain_t *domain) {
    (void)domain;
    if (!g_armed) {
        BM_LOGE(TAG, "trigger: not armed");
        return BM_ERR_NOT_INIT;
    }
    g_triggered = 1;
    BM_LOGI(TAG, "trigger");
    return BM_OK;
}

/** 安全停止：清除配置与武装状态 */
void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 0;
    g_armed = 0;
    g_triggered = 0;
    BM_LOGI(TAG, "safe_stop");
}

/** 测试辅助：查询是否已触发 */
int bm_sync_hal_native_triggered(void) {
    return g_triggered;
}
