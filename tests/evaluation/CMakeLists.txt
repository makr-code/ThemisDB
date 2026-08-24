cmake_minimum_required(VERSION 3.20)

# Evaluation module focused tests
# Tests cover: retrieval_metrics, ablation_framework, benchmark_matrix, artifact_lifecycle

file(GLOB EVALUATION_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS EVALUATION_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_evaluation_${_stem}_focused")
    set(_ctest  "${_stem}_EvaluationFocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    # Map each test to the evaluation source files it exercises
    if(_stem STREQUAL "test_evaluation_retrieval_metrics_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/evaluation/src/retrieval_metrics.cc
        )
    elseif(_stem STREQUAL "test_evaluation_ablation_framework_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/evaluation/src/retrieval_metrics.cc
            ${THEMIS_ROOT_DIR}/src/evaluation/src/ablation_framework.cc
        )
    elseif(_stem STREQUAL "test_evaluation_benchmark_matrix_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/evaluation/src/benchmark_matrix.cc
        )
    elseif(_stem STREQUAL "test_evaluation_artifact_lifecycle_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/evaluation/src/artifact_lifecycle.cc
        )
    endif()

    add_executable(${_target} "${_src}" ${_extra_sources})
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/src/evaluation/include
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )

    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(${_target} PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE evaluation
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS evaluation
    )
endforeach()

