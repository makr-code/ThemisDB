if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: performance")

themis_add_standard_benchmark(bench_performance_release_gates bench_performance_release_gates.cpp)
themis_add_standard_benchmark(bench_random_access_prefetch bench_random_access_prefetch.cpp)
