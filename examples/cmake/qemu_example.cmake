set(ZEPLOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
    "Path to the zeplod-baremetal framework")

set(BM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BM_ENABLE_MODULE ${EXAMPLE_ENABLE_MODULE} CACHE BOOL "" FORCE)
set(BM_ENABLE_CHANNEL ${EXAMPLE_ENABLE_CHANNEL} CACHE BOOL "" FORCE)
set(BM_ENABLE_SHELL ${EXAMPLE_ENABLE_SHELL} CACHE BOOL "" FORCE)
set(BM_ENABLE_WDG ${EXAMPLE_ENABLE_WDG} CACHE BOOL "" FORCE)
set(BM_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h" CACHE FILEPATH "" FORCE)

add_subdirectory("${ZEPLOD_ROOT}" zeplod EXCLUDE_FROM_ALL)

function(bm_add_qemu_example TARGET)
    set(options)
    set(one_value_args)
    set(multi_value_args FRAMEWORK_LIBS HAL_SOURCES EXTRA_SOURCES)
    cmake_parse_arguments(EX "${options}" "${one_value_args}"
        "${multi_value_args}" ${ARGN})

    add_executable(${TARGET}.elf
        main.c
        "${ZEPLOD_ROOT}/examples/common/example_support.c"
        "${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s"
        "${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/crt0_qemu.c"
        ${EX_HAL_SOURCES}
        ${EX_EXTRA_SOURCES}
    )

    if(EX_FRAMEWORK_LIBS)
        target_sources(${TARGET}.elf PRIVATE
            "${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_critical_qemu.c"
        )
    endif()

    target_include_directories(${TARGET}.elf PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${ZEPLOD_ROOT}/examples/common"
    )
    target_link_libraries(${TARGET}.elf PRIVATE bm_config ${EX_FRAMEWORK_LIBS})
    target_link_options(${TARGET}.elf PRIVATE
        "-T${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld"
        "-nostartfiles"
        "-Wl,--gc-sections"
    )
endfunction()
