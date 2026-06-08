#include "bm_hal_uart.h"

/* QEMU micro:bit UART0 base */
#define UART0_BASE  0x40002000
#define UART_TXD    (*(volatile uint32_t *)(UART0_BASE + 0x51C))
#define UART_TXDRDY (*(volatile uint32_t *)(UART0_BASE + 0x11C))

int bm_hal_uart_init(void *config) { (void)config; return 0; }

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        UART_TXD = data[i];
        UART_TXDRDY = 0;
    }
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len; return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) { (void)cb; }
