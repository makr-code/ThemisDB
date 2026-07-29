if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: themis")

themis_add_standard_benchmark(bench_themis_core bench_themis_core.cpp)
themis_add_standard_benchmark(bench_themis_release_gates bench_themis_release_gates.cpp)
