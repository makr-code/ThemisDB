if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: transaction")

themis_add_standard_benchmark(bench_branch_manager bench_branch_manager.cpp)
themis_add_standard_benchmark(bench_transaction_throughput bench_transaction_throughput.cpp)
themis_add_standard_benchmark(bench_transaction_phase4 bench_transaction_phase4.cpp)

themis_register_benchmark_ctest(
    NAME         bench_transaction_phase4
    TARGET       bench_transaction_phase4
    MODULE       "transaction"
    TIMEOUT      300
    EXTRA_LABELS release_critical phase4 transaction benchmark-baseline wave_a_benchmark_server_query_storage_transaction
    MIN_TIME     0.2s
)

if(TARGET bench_transaction_throughput)
    themis_register_benchmark_ctest(
        NAME         "bench_transaction_throughput_wave_a_smoke"
        TARGET       "bench_transaction_throughput"
        MODULE       "transaction"
        KIND         "wave_a"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_a_benchmark_sharding_transaction benchmark-baseline throughput
    )
endif()

themis_add_standard_benchmark(bench_transaction_dedicated_gates bench_transaction_dedicated_gates.cpp)

if(TARGET bench_transaction_dedicated_gates)
    themis_register_benchmark_ctest(
        NAME         "bench_transaction_dedicated_gates_wave_d"
        TARGET       "bench_transaction_dedicated_gates"
        MODULE       "transaction"
        KIND         "wave_d"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_d benchmark not_release_critical transaction
    )
endif()
