# QEMU test firmware build — requires cross-compilation toolchain
# Usage: cmake -B build_qemu -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake ..

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake)

# TODO: Add QEMU test executable targets in M4+ phase
