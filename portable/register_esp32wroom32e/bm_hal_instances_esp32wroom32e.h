/**
 * @file bm_hal_instances_esp32wroom32e.h
 * @brief ESP32-WROOM-32E 模组/开发板参考引脚映射
 *
 * 典型 ESP32-DevKitC：UART0 控制台 U0TXD=GPIO1、U0RXD=GPIO3。
 * GPIO 矩阵与时钟初始化由应用或 ESP-IDF 启动代码完成。
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-12
 */
#ifndef BM_HAL_INSTANCES_ESP32WROOM32E_H
#define BM_HAL_INSTANCES_ESP32WROOM32E_H

#include "bm_hal_regs_esp32.h"

/** 控制台 UART（ROM / 默认下载串口） */
#define BM_ESP32WROOM32E_UART_CONSOLE_BASE  BM_ESP32_UART0_BASE

/** 系统 tick 定时器：Timer Group 0 Timer 0 */
#define BM_ESP32WROOM32E_TIMER_TICK_BASE     BM_ESP32_TG0_BASE

/** TG0_T0_LEVEL 中断号（ESP32 技术参考手册） */
#define BM_ESP32WROOM32E_TG0_T0_IRQ          52

#endif /* BM_HAL_INSTANCES_ESP32WROOM32E_H */
