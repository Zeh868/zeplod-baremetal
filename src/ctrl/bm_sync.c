/**
 * @file bm_sync.c
 * @brief 多控制实例硬件同步域实现
 *
 * 校验域描述后委托 HAL 完成配置、武装与触发。
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
#include "bm_sync.h"
#include "bm_log.h"

int bm_sync_hal_configure(const bm_sync_domain_t *domain);
int bm_sync_hal_arm(const bm_sync_domain_t *domain);
int bm_sync_hal_trigger(const bm_sync_domain_t *domain);
void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain);

static const bm_sync_domain_t *g_active_domain;

/**
 * @brief 校验同步域描述符字段完整性
 *
 * @param domain 同步域描述符指针
 * @return BM_OK 有效；BM_ERR_INVALID 参数无效
 */
static int validate_domain(const bm_sync_domain_t *domain) {
    if (!domain || !domain->master_timer || !domain->members ||
        !domain->phase_ticks || domain->member_count == 0u) {
        BM_LOGE("sync", "invalid domain descriptor");
        return BM_ERR_INVALID;
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
    rc = bm_sync_hal_configure(domain);
    if (rc != BM_OK) {
        BM_LOGE("sync", "hal configure failed rc=%d", rc);
        return rc;
    }
    g_active_domain = domain;
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
    } else {
        BM_LOGI("sync", "armed");
    }
    return rc;
}

/**
 * @brief 触发同步域，启动成员定时器对齐
 *
 * @param domain 同步域描述符指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NOT_INIT 未先配置；其他为 HAL 错误码
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
    rc = bm_sync_hal_trigger(domain);
    if (rc != BM_OK) {
        BM_LOGE("sync", "hal trigger failed rc=%d", rc);
    } else {
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
    }
    if (g_active_domain == domain) {
        g_active_domain = NULL;
    }
    BM_LOGI("sync", "safe stop");
}
