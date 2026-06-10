#include "bm_resource.h"

#include <stddef.h>

typedef struct {
    bm_resource_kind_t kind;
    uintptr_t key;
    bm_resource_access_t access;
    uintptr_t share_group;
    uint32_t instance_index;
} flat_claim_t;

#ifndef BM_CONFIG_MAX_CTRL_INSTANCES
#define BM_CONFIG_MAX_CTRL_INSTANCES 16u
#endif

#ifndef BM_CONFIG_MAX_RESOURCE_CLAIMS
#define BM_CONFIG_MAX_RESOURCE_CLAIMS 64u
#endif

static int claims_compatible(const flat_claim_t *a, const flat_claim_t *b) {
    if (a->kind != b->kind || a->key != b->key) {
        return 1;
    }
    if (a->instance_index == b->instance_index) {
        return 1;
    }

    if (a->access == BM_RESOURCE_EXCLUSIVE ||
        b->access == BM_RESOURCE_EXCLUSIVE) {
        return 0;
    }

    if (a->access == BM_RESOURCE_OWNER && b->access == BM_RESOURCE_OWNER) {
        return 0;
    }

    if ((a->access == BM_RESOURCE_OWNER &&
         b->access == BM_RESOURCE_SHARED_READ) ||
        (a->access == BM_RESOURCE_SHARED_READ &&
         b->access == BM_RESOURCE_OWNER)) {
        return (a->share_group != 0u && a->share_group == b->share_group);
    }

    if (a->access == BM_RESOURCE_SHARED_READ &&
        b->access == BM_RESOURCE_SHARED_READ) {
        return 1;
    }

    if (a->access == BM_RESOURCE_SHARED_COORDINATED &&
        b->access == BM_RESOURCE_SHARED_COORDINATED) {
        return (a->share_group != 0u && a->share_group == b->share_group);
    }

    return 0;
}

static int validate_owner_reader_pairs(const flat_claim_t *flat,
                                       uint32_t flat_count) {
    uint32_t i;
    uint32_t j;

    for (i = 0u; i < flat_count; ++i) {
        uint32_t owner_count = 0u;
        uint32_t reader_count = 0u;

        for (j = 0u; j < flat_count; ++j) {
            if (flat[i].kind != flat[j].kind || flat[i].key != flat[j].key) {
                continue;
            }
            if (flat[j].access == BM_RESOURCE_OWNER) {
                owner_count++;
            }
            if (flat[j].access == BM_RESOURCE_SHARED_READ) {
                reader_count++;
            }
        }

        if (reader_count > 0u) {
            if (owner_count != 1u) {
                return BM_ERR_INVALID;
            }
            for (j = 0u; j < flat_count; ++j) {
                if (flat[i].kind != flat[j].kind || flat[i].key != flat[j].key) {
                    continue;
                }
                if (flat[j].access == BM_RESOURCE_OWNER ||
                    flat[j].access == BM_RESOURCE_SHARED_READ) {
                    if (flat[j].share_group == 0u) {
                        return BM_ERR_INVALID;
                    }
                }
            }
        }
    }

    return BM_OK;
}

int bm_resource_check_conflicts(const bm_resource_claim_t *const *claims,
                                const uint32_t *claim_counts,
                                uint32_t instance_count) {
    flat_claim_t flat[BM_CONFIG_MAX_RESOURCE_CLAIMS];
    uint32_t flat_count = 0u;
    uint32_t i;
    uint32_t j;
    uint32_t k;

    if (!claims || !claim_counts) {
        return BM_ERR_INVALID;
    }
    if (instance_count == 0u || instance_count > BM_CONFIG_MAX_CTRL_INSTANCES) {
        return BM_ERR_INVALID;
    }

    for (i = 0u; i < instance_count; ++i) {
        if (claim_counts[i] > 0u && !claims[i]) {
            return BM_ERR_INVALID;
        }
        for (j = 0u; j < claim_counts[i]; ++j) {
            if (flat_count >= BM_CONFIG_MAX_RESOURCE_CLAIMS) {
                return BM_ERR_OVERFLOW;
            }
            flat[flat_count].kind = claims[i][j].kind;
            flat[flat_count].key = claims[i][j].key;
            flat[flat_count].access = claims[i][j].access;
            flat[flat_count].share_group = claims[i][j].share_group;
            flat[flat_count].instance_index = i;
            flat_count++;
        }
    }

    for (i = 0u; i < flat_count; ++i) {
        for (j = i + 1u; j < flat_count; ++j) {
            if (!claims_compatible(&flat[i], &flat[j])) {
                return BM_ERR_BUSY;
            }
        }
    }

    for (i = 0u; i < flat_count; ++i) {
        if (flat[i].access == BM_RESOURCE_SHARED_COORDINATED) {
            int coordinated_peer = 0;
            for (k = 0u; k < flat_count; ++k) {
                if (i == k) {
                    continue;
                }
                if (flat[i].kind == flat[k].kind &&
                    flat[i].key == flat[k].key &&
                    flat[k].access != BM_RESOURCE_SHARED_COORDINATED) {
                    return BM_ERR_BUSY;
                }
                if (flat[i].kind == flat[k].kind &&
                    flat[i].key == flat[k].key &&
                    flat[k].access == BM_RESOURCE_SHARED_COORDINATED) {
                    coordinated_peer = 1;
                }
            }
            if (!coordinated_peer && flat_count > 1u) {
                for (k = 0u; k < flat_count; ++k) {
                    if (i != k && flat[i].kind == flat[k].kind &&
                        flat[i].key == flat[k].key) {
                        break;
                    }
                }
            }
            if (flat[i].share_group == 0u) {
                return BM_ERR_INVALID;
            }
        }
    }

    return validate_owner_reader_pairs(flat, flat_count);
}
