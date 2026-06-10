/**
 * @file bm_hal_uart_ch32v003.c
 * @brief CH32V003 UART HAL 参考桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_uart.h"
#include "bm_log.h"
#include "bm_types.h"

#define TAG "hal_uart"

int bm_hal_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG, "init: USART1 stub (add WCH LL driver)");
    return BM_OK;
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return BM_OK;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}
