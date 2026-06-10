add_library(bm_hal_ch32v003 STATIC
    bm_hal_critical_ch32v003.c
    bm_hal_memory_ch32v003.c
    bm_hal_timer_ch32v003.c
    bm_hal_uart_ch32v003.c
    bm_hal_wdg_ch32v003.c
)
get_filename_component(BM_FRAMEWORK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
target_include_directories(bm_hal_ch32v003 PUBLIC
    ${BM_FRAMEWORK_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(bm_hal_ch32v003 PUBLIC bm_config)
