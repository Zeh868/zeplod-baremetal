/**
 * @file bm_exec.c
 * @brief 确定性执行实例的批量生命周期管理实现
 *
 * 校验实例与资源声明，组装 HRT 调度表，协调 init/start/stop 与硬件绑定。
 * @author zeh (china_qzh@163.com)
 * @version 2.1
 * @date 2026-06-12
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-11       1.1            zeh            SIL-2 会话状态与 start 失败回滚
 * 2026-06-11       1.2            zeh            start_all 先启实例后启 HRT；NULL 前置校验
 * 2026-06-11       1.3            zeh            Periodic 槽会话守卫与 slot_count 上界
 * 2026-06-12       2.0            zeh            直接迁移为领域中性 bm_exec
 * 2026-06-12       2.1            zeh            Block/Frame 槽与 bm_stream 绑定
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm_exec.h"
#include "bm_stream.h"
#include "bm_critical_wrap.h"
#include "bm_hrt.h"
#include "bm_log.h"

#include <string.h>

/** 实例槽位绑定：用于 HRT/硬件回调上下文 */
typedef struct {
    const bm_exec_t *instance;
    const bm_exec_slot_t *slot;
} bm_exec_binding_t;

static const bm_exec_t *g_instances[BM_CONFIG_MAX_EXEC_INSTANCES];
static uint32_t g_instance_count;
static bm_exec_binding_t g_bindings[BM_CONFIG_MAX_EXEC_SLOTS];
static uint32_t g_binding_count;
static bm_hrt_slot_t g_hrt_slots[BM_CONFIG_HRT_MAX_SLOTS];
static uint32_t g_hrt_slot_count;
static uint32_t g_init_done_count;
static volatile bm_exec_session_t g_session;

static void exec_set_session(bm_exec_session_t session) {
    bm_irq_state_t irq_state = BM_CRITICAL_ENTER();

    g_session = session;
    BM_CRITICAL_EXIT(irq_state);
}

/**
 * @brief HRT/硬件回调入口，执行绑定槽位的 run 函数
 *
 * @param context 指向 bm_exec_binding_t 的上下文指针
 */
static void bm_exec_run_binding(void *context) {
    const bm_exec_binding_t *binding = (const bm_exec_binding_t *)context;

    if (!binding || !binding->slot || !binding->slot->run || !binding->instance) {
        return;
    }
    /* volatile 单字枚举读，Cortex-M 上原子；ISR 内勿改用 bm_exec_get_session() */
    if (g_session != BM_EXEC_SESSION_STARTED) {
        return;
    }
    binding->slot->run(binding->instance);
}

/**
 * @brief 清零运行时全局状态（实例表、绑定表、HRT 槽）
 */
static void exec_clear_runtime(void) {
    {
        uint32_t n;
        for (n = 0u; n < BM_CONFIG_MAX_EXEC_INSTANCES; ++n) {
            g_instances[n] = NULL;
        }
    }
    g_instance_count = 0u;
    memset(g_bindings, 0, sizeof(g_bindings));
    g_binding_count = 0u;
    memset(g_hrt_slots, 0, sizeof(g_hrt_slots));
    g_hrt_slot_count = 0u;
    g_init_done_count = 0u;
    exec_set_session(BM_EXEC_SESSION_NONE);
}

static void bm_stream_exec_ready(bm_stream_t *stream,
                               bm_block_t *block,
                               void *context) {
    const bm_exec_binding_t *binding = (const bm_exec_binding_t *)context;

    (void)stream;
    if (!binding || !binding->slot || !binding->instance) {
        return;
    }
    if (g_session != BM_EXEC_SESSION_STARTED) {
        return;
    }
    if (!binding->slot->run_block) {
        return;
    }
    block->state = BM_BLOCK_STATE_PROCESSING;
    binding->slot->run_block(binding->instance, block);
}

static void exec_bind_stream_slots(void) {
    uint32_t i;
    uint32_t s;
    uint32_t b;

    for (i = 0u; i < g_instance_count; ++i) {
        const bm_exec_t *inst = g_instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_exec_slot_t *slot = &inst->slots[s];
            bm_exec_binding_t *binding = NULL;

            if (slot->kind != BM_EXEC_SLOT_BLOCK &&
                slot->kind != BM_EXEC_SLOT_FRAME) {
                continue;
            }
            if (!slot->stream || !slot->run_block) {
                continue;
            }
            for (b = 0u; b < g_binding_count; ++b) {
                if (g_bindings[b].instance == inst &&
                    g_bindings[b].slot == slot) {
                    binding = &g_bindings[b];
                    break;
                }
            }
            if (binding) {
                bm_stream_set_ready_handler(slot->stream,
                                            bm_stream_exec_ready,
                                            binding);
            }
        }
    }
}

