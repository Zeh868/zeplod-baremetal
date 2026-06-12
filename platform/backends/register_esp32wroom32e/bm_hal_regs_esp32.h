/**
 * @file bm_hal_regs_esp32.h
 * @brief ESP32 外设寄存器最小映射（参考实现用，可替换为 ESP-IDF soc 头）
 *
 * 地址依据 ESP32 技术参考手册（ESP32-WROOM-32E 模组内芯为 ESP32）。
 * 应用可定义 BM_ESP32_USE_IDF_HEADERS 并包含 esp32/rom/uart.h 等。
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-12
 */
#ifndef BM_HAL_REGS_ESP32_H
#define BM_HAL_REGS_ESP32_H

#include <stdint.h>

#ifndef BM_ESP32_UART0_BASE
#define BM_ESP32_UART0_BASE  0x3FF40000u
#endif

#ifndef BM_ESP32_TG0_BASE
#define BM_ESP32_TG0_BASE    0x3FF5F000u
#endif

#ifndef BM_ESP32_GPIO_BASE
#define BM_ESP32_GPIO_BASE   0x3FF44000u
#endif

#ifndef BM_ESP32_DPORT_BASE
#define BM_ESP32_DPORT_BASE  0x3FF00000u
#endif

#ifndef BM_ESP32_MWDT0_BASE
#define BM_ESP32_MWDT0_BASE  0x3FF60000u
#endif

/** 典型 APB 时钟（bootloader 配置后约 80 MHz） */
#ifndef BM_ESP32_APB_CLK_HZ
#define BM_ESP32_APB_CLK_HZ  80000000u
#endif

#define BM_REG32(addr) (*(volatile uint32_t *)(addr))

/* UART0 */
#define BM_UART_FIFO(base)       BM_REG32((base) + 0x00u)
#define BM_UART_INT_RAW(base)    BM_REG32((base) + 0x04u)
#define BM_UART_INT_ST(base)     BM_REG32((base) + 0x08u)
#define BM_UART_INT_ENA(base)    BM_REG32((base) + 0x0Cu)
#define BM_UART_INT_CLR(base)    BM_REG32((base) + 0x10u)
#define BM_UART_CLKDIV(base)     BM_REG32((base) + 0x14u)
#define BM_UART_STATUS(base)     BM_REG32((base) + 0x20u)

#define BM_UART_STATUS_TXFIFO_CNT_SHIFT  16u
#define BM_UART_STATUS_TXFIFO_CNT_MASK   0x3Fu

/* Timer Group 0 — Timer 0 */
#define BM_TG_T0CONFIG(base)     BM_REG32((base) + 0x00u)
#define BM_TG_T0LO(base)         BM_REG32((base) + 0x04u)
#define BM_TG_T0HI(base)         BM_REG32((base) + 0x08u)
#define BM_TG_T0UPDATE(base)     BM_REG32((base) + 0x0Cu)
#define BM_TG_T0ALARMLO(base)    BM_REG32((base) + 0x10u)
#define BM_TG_T0ALARMHI(base)    BM_REG32((base) + 0x14u)
#define BM_TG_T0LOADLO(base)     BM_REG32((base) + 0x18u)
#define BM_TG_T0LOADHI(base)     BM_REG32((base) + 0x1Cu)
#define BM_TG_T0LOAD(base)       BM_REG32((base) + 0x20u)
#define BM_TG_INT_ENA(base)      BM_REG32((base) + 0x98u)
#define BM_TG_INT_ST(base)       BM_REG32((base) + 0x9Cu)
#define BM_TG_INT_CLR(base)      BM_REG32((base) + 0xA0u)

#define BM_TG_T0CONFIG_EN            (1u << 31)
#define BM_TG_T0CONFIG_ALARM_EN      (1u << 30)
#define BM_TG_T0CONFIG_LEVEL_INT_EN  (1u << 29)
#define BM_TG_T0CONFIG_AUTORELOAD    (1u << 14)
#define BM_TG_T0CONFIG_INCREASE      (1u << 13)
#define BM_TG_T0CONFIG_DIVIDER_SHIFT 0u
#define BM_TG_T0CONFIG_DIVIDER_MASK  0x3FFu

#define BM_TG_INT_T0                 (1u << 0)

/* Main Watchdog Timer 0 */
#define BM_MWDT_CONFIG(base)         BM_REG32((base) + 0x00u)
#define BM_MWDT_FEED(base)           BM_REG32((base) + 0x04u)

#define BM_MWDT_CONFIG_EN            (1u << 0)

#endif /* BM_HAL_REGS_ESP32_H */
