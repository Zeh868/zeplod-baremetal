/**
 * @file bm_port.c
 * @brief Zeplod Port 模板 — 类比 FreeRTOS portable/.../port.c
 *
 * 复制到应用工程，对接厂商 HAL。文档：docs/14-Port移植层.md
 */

#include <stddef.h>
#include <stdint.h>

#include "bm_drv_critical.h"
#include "bm_drv_memory.h"
#include "bm_drv_timer.h"
#include "bm_drv_uart.h"
#include "bm_types.h"

static inline uint32_t port_read_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r"(primask));
    return primask;
}

static inline void port_write_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r"(primask) : "memory");
}

static bm_irq_state_t port_critical_enter(void) {
    bm_irq_state_t state = port_read_primask();
    __asm volatile ("cpsid i" ::: "memory");
    return state;
}

static void port_critical_exit(bm_irq_state_t state) {
    port_write_primask(state);
}

const struct bm_critical_driver_api bm_drv_critical_api = {
    port_critical_enter,
    port_critical_exit,
};

static void port_memory_release(void) {
    __asm volatile ("dmb" ::: "memory");
}

static void port_memory_full(void) {
    __asm volatile ("dsb" ::: "memory");
}

const struct bm_memory_driver_api bm_drv_memory_api = {
    port_memory_release,
    port_memory_full,
};

static void (*g_tick_cb)(void);
static uint32_t g_tick_hz;

static int port_timer_init(uint32_t freq_hz) {
    g_tick_hz = freq_hz;
    return 0;
}

static void port_timer_stop(void) {
}

static uint32_t port_timer_get_ticks(void) {
    return 0u;
}

static uint32_t port_timer_get_freq(void) {
    return g_tick_hz;
}

static void port_timer_set_callback(void (*cb)(void)) {
    g_tick_cb = cb;
}

const struct bm_timer_driver_api bm_drv_timer_api = {
    port_timer_init,
    port_timer_stop,
    port_timer_get_ticks,
    port_timer_get_freq,
    port_timer_set_callback,
};

void bm_port_timer_isr(void) {
    if (g_tick_cb) {
        g_tick_cb();
    }
}

static int port_uart_init(void *config) {
    (void)config;
    return 0;
}

static int port_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return 0;
}

static size_t port_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void port_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}

const struct bm_uart_driver_api bm_drv_uart_api = {
    port_uart_init,
    port_uart_send,
    port_uart_recv,
    port_uart_set_rx_callback,
};
