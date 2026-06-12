cmake_minimum_required(VERSION 3.20)
project(interrupt_demo C ASM)

set(EXAMPLE_ENABLE_MODULE OFF)
set(EXAMPLE_ENABLE_CHANNEL OFF)
set(EXAMPLE_ENABLE_SHELL OFF)
set(EXAMPLE_ENABLE_WDG OFF)
include(../cmake/qemu_example.cmake)

bm_add_qemu_example(interrupt_demo
    FRAMEWORK_LIBS bm_core
    EXTRA_SOURCES interrupt_timer.c
)
