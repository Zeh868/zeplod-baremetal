cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

if(NATIVE_SIM_BUILD)
    project(hrt_servo_stub_native C)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_CTRL_INST ON)
    set(EXAMPLE_ENABLE_SYNC OFF)
    include(../cmake/native_sim_example.cmake)
    bm_add_native_sim_example(hrt_servo_stub
        FRAMEWORK_LIBS bm_core bm_hrt bm_ticker bm_ctrl_inst
    )
else()
    project(hrt_servo_stub C ASM)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_CTRL_INST ON)
    set(EXAMPLE_ENABLE_SYNC OFF)
    include(../cmake/qemu_example.cmake)
    bm_add_qemu_example(hrt_servo_stub
        FRAMEWORK_LIBS bm_core bm_hrt bm_ticker bm_ctrl_inst
        HAL_SOURCES
            ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
            ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
    )
endif()
