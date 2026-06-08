#include "bm_hal_uart.h"
#include <stdio.h>

static void (*rx_cb)(uint8_t c) = NULL;

int bm_hal_uart_init(void *config) {
    (void)config;
    return 0;
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len;
    return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    rx_cb = cb;
}
