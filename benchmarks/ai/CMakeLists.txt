if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: ai")

themis_add_standard_benchmark(bench_ai_plugin_generator bench_ai_plugin_generator.cpp)
