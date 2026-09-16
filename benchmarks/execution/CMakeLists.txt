if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: execution")

# EX-BM-01..04: Execution engine dedicated Wave D benchmark gates
themis_add_standard_benchmark(bench_execution_dedicated_gates bench_execution_dedicated_gates.cpp)
