/**
 * @file bm_sync_hal_qemu.c
 * @brief QEMU Cortex-M0 同步域 HAL 实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 */

#include "bm_sync.h"
#include "bm_log.h"

#define TAG "hal_sync"

static int g_configured;
static int g_armed;

int bm_sync_hal_configure(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 1;
    g_armed = 0;
    BM_LOGI(TAG, "configure");
    return BM_OK;
}

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

int bm_sync_hal_trigger(const bm_sync_domain_t *domain) {
    uint32_t i;
    uint32_t s;

    if (!g_armed || !domain) {
        BM_LOGE(TAG, "trigger: not armed");
        return BM_ERR_NOT_INIT;
    }

    for (i = 0u; i < domain->member_count; ++i) {
        const bm_ctrl_inst_t *inst = domain->members[i];
        if (!inst || !inst->slots) {
            continue;
        }
        for (s = 0u; s < inst->slot_count; ++s) {
            if (inst->slots[s].step) {
                inst->slots[s].step(inst);
            }
        }
    }
    BM_LOGI(TAG, "trigger");
    return BM_OK;
}

void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 0;
    g_armed = 0;
    BM_LOGI(TAG, "safe_stop");
}
