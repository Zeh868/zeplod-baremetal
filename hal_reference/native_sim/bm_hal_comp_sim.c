/**
 * @file bm_hal_comp_sim.c
 * @brief 原生仿真环境比较器 HAL 实现（阈值与锁存）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_comp.h"
#include "bm_log.h"

#define TAG "hal_comp"

struct bm_hal_comp {
    uint32_t id;
};

typedef struct {
    uint16_t threshold;
    int latched;
} comp_sim_state_t;

static comp_sim_state_t g_comp_state[2];

const bm_hal_comp_t BM_HAL_COMP_SIM0 = { 0u };

/** 设置比较器阈值（仿真注入） */
void bm_hal_comp_sim_set_threshold(const bm_hal_comp_t *comp, uint16_t threshold) {
    if (!comp || comp->id >= 2u) {
        BM_LOGW(TAG, "set_threshold: invalid comp=%p", (const void *)comp);
        return;
    }
    g_comp_state[comp->id].threshold = threshold;
}

/** 注入采样值，超过阈值则锁存 */
void bm_hal_comp_sim_trip(const bm_hal_comp_t *comp, uint16_t sample) {
    if (!comp || comp->id >= 2u) {
        BM_LOGW(TAG, "trip: invalid comp=%p", (const void *)comp);
        return;
    }
    if (sample >= g_comp_state[comp->id].threshold) {
        g_comp_state[comp->id].latched = 1;
    }
}

/** 清除比较器锁存状态 */
int bm_hal_comp_clear_latch(const bm_hal_comp_t *comp) {
    if (!comp || comp->id >= 2u) {
        BM_LOGE(TAG, "clear_latch: invalid comp=%p", (const void *)comp);
        return BM_ERR_INVALID;
    }
    g_comp_state[comp->id].latched = 0;
    return BM_OK;
}

/** 查询比较器是否已锁存（仿真辅助） */
int bm_hal_comp_sim_is_latched(const bm_hal_comp_t *comp) {
    if (!comp || comp->id >= 2u) {
        return 0;
    }
    return g_comp_state[comp->id].latched;
}
