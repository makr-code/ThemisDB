if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: query")

themis_add_standard_benchmark(bench_query bench_query.cpp)
themis_add_standard_benchmark(bench_phase4_performance bench_phase4_performance.cpp)
themis_add_standard_benchmark(bench_geospatial_phase2 bench_geospatial_phase2.cpp)
themis_add_standard_benchmark(bench_query_dedicated_gates bench_query_dedicated_gates.cpp)

if(TARGET bench_phase4_performance)
    themis_register_benchmark_ctest(
        NAME         "bench_query_phase4_performance_smoke"
        TARGET       "bench_phase4_performance"
        MODULE       "query"
        KIND         "wave_a"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_a_benchmark_server_query_storage_transaction benchmark-baseline phase4
    )
endif()

if(TARGET bench_query_dedicated_gates)
    themis_register_benchmark_ctest(
        NAME         "bench_query_dedicated_gates_wave_d"
        TARGET       "bench_query_dedicated_gates"
        MODULE       "query"
        KIND         "wave_d"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_d benchmark not_release_critical query
    )
endif()
