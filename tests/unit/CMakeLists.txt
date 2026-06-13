function(bm_add_test TEST_NAME TEST_SRC TEST_LIBRARY)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE
        unity
        ${TEST_LIBRARY}
        bm_hal_native
    )
    bm_target_internal_includes(${TEST_NAME})
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

bm_add_test(test_skeleton test_skeleton.c bm_core)
bm_add_test(test_critical test_critical.c bm_core)
bm_add_test(test_log test_log.c bm_core)
bm_add_test(test_ultra test_ultra.c bm_core)
target_compile_definitions(test_ultra PRIVATE BM_ENABLE_ULTRA_TEST_HOOK=1)
bm_add_test(test_mempool test_mempool.c bm_core)
bm_add_test(test_event test_event.c bm_core)
target_compile_definitions(test_event PRIVATE BM_ENABLE_EVENT_TEST_HOOK=1)
bm_add_test(test_wdg test_wdg.c bm_wdg)
target_include_directories(test_wdg PRIVATE
    ${CMAKE_SOURCE_DIR}/portable/native_sim)

if(BM_ENABLE_MODULE)
    bm_add_test(test_module test_module.c bm_module)
endif()

if(BM_ENABLE_CHANNEL)
    bm_add_test(test_channel test_channel.c bm_channel)
endif()

if(BM_ENABLE_SHELL)
    bm_add_test(test_shell test_shell.c bm_shell)
endif()

if(BM_ENABLE_HRT)
    bm_add_test(test_hrt test_hrt.c bm_hrt)
    target_include_directories(test_hrt PRIVATE
        ${CMAKE_SOURCE_DIR}/portable/native_sim)
endif()

if(BM_ENABLE_TICKER)
    bm_add_test(test_ticker test_ticker.c bm_ticker)
    target_include_directories(test_ticker PRIVATE
        ${CMAKE_SOURCE_DIR}/portable/native_sim)
endif()

if(TARGET bm_resource)
    bm_add_test(test_resource_claim test_resource_claim.c bm_resource)
endif()

if(BM_ENABLE_EXEC)
    bm_add_test(test_exec test_exec.c bm_exec)
    target_include_directories(test_exec PRIVATE
        ${CMAKE_SOURCE_DIR}/portable/native_sim)
endif()

if(TARGET bm_stream)
    bm_add_test(test_stream test_stream.c bm_stream)
elseif(BM_ENABLE_EXEC)
    bm_add_test(test_stream test_stream.c bm_exec)
endif()

bm_add_test(test_hal_dma_stream test_hal_dma_stream.c bm_core)
target_include_directories(test_hal_dma_stream PRIVATE
    ${CMAKE_SOURCE_DIR}/portable/native_sim)

if(BM_ENABLE_ALGORITHM)
    bm_add_test(test_algorithm test_algorithm.c bm_algorithm)
    if(NOT MSVC)
        target_link_libraries(test_algorithm PRIVATE m)
    endif()
endif()

if(TARGET bm_pipeline)
    bm_add_test(test_pipeline test_pipeline.c bm_pipeline)
endif()

if(TARGET bm_component_motor)
    bm_add_test(test_motor_foc_sensored
        test_motor_foc_sensored.c
        bm_component_motor)
    target_link_libraries(test_motor_foc_sensored PRIVATE bm_core)
    target_include_directories(test_motor_foc_sensored PRIVATE
        ${CMAKE_SOURCE_DIR}/portable/native_sim)
endif()

if(BM_ENABLE_SYNC)
    add_executable(test_sync test_sync.c)
    target_link_libraries(test_sync PRIVATE unity bm_exec bm_hal_native)
    bm_target_internal_includes(test_sync)
    target_include_directories(test_sync PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
        ${CMAKE_SOURCE_DIR}/portable/native_sim)
    add_test(NAME test_sync COMMAND test_sync)
endif()

if(BM_BUILD_TESTS)
    add_executable(test_snapshot test_snapshot.c)
    target_link_libraries(test_snapshot PRIVATE unity bm_core bm_hal_native)
    bm_target_internal_includes(test_snapshot)
    target_include_directories(test_snapshot PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
        ${CMAKE_SOURCE_DIR}/portable/native_sim)
    add_test(NAME test_snapshot COMMAND test_snapshot)

    add_executable(test_priority_mask
        test_priority_mask.c
        fakes/bm_hal_priority_mask_fake.c
    )
    target_compile_definitions(test_priority_mask PRIVATE
        BM_CONFIG_ENABLE_PRIORITY_MASK=1
        BM_HAL_HAS_PRIORITY_MASK=1
    )
    target_link_libraries(test_priority_mask PRIVATE unity bm_config)
    bm_target_internal_includes(test_priority_mask)
    target_include_directories(test_priority_mask PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
        ${CMAKE_SOURCE_DIR}/tests/unit/fakes)
    add_test(NAME test_priority_mask COMMAND test_priority_mask)
endif()
