/**
 * @file bm_drv_singleton_stm32g4.c
 * @brief STM32G4 寄存器后端单例驱动（临界区/屏障/定时器/UART/看门狗）
 */
#ifndef BM_HAL_HAS_PRIORITY_MASK
#define BM_HAL_HAS_PRIORITY_MASK 1
#endif

#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 4
#endif

#include "bm_drv_critical.h"
#include "bm_drv_memory.h"
#include "bm_drv_timer.h"
#include "bm_drv_uart.h"
#include "bm_drv_wdg.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_log.h"
#include "bm_types.h"

#define TAG_UART "hal_uart"
#define TAG_WDG  "hal_wdg"

static void (*g_tick_callback)(void);
static uint32_t g_tick_freq;

static inline uint32_t read_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r"(primask));
    return primask;
}

static inline void write_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r"(primask) : "memory");
}

static inline uint32_t read_basepri(void) {
    uint32_t basepri;
    __asm volatile ("mrs %0, basepri" : "=r"(basepri));
    return basepri;
}

static inline void write_basepri(uint32_t basepri) {
    __asm volatile ("msr basepri, %0" :: "r"(basepri) : "memory");
}

static bm_irq_state_t stm32_critical_enter(void) {
    bm_irq_state_t state = read_primask();
    __asm volatile ("cpsid i" ::: "memory");
    return state;
}

static void stm32_critical_exit(bm_irq_state_t state) {
    write_primask(state);
}

#if BM_HAL_HAS_PRIORITY_MASK
static bm_irq_state_t stm32_critical_enter_below(uint8_t threshold) {
    bm_irq_state_t packed = read_basepri();
    packed |= (read_primask() << 8);
    write_basepri((uint32_t)threshold << (8 - __NVIC_PRIO_BITS));
    return packed;
}

static void stm32_critical_exit_below(bm_irq_state_t previous_state) {
    write_basepri((uint32_t)(previous_state & 0xFFu));
    write_primask((uint32_t)(previous_state >> 8));
}
#endif

const struct bm_critical_driver_api bm_drv_critical_api = {
    stm32_critical_enter,
    stm32_critical_exit,
#if BM_HAL_HAS_PRIORITY_MASK
    stm32_critical_enter_below,
    stm32_critical_exit_below,
#endif
};

static void stm32_memory_release(void) {
    __asm volatile ("dmb" ::: "memory");
}

static void stm32_memory_full(void) {
    __asm volatile ("dsb" ::: "memory");
}

const struct bm_memory_driver_api bm_drv_memory_api = {
    stm32_memory_release,
    stm32_memory_full,
};

void TIM6_DAC_IRQHandler(void) {
    if ((BM_TIM_SR(BM_STM32G4_TIM6_BASE) & BM_TIM_SR_UIF) == 0u) {
        return;
    }
    BM_TIM_SR(BM_STM32G4_TIM6_BASE) &= ~BM_TIM_SR_UIF;
    if (g_tick_callback) {
        g_tick_callback();
    }
}

static int stm32_timer_init(uint32_t freq_hz) {
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

static void stm32_timer_stop(void) {
    BM_TIM_DIER(BM_STM32G4_TIM6_BASE) = 0u;
    BM_TIM_CR1(BM_STM32G4_TIM6_BASE) = 0u;
    g_tick_callback = NULL;
}

static uint32_t stm32_timer_get_ticks(void) {
    return BM_TIM_CNT(BM_STM32G4_TIM6_BASE);
}

static uint32_t stm32_timer_get_freq(void) {
    return g_tick_freq;
}

static void stm32_timer_set_callback(void (*cb)(void)) {
    g_tick_callback = cb;
}

const struct bm_timer_driver_api bm_drv_timer_api = {
    stm32_timer_init,
    stm32_timer_stop,
    stm32_timer_get_ticks,
    stm32_timer_get_freq,
    stm32_timer_set_callback,
};

static int stm32_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG_UART, "init: USART2 stub (add Cube LL + GPIO)");
    return BM_OK;
}

static int stm32_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return BM_OK;
}

static size_t stm32_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void stm32_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}

const struct bm_uart_driver_api bm_drv_uart_api = {
    stm32_uart_init,
    stm32_uart_send,
    stm32_uart_recv,
    stm32_uart_set_rx_callback,
};

static int stm32_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    BM_LOGI(TAG_WDG, "init: IWDG stub (configure prescaler/reload)");
    return BM_OK;
}

static void stm32_wdg_feed(void) {
}

const struct bm_wdg_driver_api bm_drv_wdg_api = {
    stm32_wdg_init,
    stm32_wdg_feed,
};
