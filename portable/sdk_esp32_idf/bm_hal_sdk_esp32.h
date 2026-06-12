/**
 * @file bm_hal_sdk_esp32.h
 * @brief ESP-IDF 头文件入口（需 IDF_PATH 或 ESP-IDF 组件构建）
 */
#ifndef BM_HAL_SDK_ESP32_H
#define BM_HAL_SDK_ESP32_H

#if defined(__has_include) && __has_include("sdkconfig.h")
#include "sdkconfig.h"
#else
#include "bm_idf_minimal_config.h"
#endif

#include "esp_err.h"
#include "driver/gptimer.h"
#include "driver/uart.h"
#include "esp_task_wdt.h"

#endif /* BM_HAL_SDK_ESP32_H */
