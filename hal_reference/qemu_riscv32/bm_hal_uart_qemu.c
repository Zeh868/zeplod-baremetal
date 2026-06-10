/**
 * @file bm_hal_uart_qemu.c
 * @brief QEMU RISC-V32 UART HAL 实现（MMIO 发送）
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

#define UART0_TX (*(volatile uint32_t *)0x10013000)

int bm_hal_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG, "init: MMIO 0x10013000");
    return 0;
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        UART0_TX = data[i];
    }
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
