if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: governance")

themis_add_standard_benchmark(bench_governance_policy_latency bench_governance_policy_latency.cpp)
themis_add_standard_benchmark(bench_governance_release_gates bench_governance_release_gates.cpp)
