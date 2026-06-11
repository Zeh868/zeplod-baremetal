/**
 * @file bm_ctrl_inst.h
 * @brief 控制实例描述与批量生命周期管理
 *
 * 定义混合域控制实例的 slot、ops 及资源声明，支持多实例统一 init/start/stop。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-11
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-11       1.1            zeh            补充 start_all 会话语义
 *
 */
#ifndef BM_CTRL_INST_H
#define BM_CTRL_INST_H

#include "bm_hal_hrt.h"
#include "bm_hrt.h"
#include "bm_resource.h"
#include "bm_types.h"

typedef struct bm_ctrl_inst bm_ctrl_inst_t;

/** 控制实例批次会话状态（单 MCU 单运行时，见 bm_runtime_model.h） */
typedef enum {
    BM_CTRL_SESSION_NONE = 0,
    BM_CTRL_SESSION_INITED,
    BM_CTRL_SESSION_STARTED,
    BM_CTRL_SESSION_STOPPING
} bm_ctrl_session_t;

/** 单步控制算法回调 */
typedef void (*bm_ctrl_step_fn_t)(const bm_ctrl_inst_t *instance);

/** 控制 slot 调度类型 */
typedef enum {
    BM_CTRL_SLOT_HARDWARE,   /**< 硬件触发（PWM/ADC 等） */
    BM_CTRL_SLOT_SCHEDULED /**< 软件定时调度 */
} bm_ctrl_slot_kind_t;

/** 控制 loop 的一个执行 slot */
typedef struct {
    bm_ctrl_slot_kind_t kind;
    uint32_t period_us;
    bm_hrt_trigger_t trigger;
    bm_ctrl_step_fn_t step;
    int (*bind_hardware)(const bm_ctrl_inst_t *instance,
                         const bm_hal_hrt_binding_t *binding);
    const char *name;
} bm_ctrl_slot_t;

/** 实例级生命周期操作 */
typedef struct {
    int (*init)(const bm_ctrl_inst_t *instance);
    int (*start)(const bm_ctrl_inst_t *instance);
    void (*safe_stop)(const bm_ctrl_inst_t *instance);
} bm_ctrl_ops_t;

/** 控制实例描述符 */
struct bm_ctrl_inst {
    uint32_t id;
    const char *name;
    void *state;
    const void *config;
    const void *resources;
    const bm_ctrl_slot_t *slots;
    uint32_t slot_count;
    const bm_resource_claim_t *claims;
    uint32_t claim_count;
    const bm_ctrl_ops_t *ops;
};

/**
 * @brief 批量初始化控制实例
 *
 * @param instances 控制实例指针数组
 * @param count 实例数量
 * @return BM_OK 全部成功；否则为首个失败实例的错误码
 */
int bm_ctrl_init_all(const bm_ctrl_inst_t *const *instances, uint32_t count);

/**
 * @brief 批量启动控制实例
 *
 * 先对各实例调用 ops->start，全部成功后再 bm_hrt_start（若存在 Scheduled 槽）。
 * 所有 Scheduled / Hardware 槽的 step 仅在会话进入 STARTED 后执行。
 *
 * @param instances 控制实例指针数组（须与 init_all 时一致）
 * @param count 实例数量
 * @return BM_OK 全部成功；BM_ERR_ALREADY 已启动；BM_ERR_NOT_INIT 未 init；
 *         BM_ERR_INVALID 参数不匹配；否则为首个失败实例的错误码
 */
int bm_ctrl_start_all(const bm_ctrl_inst_t *const *instances, uint32_t count);

/**
 * @brief 批量安全停止控制实例
 *
 * 始终使用 init_all 保存的内部实例表执行停机。外部数组仅保留用于 API
 * 兼容，不会被解引用，也不会决定实际停止对象。
 *
 * @param instances 可为 NULL；不参与实际停机
 * @param count 外部实例数量，仅用于参数一致性诊断
 */
void bm_ctrl_safe_stop_all(const bm_ctrl_inst_t *const *instances,
                           uint32_t count);

/**
 * @brief 按 ID 查找控制实例
 *
 * @param instances 控制实例指针数组
 * @param count 实例数量
 * @param id 目标实例 ID
 * @return 匹配的实例指针；未找到时返回 NULL
 */
const bm_ctrl_inst_t *bm_ctrl_find(const bm_ctrl_inst_t *const *instances,
                                   uint32_t count,
                                   uint32_t id);

/**
 * @brief 查询当前控制批次会话状态（只读，临界区内快照）
 */
bm_ctrl_session_t bm_ctrl_get_session(void);

#endif /* BM_CTRL_INST_H */
