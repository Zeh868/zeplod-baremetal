add_library(bm_hal_native STATIC
    bm_hal_critical_native.c
    bm_hal_uart_native.c
    bm_hal_timer_native.c
    bm_hal_wdg_native.c
    bm_hal_memory_native.c
    bm_hal_pwm_sim.c
    bm_hal_adc_sim.c
    bm_hal_comp_sim.c
    bm_hal_encoder_sim.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_hal_native PUBLIC
    ${BM_FRAMEWORK_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