static void exec_unbind_stream_slots(void) {
    uint32_t i;
    uint32_t s;

    for (i = 0u; i < g_instance_count; ++i) {
        const bm_exec_t *inst = g_instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_exec_slot_t *slot = &inst->slots[s];
            if (slot->kind == BM_EXEC_SLOT_BLOCK ||
                slot->kind == BM_EXEC_SLOT_FRAME) {
                if (slot->stream) {
                    bm_stream_set_ready_handler(slot->stream, NULL, NULL);
                }
            }
        }
    }
}

/**
 * @brief 解绑所有硬件槽位的外部中断/定时器
 */
static void exec_unbind_all_hardware(void) {
    uint32_t i;
    uint32_t s;

    exec_unbind_stream_slots();
    for (i = 0u; i < g_instance_count; ++i) {
        const bm_exec_t *inst = g_instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_exec_slot_t *slot = &inst->slots[s];
            if (slot->kind == BM_EXEC_SLOT_HARDWARE && slot->bind) {
                (void)slot->bind(inst, NULL);
            }
        }
    }
}

/**
 * @brief 停止 HRT、安全停机、解绑硬件并清空运行时状态
 */
static void exec_teardown_session(void) {
    uint32_t i;

    exec_set_session(BM_EXEC_SESSION_STOPPING);
    bm_hrt_stop();
    exec_unbind_all_hardware();

    if (g_instance_count > 0u) {
        for (i = g_instance_count; i > 0u; --i) {
            const bm_exec_t *inst = g_instances[i - 1u];
            if (inst && inst->ops && inst->ops->safe_stop) {
                inst->ops->safe_stop(inst);
            }
        }
    }

    bm_hrt_reset();
    exec_clear_runtime();
}

/**
 * @brief 按逆序回滚已完成的实例 init，调用 safe_stop
 */
static void exec_rollback_inits(void) {
    while (g_init_done_count > 0u) {
        g_init_done_count--;
        const bm_exec_t *inst = g_instances[g_init_done_count];
        if (inst->ops && inst->ops->safe_stop) {
            inst->ops->safe_stop(inst);
        }
    }
}

/**
 * @brief start/init 失败回滚：安全停机全部已 init 实例并释放资源
 */
static void exec_abort_session(void) {
    exec_set_session(BM_EXEC_SESSION_STOPPING);
    bm_hrt_stop();
    exec_unbind_all_hardware();
    exec_rollback_inits();
    bm_hrt_reset();
    exec_clear_runtime();
}

/**
 * @brief 校验单个执行实例描述符与槽位配置
 *
 * @param inst 执行实例指针
 * @return BM_OK 有效；BM_ERR_INVALID 字段或槽位配置无效
 */
static int validate_instance(const bm_exec_t *inst) {
    uint32_t s;

    if (!inst || !inst->ops) {
        return BM_ERR_INVALID;
    }
    if (inst->slot_count > 0u && !inst->slots) {
        return BM_ERR_INVALID;
    }
    if (inst->slot_count == 0u && inst->slots != NULL) {
        return BM_ERR_INVALID;
    }
    if (!inst->ops->init || !inst->ops->start || !inst->ops->safe_stop) {
        return BM_ERR_INVALID;
    }
    if (inst->claim_count > 0u && !inst->claims) {
        return BM_ERR_INVALID;
    }
    if (inst->slot_count > BM_CONFIG_MAX_EXEC_SLOTS) {
        return BM_ERR_INVALID;
    }

    for (s = 0u; s < inst->slot_count; ++s) {
        const bm_exec_slot_t *slot = &inst->slots[s];
        if (slot->kind == BM_EXEC_SLOT_PERIODIC) {
            if (slot->bind != NULL) {
                return BM_ERR_INVALID;
            }
            if (!slot->run) {
                return BM_ERR_INVALID;
            }
            if (bm_hrt_validate_period_us(slot->period_us) != BM_OK) {
                return BM_ERR_INVALID;
            }
        } else if (slot->kind == BM_EXEC_SLOT_HARDWARE) {
            if (!slot->bind || !slot->run) {
                return BM_ERR_INVALID;
            }
            if (slot->period_us != 0u || slot->deadline_us != 0u || slot->stream) {
                return BM_ERR_INVALID;
            }
        } else if (slot->kind == BM_EXEC_SLOT_BLOCK ||
                   slot->kind == BM_EXEC_SLOT_FRAME) {
            if (!slot->run_block || !slot->stream) {
                return BM_ERR_INVALID;
            }
            if (slot->deadline_us == 0u) {
                return BM_ERR_INVALID;
            }
            if (slot->bind != NULL || slot->run != NULL) {
                return BM_ERR_INVALID;
            }
        } else {
            return BM_ERR_INVALID;
        }
    }

    return BM_OK;
}

