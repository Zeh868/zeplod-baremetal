# stream_vibration_diag：bm_stream + bm_pipeline RMS/Goertzel 振动诊断（M7 K2）
cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

set(BM_ENABLE_ALGORITHM ON CACHE BOOL "" FORCE)
set(BM_ENABLE_PIPELINE ON CACHE BOOL "" FORCE)
set(BM_ENABLE_STREAM ON CACHE BOOL "" FORCE)

if(NATIVE_SIM_BUILD)
    project(stream_vibration_diag_native C)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/native_sim_example.cmake)
    file(GLOB VIB_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_native_sim_example(stream_vibration_diag
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec bm_algorithm bm_pipeline
        EXTRA_SOURCES ${VIB_MODULES}
    )
else()
    project(stream_vibration_diag C ASM)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/qemu_example.cmake)
    file(GLOB VIB_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_qemu_example(stream_vibration_diag
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec bm_algorithm bm_pipeline
        EXTRA_SOURCES ${VIB_MODULES}
    )
endif()
