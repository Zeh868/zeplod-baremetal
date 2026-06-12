add_library(bm_backend_register_stm32g4 STATIC
    bm_drv_singleton_stm32g4.c
    bm_drv_pwm_stm32g4.c
    bm_drv_adc_stm32g4.c
    bm_drv_comp_stm32g4.c
    bm_drv_encoder_stm32g4.c
    bm_sync_hal_stm32g4.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_backend_register_stm32g4 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(bm_backend_register_stm32g4 PUBLIC
    BM_DRV_HAS_BACKEND
    BM_HAL_HAS_PRIORITY_MASK=1
)
target_link_libraries(bm_backend_register_stm32g4 PUBLIC bm_hal bm_config)

add_library(bm_hal_stm32g4 ALIAS bm_backend_register_stm32g4)