/**
 * @brief 校验实例 ID 在批次内唯一
 *
 * @param instances 实例指针数组
 * @param count 实例数量
 * @return BM_OK 唯一；BM_ERR_ALREADY 存在重复 ID
 */
static int validate_unique_ids(const bm_exec_t *const *instances,
                               uint32_t count) {
    uint32_t i;
    uint32_t j;

    for (i = 0u; i < count; ++i) {
        if (!instances[i]) {
            return BM_ERR_INVALID;
        }
    }
    for (i = 0u; i < count; ++i) {
        for (j = i + 1u; j < count; ++j) {
            if (instances[i]->id == instances[j]->id) {
                return BM_ERR_ALREADY;
            }
        }
    }
    return BM_OK;
}

/**
 * @brief 组装绑定表与 HRT 调度槽表
 *
 * @param instances 实例指针数组
 * @param count 实例数量
 * @return BM_OK 成功；BM_ERR_OVERFLOW 槽位表溢出
 */
static int assemble_tables(const bm_exec_t *const *instances,
                           uint32_t count) {
    uint32_t i;
    uint32_t s;

    g_binding_count = 0u;
    g_hrt_slot_count = 0u;

    for (i = 0u; i < count; ++i) {
        const bm_exec_t *inst = instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_exec_slot_t *slot = &inst->slots[s];

            if (g_binding_count >= BM_CONFIG_MAX_EXEC_SLOTS) {
                return BM_ERR_OVERFLOW;
            }
            g_bindings[g_binding_count].instance = inst;
            g_bindings[g_binding_count].slot = slot;
            g_binding_count++;

            if (slot->kind != BM_EXEC_SLOT_PERIODIC) {
                continue;
            }
            if (g_hrt_slot_count >= BM_CONFIG_HRT_MAX_SLOTS) {
                return BM_ERR_OVERFLOW;
            }
            g_hrt_slots[g_hrt_slot_count].period_us = slot->period_us;
            g_hrt_slots[g_hrt_slot_count].trigger = BM_HRT_TRIGGER_TIMER;
            g_hrt_slots[g_hrt_slot_count].callback = bm_exec_run_binding;
            g_hrt_slots[g_hrt_slot_count].context =
                &g_bindings[g_binding_count - 1u];
            g_hrt_slots[g_hrt_slot_count].name = slot->name;
            g_hrt_slot_count++;
        }
    }

    return BM_OK;
}

/**
 * @brief 批量初始化执行实例（校验、资源检查、HRT 与硬件绑定）
 *
 * @param instances 实例指针数组
 * @param count 实例数量
 * @return BM_OK 成功；负值为各阶段错误码
 */
int bm_exec_init_all(const bm_exec_t *const *instances, uint32_t count) {
    const bm_resource_claim_t *claim_ptrs[BM_CONFIG_MAX_EXEC_INSTANCES];
    uint32_t claim_counts[BM_CONFIG_MAX_EXEC_INSTANCES];
    uint32_t i;
    uint32_t s;
    int rc;

    if (!instances || count == 0u || count > BM_CONFIG_MAX_EXEC_INSTANCES) {
        BM_LOGE("exec", "init_all invalid count=%u", (unsigned)count);
        return BM_ERR_INVALID;
    }

    exec_teardown_session();

    rc = validate_unique_ids(instances, count);
    if (rc != BM_OK) {
        BM_LOGE("exec", "init_all duplicate instance id");
        return rc;
    }

    for (i = 0u; i < count; ++i) {
        if (!instances[i]) {
            return BM_ERR_INVALID;
        }
        rc = validate_instance(instances[i]);
        if (rc != BM_OK) {
            return rc;
        }
        claim_ptrs[i] = instances[i]->claims;
        claim_counts[i] = instances[i]->claim_count;
    }

    rc = bm_resource_check_conflicts(claim_ptrs, claim_counts, count);
    if (rc != BM_OK) {
        BM_LOGE("exec", "init_all resource conflict rc=%d", rc);
        return rc;
    }

    for (i = 0u; i < count; ++i) {
        g_instances[i] = instances[i];
    }
    g_instance_count = count;

    rc = assemble_tables(instances, count);
    if (rc != BM_OK) {
        exec_clear_runtime();
        return rc;
    }

    if (g_hrt_slot_count > 0u) {
        rc = bm_hrt_init(g_hrt_slots, g_hrt_slot_count);
        if (rc != BM_OK) {
            exec_clear_runtime();
            return rc;
        }
    }

    for (i = 0u; i < count; ++i) {
        rc = instances[i]->ops->init(instances[i]);
        if (rc != BM_OK) {
            BM_LOGE("exec", "init failed inst id=%u rc=%d",
                    (unsigned)instances[i]->id, rc);
            exec_abort_session();
            return rc;
        }
        g_init_done_count++;
    }

    for (i = 0u; i < count; ++i) {
        const bm_exec_t *inst = instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_exec_slot_t *slot = &inst->slots[s];
            bm_hal_hrt_binding_t hal_binding;
            bm_exec_binding_t *binding = NULL;
            uint32_t b;

            if (slot->kind != BM_EXEC_SLOT_HARDWARE) {
                continue;
            }

            hal_binding.callback = bm_exec_run_binding;
            for (b = 0u; b < g_binding_count; ++b) {
                if (g_bindings[b].instance == inst &&
                    g_bindings[b].slot == slot) {
                    binding = &g_bindings[b];
                    break;
                }
            }
            if (!binding) {
                rc = BM_ERR_INVALID;
            } else {
                hal_binding.context = binding;
                rc = slot->bind(inst, &hal_binding);
            }
            if (rc != BM_OK) {
                exec_abort_session();
                return rc;
            }
        }
    }

    exec_set_session(BM_EXEC_SESSION_INITED);
    exec_bind_stream_slots();
    BM_LOGI("exec", "init_all ok count=%u hrt_slots=%u",
            (unsigned)count, (unsigned)g_hrt_slot_count);
    return BM_OK;
}

