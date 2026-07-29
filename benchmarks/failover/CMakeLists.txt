if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: failover")

themis_add_standard_benchmark(bench_failover_release_gates bench_failover_release_gates.cpp)
