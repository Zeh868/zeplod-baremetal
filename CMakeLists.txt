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
option(BM_ENABLE_EXEC "Enable deterministic execution instances" OFF)
option(BM_ENABLE_STREAM "Enable block/frame stream (bm_stream)" OFF)
option(BM_ENABLE_PIPELINE "Enable static linear bm_pipeline" OFF)
option(BM_ENABLE_ALGORITHM "Enable deterministic algorithm library (bm_algorithm)" OFF)
option(BM_ENABLE_COMPONENT_MOTOR "Enable motor domain components" OFF)
option(BM_ENABLE_COMPONENT_BMS "Enable BMS domain components" OFF)
option(BM_ENABLE_COMPONENT_POWER "Enable power domain components" OFF)
option(BM_ENABLE_SYNC "Enable sync domain" OFF)
option(BM_SYNC_HAL_NATIVE "Link native_sim sync HAL into bm_exec" OFF)
option(BM_SYNC_HAL_QEMU "Link QEMU sync HAL into bm_exec" OFF)
option(BM_BUILD_TESTS "Build unit tests" ON)
set(BM_BACKEND "" CACHE STRING
    "Portable backend: native_sim, sdk_stm32g4, sdk_esp32_idf, qemu_cortex_m0, external, ...")

if(BM_ENABLE_EXEC AND NOT BM_ENABLE_HRT)
    message(FATAL_ERROR "BM_ENABLE_EXEC requires BM_ENABLE_HRT")
endif()

if(BM_ENABLE_SYNC AND NOT BM_ENABLE_EXEC)
    message(FATAL_ERROR "BM_ENABLE_SYNC requires BM_ENABLE_EXEC")
endif()
set(BM_CONFIG_FILE "" CACHE FILEPATH "Configuration header force-included in all targets")

if(NOT BM_CONFIG_FILE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h)
    set(BM_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/bm_include_dirs.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/bm_config_export.cmake)

if(BM_BUILD_TESTS)
    set(BM_ENABLE_STREAM ON CACHE BOOL "" FORCE)
    set(BM_ENABLE_PIPELINE ON CACHE BOOL "" FORCE)
    set(BM_ENABLE_ALGORITHM ON CACHE BOOL "" FORCE)
    set(BM_ENABLE_COMPONENT_MOTOR ON CACHE BOOL "" FORCE)
    set(BM_ENABLE_COMPONENT_BMS ON CACHE BOOL "" FORCE)
    set(BM_ENABLE_COMPONENT_POWER ON CACHE BOOL "" FORCE)
    if(NOT BM_BACKEND)
        set(BM_BACKEND "native_sim")
    endif()
endif()

add_library(bm_config INTERFACE)
target_include_directories(bm_config INTERFACE ${BM_INCLUDE_BACKEND_DIRS})
bm_export_config_enable(bm_config)

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
    Source/hal/bm_hal_critical.c
    Source/hal/bm_hal_uart.c
    Source/hal/bm_hal_timer.c
    Source/hal/bm_hal_wdg.c
    Source/hal/bm_hal_memory.c
    Source/hal/bm_hal_pwm.c
    Source/hal/bm_hal_adc.c
    Source/hal/bm_hal_comp.c
    Source/hal/bm_hal_encoder.c
    Source/hal/bm_hal_dma_stream.c
)
target_link_libraries(bm_hal PUBLIC bm_config)
bm_target_internal_includes(bm_hal)

if(BM_ENABLE_PIPELINE AND NOT BM_ENABLE_STREAM AND NOT BM_ENABLE_EXEC)
    message(FATAL_ERROR "BM_ENABLE_PIPELINE requires BM_ENABLE_STREAM or BM_ENABLE_EXEC")
endif()

if(BM_BACKEND STREQUAL "external")
    target_compile_definitions(bm_hal PRIVATE BM_DRV_HAS_BACKEND)
elseif(BM_BACKEND)
    target_compile_definitions(bm_hal PRIVATE BM_DRV_HAS_BACKEND)
    add_subdirectory(portable/${BM_BACKEND})
endif()

