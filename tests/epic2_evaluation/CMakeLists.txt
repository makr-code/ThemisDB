# EPIC 2 Evaluation contract tests — placeholder only.
# Test source files (*.cc) are out of scope for this PR and will be added separately.

# Test sources will be listed here with the implementation PR:
# set(EPIC2_EVALUATION_TESTS ...)

# Wired when GTest and the evaluation scaffold sources are available.
# Uncomment and adapt the block below once the parent CMakeLists adds
# the evaluation library target (epic2_evaluation_lib).
#
# foreach(test_src IN LISTS EPIC2_EVALUATION_TESTS)
#     get_filename_component(test_name ${test_src} NAME_WE)
#     add_executable(${test_name} ${test_src})
#     target_link_libraries(${test_name} PRIVATE epic2_evaluation_lib GTest::gtest_main)
#     add_test(NAME ${test_name} COMMAND ${test_name})
# endforeach()

