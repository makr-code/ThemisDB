# EPIC 2 evaluation tests — query planner (EPIC 2.5, Phases 2-4).
# See docs/EPIC2_QUERY_PLANNER.md and tests/epic2_evaluation/README.md.
#
# Test source files for other EPIC 2 sub-issues (*.cc) are out of scope for
# this PR and will be added separately.

set(EPIC2_EVALUATION_TESTS
    query_planner_test.cc
)

# Wire tests when the epic2_evaluation_lib target and GTest are available.
if(TARGET epic2_evaluation_lib AND TARGET GTest::gtest_main)
    foreach(test_src IN LISTS EPIC2_EVALUATION_TESTS)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${THEMIS_ROOT_DIR}/src/evaluation
        )
        target_link_libraries(${test_name} PRIVATE
            epic2_evaluation_lib
            GTest::gtest_main
        )
        target_compile_features(${test_name} PRIVATE cxx_std_17)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

