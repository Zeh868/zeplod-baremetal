/**
 * @file bm_hal_timer_stm32g4.c
 * @brief STM32G4 定时器 HAL 参考实现（TIM6 作为 HRT Tick 源）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_timer.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

static void (*g_tick_callback)(void);
static uint32_t g_tick_freq;

/** TIM6 更新中断（HRT Tick） */
void TIM6_DAC_IRQHandler(void) {
    if ((BM_TIM_SR(BM_STM32G4_TIM6_BASE) & BM_TIM_SR_UIF) == 0u) {
        return;
    }
    BM_TIM_SR(BM_STM32G4_TIM6_BASE) &= ~BM_TIM_SR_UIF;
    if (g_tick_callback) {
        g_tick_callback();
    }
}

int bm_hal_timer_init(uint32_t freq_hz) {
    uint32_t psc;
    uint32_t arr;

    if (freq_hz == 0u) {
        return BM_ERR_INVALID;
    }

    g_tick_freq = freq_hz;
    psc = 169u;
    arr = (170000000u / ((psc + 1u) * freq_hz)) - 1u;
    if (arr == 0u) {
        return BM_ERR_INVALID;
    }

    BM_TIM_CR1(BM_STM32G4_TIM6_BASE) = 0u;
    BM_TIM_PSC(BM_STM32G4_TIM6_BASE) = psc;
    BM_TIM_ARR(BM_STM32G4_TIM6_BASE) = arr;
    BM_TIM_EGR(BM_STM32G4_TIM6_BASE) = 1u;
    BM_TIM_DIER(BM_STM32G4_TIM6_BASE) = BM_TIM_DIER_UIE;
    BM_TIM_CR1(BM_STM32G4_TIM6_BASE) = BM_TIM_CR1_CEN;
    return BM_OK;
}

void bm_hal_timer_stop(void) {
    BM_TIM_DIER(BM_STM32G4_TIM6_BASE) = 0u;
    BM_TIM_CR1(BM_STM32G4_TIM6_BASE) = 0u;
    g_tick_callback = NULL;
}

uint32_t bm_hal_timer_get_ticks(void) {
    return BM_TIM_CNT(BM_STM32G4_TIM6_BASE);
}

uint32_t bm_hal_timer_get_freq(void) {
    return g_tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    g_tick_callback = cb;
}
