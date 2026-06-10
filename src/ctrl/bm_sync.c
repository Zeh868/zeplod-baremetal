#include "bm_sync.h"

int bm_sync_hal_configure(const bm_sync_domain_t *domain);
int bm_sync_hal_arm(const bm_sync_domain_t *domain);
int bm_sync_hal_trigger(const bm_sync_domain_t *domain);
void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain);

static const bm_sync_domain_t *g_active_domain;

static int validate_domain(const bm_sync_domain_t *domain) {
    if (!domain || !domain->master_timer || !domain->members ||
        !domain->phase_ticks || domain->member_count == 0u) {
        return BM_ERR_INVALID;
    }
    return BM_OK;
}

int bm_sync_configure(const bm_sync_domain_t *domain) {
    int rc = validate_domain(domain);
    if (rc != BM_OK) {
        return rc;
    }
    rc = bm_sync_hal_configure(domain);
    if (rc != BM_OK) {
        return rc;
    }
    g_active_domain = domain;
    return BM_OK;
}

int bm_sync_arm(const bm_sync_domain_t *domain) {
    int rc = validate_domain(domain);
    if (rc != BM_OK) {
        return rc;
    }
    if (g_active_domain != domain) {
        return BM_ERR_NOT_INIT;
    }
    return bm_sync_hal_arm(domain);
}

int bm_sync_trigger(const bm_sync_domain_t *domain) {
    int rc = validate_domain(domain);
    if (rc != BM_OK) {
        return rc;
    }
    if (g_active_domain != domain) {
        return BM_ERR_NOT_INIT;
    }
    return bm_sync_hal_trigger(domain);
}

void bm_sync_safe_stop(const bm_sync_domain_t *domain) {
    if (domain) {
        bm_sync_hal_safe_stop(domain);
    }
    if (g_active_domain == domain) {
        g_active_domain = NULL;
    }
}
