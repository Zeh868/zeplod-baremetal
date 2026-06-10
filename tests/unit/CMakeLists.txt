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

if(BM_ENABLE_MODULE)
    bm_add_test(test_module test_module.c bm_module)
endif()

if(BM_ENABLE_CHANNEL)
    bm_add_test(test_channel test_channel.c bm_channel)
endif()

if(BM_ENABLE_SHELL)
    bm_add_test(test_shell test_shell.c bm_shell)
endif()
