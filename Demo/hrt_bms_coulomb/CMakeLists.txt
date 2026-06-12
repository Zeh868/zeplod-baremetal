cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

if(NATIVE_SIM_BUILD)
    project(hrt_bms_coulomb_native C)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_CTRL_INST ON)
    include(../cmake/native_sim_example.cmake)
    bm_add_native_sim_example(hrt_bms_coulomb
        FRAMEWORK_LIBS bm_core bm_hrt bm_ticker bm_ctrl_inst
    )
else()
    project(hrt_bms_coulomb C ASM)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_CTRL_INST ON)
    include(../cmake/qemu_example.cmake)
    bm_add_qemu_example(hrt_bms_coulomb
        FRAMEWORK_LIBS bm_core bm_hrt bm_ticker bm_ctrl_inst
    )
endif()
