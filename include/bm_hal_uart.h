#ifndef BM_HAL_UART_H
#define BM_HAL_UART_H

#include <stdint.h>
#include <stddef.h>

int bm_hal_uart_init(void *config);
int bm_hal_uart_send(const uint8_t *data, size_t len);
size_t bm_hal_uart_recv(uint8_t *data, size_t max_len);
void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c));

#endif
