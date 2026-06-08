#include "bm_hal_uart.h"

#define UART0_TX (*(volatile uint32_t *)0x10013000)

int bm_hal_uart_init(void *config) { (void)config; return 0; }

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        UART0_TX = data[i];
    }
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len; return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) { (void)cb; }
