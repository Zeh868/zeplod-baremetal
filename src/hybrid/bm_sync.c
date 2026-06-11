/**
 * @file bm_sync.c
 * @brief 多控制实例硬件同步域实现
 *
 * 校验域描述后委托 HAL 完成配置、武装与触发。
 * @author zeh (china_qzh@163.com)
 * @version 1.2
 * @date 2026-06-11
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-11       1.1            zeh            域切换清理与框架层 armed 状态
 * 2026-06-11       1.2            zeh            member_count 上界校验
 * 2026-06-11       1.3            zeh            phase_ticks 上界校验
 *
 */
#include "bm_sync.h"
#include "bm_config.h"
#include "bm_sync_hal.h"
#include "bm_log.h"

#include <stdbool.h>

static const bm_sync_domain_t *g_active_domain;
static bool g_armed;

/**
 * @brief 校验同步域描述符字段完整性
 *
 * @param domain 同步域描述符指针
 * @return BM_OK 有效；BM_ERR_INVALID 参数无效
 */
static int validate_domain(const bm_sync_domain_t *domain) {
    uint32_t i;

    if (!domain || !domain->master_timer || !domain->members ||
        !domain->phase_ticks || domain->member_count == 0u) {
        BM_LOGE("sync", "invalid domain descriptor");
        return BM_ERR_INVALID;
    }
    if (domain->member_count > BM_CONFIG_MAX_SYNC_MEMBERS) {
        BM_LOGE("sync", "member_count overflow %u",
                (unsigned)domain->member_count);
        return BM_ERR_INVALID;
    }
    for (i = 0u; i < domain->member_count; ++i) {
        if (!domain->members[i]) {
            BM_LOGE("sync", "null member at index %u", (unsigned)i);
            return BM_ERR_INVALID;
        }
        if (domain->phase_ticks[i] > BM_CONFIG_SYNC_MAX_PHASE_TICKS) {
            BM_LOGE("sync", "phase_ticks[%u] overflow %u",
                    (unsigned)i, (unsigned)domain->phase_ticks[i]);
            return BM_ERR_INVALID;
        }
    }
    return BM_OK;
}

/**
 * @brief 配置同步域并委托 HAL 完成硬件设置
 *
 * @param domain 同步域描述符指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；其他为 HAL 错误码
 */
int bm_sync_configure(const bm_sync_domain_t *domain) {
    int rc = validate_domain(domain);
    if (rc != BM_OK) {
        return rc;
    }
    if (g_active_domain != NULL && g_active_domain != domain) {
        bm_sync_hal_safe_stop(g_active_domain);
        g_active_domain = NULL;
        g_armed = false;
    }
    rc = bm_sync_hal_configure(domain);
    if (rc != BM_OK) {
        BM_LOGE("sync", "hal configure failed rc=%d", rc);
        return rc;
    }
    g_active_domain = domain;
    g_armed = false;
    BM_LOGI("sync", "configured %u members", (unsigned)domain->member_count);
    return BM_OK;
}

/**
 * @brief 武装已配置的同步域，准备触发
 *
 * @param domain 同步域描述符指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NOT_INIT 未先配置；其他为 HAL 错误码
 */
int bm_sync_arm(const bm_sync_domain_t *domain) {
    int rc = validate_domain(domain);
    if (rc != BM_OK) {
        return rc;
    }
    if (g_active_domain != domain) {
        BM_LOGW("sync", "arm before configure");
        return BM_ERR_NOT_INIT;
    }
    rc = bm_sync_hal_arm(domain);
    if (rc != BM_OK) {
        BM_LOGE("sync", "hal arm failed rc=%d", rc);
        g_armed = false;
    } else {
        g_armed = true;
        BM_LOGI("sync", "armed");
    }
    return rc;
}

/**
 * @brief 触发同步域，启动成员定时器对齐
 *
 * @param domain 同步域描述符指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NOT_INIT 未先配置或未武装；其他为 HAL 错误码
 */
int bm_sync_trigger(const bm_sync_domain_t *domain) {
    int rc = validate_domain(domain);
    if (rc != BM_OK) {
        return rc;
    }
    if (g_active_domain != domain) {
        BM_LOGW("sync", "trigger before configure");
        return BM_ERR_NOT_INIT;
    }
    if (!g_armed) {
        BM_LOGW("sync", "trigger before arm");
        return BM_ERR_NOT_INIT;
    }
    rc = bm_sync_hal_trigger(domain);
    if (rc != BM_OK) {
        BM_LOGE("sync", "hal trigger failed rc=%d", rc);
        g_armed = false;
    } else {
        g_armed = false;
        BM_LOGD("sync", "triggered");
    }
    return rc;
}

/**
 * @brief 安全停止同步域并清除活动域记录
 *
 * @param domain 同步域描述符指针（可为 NULL，仅清除活动域）
 */
void bm_sync_safe_stop(const bm_sync_domain_t *domain) {
    if (domain) {
        bm_sync_hal_safe_stop(domain);
        if (g_active_domain == domain) {
            g_active_domain = NULL;
        }
    } else {
        if (g_active_domain) {
            bm_sync_hal_safe_stop(g_active_domain);
        }
        g_active_domain = NULL;
    }
    g_armed = false;
    BM_LOGI("sync", "safe stop");
}
