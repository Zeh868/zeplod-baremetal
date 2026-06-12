add_library(bm_backend_register_ch32v003 STATIC
    bm_drv_singleton_ch32v003.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_backend_register_ch32v003 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(bm_backend_register_ch32v003 PUBLIC BM_DRV_HAS_BACKEND)
target_link_libraries(bm_backend_register_ch32v003 PUBLIC bm_hal bm_config)
bm_target_internal_includes(bm_backend_register_ch32v003)

add_library(bm_hal_ch32v003 ALIAS bm_backend_register_ch32v003)
