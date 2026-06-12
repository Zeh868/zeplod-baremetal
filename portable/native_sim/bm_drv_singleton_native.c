/**
 * @file bm_drv_singleton_native.c
 * @brief native_sim 单例驱动 API（临界区/内存屏障/定时器/UART/看门狗）
 */
#include "bm_drv_critical.h"
#include "bm_types.h"
#include "bm_drv_memory.h"
#include "bm_drv_timer.h"
#include "bm_drv_uart.h"
#include "bm_drv_wdg.h"
#include "bm_hal_timer_native.h"
#include "bm_hal_wdg_native.h"
#include "bm_log.h"

#include <stdio.h>
#include <stdint.h>

#define TAG_TIMER "hal_timer"
#define TAG_UART  "hal_uart"
#define TAG_WDG   "hal_wdg"

/* --- critical --- */
static volatile bm_irq_state_t g_native_irq_state;

static bm_irq_state_t native_critical_enter(void) {
    bm_irq_state_t previous = g_native_irq_state;
    g_native_irq_state = 1;
    return previous;
}

static void native_critical_exit(bm_irq_state_t state) {
    g_native_irq_state = state;
}

const struct bm_critical_driver_api bm_drv_critical_api = {
    native_critical_enter,
    native_critical_exit,
};

/* --- memory --- */
static void native_memory_release(void) {
}

static void native_memory_full(void) {
}

const struct bm_memory_driver_api bm_drv_memory_api = {
    native_memory_release,
    native_memory_full,
};

/* --- timer --- */
static uint32_t g_tick_freq = 1000u;
static uint32_t g_tick_count;
static void (*g_tick_callback)(void);

static int native_timer_init(uint32_t freq_hz) {
    g_tick_freq = freq_hz ? freq_hz : 1000u;
    BM_LOGI(TAG_TIMER, "init: freq_hz=%u", g_tick_freq);
    return BM_OK;
}

static void native_timer_stop(void) {
    g_tick_callback = NULL;
    BM_LOGI(TAG_TIMER, "stop");
}

static uint32_t native_timer_get_ticks(void) {
    return g_tick_count;
}

static uint32_t native_timer_get_freq(void) {
    return g_tick_freq;
}

static void native_timer_set_callback(void (*cb)(void)) {
    g_tick_callback = cb;
}

const struct bm_timer_driver_api bm_drv_timer_api = {
    native_timer_init,
    native_timer_stop,
    native_timer_get_ticks,
    native_timer_get_freq,
    native_timer_set_callback,
};

void bm_hal_timer_native_advance_ticks(uint32_t delta) {
    uint32_t i;

    for (i = 0u; i < delta; ++i) {
        g_tick_count++;
        if (g_tick_callback) {
            g_tick_callback();
        }
    }
}

void bm_hal_timer_native_jump_ticks(uint32_t delta) {
    g_tick_count += delta;
    if (g_tick_callback) {
        g_tick_callback();
    }
}

void bm_hal_timer_native_reset_ticks(void) {
    g_tick_count = 0u;
}

void bm_hal_timer_native_deinit(void) {
    g_tick_freq = 0u;
    g_tick_count = 0u;
    g_tick_callback = NULL;
}

/* --- uart --- */
static void (*g_uart_rx_cb)(uint8_t c);

static int native_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG_UART, "init: native stdout backend");
    return BM_OK;
}

static int native_uart_send(const uint8_t *data, size_t len) {
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return BM_OK;
}

static size_t native_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void native_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    g_uart_rx_cb = cb;
    (void)g_uart_rx_cb;
}

const struct bm_uart_driver_api bm_drv_uart_api = {
    native_uart_init,
    native_uart_send,
    native_uart_recv,
    native_uart_set_rx_callback,
};

/* --- wdg --- */
static uint32_t g_wdg_feed_count;

static int native_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    BM_LOGI(TAG_WDG, "init: timeout_ms=%u (stub)", timeout_ms);
    return BM_OK;
}

static void native_wdg_feed(void) {
    g_wdg_feed_count++;
}

const struct bm_wdg_driver_api bm_drv_wdg_api = {
    native_wdg_init,
    native_wdg_feed,
};

uint32_t bm_hal_wdg_native_get_feed_count(void) {
    return g_wdg_feed_count;
}

void bm_hal_wdg_native_reset_feed_count(void) {
    g_wdg_feed_count = 0u;
}
