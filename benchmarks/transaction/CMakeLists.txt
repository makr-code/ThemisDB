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
    EXTRA_LABELS release_critical phase4 transaction benchmark-baseline
    MIN_TIME     0.2s
)