/**
 * @brief 批量启动已初始化的执行实例
 *
 * @param instances 实例指针数组（须与 init_all 时一致）
 * @param count 实例数量
 * @return BM_OK 成功；BM_ERR_INVALID 参数不匹配；负值为 start 失败码
 */
static int exec_ensure_hrt_started(void) {
    int rc;

    if (g_hrt_slot_count == 0u) {
        return BM_OK;
    }
    rc = bm_hrt_start();
    if (rc == BM_ERR_ALREADY) {
        return BM_OK;
    }
    return rc;
}

int bm_exec_start_all(const bm_exec_t *const *instances, uint32_t count) {
    uint32_t i;
    int rc;

    if (!instances || count == 0u || count != g_instance_count) {
        return BM_ERR_INVALID;
    }
    /* volatile 单字枚举读，Cortex-M 上原子；主上下文快速路径，勿改用 bm_exec_get_session() */
    if (g_session == BM_EXEC_SESSION_STARTED) {
        return BM_ERR_ALREADY;
    }
    if (g_session != BM_EXEC_SESSION_INITED) {
        return BM_ERR_NOT_INIT;
    }

    for (i = 0u; i < count; ++i) {
        if (!instances[i] || instances[i] != g_instances[i]) {
            return BM_ERR_INVALID;
        }
        rc = instances[i]->ops->start(instances[i]);
        if (rc != BM_OK) {
            BM_LOGE("exec", "start failed inst id=%u rc=%d",
                    (unsigned)instances[i]->id, rc);
            exec_abort_session();
            return rc;
        }
    }

    rc = exec_ensure_hrt_started();
    if (rc != BM_OK) {
        BM_LOGE("exec", "hrt start failed rc=%d", rc);
        exec_abort_session();
        return rc;
    }
    exec_set_session(BM_EXEC_SESSION_STARTED);

    BM_LOGI("exec", "start_all ok count=%u", (unsigned)count);
    return BM_OK;
}

/**
 * @brief 安全停止所有实例并释放 HRT/硬件绑定
 *
 * @param instances 实例指针数组（可为 NULL，则使用内部记录）
 * @param count 实例数量
 */
void bm_exec_safe_stop_all(const bm_exec_t *const *instances,
                           uint32_t count) {
    if ((instances == NULL && count > 0u) ||
        (instances != NULL && count != g_instance_count)) {
        BM_LOGW("exec", "safe_stop_all ignored external instance table");
    }

    exec_teardown_session();
    BM_LOGI("exec", "safe_stop_all done");
}

/**
 * @brief 查询当前执行批次会话状态
 *
 * @return 临界区内读取的会话状态快照
 */
bm_exec_session_t bm_exec_get_session(void) {
    bm_irq_state_t irq_state = BM_CRITICAL_ENTER();
    bm_exec_session_t session = g_session;

    BM_CRITICAL_EXIT(irq_state);
    return session;
}

/**
 * @brief 按 ID 在实例数组中查找执行实例
 *
 * @param instances 实例指针数组
 * @param count 实例数量
 * @param id 目标实例 ID
 * @return 匹配的实例指针；未找到返回 NULL
 */
const bm_exec_t *bm_exec_find(const bm_exec_t *const *instances,
                                   uint32_t count,
                                   uint32_t id) {
    uint32_t i;

    if (!instances) {
        return NULL;
    }

    for (i = 0u; i < count; ++i) {
        if (instances[i] && instances[i]->id == id) {
            return instances[i];
        }
    }

    return NULL;
}
