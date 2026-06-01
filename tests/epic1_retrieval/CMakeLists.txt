# EPIC 1 Retrieval contract tests — placeholder only.
# Test source files (*.cc) are out of scope for this PR and will be added separately.

# Test sources will be listed here with the implementation PR:
# set(EPIC1_RETRIEVAL_TESTS ...)

# Wired when GTest and the retrieval scaffold sources are available.
# Uncomment and adapt the block below once the parent CMakeLists adds
# the retrieval library target (epic1_retrieval_lib).
#
# foreach(test_src IN LISTS EPIC1_RETRIEVAL_TESTS)
#     get_filename_component(test_name ${test_src} NAME_WE)
#     add_executable(${test_name} ${test_src})
#     target_link_libraries(${test_name} PRIVATE epic1_retrieval_lib GTest::gtest_main)
#     add_test(NAME ${test_name} COMMAND ${test_name})
# endforeach()

