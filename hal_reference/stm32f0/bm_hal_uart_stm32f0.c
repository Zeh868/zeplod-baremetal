#include "bm_hal_uart.h"

/* STM32F0 UART1 stub — user should replace with actual register addresses */
int bm_hal_uart_init(void *config) { (void)config; return 0; }
int bm_hal_uart_send(const uint8_t *data, size_t len) { (void)data; (void)len; return 0; }
size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) { (void)data; (void)max_len; return 0; }
void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) { (void)cb; }
