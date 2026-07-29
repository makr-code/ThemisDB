if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: updates")

themis_add_standard_benchmark(bench_updates_release_gates bench_updates_release_gates.cpp)
