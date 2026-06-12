cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

if(NATIVE_SIM_BUILD)
    project(multi_axis_sync_native C)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER OFF)
    set(EXAMPLE_ENABLE_CTRL_INST ON)
    set(EXAMPLE_ENABLE_SYNC ON)
    include(../cmake/native_sim_example.cmake)
    bm_add_native_sim_example(multi_axis_sync
        FRAMEWORK_LIBS bm_core bm_hrt bm_ctrl_inst
    )
else()
    project(multi_axis_sync C ASM)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER OFF)
    set(EXAMPLE_ENABLE_CTRL_INST ON)
    set(EXAMPLE_ENABLE_SYNC ON)
    include(../cmake/qemu_example.cmake)
    bm_add_qemu_example(multi_axis_sync
        FRAMEWORK_LIBS bm_core bm_hrt bm_ctrl_inst
    )
endif()
