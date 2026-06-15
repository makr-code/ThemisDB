if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: analytics")

themis_add_standard_benchmark(bench_diff_engine bench_diff_engine.cpp)
