if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: updates")

themis_add_standard_benchmark(bench_updates_release_gates bench_updates_release_gates.cpp)

# Block 3: Benchmark Depth Expansion (v1.1.0 GA)
themis_add_standard_benchmark(bench_updates_coordinated_hardening bench_updates_coordinated_hardening.cpp)
themis_add_standard_benchmark(bench_updates_canary_resilience bench_updates_canary_resilience.cpp)
themis_add_standard_benchmark(bench_updates_schema_diversity bench_updates_schema_diversity.cpp)
themis_add_standard_benchmark(bench_updates_long_run_stability bench_updates_long_run_stability.cpp)

# Phase 5: Cluster stress benchmarks (Q4 2026)
themis_add_standard_benchmark(bench_updates_cluster_stress_q4 bench_updates_cluster_stress_q4.cpp)
