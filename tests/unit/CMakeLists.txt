function(bm_add_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE unity bm_core bm_hal_native)
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit/unity
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

bm_add_test(test_skeleton test_skeleton.c)
bm_add_test(test_ultra test_ultra.c)
bm_add_test(test_mempool test_mempool.c)
bm_add_test(test_event test_event.c)

if(BM_ENABLE_MODULE)
    bm_add_test(test_module test_module.c)
endif()
