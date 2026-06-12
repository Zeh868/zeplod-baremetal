cmake_minimum_required(VERSION 3.20)
project(zeplod_integration_smoke C)

# PC 冒烟：用 native_sim 作参考 Port。真机见 CMakeLists.append.txt（external + bm_port.c）。
set(ZEPLOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
include(${ZEPLOD_ROOT}/cmake/zeplod.cmake)

zeplod_configure(
    ROOT    ${ZEPLOD_ROOT}
    PROFILE minimal
    BACKEND native_sim
    CONFIG  ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h
)

add_executable(zeplod_integration_smoke test_consumer.c)
zeplod_link(zeplod_integration_smoke)
