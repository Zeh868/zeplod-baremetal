add_library(bm_hal_native STATIC
    bm_hal_uart_native.c
    bm_hal_timer_native.c
    bm_hal_wdg_native.c
)
target_include_directories(bm_hal_native PUBLIC ${CMAKE_SOURCE_DIR}/include)
