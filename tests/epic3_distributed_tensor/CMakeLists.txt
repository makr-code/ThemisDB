# EPIC 3 Distributed Tensor contract tests — placeholder only.
# Test source files (*.cc) are out of scope for this PR and will be added separately.

# Test sources will be listed here with the implementation PR:
# set(EPIC3_DISTRIBUTED_TENSOR_TESTS ...)

# Wired when GTest and the distributed_tensor scaffold sources are available.
# Uncomment and adapt the block below once the parent CMakeLists adds
# the distributed_tensor library target (epic3_distributed_tensor_lib).
#
# foreach(test_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TESTS)
#     get_filename_component(test_name ${test_src} NAME_WE)
#     add_executable(${test_name} ${test_src})
#     target_link_libraries(${test_name} PRIVATE epic3_distributed_tensor_lib GTest::gtest_main)
#     add_test(NAME ${test_name} COMMAND ${test_name})
# endforeach()

