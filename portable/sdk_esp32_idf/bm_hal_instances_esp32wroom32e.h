/**
 * @file bm_hal_instances_esp32wroom32e.h
 * @brief ESP32-WROOM-32E 模组/开发板参考映射（ESP-IDF 驱动）
 *
 * 典型 ESP32-DevKitC：UART0 控制台 U0TXD=GPIO1、U0RXD=GPIO3。
 * 引脚与时钟由 ESP-IDF 启动代码 / board support 完成。
 */
#ifndef BM_HAL_INSTANCES_ESP32WROOM32E_H
#define BM_HAL_INSTANCES_ESP32WROOM32E_H

#include "bm_hal_sdk_esp32.h"

/** 控制台 UART（ROM / 默认下载串口） */
#define BM_ESP32WROOM32E_UART_PORT  CONFIG_ESP_CONSOLE_UART_NUM

/** 系统 tick：GPTimer（替代手写 Timer Group 寄存器） */
#define BM_ESP32WROOM32E_TICK_TIMER_RESOLUTION_HZ  1000000u

#endif /* BM_HAL_INSTANCES_ESP32WROOM32E_H */
