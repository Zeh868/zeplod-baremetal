cmake_minimum_required(VERSION 3.20)
project(zeplod-baremetal C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

option(BM_ENABLE_MODULE "Enable bm-module" ON)
option(BM_ENABLE_CHANNEL "Enable bm-channel (deferred)" OFF)
option(BM_ENABLE_SHELL "Enable bm-shell (deferred)" OFF)
option(BM_ENABLE_WDG "Enable bm-wdg" ON)

set(BM_CORE_SRCS
    src/core/bm_mempool.c
    src/core/bm_critical.c
    src/core/bm_event.c
)

if(BM_ENABLE_WDG AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/core/bm_wdg.c)
    list(APPEND BM_CORE_SRCS src/core/bm_wdg.c)
endif()

if(BM_ENABLE_MODULE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/module/bm_module.c)
    list(APPEND BM_CORE_SRCS src/module/bm_module.c)
endif()

add_library(bm_core STATIC ${BM_CORE_SRCS})
target_include_directories(bm_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Tests
option(BM_BUILD_TESTS "Build unit tests" ON)
if(BM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit/unity)
    add_subdirectory(tests/unit)
endif()

add_subdirectory(hal_reference/native_sim EXCLUDE_FROM_ALL)
