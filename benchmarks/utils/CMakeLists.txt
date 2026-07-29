if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: utils")

themis_add_standard_benchmark(bench_utils_release_gates bench_utils_release_gates.cpp)
