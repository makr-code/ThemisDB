if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: transaction")

themis_add_standard_benchmark(bench_branch_manager bench_branch_manager.cpp)
themis_add_standard_benchmark(bench_transaction_throughput bench_transaction_throughput.cpp)
