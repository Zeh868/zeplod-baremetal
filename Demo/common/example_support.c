/**
 * @file example_support.c
 * @brief 示例公共辅助函数实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "example_support.h"

#include "hal/bm_hal_uart.h"
#include "bm_log.h"

#include <stddef.h>
#include <string.h>

#define TAG "example"

void example_print(const char *text) {
    if (text) {
        bm_hal_uart_send((const uint8_t *)text, strlen(text));
    }
}

void example_print_u32(uint32_t value) {
    char reversed[10];
    char output[11];
    size_t count = 0;

    /* 将数字倒序存入 reversed */
    do {
        reversed[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value > 0U);

    /* 反转得到正序十进制字符串 */
    for (size_t i = 0; i < count; i++) {
        output[i] = reversed[count - 1U - i];
    }
    output[count] = '\0';
    BM_LOGD(TAG, "print_u32: %s", output);
    bm_hal_uart_send((const uint8_t *)output, count);
}

void example_delay_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; i++) {
    }
}
