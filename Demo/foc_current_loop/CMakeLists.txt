# foc_current_loop：native_sim / QEMU 有感电流环示例
cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

set(BM_ENABLE_ALGORITHM ON CACHE BOOL "" FORCE)

if(NATIVE_SIM_BUILD)
    project(foc_current_loop_native C)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/native_sim_example.cmake)
    file(GLOB FOC_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_native_sim_example(foc_current_loop
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec bm_algorithm
        EXTRA_SOURCES ${FOC_MODULES}
    )
else()
    project(foc_current_loop C ASM)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/qemu_example.cmake)
    file(GLOB FOC_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_qemu_example(foc_current_loop
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec bm_algorithm
        EXTRA_SOURCES ${FOC_MODULES}
    )
endif()
