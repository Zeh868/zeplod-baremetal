cmake_minimum_required(VERSION 3.20)
project(full_system C ASM)

set(EXAMPLE_ENABLE_MODULE ON)
set(EXAMPLE_ENABLE_CHANNEL OFF)
set(EXAMPLE_ENABLE_SHELL OFF)
set(EXAMPLE_ENABLE_WDG ON)
include(../cmake/qemu_example.cmake)

file(GLOB FULL_SYSTEM_MODULES CONFIGURE_DEPENDS modules/*.c)

bm_add_qemu_example(full_system
    FRAMEWORK_LIBS bm_core bm_module bm_wdg
    EXTRA_SOURCES ${FULL_SYSTEM_MODULES}
)
