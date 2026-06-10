function(bm_add_test TEST_NAME TEST_SRC TEST_LIBRARY)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE
        unity
        ${TEST_LIBRARY}
        bm_hal_native
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

bm_add_test(test_skeleton test_skeleton.c bm_core)
bm_add_test(test_ultra test_ultra.c bm_core)
bm_add_test(test_mempool test_mempool.c bm_core)
bm_add_test(test_event test_event.c bm_core)
bm_add_test(test_wdg test_wdg.c bm_wdg)
target_include_directories(test_wdg PRIVATE
    ${CMAKE_SOURCE_DIR}/hal_reference/native_sim)

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
        ${CMAKE_SOURCE_DIR}/hal_reference/native_sim)
endif()

if(BM_ENABLE_TICKER)
    bm_add_test(test_ticker test_ticker.c bm_ticker)
    target_include_directories(test_ticker PRIVATE
        ${CMAKE_SOURCE_DIR}/hal_reference/native_sim)
endif()

if(TARGET bm_resource)
    bm_add_test(test_resource_claim test_resource_claim.c bm_resource)
endif()

if(BM_ENABLE_CTRL_INST)
    bm_add_test(test_ctrl_inst test_ctrl_inst.c bm_ctrl_inst)
    target_include_directories(test_ctrl_inst PRIVATE
        ${CMAKE_SOURCE_DIR}/hal_reference/native_sim)
endif()

if(BM_ENABLE_SYNC)
    add_executable(test_sync test_sync.c)
    target_link_libraries(test_sync PRIVATE unity bm_ctrl_inst)
    target_include_directories(test_sync PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
        ${CMAKE_SOURCE_DIR}/hal_reference/native_sim)
    add_test(NAME test_sync COMMAND test_sync)
endif()

if(BM_BUILD_TESTS)
    add_executable(test_snapshot test_snapshot.c)
    target_link_libraries(test_snapshot PRIVATE unity bm_hal_native)
    target_include_directories(test_snapshot PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
        ${CMAKE_SOURCE_DIR}/hal_reference/native_sim)
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
    target_include_directories(test_priority_mask PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
        ${CMAKE_SOURCE_DIR}/tests/unit/fakes)
    add_test(NAME test_priority_mask COMMAND test_priority_mask)
endif()
