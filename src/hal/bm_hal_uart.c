/**
 * @file bm_hal_uart.c
 * @brief UART HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_uart.h"
#include "bm_hal_weak.h"
#include "bm_types.h"

BM_HAL_WEAK
int bm_hal_uart_init(void *config) {
    (void)config;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
int bm_hal_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

BM_HAL_WEAK
void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}