add_library(bm_core STATIC
    Source/core/bm_mempool.c
    Source/core/bm_critical.c
    Source/core/bm_event.c
    Source/core/bm_log.c
    Source/core/bm_log_output.c
    Source/core/bm_ultra.c
)
target_link_libraries(bm_core PUBLIC bm_config bm_hal)
bm_target_internal_includes(bm_core)
if(BM_BUILD_TESTS)
    target_compile_definitions(bm_core PRIVATE
        BM_ENABLE_EVENT_TEST_HOOK=1
        BM_ENABLE_ULTRA_TEST_HOOK=1
    )
endif()

add_library(bm_framework INTERFACE)
target_link_libraries(bm_framework INTERFACE bm_core bm_hal)

if(BM_ENABLE_MODULE)
    add_library(bm_module STATIC Source/core/bm_module.c)
    target_link_libraries(bm_module PUBLIC bm_core)
    bm_target_internal_includes(bm_module)
    target_link_libraries(bm_framework INTERFACE bm_module)
endif()

if(BM_ENABLE_CHANNEL)
    add_library(bm_channel STATIC Source/core/bm_channel.c)
    target_link_libraries(bm_channel PUBLIC bm_core)
    bm_target_internal_includes(bm_channel)
    target_link_libraries(bm_framework INTERFACE bm_channel)
endif()

if(BM_ENABLE_SHELL)
    add_library(bm_shell STATIC Source/core/bm_shell.c)
    target_link_libraries(bm_shell PUBLIC bm_core)
    bm_target_internal_includes(bm_shell)
    target_link_libraries(bm_framework INTERFACE bm_shell)
endif()

if(BM_ENABLE_WDG)
    add_library(bm_wdg STATIC Source/core/bm_wdg.c)
    target_link_libraries(bm_wdg PUBLIC bm_core)
    bm_target_internal_includes(bm_wdg)
    target_link_libraries(bm_framework INTERFACE bm_wdg)
endif()

if(BM_ENABLE_HRT)
    add_library(bm_hrt STATIC Source/hybrid/bm_hrt.c)
    target_link_libraries(bm_hrt PUBLIC bm_core)
    bm_target_internal_includes(bm_hrt)
    target_link_libraries(bm_framework INTERFACE bm_hrt)
endif()

if(BM_ENABLE_TICKER)
    add_library(bm_ticker STATIC Source/hybrid/bm_ticker.c)
    target_link_libraries(bm_ticker PUBLIC bm_core)
    bm_target_internal_includes(bm_ticker)
    target_link_libraries(bm_framework INTERFACE bm_ticker)
endif()

if(BM_BUILD_TESTS OR BM_ENABLE_EXEC)
    if(NOT TARGET bm_resource)
        add_library(bm_resource STATIC Source/hybrid/bm_resource.c)
        target_link_libraries(bm_resource PUBLIC bm_core)
        bm_target_internal_includes(bm_resource)
    endif()
endif()

if(BM_ENABLE_EXEC)
    add_library(bm_exec STATIC
        Source/hybrid/bm_exec.c
        Source/hybrid/bm_stream.c
    )
    target_link_libraries(bm_exec PUBLIC bm_hrt bm_core bm_resource)
    bm_target_internal_includes(bm_exec)
    if(BM_ENABLE_SYNC)
        target_sources(bm_exec PRIVATE Source/hybrid/bm_sync.c)
        if(BM_SYNC_HAL_NATIVE OR BM_BUILD_TESTS)
            target_sources(bm_exec PRIVATE
                portable/native_sim/bm_sync_hal_native.c)
        elseif(BM_SYNC_HAL_QEMU)
            target_sources(bm_exec PRIVATE
                portable/qemu_cortex_m0/bm_sync_hal_qemu.c)
        else()
            message(WARNING
                "BM_ENABLE_SYNC without BM_SYNC_HAL_NATIVE/QEMU: "
                "linking bm_sync_hal_stub.c (not for production)")
            target_sources(bm_exec PRIVATE Source/hybrid/bm_sync_hal_stub.c)
        endif()
    endif()
    target_link_libraries(bm_framework INTERFACE bm_exec)
