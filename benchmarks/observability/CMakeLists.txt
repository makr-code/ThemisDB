if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: observability")

themis_add_standard_benchmark(bench_observability_goals         bench_observability_goals.cpp)
themis_add_standard_benchmark(bench_observability_release_gates bench_observability_release_gates.cpp)
themis_add_standard_benchmark(bench_observability_block_a_gates bench_observability_block_a_gates.cpp)
themis_add_standard_benchmark(bench_observability_block_b_gates bench_observability_block_b_gates.cpp)
