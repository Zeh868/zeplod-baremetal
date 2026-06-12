# 交叉编译 Zeplod 静态库（库本体，不含 Port）
#
#   cmake -B build -S integration/static-lib \
#       -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake \
#       -DZEPLOD_PROFILE=event \
#       -DZEPLOD_CONFIG=../../integration/cmake-snippet/bm_config.h
#   cmake --build build
#
# 产物：build/_zeplod_lib_build/**/libbm_*.a
# 应用工程链接这些 .a，并自行编译 integration/port/bm_port.c

cmake_minimum_required(VERSION 3.20)
project(zeplod-static-libs C)

set(ZEPLOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(ZEPLOD_PROFILE "event" CACHE STRING "minimal | event | servo | full")
set(ZEPLOD_CONFIG "${ZEPLOD_ROOT}/integration/cmake-snippet/bm_config.h"
    CACHE FILEPATH "bm_config.h for library build")

include(${ZEPLOD_ROOT}/cmake/zeplod_library.cmake)
zeplod_build_libraries(
    ROOT    ${ZEPLOD_ROOT}
    PROFILE ${ZEPLOD_PROFILE}
    CONFIG  ${ZEPLOD_CONFIG}
)

zeplod_library_targets(_lib_targets)
message(STATUS "Zeplod static libraries: ${_lib_targets}")
message(STATUS "Copy libbm_*.a + include/ to your project; compile port/bm_port.c in app.")
