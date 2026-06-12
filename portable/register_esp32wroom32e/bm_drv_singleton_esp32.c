/**
 * @file bm_drv_singleton_esp32.c
 * @brief ESP32-WROOM-32E 寄存器后端单例驱动
 */
#include "bm_drv_critical.h"
#include "bm_drv_memory.h"
#include "bm_drv_timer.h"
#include "bm_drv_uart.h"
#include "bm_drv_wdg.h"
#include "bm_hal_instances_esp32wroom32e.h"
#include "bm_hal_regs_esp32.h"
#include "bm_log.h"
#include "bm_types.h"

#include <stddef.h>
#include <stdint.h>

#define BM_TG_TICK_BASE      BM_ESP32WROOM32E_TIMER_TICK_BASE
#define BM_UART_CONSOLE_BASE BM_ESP32WROOM32E_UART_CONSOLE_BASE
#define BM_UART_DEFAULT_BAUD 115200u
#define TAG_WDG              "hal_wdg"

#define BM_XTENSA_PS_INTLEVEL_MASK  0x0Fu
#define BM_XTENSA_INTLEVEL_OFF      15u

static void (*g_tick_callback)(void);
static uint32_t g_tick_freq;
static void (*g_rx_callback)(uint8_t c);
static int g_wdt_enabled;

static inline uint32_t bm_xtensa_read_ps(void) {
    uint32_t ps;
    __asm__ volatile ("rsr %0, ps" : "=a"(ps));
    return ps;
}

static inline void bm_xtensa_write_ps(uint32_t ps) {
    __asm__ volatile ("wsr %0, ps" :: "a"(ps) : "memory");
    __asm__ volatile ("esync");
}

static bm_irq_state_t esp32_critical_enter(void) {
    uint32_t ps = bm_xtensa_read_ps();
    uint32_t masked = (ps & ~BM_XTENSA_PS_INTLEVEL_MASK) | BM_XTENSA_INTLEVEL_OFF;
    bm_xtensa_write_ps(masked);
    return (bm_irq_state_t)ps;
}

static void esp32_critical_exit(bm_irq_state_t state) {
    bm_xtensa_write_ps((uint32_t)state);
}

const struct bm_critical_driver_api bm_drv_critical_api = {
    esp32_critical_enter,
    esp32_critical_exit,
};

static void esp32_memory_release(void) {
    __asm__ volatile ("memw" ::: "memory");
}

static void esp32_memory_full(void) {
    __asm__ volatile ("memw" ::: "memory");
}

const struct bm_memory_driver_api bm_drv_memory_api = {
    esp32_memory_release,
    esp32_memory_full,
};

static uint32_t esp32_tg_counter_lo(void) {
    BM_TG_T0UPDATE(BM_TG_TICK_BASE) = 1u;
    return BM_TG_T0LO(BM_TG_TICK_BASE);
}

void bm_hal_timer_esp32_isr(void) {
    if ((BM_TG_INT_ST(BM_TG_TICK_BASE) & BM_TG_INT_T0) == 0u) {
        return;
    }
    BM_TG_INT_CLR(BM_TG_TICK_BASE) = BM_TG_INT_T0;
    if (g_tick_callback) {
        g_tick_callback();
    }
}

