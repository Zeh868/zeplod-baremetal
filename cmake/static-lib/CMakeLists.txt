# 交叉编译 Zeplod 静态库（库本体，不含 Port）
# 文档：docs/15-静态库构建.md

cmake_minimum_required(VERSION 3.20)
project(zeplod-static-libs C)

set(ZEPLOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(ZEPLOD_PROFILE "event" CACHE STRING "minimal | event | servo | full")
set(ZEPLOD_CONFIG "${ZEPLOD_ROOT}/cmake/integration-snippet/bm_config.h"
    CACHE FILEPATH "bm_config.h for library build")

include(${ZEPLOD_ROOT}/cmake/zeplod_library.cmake)
zeplod_build_libraries(
    ROOT    ${ZEPLOD_ROOT}
    PROFILE ${ZEPLOD_PROFILE}
    CONFIG  ${ZEPLOD_CONFIG}
)

zeplod_library_targets(_lib_targets)
message(STATUS "Zeplod static libraries: ${_lib_targets}")
message(STATUS "See docs/15-静态库构建.md")
