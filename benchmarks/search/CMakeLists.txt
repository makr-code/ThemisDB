if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: search")

themis_add_standard_benchmark(bench_search_release_gates bench_search_release_gates.cpp)
