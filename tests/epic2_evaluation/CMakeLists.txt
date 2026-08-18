# EPIC 2 evaluation tests — query planner (EPIC 2.5, Phases 2-4),
# approximation governance rules (EPIC 2.4, Issue #5440), and
# EPIC 2.2 hardware_profile + benchmark_matrix tests.
# See docs/EPIC2_QUERY_PLANNER.md and tests/epic2_evaluation/README.md.

# ============================================================================
# query_planner_test — EPIC 2.5 query planner GTest suite
# approximation_rules_test — EPIC 2.4 approximation governance GTest suite
# ============================================================================

set(EPIC2_EVALUATION_TESTS
    query_planner_test.cc
    approximation_rules_test.cc
)

# Wire tests when the epic2_evaluation_lib target and GTest are available.
if(TARGET epic2_evaluation_lib AND TARGET GTest::gtest_main)
    foreach(test_src IN LISTS EPIC2_EVALUATION_TESTS)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${THEMIS_ROOT_DIR}/src/evaluation
        )
        # Base planner library; approximation test also needs approximation lib.
        if(test_src STREQUAL "approximation_rules_test.cc" AND
           TARGET epic2_approximation_lib)
            target_link_libraries(${test_name} PRIVATE
                epic2_approximation_lib
                GTest::gtest_main
            )
        else()
            target_link_libraries(${test_name} PRIVATE
                epic2_evaluation_lib
                GTest::gtest_main
            )
        endif()
        target_compile_features(${test_name} PRIVATE cxx_std_17)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

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

# ============================================================================
# retrieval_metrics_test — GTest coverage for retrieval metrics (Issue #5439)
# ============================================================================

if(NOT TARGET retrieval_metrics_test)
    find_package(GTest QUIET)
    if(GTest_FOUND AND TARGET themis_evaluation)
        add_executable(retrieval_metrics_test
            retrieval_metrics_test.cc
        )

        target_include_directories(retrieval_metrics_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/evaluation
        )

        target_link_libraries(retrieval_metrics_test PRIVATE
            themis_evaluation
            GTest::gtest_main
        )

        target_compile_features(retrieval_metrics_test PRIVATE cxx_std_17)

        add_test(
            NAME retrieval_metrics_test
            COMMAND retrieval_metrics_test
        )
        set_tests_properties(retrieval_metrics_test PROPERTIES
            LABELS "evaluation;epic2;metrics"
        )
    else()
        message(STATUS "retrieval_metrics_test: GTest or themis_evaluation not available – skipped")
    endif()
endif()

# ============================================================================
# artifact_lifecycle_test — GTest coverage for artifact lifecycle (EPIC 2.6)
# ============================================================================

if(NOT TARGET artifact_lifecycle_test)
    find_package(GTest QUIET)
    if(GTest_FOUND AND TARGET themis_evaluation)
        add_executable(artifact_lifecycle_test
            artifact_lifecycle_test.cc
        )

        target_include_directories(artifact_lifecycle_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/evaluation
        )

        target_link_libraries(artifact_lifecycle_test PRIVATE
            themis_evaluation
            GTest::gtest_main
        )

        target_compile_features(artifact_lifecycle_test PRIVATE cxx_std_17)

        add_test(
            NAME artifact_lifecycle_test
            COMMAND artifact_lifecycle_test
        )
        set_tests_properties(artifact_lifecycle_test PROPERTIES
            LABELS "evaluation;epic2;lifecycle"
        )
    else()
        message(STATUS "artifact_lifecycle_test: GTest or themis_evaluation not available – skipped")
    endif()
endif()

# ============================================================================
# ablation_framework_test — GTest coverage for ablation framework (Issue #5439)
# ============================================================================

if(NOT TARGET ablation_framework_test)
    find_package(GTest QUIET)
    if(GTest_FOUND AND TARGET themis_evaluation)
        add_executable(ablation_framework_test
            ablation_framework_test.cc
        )

        target_include_directories(ablation_framework_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/evaluation
        )

        target_link_libraries(ablation_framework_test PRIVATE
            themis_evaluation
            GTest::gtest_main
        )

        target_compile_features(ablation_framework_test PRIVATE cxx_std_17)

        add_test(
            NAME ablation_framework_test
            COMMAND ablation_framework_test
        )
        set_tests_properties(ablation_framework_test PROPERTIES
            LABELS "evaluation;epic2;ablation"
        )
    else()
        message(STATUS "ablation_framework_test: GTest or themis_evaluation not available – skipped")
    endif()
endif()

# ============================================================================
# test_query_planner_cache — Phase 3 P3-01: Query Optimizer Hardening (28 tests)
# ============================================================================

if(NOT TARGET test_query_planner_cache)
    find_package(GTest QUIET)
    if(GTest_FOUND AND TARGET themis_core)
        add_executable(test_query_planner_cache
            test_query_planner_cache.cc
        )

        target_include_directories(test_query_planner_cache PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/evaluation
        )

        target_link_libraries(test_query_planner_cache PRIVATE
            ${TEST_LIBS}
            themis_core
            GTest::gtest_main
            spdlog::spdlog
            Threads::Threads
            OpenSSL::Crypto
        )

        target_compile_features(test_query_planner_cache PRIVATE cxx_std_17)
        target_compile_definitions(test_query_planner_cache PRIVATE THEMIS_TEST_BUILD=1)

        add_test(
            NAME test_query_planner_cache
            COMMAND test_query_planner_cache
        )
        set_tests_properties(test_query_planner_cache PROPERTIES
            LABELS "phase3;p3-01;query_optimization;evaluation"
        )
    else()
        message(STATUS "test_query_planner_cache: GTest or themis_core not available – skipped")
    endif()
endif()
