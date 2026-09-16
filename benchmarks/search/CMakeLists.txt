if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: search")

themis_add_standard_benchmark(bench_search_release_gates bench_search_release_gates.cpp)
themis_add_standard_benchmark(bench_advanced_scenarios bench_advanced_scenarios.cpp)
themis_add_standard_benchmark(bench_distributed_merge_gates bench_distributed_merge_gates.cpp)
themis_add_standard_benchmark(bench_hybrid_fusion_gates bench_hybrid_fusion_gates.cpp)
themis_add_standard_benchmark(bench_layered_retrieval_phase5 bench_layered_retrieval_phase5.cpp)
themis_add_standard_benchmark(bench_vector_search_gates bench_vector_search_gates.cpp)

if(TARGET bench_layered_retrieval_phase5)
    themis_register_benchmark_ctest(
        NAME         "bench_search_layered_retrieval_phase5_smoke"
        TARGET       "bench_layered_retrieval_phase5"
        MODULE       "search"
        KIND         "wave_a"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_a_benchmark_search_index_tensor_graph_llm benchmark-baseline phase5
    )
endif()
