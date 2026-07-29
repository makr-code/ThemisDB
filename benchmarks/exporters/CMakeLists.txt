if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: exporters")

themis_add_standard_benchmark(bench_exporters              bench_exporters.cpp)
themis_add_standard_benchmark(bench_exporters_release_gates bench_exporters_release_gates.cpp)
