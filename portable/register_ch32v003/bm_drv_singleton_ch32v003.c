/**
 * @file bm_drv_singleton_ch32v003.c
 * @brief CH32V003 参考后端单例驱动（桩实现，可替换为 WCH SDK 后端）
 */
#include "bm_drv_critical.h"
#include "bm_drv_memory.h"
#include "bm_drv_timer.h"
#include "bm_drv_uart.h"
#include "bm_drv_wdg.h"
#include "bm_log.h"
#include "bm_types.h"

#include <stdint.h>

#define TAG_TIMER "hal_timer"
#define TAG_UART  "hal_uart"
#define TAG_WDG   "hal_wdg"

static inline uintptr_t ch32_read_mstatus(void) {
    uintptr_t value;
    __asm volatile ("csrr %0, mstatus" : "=r"(value));
    return value;
}

static inline void ch32_write_mstatus(uintptr_t value) {
    __asm volatile ("csrw mstatus, %0" :: "r"(value) : "memory");
}

static bm_irq_state_t ch32_critical_enter(void) {
    uintptr_t state = ch32_read_mstatus();
    __asm volatile ("csrc mstatus, 8" ::: "memory");
    return (bm_irq_state_t)state;
}

static void ch32_critical_exit(bm_irq_state_t state) {
    ch32_write_mstatus((uintptr_t)state);
}

const struct bm_critical_driver_api bm_drv_critical_api = {
    ch32_critical_enter,
    ch32_critical_exit,
};

static void ch32_memory_release(void) {
    __asm volatile ("fence rw, rw" ::: "memory");
}

static void ch32_memory_full(void) {
    __asm volatile ("fence rw, rw" ::: "memory");
}

const struct bm_memory_driver_api bm_drv_memory_api = {
    ch32_memory_release,
    ch32_memory_full,
};

static uint32_t g_tick_freq = 1000u;
static void (*g_tick_cb)(void);

static int ch32_timer_init(uint32_t freq_hz) {
    g_tick_freq = (freq_hz > 0u) ? freq_hz : 1000u;
    BM_LOGI(TAG_TIMER, "init: freq_hz=%u (stub)", g_tick_freq);
    return BM_OK;
}

static void ch32_timer_stop(void) {
    g_tick_cb = NULL;
}

static uint32_t ch32_timer_get_ticks(void) {
    return 0u;
}

static uint32_t ch32_timer_get_freq(void) {
    return g_tick_freq;
}

static void ch32_timer_set_callback(void (*cb)(void)) {
    g_tick_cb = cb;
}

const struct bm_timer_driver_api bm_drv_timer_api = {
    ch32_timer_init,
    ch32_timer_stop,
    ch32_timer_get_ticks,
    ch32_timer_get_freq,
    ch32_timer_set_callback,
};

static int ch32_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG_UART, "init: USART1 stub (add WCH LL driver)");
    return BM_OK;
}

static int ch32_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return BM_OK;
}

static size_t ch32_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void ch32_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}

const struct bm_uart_driver_api bm_drv_uart_api = {
    ch32_uart_init,
    ch32_uart_send,
    ch32_uart_recv,
    ch32_uart_set_rx_callback,
};

static int ch32_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    BM_LOGI(TAG_WDG, "init: IWDG stub");
    return BM_OK;
}

static void ch32_wdg_feed(void) {
}

const struct bm_wdg_driver_api bm_drv_wdg_api = {
    ch32_wdg_init,
    ch32_wdg_feed,
};
