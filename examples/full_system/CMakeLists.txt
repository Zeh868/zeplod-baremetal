cmake_minimum_required(VERSION 3.20)
project(full_system C ASM)

set(EXAMPLE_ENABLE_MODULE ON)
set(EXAMPLE_ENABLE_CHANNEL OFF)
set(EXAMPLE_ENABLE_SHELL OFF)
set(EXAMPLE_ENABLE_WDG ON)
include(../cmake/qemu_example.cmake)

bm_add_qemu_example(full_system
    FRAMEWORK_LIBS bm_core bm_module bm_wdg
    HAL_SOURCES
        ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
        ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
        ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c
)
