cmake_minimum_required(VERSION 3.20)
project(ultra_blink C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ZEPLOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    CACHE PATH "Path to zeplod-baremetal framework root")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "-T${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles -Wl,--gc-sections")

include_directories(
    ${ZEPLOD_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(ultra_blink.elf
    main.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
)
