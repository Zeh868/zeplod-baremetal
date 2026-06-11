# QEMU test firmware build
# Usage from project root:
#   cmake -B build_qemu -S tests/qemu -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake
#   cmake --build build_qemu
#
# The toolchain file MUST be passed on the command line (or via preset)
# because CMake reads it before processing this file.

cmake_minimum_required(VERSION 3.20)

# If no toolchain was specified, default to ARM Cortex-M0.
# NOTE: This only works when this directory is built standalone.
# When included from a parent project the toolchain is already fixed.
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/toolchain-arm-none-eabi.cmake
        CACHE FILEPATH "Toolchain file" FORCE)
endif()

project(qemu-tests C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source root (two levels up from tests/qemu/)
set(BM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${BM_ROOT}/include)

# Core library sources
set(BM_CORE_SRCS
    ${BM_ROOT}/src/core/bm_event.c
    ${BM_ROOT}/src/core/bm_mempool.c
    ${BM_ROOT}/src/core/bm_critical.c
    ${BM_ROOT}/src/core/bm_wdg.c
)

# ---------------------------------------------------------------------------
# QEMU Cortex-M0 target (micro:bit machine)
# ---------------------------------------------------------------------------
set(QEMU_CM0_HAL_SRCS
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_critical_qemu.c
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
)

set(BM_HYBRID_SRCS
    ${BM_ROOT}/src/hybrid/bm_hrt.c
)

add_executable(qemu_smoke_cm0.elf
    test_qemu_smoke.c
    ${BM_CORE_SRCS}
    ${QEMU_CM0_HAL_SRCS}
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/crt0_qemu.c
)

add_executable(qemu_hybrid_smoke_cm0.elf
    test_hybrid_smoke.c
    ${BM_CORE_SRCS}
    ${BM_HYBRID_SRCS}
    ${QEMU_CM0_HAL_SRCS}
    ${BM_ROOT}/hal_reference/qemu_cortex_m0/crt0_qemu.c
)

target_link_options(qemu_smoke_cm0.elf PRIVATE
    -T${BM_ROOT}/hal_reference/qemu_cortex_m0/linker.ld
    -nostartfiles
    -Wl,--gc-sections
)

target_link_options(qemu_hybrid_smoke_cm0.elf PRIVATE
    -T${BM_ROOT}/hal_reference/qemu_cortex_m0/linker.ld
    -nostartfiles
    -Wl,--gc-sections
)

# QEMU run helper (micro:bit machine has UART0 mapped to semihosting/stdio)
find_program(QEMU_ARM qemu-system-arm)
if(QEMU_ARM)
    message(STATUS "Found qemu-system-arm: ${QEMU_ARM}")

    add_custom_target(run-smoke-cm0
        COMMAND ${QEMU_ARM}
            -machine microbit
            -cpu cortex-m0
            -kernel $<TARGET_FILE:qemu_smoke_cm0.elf>
            --semihosting
            -display none
        DEPENDS qemu_smoke_cm0.elf
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running Cortex-M0 smoke test in QEMU"
    )

    add_custom_target(run-hybrid-smoke-cm0
        COMMAND ${QEMU_ARM}
            -machine microbit
            -cpu cortex-m0
            -kernel $<TARGET_FILE:qemu_hybrid_smoke_cm0.elf>
            --semihosting
            -display none
        DEPENDS qemu_hybrid_smoke_cm0.elf
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running hybrid HRT smoke test in QEMU"
    )
else()
    message(WARNING "qemu-system-arm not found; run-smoke-cm0 target unavailable")
endif()

# ---------------------------------------------------------------------------
# QEMU RISC-V 32 target (sifive_e machine) — toolchain required
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv" OR CMAKE_C_COMPILER MATCHES "riscv")
    set(QEMU_RV32_HAL_SRCS
        ${BM_ROOT}/hal_reference/qemu_riscv32/bm_hal_uart_qemu.c
        ${BM_ROOT}/hal_reference/qemu_riscv32/bm_hal_timer_qemu.c
        ${BM_ROOT}/hal_reference/qemu_riscv32/bm_hal_wdg_qemu.c
        ${BM_ROOT}/hal_reference/qemu_riscv32/startup_qemu_rv32.s
    )

    add_executable(qemu_smoke_rv32.elf
        test_qemu_smoke.c
        ${BM_CORE_SRCS}
        ${QEMU_RV32_HAL_SRCS}
    )

    target_link_options(qemu_smoke_rv32.elf PRIVATE
        -T${BM_ROOT}/hal_reference/qemu_riscv32/linker.ld
        -nostartfiles
        -Wl,--gc-sections
    )

    find_program(QEMU_RISCV qemu-system-riscv32)
    if(QEMU_RISCV)
        add_custom_target(run-smoke-rv32
            COMMAND ${QEMU_RISCV}
                -machine sifive_e
                -kernel $<TARGET_FILE:qemu_smoke_rv32.elf>
                -nographic
                -serial stdio
            DEPENDS qemu_smoke_rv32.elf
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running RISC-V 32 smoke test in QEMU"
        )
    endif()
endif()
