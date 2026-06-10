/**
 * @file bm_hrt.h
 * @brief 高分辨率定时（HRT）调度器
 *
 * 管理微秒级周期 slot，支持定时器、PWM 更新、ADC 完成等多种触发源。
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
#ifndef BM_HRT_H
#define BM_HRT_H

#include "bm_types.h"

/** HRT slot 到期回调 */
typedef void (*bm_hrt_callback_t)(void *context);

/** 硬件触发源类型 */
typedef enum {
    BM_HRT_TRIGGER_TIMER,
    BM_HRT_TRIGGER_PWM_UPDATE,
    BM_HRT_TRIGGER_ADC_COMPLETE
} bm_hrt_trigger_t;

/** HRT 调度 slot 描述 */
typedef struct {
    uint32_t period_us;
    bm_hrt_trigger_t trigger;
    bm_hrt_callback_t callback;
    void *context;
    const char *name;
} bm_hrt_slot_t;

/**
 * @brief 初始化 HRT 调度器并注册 slot 表
 *
 * @param slots slot 描述数组
 * @param slot_count slot 数量
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；其他为平台错误码
 */
int bm_hrt_init(const bm_hrt_slot_t *slots, uint32_t slot_count);

/**
 * @brief 启动 HRT 调度
 *
 * @return BM_OK 成功；BM_ERR_NOT_INIT 未初始化；其他为平台错误码
 */
int bm_hrt_start(void);

/**
 * @brief 停止 HRT 调度
 */
void bm_hrt_stop(void);

/**
 * @brief 重置 HRT 调度器状态
 */
void bm_hrt_reset(void);

/**
 * @brief 错过 deadline 时的弱符号钩子（可覆盖）
 *
 * @param slot 触发 miss 的 slot 描述指针
 */
void bm_hrt_deadline_missed_hook(const bm_hrt_slot_t *slot);

#endif /* BM_HRT_H */
