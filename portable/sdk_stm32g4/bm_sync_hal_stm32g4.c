/**
 * @file bm_sync_hal_stm32g4.c
 * @brief STM32G4 同步域 HAL（CMSIS 主从定时器占位）
 */
#include "bm_sync.h"
#include "bm_hal_sdk_stm32g4.h"
#include "bm_log.h"
#include "bm_types.h"

#define TAG "sync_hal"

static int g_configured;
static int g_armed;

int bm_sync_hal_configure(const bm_sync_domain_t *domain) {
    if (!domain || !domain->master_timer || domain->member_count == 0u) {
        return BM_ERR_INVALID;
    }
    g_configured = 1;
    g_armed = 0;
    BM_LOGI(TAG, "configure: %s members=%u",
            domain->name ? domain->name : "(null)", domain->member_count);
    return BM_OK;
}

int bm_sync_hal_arm(const bm_sync_domain_t *domain) {
    (void)domain;
    if (!g_configured) {
        return BM_ERR_NOT_INIT;
    }
    g_armed = 1;
    return BM_OK;
}

int bm_sync_hal_trigger(const bm_sync_domain_t *domain) {
    uint32_t i;

    (void)domain;
    if (!g_armed) {
        return BM_ERR_NOT_INIT;
    }

    for (i = 0u; i < domain->member_count; ++i) {
        TIM1->EGR = TIM_EGR_UG;
    }
    return BM_OK;
}

void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 0;
    g_armed = 0;
    TIM1->BDTR &= (uint32_t)~TIM_BDTR_MOE;
}
