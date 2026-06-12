/**
 * @file bm_sync_hal_stub.c
 * @brief 同步域 HAL 空实现（未链接平台 HAL 时使用）
 *
 * 所有操作返回 BM_ERR_NOT_INIT，safe_stop 为空操作。
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

/**
 * @brief 同步域 HAL 配置桩实现（未链接平台 HAL）
 *
 * @param domain 同步域描述符指针
 * @return BM_ERR_NOT_INIT 始终返回未初始化
 */
int bm_sync_hal_configure(const bm_sync_domain_t *domain) {
    (void)domain;
    BM_LOGW("sync_hal", "stub configure: no HAL linked");
    return BM_ERR_NOT_INIT;
}

/**
 * @brief 同步域 HAL 武装桩实现（未链接平台 HAL）
 *
 * @param domain 同步域描述符指针
 * @return BM_ERR_NOT_INIT 始终返回未初始化
 */
int bm_sync_hal_arm(const bm_sync_domain_t *domain) {
    (void)domain;
    BM_LOGW("sync_hal", "stub arm: no HAL linked");
    return BM_ERR_NOT_INIT;
}

/**
 * @brief 同步域 HAL 触发桩实现（未链接平台 HAL）
 *
 * @param domain 同步域描述符指针
 * @return BM_ERR_NOT_INIT 始终返回未初始化
 */
int bm_sync_hal_trigger(const bm_sync_domain_t *domain) {
    (void)domain;
    BM_LOGW("sync_hal", "stub trigger: no HAL linked");
    return BM_ERR_NOT_INIT;
}

/**
 * @brief 同步域 HAL 安全停止桩实现（空操作）
 *
 * @param domain 同步域描述符指针
 */
void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain) {
    (void)domain;
    BM_LOGW("sync_hal", "stub safe_stop: no HAL linked");
}
