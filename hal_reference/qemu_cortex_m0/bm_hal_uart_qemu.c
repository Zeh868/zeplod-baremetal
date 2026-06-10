/**
 * @file bm_hal_uart_qemu.c
 * @brief QEMU Cortex-M0 UART HAL 实现（semihosting 输出）
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

/* QEMU micro:bit UART0 寄存器基址（nRF51） */
#define UART0_BASE  0x40002000
#define UART_TXD    (*(volatile uint32_t *)(UART0_BASE + 0x51C))
#define UART_TXDRDY (*(volatile uint32_t *)(UART0_BASE + 0x11C))

/**
 * ARM semihosting 写操作（QEMU --semihosting 下可用）。
 * QEMU 对 SYS_WRITE 期望缓冲区指针直接在 R1 中。
 */
static void semihosting_write(const uint8_t *data, size_t len) {
    (void)len;
    __asm volatile (
        "movs r0, #0x04\n"
        "movs r1, %0\n"
        "bkpt 0xAB\n"
        :
        : "r" (data)
        : "r0", "r1", "memory"
    );
}

int bm_hal_uart_init(void *config) {
    (void)config;
    BM_LOGI(TAG, "init: semihosting backend");
    return 0;
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    /* nRF51 UART 需初始化，QEMU 控制台走 semihosting */
    semihosting_write(data, len);
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
