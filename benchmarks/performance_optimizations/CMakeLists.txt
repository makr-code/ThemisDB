# performance_optimizations benchmark module

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: performance_optimizations")

# Standard Google Benchmark sources in this module.
themis_add_standard_benchmark(benchmark_huge_pages benchmark_huge_pages.cpp)
themis_add_standard_benchmark(benchmark_lirs_cache benchmark_lirs_cache.cpp)
themis_add_standard_benchmark(benchmark_mimalloc benchmark_mimalloc.cpp)
themis_add_standard_benchmark(benchmark_rcu_index benchmark_rcu_index.cpp)

# Uses llm::GPUSafeFailManager and should follow LLM feature-gating.
if(THEMIS_ENABLE_LLM)
    themis_add_standard_benchmark(benchmark_safe_fail benchmark_safe_fail.cpp)
else()
    message(STATUS "Skipping benchmark_safe_fail (THEMIS_ENABLE_LLM=OFF)")
endif()

# Intentionally excluded:
# - phase2/benchmark_phase2.cpp is currently a disabled placeholder benchmark
#   ("pending implementation sync") and is not part of default benchmark builds.