elseif(BM_ENABLE_STREAM)
    add_library(bm_stream STATIC Source/hybrid/bm_stream.c)
    target_link_libraries(bm_stream PUBLIC bm_core bm_config)
    bm_target_internal_includes(bm_stream)
    target_link_libraries(bm_framework INTERFACE bm_stream)
endif()

if(BM_ENABLE_PIPELINE)
    add_library(bm_pipeline STATIC Source/hybrid/bm_pipeline.c)
    target_link_libraries(bm_pipeline PUBLIC bm_core bm_config)
    bm_target_internal_includes(bm_pipeline)
    if(TARGET bm_exec)
        target_link_libraries(bm_pipeline PUBLIC bm_exec)
    elseif(TARGET bm_stream)
        target_link_libraries(bm_pipeline PUBLIC bm_stream)
    endif()
    target_link_libraries(bm_framework INTERFACE bm_pipeline)
endif()

if(BM_ENABLE_ALGORITHM)
    add_library(bm_algorithm STATIC
        Source/algorithm/bm_algo_common.c
        Source/algorithm/bm_algo_control.c
        Source/algorithm/bm_algo_filter.c
        Source/algorithm/bm_algo_profile.c
        Source/algorithm/bm_algo_motion.c
        Source/algorithm/bm_algo_motor.c
        Source/algorithm/bm_algo_power.c
        Source/algorithm/bm_algo_battery.c
        Source/algorithm/bm_algo_fft.c
        Source/algorithm/bm_algo_spectral.c
        Source/algorithm/bm_algo_resample.c
        Source/algorithm/bm_algo_fusion.c
        Source/algorithm/bm_algo_audio.c
        Source/algorithm/bm_algo_image.c
        Source/algorithm/bm_algo_features.c
        Source/algorithm/bm_algo_calibration.c
        Source/algorithm/bm_algo_statistics.c
        Source/algorithm/bm_algo_comm.c
        Source/algorithm/bm_algo_signal_quality.c
        Source/algorithm/bm_algo_estimator.c
        Source/algorithm/bm_algo_fixed.c
    )
    target_link_libraries(bm_algorithm PUBLIC bm_config)
    if(NOT MSVC)
        target_link_libraries(bm_algorithm PUBLIC m)
    endif()
    bm_target_internal_includes(bm_algorithm)
    target_link_libraries(bm_framework INTERFACE bm_algorithm)
endif()

if(BM_ENABLE_COMPONENT_MOTOR)
    if(NOT BM_ENABLE_ALGORITHM)
        message(FATAL_ERROR "BM_ENABLE_COMPONENT_MOTOR requires BM_ENABLE_ALGORITHM")
    endif()
    add_library(bm_component_motor STATIC
        Source/component/motor_foc_sensored.c
    )
    target_link_libraries(bm_component_motor PUBLIC bm_algorithm bm_hal bm_config)
    bm_target_internal_includes(bm_component_motor)
    target_link_libraries(bm_framework INTERFACE bm_component_motor)
endif()

if(BM_ENABLE_COMPONENT_BMS)
    if(NOT BM_ENABLE_ALGORITHM)
        message(FATAL_ERROR "BM_ENABLE_COMPONENT_BMS requires BM_ENABLE_ALGORITHM")
    endif()
    add_library(bm_component_bms STATIC
        Source/component/bms_estimation.c
    )
    target_link_libraries(bm_component_bms PUBLIC bm_algorithm bm_config)
    bm_target_internal_includes(bm_component_bms)
    target_link_libraries(bm_framework INTERFACE bm_component_bms)
endif()

if(BM_ENABLE_COMPONENT_POWER)
    if(NOT BM_ENABLE_ALGORITHM)
        message(FATAL_ERROR "BM_ENABLE_COMPONENT_POWER requires BM_ENABLE_ALGORITHM")
    endif()
    add_library(bm_component_power STATIC
        Source/component/power_control.c
    )
    target_link_libraries(bm_component_power PUBLIC bm_algorithm bm_config)
    bm_target_internal_includes(bm_component_power)
    target_link_libraries(bm_framework INTERFACE bm_component_power)
endif()

if(BM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit/unity)
    add_subdirectory(tests/unit)
endif()
