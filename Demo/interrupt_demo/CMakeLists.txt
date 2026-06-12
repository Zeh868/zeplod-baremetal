cmake_minimum_required(VERSION 3.20)
project(interrupt_demo C ASM)

set(EXAMPLE_ENABLE_MODULE ON)
set(EXAMPLE_ENABLE_CHANNEL OFF)
set(EXAMPLE_ENABLE_SHELL OFF)
set(EXAMPLE_ENABLE_WDG OFF)
include(../cmake/qemu_example.cmake)

file(GLOB INTERRUPT_DEMO_MODULES CONFIGURE_DEPENDS modules/*.c)
list(APPEND INTERRUPT_DEMO_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/app_irq.c)

bm_add_qemu_example(interrupt_demo
    FRAMEWORK_LIBS bm_core bm_module
    EXTRA_SOURCES interrupt_timer.c ${INTERRUPT_DEMO_MODULES}
)
