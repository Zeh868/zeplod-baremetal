#include "bm_sync.h"

static int g_configured;
static int g_armed;
static int g_triggered;

int bm_sync_hal_configure(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 1;
    g_armed = 0;
    g_triggered = 0;
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
    (void)domain;
    if (!g_armed) {
        return BM_ERR_NOT_INIT;
    }
    g_triggered = 1;
    return BM_OK;
}

void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain) {
    (void)domain;
    g_configured = 0;
    g_armed = 0;
    g_triggered = 0;
}

int bm_sync_hal_native_triggered(void) {
    return g_triggered;
}
