/**
 * @file bm_hal_uart_native.c
 * @brief 原生仿真环境 UART HAL 实现（stdout 输出）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_uart.h"
#include "bm_log.h"

#include <stdio.h>

#define TAG "hal_uart"

static void (*rx_cb)(uint8_t c) = NULL;

/** 初始化 UART（原生仿真无硬件配置） */
int bm_hal_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG, "init: native stdout backend");
    return 0;
}

/** 通过 stdout 发送数据 */
int bm_hal_uart_send(const uint8_t *data, size_t len) {
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return 0;
}

/** 接收数据（原生仿真暂不支持，始终返回 0） */
size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    rx_cb = cb;
}
