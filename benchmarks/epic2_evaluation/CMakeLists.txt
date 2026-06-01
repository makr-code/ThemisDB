# Documentation-first CMake scaffold for planned benchmarks.

set(EPIC2_EVALUATION_PLANNED_BENCHMARKS
    hardware_profile_bench.cc
    benchmark_matrix_bench.cc
    metrics_computation_bench.cc
    planner_decision_bench.cc
    artifact_staleness_bench.cc
    storage_strategy_bench.cc
)

# Benchmark targets will be enabled incrementally alongside implementation work.
