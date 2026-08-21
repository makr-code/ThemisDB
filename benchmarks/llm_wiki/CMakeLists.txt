if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: llm_wiki")

themis_add_standard_benchmark(bench_wikipedia_throughput bench_wikipedia_throughput.cpp)