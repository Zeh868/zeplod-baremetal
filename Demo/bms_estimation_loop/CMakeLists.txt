# bms_estimation_loop：BMS SOC 估算组件 + 仿真 plant
cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

set(BM_ENABLE_ALGORITHM ON CACHE BOOL "" FORCE)
set(BM_ENABLE_COMPONENT_BMS ON CACHE BOOL "" FORCE)

if(NATIVE_SIM_BUILD)
    project(bms_estimation_loop_native C)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/native_sim_example.cmake)
    file(GLOB BMS_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_native_sim_example(bms_estimation_loop
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec
                       bm_algorithm bm_component_bms
        EXTRA_SOURCES ${BMS_MODULES}
    )
else()
    project(bms_estimation_loop C ASM)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/qemu_example.cmake)
    file(GLOB BMS_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_qemu_example(bms_estimation_loop
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec
                       bm_algorithm bm_component_bms
        EXTRA_SOURCES ${BMS_MODULES}
    )
endif()
