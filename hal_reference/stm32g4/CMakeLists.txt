add_library(bm_hal_stm32g4 STATIC
    bm_hal_critical_stm32g4.c
    bm_hal_memory_stm32g4.c
    bm_hal_timer_stm32g4.c
    bm_hal_uart_stm32g4.c
    bm_hal_wdg_stm32g4.c
    bm_hal_pwm_stm32g4.c
    bm_hal_adc_stm32g4.c
    bm_hal_comp_stm32g4.c
    bm_hal_encoder_stm32g4.c
    bm_sync_hal_stm32g4.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_hal_stm32g4 PUBLIC
    ${BM_FRAMEWORK_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(bm_hal_stm32g4 PUBLIC bm_config)
