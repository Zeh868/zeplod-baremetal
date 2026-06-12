/**
 * @file bm_resource.h
 * @brief 硬件资源声明与冲突检测
 *
 * 控制实例通过 bm_resource_claim_t 声明 PWM、ADC、定时器等资源的访问模式，
 * 启动前由框架校验跨实例冲突。
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
#ifndef BM_RESOURCE_H
#define BM_RESOURCE_H

#include "bm_types.h"

#include <stdint.h>

/** 硬件资源类别 */
typedef enum {
    BM_RESOURCE_PWM,
    BM_RESOURCE_ADC_GROUP,
    BM_RESOURCE_TIMER,
    BM_RESOURCE_DMA_CHANNEL,
    BM_RESOURCE_GPIO,
    BM_RESOURCE_COMP,
    BM_RESOURCE_IRQ
} bm_resource_kind_t;

/** 资源访问模式 */
typedef enum {
    BM_RESOURCE_EXCLUSIVE,           /**< 独占 */
    BM_RESOURCE_OWNER,               /**< 所有者（可协调共享） */
    BM_RESOURCE_SHARED_READ,         /**< 只读共享 */
    BM_RESOURCE_SHARED_COORDINATED   /**< 协调式共享 */
} bm_resource_access_t;

/** 单条资源声明 */
typedef struct {
    bm_resource_kind_t kind;
    uintptr_t key;
    bm_resource_access_t access;
    uintptr_t share_group;
    const char *name;
} bm_resource_claim_t;

/**
 * @brief 检查多实例资源声明是否存在冲突
 *
 * 非可重入，仅限主上下文调用（使用内部静态展平缓冲）。
 *
 * @param claims 各实例资源声明数组的指针数组
 * @param claim_counts 各实例声明条数数组
 * @param instance_count 实例数量
 * @return BM_OK 无冲突；BM_ERR_INVALID 参数无效；其他为冲突错误码
 */
int bm_resource_check_conflicts(const bm_resource_claim_t *const *claims,
                                const uint32_t *claim_counts,
                                uint32_t instance_count);

#endif /* BM_RESOURCE_H */
