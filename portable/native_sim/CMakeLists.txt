add_library(bm_backend_native_sim STATIC
    bm_drv_singleton_native.c
    bm_drv_pwm_native.c
    bm_drv_adc_native.c
    bm_drv_comp_native.c
    bm_drv_encoder_native.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_backend_native_sim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(bm_backend_native_sim PUBLIC BM_DRV_HAS_BACKEND)
target_link_libraries(bm_backend_native_sim PUBLIC bm_hal bm_config)
bm_target_internal_includes(bm_backend_native_sim)

add_library(bm_hal_native ALIAS bm_backend_native_sim)
