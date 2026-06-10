cmake_minimum_required(VERSION 3.20)
project(zeplod-baremetal C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BM_ENABLE_MODULE "Enable bm-module" ON)
option(BM_ENABLE_CHANNEL "Enable bm-channel" OFF)
option(BM_ENABLE_SHELL "Enable bm-shell" OFF)
option(BM_ENABLE_WDG "Enable bm-wdg" ON)
option(BM_BUILD_TESTS "Build unit tests" ON)
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

add_library(bm_core STATIC
    src/core/bm_mempool.c
    src/core/bm_critical.c
    src/core/bm_event.c
)
target_link_libraries(bm_core PUBLIC bm_config)

add_library(bm_framework INTERFACE)
target_link_libraries(bm_framework INTERFACE bm_core)

if(BM_ENABLE_MODULE)
    add_library(bm_module STATIC src/module/bm_module.c)
    target_link_libraries(bm_module PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_module)
endif()

if(BM_ENABLE_CHANNEL)
    add_library(bm_channel STATIC src/channel/bm_channel.c)
    target_link_libraries(bm_channel PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_channel)
endif()

if(BM_ENABLE_SHELL)
    add_library(bm_shell STATIC src/shell/bm_shell.c)
    target_link_libraries(bm_shell PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_shell)
endif()

if(BM_ENABLE_WDG)
    add_library(bm_wdg STATIC src/core/bm_wdg.c)
    target_link_libraries(bm_wdg PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_wdg)
endif()

if(BM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit/unity)
    add_subdirectory(tests/unit)
endif()

add_subdirectory(hal_reference/native_sim EXCLUDE_FROM_ALL)
