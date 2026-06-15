if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: query")

themis_add_standard_benchmark(bench_query bench_query.cpp)
