# Documentation-first CMake scaffold for planned benchmarks.

set(EPIC2_EVALUATION_PLANNED_BENCHMARKS
    hardware_profile_bench.cc
    benchmark_matrix_bench.cc
    metrics_computation_bench.cc
    artifact_staleness_bench.cc
    storage_strategy_bench.cc
)

# planner_decision_bench — wired once epic2_evaluation_lib is available (Phase 7).
if(TARGET epic2_evaluation_lib)
    add_executable(planner_decision_bench planner_decision_bench.cc)
    target_include_directories(planner_decision_bench PRIVATE
        ${THEMIS_ROOT_DIR}/src/evaluation
    )
    target_link_libraries(planner_decision_bench PRIVATE epic2_evaluation_lib)
    target_compile_features(planner_decision_bench PRIVATE cxx_std_17)
endif()

# Benchmark targets will be enabled incrementally alongside implementation work.
