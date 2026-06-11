cmake_minimum_required(VERSION 3.20)
project(zeplod-baremetal C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

option(BM_ENABLE_MODULE "Enable bm-module" ON)
option(BM_ENABLE_CHANNEL "Enable bm-channel" OFF)
option(BM_ENABLE_SHELL "Enable bm-shell" OFF)
option(BM_ENABLE_WDG "Enable bm-wdg" ON)
option(BM_ENABLE_HRT "Enable bm-hrt" OFF)
option(BM_ENABLE_TICKER "Enable bm-ticker" OFF)
option(BM_ENABLE_CTRL_INST "Enable multi-instance control layer" OFF)
option(BM_ENABLE_SYNC "Enable sync domain" OFF)
option(BM_SYNC_HAL_NATIVE "Link native_sim sync HAL into bm_ctrl_inst" OFF)
option(BM_SYNC_HAL_QEMU "Link QEMU sync HAL into bm_ctrl_inst" OFF)
option(BM_BUILD_TESTS "Build unit tests" ON)

if(BM_ENABLE_CTRL_INST AND NOT BM_ENABLE_HRT)
    message(FATAL_ERROR "BM_ENABLE_CTRL_INST requires BM_ENABLE_HRT")
endif()

if(BM_ENABLE_SYNC AND NOT BM_ENABLE_CTRL_INST)
    message(FATAL_ERROR "BM_ENABLE_SYNC requires BM_ENABLE_CTRL_INST")
endif()
set(BM_CONFIG_FILE "" CACHE FILEPATH "Configuration header force-included in all targets")

if(NOT BM_CONFIG_FILE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h)
    set(BM_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h)
endif()

add_library(bm_config INTERFACE)
target_include_directories(bm_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BM_CONFIG_FILE)
    get_filename_component(BM_CONFIG_FILE "${BM_CONFIG_FILE}" ABSOLUTE)
    if(NOT EXISTS "${BM_CONFIG_FILE}")
        message(FATAL_ERROR "BM_CONFIG_FILE does not exist: ${BM_CONFIG_FILE}")
    endif()
    target_compile_options(bm_config INTERFACE
        "$<$<C_COMPILER_ID:MSVC>:/FI${BM_CONFIG_FILE}>"
        "$<$<NOT:$<C_COMPILER_ID:MSVC>>:-include${BM_CONFIG_FILE}>"
    )
endif()

add_library(bm_hal STATIC
    src/hal/bm_hal_critical.c
    src/hal/bm_hal_uart.c
    src/hal/bm_hal_timer.c
    src/hal/bm_hal_wdg.c
    src/hal/bm_hal_memory.c
    src/hal/bm_hal_pwm.c
    src/hal/bm_hal_adc.c
    src/hal/bm_hal_comp.c
    src/hal/bm_hal_encoder.c
)
target_link_libraries(bm_hal PUBLIC bm_config)

add_library(bm_core STATIC
    src/core/bm_mempool.c
    src/core/bm_critical.c
    src/core/bm_event.c
    src/core/bm_log.c
    src/core/bm_ultra.c
)
target_link_libraries(bm_core PUBLIC bm_config bm_hal)
if(BM_BUILD_TESTS)
    target_compile_definitions(bm_core PRIVATE
        BM_ENABLE_EVENT_TEST_HOOK=1
        BM_ENABLE_ULTRA_TEST_HOOK=1
    )
endif()

add_library(bm_framework INTERFACE)
target_link_libraries(bm_framework INTERFACE bm_core bm_hal)

if(BM_ENABLE_MODULE)
    add_library(bm_module STATIC src/core/bm_module.c)
    target_link_libraries(bm_module PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_module)
endif()

if(BM_ENABLE_CHANNEL)
    add_library(bm_channel STATIC src/core/bm_channel.c)
    target_link_libraries(bm_channel PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_channel)
endif()

if(BM_ENABLE_SHELL)
    add_library(bm_shell STATIC src/core/bm_shell.c)
    target_link_libraries(bm_shell PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_shell)
endif()

if(BM_ENABLE_WDG)
    add_library(bm_wdg STATIC src/core/bm_wdg.c)
    target_link_libraries(bm_wdg PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_wdg)
endif()

if(BM_ENABLE_HRT)
    add_library(bm_hrt STATIC src/hybrid/bm_hrt.c)
    target_link_libraries(bm_hrt PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_hrt)
endif()

if(BM_ENABLE_TICKER)
    add_library(bm_ticker STATIC src/hybrid/bm_ticker.c)
    target_link_libraries(bm_ticker PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_ticker)
endif()

if(BM_BUILD_TESTS OR BM_ENABLE_CTRL_INST)
    if(NOT TARGET bm_resource)
        add_library(bm_resource STATIC src/hybrid/bm_resource.c)
        target_link_libraries(bm_resource PUBLIC bm_core)
    endif()
endif()

if(BM_ENABLE_CTRL_INST)
    add_library(bm_ctrl_inst STATIC src/hybrid/bm_ctrl_inst.c)
    target_link_libraries(bm_ctrl_inst PUBLIC bm_hrt bm_core bm_resource)
    if(BM_ENABLE_SYNC)
        target_sources(bm_ctrl_inst PRIVATE src/hybrid/bm_sync.c)
        if(BM_SYNC_HAL_NATIVE OR BM_BUILD_TESTS)
            target_sources(bm_ctrl_inst PRIVATE
                hal_reference/native_sim/bm_sync_hal_native.c)
        elseif(BM_SYNC_HAL_QEMU)
            target_sources(bm_ctrl_inst PRIVATE
                hal_reference/qemu_cortex_m0/bm_sync_hal_qemu.c)
        else()
            target_sources(bm_ctrl_inst PRIVATE src/hybrid/bm_sync_hal_stub.c)
        endif()
    endif()
    target_link_libraries(bm_framework INTERFACE bm_ctrl_inst)
endif()

if(BM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit/unity)
    add_subdirectory(tests/unit)
endif()

add_subdirectory(hal_reference/native_sim EXCLUDE_FROM_ALL)
add_subdirectory(hal_reference/stm32g4 EXCLUDE_FROM_ALL)
add_subdirectory(hal_reference/ch32v003 EXCLUDE_FROM_ALL)
