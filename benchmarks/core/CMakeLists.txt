if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: core")

themis_add_standard_benchmark(bench_core_performance bench_core_performance.cpp)
