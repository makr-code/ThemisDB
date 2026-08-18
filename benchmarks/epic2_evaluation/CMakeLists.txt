if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: epic2_evaluation")

# ============================================================================
# planner_decision_bench — EPIC 2.5 query planner overhead benchmark
# Activated when epic2_evaluation_lib is available (requires THEMIS_BUILD_EPIC2).
# ============================================================================
if(TARGET epic2_evaluation_lib)
    add_executable(planner_decision_bench planner_decision_bench.cc)
    target_include_directories(planner_decision_bench PRIVATE
        ${THEMIS_ROOT_DIR}/src/evaluation
    )
    target_link_libraries(planner_decision_bench PRIVATE epic2_evaluation_lib)
    target_compile_features(planner_decision_bench PRIVATE cxx_std_17)
    
    # Phase 5: planner_error_path_bench for Category C error handling overhead
    add_executable(planner_error_path_bench planner_error_path_bench.cc)
    target_include_directories(planner_error_path_bench PRIVATE
        ${THEMIS_ROOT_DIR}/src/evaluation
    )
    target_link_libraries(planner_error_path_bench PRIVATE epic2_evaluation_lib)
    target_compile_features(planner_error_path_bench PRIVATE cxx_std_17)
endif()

# ============================================================================
# benchmark_matrix_bench — activated (Issue #5438)
# ============================================================================

if(NOT TARGET benchmark_matrix_bench)
    find_package(benchmark QUIET CONFIG)
    if(benchmark_FOUND AND TARGET themis_evaluation)
        add_executable(benchmark_matrix_bench
            benchmark_matrix_bench.cc
        )

        target_include_directories(benchmark_matrix_bench PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/evaluation/include
        )

        target_link_libraries(benchmark_matrix_bench PRIVATE
            themis_evaluation
            benchmark::benchmark
            benchmark::benchmark_main
        )

        target_compile_features(benchmark_matrix_bench PRIVATE cxx_std_17)

        target_compile_definitions(benchmark_matrix_bench PRIVATE
            THEMIS_BENCHMARK_BUILD=1
        )

        install(TARGETS benchmark_matrix_bench
            RUNTIME DESTINATION bin/benchmarks
            COMPONENT benchmarks
        )

        add_custom_target(run_benchmark_matrix
            COMMAND benchmark_matrix_bench --benchmark_min_time=0.1
            DEPENDS benchmark_matrix_bench
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running EPIC 2.2 benchmark matrix scenarios..."
        )

        message(STATUS "  benchmark_matrix_bench: HNSW/DiskANN/Tensor/Graph/LLM-LoRA matrix (Issue #5438)")
    else()
        message(STATUS "benchmark_matrix_bench: Google Benchmark or themis_evaluation not available – skipped")
    endif()
endif()

# storage_strategy_bench and other planned benchmarks use the standard macro
# when themis_distributed_tensor is available.
set(EPIC2_EVALUATION_PLANNED_BENCHMARKS
    storage_strategy_bench.cc
)

foreach(_src IN LISTS EPIC2_EVALUATION_PLANNED_BENCHMARKS)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "bench_epic2_evaluation_${_stem}")
    themis_add_standard_benchmark(${_target} "${_src}")
    if(TARGET ${_target})
        target_include_directories(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/benchmarks
        )
        if(TARGET themis_distributed_tensor)
            target_link_libraries(${_target} PRIVATE themis_distributed_tensor)
        endif()
    endif()
endforeach()
