set(ZEPLOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
    "Path to the zeplod-baremetal framework")

set(BM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BM_ENABLE_MODULE ${EXAMPLE_ENABLE_MODULE} CACHE BOOL "" FORCE)
set(BM_ENABLE_CHANNEL ${EXAMPLE_ENABLE_CHANNEL} CACHE BOOL "" FORCE)
set(BM_ENABLE_SHELL ${EXAMPLE_ENABLE_SHELL} CACHE BOOL "" FORCE)
set(BM_ENABLE_WDG ${EXAMPLE_ENABLE_WDG} CACHE BOOL "" FORCE)
set(BM_ENABLE_HRT ${EXAMPLE_ENABLE_HRT} CACHE BOOL "" FORCE)
set(BM_ENABLE_TICKER ${EXAMPLE_ENABLE_TICKER} CACHE BOOL "" FORCE)
set(BM_ENABLE_CTRL_INST ${EXAMPLE_ENABLE_CTRL_INST} CACHE BOOL "" FORCE)
set(BM_ENABLE_SYNC ${EXAMPLE_ENABLE_SYNC} CACHE BOOL "" FORCE)
if(EXAMPLE_ENABLE_SYNC)
    set(BM_SYNC_HAL_QEMU ON CACHE BOOL "" FORCE)
    set(BM_SYNC_HAL_NATIVE OFF CACHE BOOL "" FORCE)
else()
    set(BM_SYNC_HAL_QEMU OFF CACHE BOOL "" FORCE)
endif()
set(BM_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h" CACHE FILEPATH "" FORCE)

set(ZEPLOD_QEMU_SIM_HAL
    ${ZEPLOD_ROOT}/hal_reference/native_sim/bm_hal_pwm_sim.c
    ${ZEPLOD_ROOT}/hal_reference/native_sim/bm_hal_adc_sim.c
    ${ZEPLOD_ROOT}/hal_reference/native_sim/bm_hal_comp_sim.c
    ${ZEPLOD_ROOT}/hal_reference/native_sim/bm_hal_encoder_sim.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_memory_qemu.c
)

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
        "${ZEPLOD_ROOT}/examples/common/hybrid_print.c"
        "${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s"
        "${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/crt0_qemu.c"
        ${ZEPLOD_QEMU_SIM_HAL}
        ${EX_HAL_SOURCES}
        ${EX_EXTRA_SOURCES}
    )

    if(EX_FRAMEWORK_LIBS)
        target_sources(${TARGET}.elf PRIVATE
            "${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_critical_qemu.c"
        )
    endif()

    target_compile_definitions(${TARGET}.elf PRIVATE
        BM_EXAMPLE_QEMU=1
        BM_CONFIG_ENABLE_LOG=0
    )

    target_include_directories(${TARGET}.elf PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${ZEPLOD_ROOT}/examples/common"
        "${ZEPLOD_ROOT}/hal_reference/native_sim"
    )
    target_link_libraries(${TARGET}.elf PRIVATE bm_config ${EX_FRAMEWORK_LIBS})
    target_link_options(${TARGET}.elf PRIVATE
        "-T${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld"
        "-nostartfiles"
        "-Wl,--gc-sections"
        "--specs=rdimon.specs"
    )
endfunction()
