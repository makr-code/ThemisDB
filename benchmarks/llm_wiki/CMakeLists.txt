if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: llm_wiki")

themis_add_standard_benchmark(bench_wikipedia_throughput bench_wikipedia_throughput.cpp)

# Wave D dedicated gates — LW-BM-01..04
# Labels: wave_d;benchmark;not_release_critical
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_llm_wiki_dedicated_gates.cpp")
    themis_add_standard_benchmark(bench_llm_wiki_dedicated_gates bench_llm_wiki_dedicated_gates.cpp)
endif()