/**
 * @file bm_resource.c
 * @brief 硬件资源声明与冲突检测实现
 *
 * 将多实例资源声明展平后检查互斥、共享读与协调访问规则。
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
#include "bm_resource.h"
#include "bm_log.h"

#include <stddef.h>

/** 展平后的资源声明条目 */
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

/**
 * @brief 判断两条资源声明是否兼容
 *
 * @param a 第一条展平声明
 * @param b 第二条展平声明
 * @return 1 兼容；0 冲突
 */
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

/**
 * @brief 校验 owner/reader 配对与 share_group 规则
 *
 * @param flat 展平后的声明数组
 * @param flat_count 声明条目数量
 * @return BM_OK 通过；BM_ERR_INVALID 配对或 share_group 无效
 */
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
                BM_LOGE("resource", "owner/reader mismatch kind=%u key=%u",
                        (unsigned)flat[i].kind, (unsigned)flat[i].key);
                return BM_ERR_INVALID;
            }
            for (j = 0u; j < flat_count; ++j) {
                if (flat[i].kind != flat[j].kind || flat[i].key != flat[j].key) {
                    continue;
                }
                if (flat[j].access == BM_RESOURCE_OWNER ||
                    flat[j].access == BM_RESOURCE_SHARED_READ) {
                    if (flat[j].share_group == 0u) {
                        BM_LOGE("resource", "missing share_group kind=%u",
                                (unsigned)flat[j].kind);
                        return BM_ERR_INVALID;
                    }
                }
            }
        }
    }

    return BM_OK;
}

/**
 * @brief 检查多实例资源声明是否存在冲突
 *
 * @param claims 各实例资源声明数组的指针数组
 * @param claim_counts 各实例声明数量数组
 * @param instance_count 实例数量
 * @return BM_OK 无冲突；BM_ERR_INVALID 参数无效；BM_ERR_OVERFLOW 声明表溢出；BM_ERR_BUSY 存在冲突
 */
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
        BM_LOGE("resource", "invalid instance_count=%u", (unsigned)instance_count);
        return BM_ERR_INVALID;
    }

    for (i = 0u; i < instance_count; ++i) {
        if (claim_counts[i] > 0u && !claims[i]) {
            return BM_ERR_INVALID;
        }
        for (j = 0u; j < claim_counts[i]; ++j) {
            if (flat_count >= BM_CONFIG_MAX_RESOURCE_CLAIMS) {
                BM_LOGE("resource", "claim table overflow");
                return BM_ERR_OVERFLOW;
            }
            if (claims[i][j].access > BM_RESOURCE_SHARED_COORDINATED) {
                return BM_ERR_INVALID;
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
                BM_LOGW("resource", "conflict inst %u vs %u kind=%u key=%u",
                        (unsigned)flat[i].instance_index,
                        (unsigned)flat[j].instance_index,
                        (unsigned)flat[i].kind, (unsigned)flat[i].key);
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
                    BM_LOGW("resource", "coordinated mixed access kind=%u",
                            (unsigned)flat[i].kind);
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

    BM_LOGD("resource", "check ok instances=%u claims=%u",
            (unsigned)instance_count, (unsigned)flat_count);
    return validate_owner_reader_pairs(flat, flat_count);
}
