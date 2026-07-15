# Documentation-first CMake scaffold for planned benchmarks.
# Issue #5438: benchmark_matrix_bench (EPIC 2.2) activated.

set(EPIC2_EVALUATION_PLANNED_BENCHMARKS
    hardware_profile_bench.cc
    benchmark_matrix_bench.cc
    metrics_computation_bench.cc
    planner_decision_bench.cc
    artifact_staleness_bench.cc
    storage_strategy_bench.cc
)

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
