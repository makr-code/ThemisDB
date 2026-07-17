# EPIC 2 Evaluation contract tests.
# Covers benchmark_matrix (EPIC 2.2, Issue #5438) and hardware_profile.

# ============================================================================
# hardware_profile_test — registered with module-focused test framework
# ============================================================================

set(EPIC2_EVALUATION_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/hardware_profile_test.cc"
)

foreach(_src IN LISTS EPIC2_EVALUATION_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_epic2_evaluation_${_stem}_focused")
    set(_ctest "${_stem}_epic2_evaluation_FocusedTests")

    add_executable(${_target}
        "${_src}"
        "${THEMIS_ROOT_DIR}/src/evaluation/src/hardware_profile.cc"
    )
    target_include_directories(${_target} PRIVATE
        "${THEMIS_ROOT_DIR}/src/evaluation/include"
        "${THEMIS_ROOT_DIR}/src"
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE epic2_evaluation
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS evaluation epic2
    )
endforeach()

# ============================================================================
# benchmark_matrix_test — GTest coverage for BenchmarkMatrix (EPIC 2.2)
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
    else()
        message(STATUS "benchmark_matrix_test: GTest or themis_evaluation not available – skipped")
    endif()
endif()
