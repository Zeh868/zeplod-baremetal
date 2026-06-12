cmake_minimum_required(VERSION 3.20)
project(ultra_blink C ASM)

set(EXAMPLE_ENABLE_MODULE OFF)
set(EXAMPLE_ENABLE_CHANNEL OFF)
set(EXAMPLE_ENABLE_SHELL OFF)
set(EXAMPLE_ENABLE_WDG OFF)
include(../cmake/qemu_example.cmake)

bm_add_qemu_example(ultra_blink)
