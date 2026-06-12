/**
 * @file bm_hal_uart.c
 * @brief UART HAL 分发层（契约 → driver API）
 */
#include "bm_drv_uart.h"
#include "bm_hal_uart.h"
#include "bm_types.h"

#ifdef BM_DRV_HAS_BACKEND
extern const struct bm_uart_driver_api bm_drv_uart_api;
#define BM_UART_DRV (&bm_drv_uart_api)
#else
static int uart_stub_init(void *config) {
    (void)config;
    return BM_ERR_NOT_INIT;
}

static int uart_stub_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return BM_ERR_NOT_INIT;
}

static size_t uart_stub_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void uart_stub_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}

static const struct bm_uart_driver_api uart_stub = {
    uart_stub_init,
    uart_stub_send,
    uart_stub_recv,
    uart_stub_set_rx_callback,
};

#define BM_UART_DRV (&uart_stub)
#endif

int bm_hal_uart_init(void *config) {
    if (!BM_UART_DRV->init) {
        return BM_ERR_NOT_INIT;
    }
    return BM_UART_DRV->init(config);
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    if (!BM_UART_DRV->send) {
        return BM_ERR_NOT_INIT;
    }
    return BM_UART_DRV->send(data, len);
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    if (!BM_UART_DRV->recv) {
        return 0u;
    }
    return BM_UART_DRV->recv(data, max_len);
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    if (BM_UART_DRV->set_rx_callback) {
        BM_UART_DRV->set_rx_callback(cb);
    }
}
