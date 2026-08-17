if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: access_model")

themis_add_standard_benchmark(bench_access_coordinator_gates  bench_access_coordinator_gates.cpp)
