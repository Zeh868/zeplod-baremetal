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
target_include_directories(bm_hal_native PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
