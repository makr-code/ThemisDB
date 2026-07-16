# Documentation-first CMake scaffold for planned benchmarks.

set(EPIC3_DISTRIBUTED_TENSOR_PLANNED_BENCHMARKS
    manifest_coordination_bench.cc
    placement_strategy_bench.cc
    integrity_verification_bench.cc
    recovery_rebuild_bench.cc
    distributed_retrieval_bench.cc
    infrastructure_bench.cc
)

# Benchmark targets will be enabled incrementally alongside implementation work.
