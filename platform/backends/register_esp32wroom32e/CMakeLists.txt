add_library(bm_backend_register_esp32wroom32e STATIC
    bm_drv_singleton_esp32.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
target_include_directories(bm_backend_register_esp32wroom32e PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(bm_backend_register_esp32wroom32e PUBLIC BM_DRV_HAS_BACKEND)
target_link_libraries(bm_backend_register_esp32wroom32e PUBLIC bm_hal bm_config)

add_library(bm_hal_esp32wroom32e ALIAS bm_backend_register_esp32wroom32e)
