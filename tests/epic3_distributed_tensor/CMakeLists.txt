# EPIC 3 Distributed Tensor contract tests
# Test source files provide comprehensive contract validation and lifecycle tests.

# Define test sources
set(EPIC3_DISTRIBUTED_TENSOR_TESTS
    tensor_artifact_classes_test.cc
    artifact_manifest_test.cc
)

# Create test executables for each test file
foreach(test_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TESTS)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE themis_distributed_tensor GTest::gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()


