# stream_block_rms：bm_stream + bm_exec 块槽 + 块级 RMS（M3 E1）
cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

if(NATIVE_SIM_BUILD)
    project(stream_block_rms_native C)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/native_sim_example.cmake)
    file(GLOB STREAM_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_native_sim_example(stream_block_rms
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec
        EXTRA_SOURCES ${STREAM_MODULES}
    )
else()
    project(stream_block_rms C ASM)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/qemu_example.cmake)
    file(GLOB STREAM_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_qemu_example(stream_block_rms
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec
        EXTRA_SOURCES ${STREAM_MODULES}
    )
    target_link_libraries(stream_block_rms.elf PRIVATE m)
endif()
