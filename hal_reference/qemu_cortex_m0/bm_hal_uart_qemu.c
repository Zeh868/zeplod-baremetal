#include "bm_hal_uart.h"

/* QEMU micro:bit UART0 base */
#define UART0_BASE  0x40002000
#define UART_TXD    (*(volatile uint32_t *)(UART0_BASE + 0x51C))
#define UART_TXDRDY (*(volatile uint32_t *)(UART0_BASE + 0x11C))

/* ARM semihosting write (works in QEMU with --semihosting).
 * QEMU expects the buffer pointer directly in R1 for SYS_WRITE. */
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

int bm_hal_uart_init(void *config) { (void)config; return 0; }

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    /* Use semihosting for QEMU console output (nRF51 UART needs init) */
    semihosting_write(data, len);
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len; return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) { (void)cb; }
