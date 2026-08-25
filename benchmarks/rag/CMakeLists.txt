if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: rag")

themis_add_standard_benchmark(bench_delegate_evaluator bench_delegate_evaluator.cpp)
themis_add_standard_benchmark(bench_fts_phase_b bench_fts_phase_b.cpp)
