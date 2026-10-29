if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: importers")

themis_add_standard_benchmark(bench_importer_throughput bench_importer_throughput.cpp)
themis_add_standard_benchmark(bench_importers_release_gates bench_importers_release_gates.cpp)
