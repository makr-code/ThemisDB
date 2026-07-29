if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: metadata")

themis_add_standard_benchmark(bench_metadata_cache bench_metadata_cache.cpp)
themis_add_standard_benchmark(bench_metadata_release_gates bench_metadata_release_gates.cpp)