static int esp32_timer_init(uint32_t freq_hz) {
    uint32_t divider;
    uint32_t alarm;
    uint32_t config;

    if (freq_hz == 0u) {
        return BM_ERR_INVALID;
    }
    g_tick_freq = freq_hz;
    divider = (BM_ESP32_APB_CLK_HZ / 1000000u);
    if (divider == 0u) {
        divider = 1u;
    }
    alarm = (1000000u / freq_hz);
    if (alarm == 0u) {
        return BM_ERR_INVALID;
    }
    alarm -= 1u;

    BM_TG_T0CONFIG(BM_TG_TICK_BASE) = 0u;
    BM_TG_T0LOADLO(BM_TG_TICK_BASE) = 0u;
    BM_TG_T0LOADHI(BM_TG_TICK_BASE) = 0u;
    BM_TG_T0LOAD(BM_TG_TICK_BASE) = 1u;
    BM_TG_T0ALARMLO(BM_TG_TICK_BASE) = alarm;
    BM_TG_T0ALARMHI(BM_TG_TICK_BASE) = 0u;
    config = BM_TG_T0CONFIG_INCREASE | BM_TG_T0CONFIG_AUTORELOAD
           | BM_TG_T0CONFIG_ALARM_EN | BM_TG_T0CONFIG_LEVEL_INT_EN
           | ((divider - 1u) & BM_TG_T0CONFIG_DIVIDER_MASK) | BM_TG_T0CONFIG_EN;
    BM_TG_INT_CLR(BM_TG_TICK_BASE) = BM_TG_INT_T0;
    BM_TG_INT_ENA(BM_TG_TICK_BASE) = BM_TG_INT_T0;
    BM_TG_T0CONFIG(BM_TG_TICK_BASE) = config;
    return BM_OK;
}

static void esp32_timer_stop(void) {
    BM_TG_INT_ENA(BM_TG_TICK_BASE) = 0u;
    BM_TG_T0CONFIG(BM_TG_TICK_BASE) = 0u;
    g_tick_callback = NULL;
}

static uint32_t esp32_timer_get_ticks(void) {
    return esp32_tg_counter_lo();
}

static uint32_t esp32_timer_get_freq(void) {
    return g_tick_freq;
}

static void esp32_timer_set_callback(void (*cb)(void)) {
    g_tick_callback = cb;
}

const struct bm_timer_driver_api bm_drv_timer_api = {
    esp32_timer_init,
    esp32_timer_stop,
    esp32_timer_get_ticks,
    esp32_timer_get_freq,
    esp32_timer_set_callback,
};

static int esp32_uart_tx_ready(void) {
    uint32_t status = BM_UART_STATUS(BM_UART_CONSOLE_BASE);
    uint32_t count = (status >> BM_UART_STATUS_TXFIFO_CNT_SHIFT)
                   & BM_UART_STATUS_TXFIFO_CNT_MASK;
    return (count < 127u) ? 1 : 0;
}

static int esp32_uart_init(void *config) {
    uint32_t baud = BM_UART_DEFAULT_BAUD;
    uint32_t clkdiv;

    if (config != NULL) {
        baud = *(const uint32_t *)config;
        if (baud == 0u) {
            baud = BM_UART_DEFAULT_BAUD;
        }
    }
    clkdiv = (BM_ESP32_APB_CLK_HZ + (baud * 8u) - 1u) / (baud * 8u);
    if (clkdiv < 2u) {
        clkdiv = 2u;
    }
    BM_UART_CLKDIV(BM_UART_CONSOLE_BASE) = clkdiv;
    BM_UART_INT_ENA(BM_UART_CONSOLE_BASE) = 0u;
    return BM_OK;
}

static int esp32_uart_send(const uint8_t *data, size_t len) {
    size_t i;

    if (data == NULL) {
        return BM_ERR_INVALID;
    }
    for (i = 0u; i < len; ++i) {
        while (!esp32_uart_tx_ready()) {
        }
        BM_UART_FIFO(BM_UART_CONSOLE_BASE) = (uint32_t)data[i];
    }
    return BM_OK;
}

static size_t esp32_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void esp32_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    g_rx_callback = cb;
    (void)g_rx_callback;
}

const struct bm_uart_driver_api bm_drv_uart_api = {
    esp32_uart_init,
    esp32_uart_send,
    esp32_uart_recv,
    esp32_uart_set_rx_callback,
};

static int esp32_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    g_wdt_enabled = 0;
    BM_LOGI(TAG_WDG, "init: MWDT stub (add IDF wdt for production)");
    return BM_OK;
}

static void esp32_wdg_feed(void) {
    if (g_wdt_enabled) {
        BM_MWDT_FEED(BM_ESP32_MWDT0_BASE) = 1u;
    }
}

const struct bm_wdg_driver_api bm_drv_wdg_api = {
    esp32_wdg_init,
    esp32_wdg_feed,
};
