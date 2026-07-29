if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: content")

themis_add_standard_benchmark(bench_content_processor_paths bench_content_processor_paths.cpp)
themis_add_standard_benchmark(bench_content_versioning bench_content_versioning.cpp)
themis_add_standard_benchmark(bench_content_release_gates bench_content_release_gates.cpp)
