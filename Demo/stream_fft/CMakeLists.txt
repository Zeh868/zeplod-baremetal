# stream_fft：DMA 块流 + RFFT 频谱峰（M3 E1）
# @author zeh (china_qzh@163.com)  @version 1.0  @date 2026-06-13
cmake_minimum_required(VERSION 3.20)

option(NATIVE_SIM_BUILD "Build for host native_sim" OFF)

set(BM_ENABLE_ALGORITHM ON CACHE BOOL "" FORCE)

if(NATIVE_SIM_BUILD)
    project(stream_fft_native C)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/native_sim_example.cmake)
    file(GLOB FFT_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_native_sim_example(stream_fft
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec bm_algorithm
        EXTRA_SOURCES ${FFT_MODULES}
    )
else()
    project(stream_fft C ASM)
    set(EXAMPLE_ENABLE_MODULE ON)
    set(EXAMPLE_ENABLE_HRT ON)
    set(EXAMPLE_ENABLE_TICKER ON)
    set(EXAMPLE_ENABLE_EXEC ON)
    include(../cmake/qemu_example.cmake)
    file(GLOB FFT_MODULES CONFIGURE_DEPENDS modules/*.c)
    bm_add_qemu_example(stream_fft
        FRAMEWORK_LIBS bm_core bm_module bm_hrt bm_ticker bm_exec bm_algorithm
        EXTRA_SOURCES ${FFT_MODULES}
    )
endif()
