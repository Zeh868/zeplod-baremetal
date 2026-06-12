set(ZEPLOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
    "Path to the zeplod-baremetal framework")

set(BM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BM_ENABLE_MODULE ${EXAMPLE_ENABLE_MODULE} CACHE BOOL "" FORCE)
set(BM_ENABLE_CHANNEL OFF CACHE BOOL "" FORCE)
set(BM_ENABLE_SHELL OFF CACHE BOOL "" FORCE)
set(BM_ENABLE_WDG OFF CACHE BOOL "" FORCE)
set(BM_ENABLE_HRT ${EXAMPLE_ENABLE_HRT} CACHE BOOL "" FORCE)
set(BM_ENABLE_TICKER ${EXAMPLE_ENABLE_TICKER} CACHE BOOL "" FORCE)
set(BM_ENABLE_CTRL_INST ${EXAMPLE_ENABLE_CTRL_INST} CACHE BOOL "" FORCE)
set(BM_ENABLE_SYNC ${EXAMPLE_ENABLE_SYNC} CACHE BOOL "" FORCE)
set(BM_SYNC_HAL_NATIVE ${EXAMPLE_ENABLE_SYNC} CACHE BOOL "" FORCE)
set(BM_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h" CACHE FILEPATH "" FORCE)
set(BM_BACKEND "native_sim" CACHE STRING "" FORCE)

add_subdirectory("${ZEPLOD_ROOT}" zeplod EXCLUDE_FROM_ALL)

function(bm_add_native_sim_example TARGET)
    set(options)
    set(one_value_args)
    set(multi_value_args FRAMEWORK_LIBS EXTRA_SOURCES)
    cmake_parse_arguments(EX "${options}" "${one_value_args}"
        "${multi_value_args}" ${ARGN})

    add_executable(${TARGET}
        main.c
        "${ZEPLOD_ROOT}/Demo/common/example_support.c"
        "${ZEPLOD_ROOT}/Demo/common/hybrid_print.c"
        ${EX_EXTRA_SOURCES}
    )
    target_compile_definitions(${TARGET} PRIVATE NATIVE_SIM)

    target_include_directories(${TARGET} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${ZEPLOD_ROOT}/Demo/common"
        "${ZEPLOD_ROOT}/portable/native_sim"
    )
    target_link_libraries(${TARGET} PRIVATE
        bm_config
        bm_hal_native
        ${EX_FRAMEWORK_LIBS}
    )
endfunction()
