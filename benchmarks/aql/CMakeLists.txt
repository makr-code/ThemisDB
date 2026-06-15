if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: aql")

themis_add_standard_benchmark(bench_aql_functions bench_aql_functions.cpp)
