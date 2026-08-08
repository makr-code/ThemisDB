if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: aql")

themis_add_standard_benchmark(bench_aql_functions bench_aql_functions.cpp)
themis_add_standard_benchmark(bench_aql_mutations bench_aql_mutations.cpp)

# Phase 6: AQL translation + validation pipeline benchmarks (self-contained mock, no LLM infra)
themis_add_standard_benchmark(bench_aql_translation   bench_aql_translation.cpp)
themis_add_standard_benchmark(bench_aql_helper_paths  bench_aql_helper_paths.cpp)

# Phase 6: Consolidated performance gate verification (AG-4, AG-5, AG-6)
themis_add_standard_benchmark(bench_aql_assistance_gates  bench_aql_assistance_gates.cpp)
