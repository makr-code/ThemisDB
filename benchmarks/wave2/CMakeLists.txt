# benchmarks/wave2/CMakeLists.txt
# Wave 2 – Cross-Module Performance Benchmarks
#
# B2-A: Mixed-path benchmarks (ingest -> index -> query pipeline)
# B2-B: Concurrency/contention benchmarks (throughput + tail latency)
# B2-C: Recovery/warm-vs-cold path benchmarks
#
# All three targets use themis_add_standard_benchmark() from the parent scope,
# which links themis_core, Google Benchmark, RocksDB, OpenSSL, spdlog,
# nlohmann_json, and Threads.

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: wave2")

# B2-A: Mixed-path cross-module benchmarks
themis_add_standard_benchmark(bench_w2a_mixed_path bench_w2a_mixed_path.cpp)

# B2-B: Concurrency and contention benchmarks
themis_add_standard_benchmark(bench_w2b_concurrency bench_w2b_concurrency.cpp)

# B2-C: Recovery and warm-vs-cold path benchmarks
themis_add_standard_benchmark(bench_w2c_recovery bench_w2c_recovery.cpp)

message(STATUS "  Wave 2 benchmarks: mixed_path, concurrency, recovery")
