/**
 * @file bm_hal_uart_stm32f0.c
 * @brief STM32F0 UART HAL 参考桩实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_uart.h"
#include "bm_log.h"

#define TAG "hal_uart"

/* STM32F0 UART1 桩 — 用户应替换为实际寄存器地址与驱动 */

int bm_hal_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG, "init: stub (replace with UART1 driver)");
    return 0;
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}
