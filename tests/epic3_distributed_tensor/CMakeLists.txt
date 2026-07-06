# EPIC 3 Distributed Tensor contract tests

set(EPIC3_DISTRIBUTED_TENSOR_TESTS
    distributed_planner_test.cc
)

foreach(test_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TESTS)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} 
        PRIVATE themis_distributed_tensor
        PRIVATE themis_tensor
        PRIVATE themis_rag
        PRIVATE GTest::gtest_main
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
