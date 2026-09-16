if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: vector_search")

themis_add_standard_benchmark(
    bench_vector_search_dedicated_gates
    bench_vector_search_dedicated_gates.cpp
)

if(TARGET bench_vector_search_dedicated_gates)
    themis_register_benchmark_ctest(
        NAME         "bench_vector_search_dedicated_gates_smoke"
        TARGET       "bench_vector_search_dedicated_gates"
        MODULE       "vector_search"
        KIND         "wave_d"
        TIMEOUT      120
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_d benchmark-baseline vector_search p95_gates
    )
endif()
