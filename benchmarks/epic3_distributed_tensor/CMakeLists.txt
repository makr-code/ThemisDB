# Documentation-first CMake scaffold for planned benchmarks.

set(EPIC3_DISTRIBUTED_TENSOR_PLANNED_BENCHMARKS
    manifest_coordination_bench.cc
    placement_strategy_bench.cc
    integrity_verification_bench.cc
    recovery_rebuild_bench.cc
    distributed_retrieval_bench.cc
    infrastructure_bench.cc
)

# Add placement strategy benchmark (implementation complete)
if(ENABLE_BENCHMARKS AND benchmark_FOUND)
    add_executable(placement_strategy_bench placement_strategy_bench.cc)
    target_link_libraries(placement_strategy_bench PRIVATE
        themis_distributed_tensor
        benchmark::benchmark
    )
endif()

# Remaining benchmark targets will be enabled incrementally alongside implementation work.
