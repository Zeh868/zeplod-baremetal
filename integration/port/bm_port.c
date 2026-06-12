/**
 * @file bm_port.c
 * @brief Zeplod 平台移植层（Port）— 类比 FreeRTOS portable/port.c
 *
 * 目标：ARM Cortex-M + GCC/Clang/Arm Compiler（Keil/IAR）。
 * PC 仿真请链 platform/backends/native_sim，勿编译本文件。
 *
 * 本文件属于 **应用工程**，不属于 Zeplod 库本体。
 * 将本文件复制到 CubeMX / MCUXpresso / Keil 工程，按注释对接厂商 HAL。
 *
 * 必须导出的符号：bm_drv_critical_api、bm_drv_memory_api
 * 按 PROFILE 选用：bm_drv_timer_api、bm_drv_uart_api、bm_drv_wdg_api 等
 *
 * 参考完整实现：platform/backends/register_stm32g4/bm_drv_singleton_stm32g4.c
 */

#include <stddef.h>
#include <stdint.h>

#include "bm_drv_critical.h"
#include "bm_drv_memory.h"
#include "bm_drv_timer.h"
#include "bm_drv_uart.h"
#include "bm_types.h"

/* 若使用 STM32 Cube HAL，取消注释并包含对应头文件 */
/* #include "stm32g4xx_hal.h" */
/* extern TIM_HandleTypeDef htim6; */
/* extern UART_HandleTypeDef huart2; */

/* --- 临界区（必须）--- */

static inline uint32_t port_read_primask(void) {
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r"(primask));
    return primask;
}

static inline void port_write_primask(uint32_t primask) {
    __asm volatile ("msr primask, %0" :: "r"(primask) : "memory");
}

static bm_irq_state_t port_critical_enter(void) {
    bm_irq_state_t state = port_read_primask();
    __asm volatile ("cpsid i" ::: "memory");
    return state;
}

static void port_critical_exit(bm_irq_state_t state) {
    port_write_primask(state);
}

const struct bm_critical_driver_api bm_drv_critical_api = {
    port_critical_enter,
    port_critical_exit,
};

/* --- 内存屏障（必须）--- */

static void port_memory_release(void) {
    __asm volatile ("dmb" ::: "memory");
}

static void port_memory_full(void) {
    __asm volatile ("dsb" ::: "memory");
}

const struct bm_memory_driver_api bm_drv_memory_api = {
    port_memory_release,
    port_memory_full,
};

/* --- 系统节拍定时器（事件/看门狗/混合域需要）--- */

static void (*g_tick_cb)(void);
static uint32_t g_tick_hz;

static int port_timer_init(uint32_t freq_hz) {
    g_tick_hz = freq_hz;
    /* TODO: 配置一个基础定时器，例如 CubeMX TIM6 1MHz 计数 */
    /* HAL_TIM_Base_Start_IT(&htim6); */
    return 0;
}

static void port_timer_stop(void) {
    /* HAL_TIM_Base_Stop_IT(&htim6); */
}

static uint32_t port_timer_get_ticks(void) {
    /* return __HAL_TIM_GET_COUNTER(&htim6); */
    return 0u;
}

static uint32_t port_timer_get_freq(void) {
    return g_tick_hz;
}

static void port_timer_set_callback(void (*cb)(void)) {
    g_tick_cb = cb;
}

const struct bm_timer_driver_api bm_drv_timer_api = {
    port_timer_init,
    port_timer_stop,
    port_timer_get_ticks,
    port_timer_get_freq,
    port_timer_set_callback,
};

/* 在 TIM 更新中断里调用（示例名 TIM6_DAC_IRQHandler） */
void bm_port_timer_isr(void) {
    if (g_tick_cb) {
        g_tick_cb();
    }
}

/* --- UART（日志 / Shell 需要）--- */

static int port_uart_init(void *config) {
    (void)config;
    /* return HAL_UART_Init(&huart2) == HAL_OK ? 0 : -1; */
    return 0;
}

static int port_uart_send(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    /* HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 100); */
    return 0;
}

static size_t port_uart_recv(uint8_t *data, size_t max_len) {
    (void)data;
    (void)max_len;
    return 0u;
}

static void port_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    (void)cb;
}

const struct bm_uart_driver_api bm_drv_uart_api = {
    port_uart_init,
    port_uart_send,
    port_uart_recv,
    port_uart_set_rx_callback,
};
