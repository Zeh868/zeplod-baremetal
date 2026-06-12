if(ESP_PLATFORM)
    idf_component_register(
        SRCS "bm_drv_singleton_esp32.c"
        INCLUDE_DIRS "."
        REQUIRES driver esp_timer esp_hw_support esp_system
    )
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BM_DRV_HAS_BACKEND)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/bm_sdk_esp32_idf.cmake)

    add_library(bm_backend_sdk_esp32_idf STATIC
        bm_drv_singleton_esp32.c
    )
    get_filename_component(BM_FRAMEWORK_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    target_include_directories(bm_backend_sdk_esp32_idf PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_definitions(bm_backend_sdk_esp32_idf PUBLIC BM_DRV_HAS_BACKEND)
    target_link_libraries(bm_backend_sdk_esp32_idf PUBLIC bm_hal bm_config)
    bm_target_internal_includes(bm_backend_sdk_esp32_idf)
    bm_sdk_esp32_idf_apply(bm_backend_sdk_esp32_idf)

    add_library(bm_hal_esp32wroom32e ALIAS bm_backend_sdk_esp32_idf)
endif()
