if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: rag")

themis_add_standard_benchmark(bench_delegate_evaluator bench_delegate_evaluator.cpp)
themis_add_standard_benchmark(bench_fts_phase_b bench_fts_phase_b.cpp)

themis_register_benchmark_ctest(
    NAME       "bench_rag_fts_phase_b_smoke"
    TARGET     "bench_fts_phase_b"
    MODULE     "rag"
    TIMEOUT    180
    MIN_TIME   "0.01s"
    EXTRA_ARGS "--benchmark_filter=BM_Fts(Phrase|Proximity)Query/10000"
)
