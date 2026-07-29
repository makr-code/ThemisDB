if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: importers")

themis_add_standard_benchmark(bench_importers_release_gates bench_importers_release_gates.cpp)
