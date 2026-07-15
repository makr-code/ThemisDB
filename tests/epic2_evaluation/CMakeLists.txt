# EPIC 2 Evaluation contract tests.
# Issue #5438: benchmark_matrix_test (EPIC 2.2) implemented.

# ============================================================================
# benchmark_matrix_test
# ============================================================================

if(NOT TARGET benchmark_matrix_test)
    find_package(GTest QUIET)
    if(GTest_FOUND AND TARGET themis_evaluation)
        add_executable(benchmark_matrix_test
            benchmark_matrix_test.cc
        )

        target_include_directories(benchmark_matrix_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/evaluation/include
        )

        target_link_libraries(benchmark_matrix_test PRIVATE
            themis_evaluation
            GTest::gtest_main
        )

        target_compile_features(benchmark_matrix_test PRIVATE cxx_std_17)

        add_test(
            NAME benchmark_matrix_test
            COMMAND benchmark_matrix_test
        )

        install(TARGETS benchmark_matrix_test
            RUNTIME DESTINATION bin/tests
            COMPONENT tests
        )
    else()
        message(STATUS "benchmark_matrix_test: GTest or themis_evaluation not available – skipped")
    endif()
endif()

