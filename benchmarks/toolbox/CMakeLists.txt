if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: toolbox")

themis_add_standard_benchmark(bench_toolbox_release_gates bench_toolbox_release_gates.cpp)
