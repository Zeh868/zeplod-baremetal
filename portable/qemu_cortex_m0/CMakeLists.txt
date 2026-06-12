set(_BM_NATIVE_SIM_DIR "${CMAKE_CURRENT_LIST_DIR}/../native_sim")

add_library(bm_backend_qemu_cortex_m0 STATIC
    bm_drv_singleton_qemu.c
    ${_BM_NATIVE_SIM_DIR}/bm_drv_pwm_native.c
    ${_BM_NATIVE_SIM_DIR}/bm_drv_adc_native.c
    ${_BM_NATIVE_SIM_DIR}/bm_drv_comp_native.c
    ${_BM_NATIVE_SIM_DIR}/bm_drv_encoder_native.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_backend_qemu_cortex_m0 PUBLIC
    ${_BM_NATIVE_SIM_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(bm_backend_qemu_cortex_m0 PUBLIC BM_DRV_HAS_BACKEND)
target_link_libraries(bm_backend_qemu_cortex_m0 PUBLIC bm_hal bm_config)
