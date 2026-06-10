/**
 * @file bm_hal_adc_sim.c
 * @brief 原生仿真环境 ADC HAL 实现（波形注入与完成回调）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_adc.h"
#include "bm_log.h"

#define BM_SIM_ADC_RANKS      16u
#define BM_SIM_ADC_INSTANCES  2u
#define TAG                   "hal_adc"

/** ADC 不透明结构体：仅保存实例 ID */
struct bm_hal_adc {
    uint32_t id;
};

/** 单路 ADC 仿真状态：各 rank 采样值与转换完成回调 */
typedef struct {
    uint16_t waveform[BM_SIM_ADC_RANKS];
    bm_hal_hrt_binding_t complete_binding;
} adc_sim_state_t;

static adc_sim_state_t g_adc_state[BM_SIM_ADC_INSTANCES];

/** 根据 ADC 句柄查找仿真状态，无效时返回 NULL */
static adc_sim_state_t *adc_state_for(const bm_hal_adc_t *adc) {
    if (!adc || adc->id >= BM_SIM_ADC_INSTANCES) {
        return NULL;
    }
    return &g_adc_state[adc->id];
}

const bm_hal_adc_t BM_HAL_ADC_SIM0 = { 0u };
const bm_hal_adc_t BM_HAL_ADC_SIM1 = { 1u };

/** 测试/仿真用：设置指定 rank 的注入采样值 */
void bm_hal_adc_sim_set_rank(const bm_hal_adc_t *adc, uint32_t rank,
                             uint16_t value) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state || rank >= BM_SIM_ADC_RANKS) {
        BM_LOGW(TAG, "set_rank: invalid adc=%p rank=%u", (const void *)adc, rank);
        return;
    }
    state->waveform[rank] = value;
}

/** 读取注入通道 rank 的采样值 */
int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state || !value || rank >= BM_SIM_ADC_RANKS) {
        BM_LOGE(TAG, "read_injected: invalid adc=%p value=%p rank=%u",
                (const void *)adc, (const void *)value, rank);
        return BM_ERR_INVALID;
    }
    *value = state->waveform[rank];
    return BM_OK;
}

/** 绑定 ADC 转换完成 HRT 回调；binding 为 NULL 时解除绑定 */
int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state) {
        BM_LOGE(TAG, "bind_complete: invalid adc=%p", (const void *)adc);
        return BM_ERR_INVALID;
    }
    if (!binding) {
        state->complete_binding.callback = NULL;
        state->complete_binding.context = NULL;
        BM_LOGI(TAG, "bind_complete: unbound adc id=%u", adc->id);
        return BM_OK;
    }
    state->complete_binding = *binding;
    BM_LOGI(TAG, "bind_complete: bound adc id=%u", adc->id);
    return BM_OK;
}

/** 仿真触发：调用已绑定的转换完成回调 */
void bm_hal_adc_sim_fire_complete(const bm_hal_adc_t *adc) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state || !state->complete_binding.callback) {
        return;
    }
    state->complete_binding.callback(state->complete_binding.context);
}
