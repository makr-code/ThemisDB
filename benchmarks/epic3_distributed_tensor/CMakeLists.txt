if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: epic3_distributed_tensor")

set(EPIC3_DISTRIBUTED_TENSOR_PLANNED_BENCHMARKS
    manifest_coordination_bench.cc
    placement_strategy_bench.cc
    integrity_verification_bench.cc
    recovery_rebuild_bench.cc
    distributed_retrieval_bench.cc
    infrastructure_bench.cc
)

set(EPIC3_DISTRIBUTED_TENSOR_BENCHMARK_METADATA
    README.md
    phase5_workload_profiles.json
    release_gate_manifest_epic3.json
    report_variance_epic3.py
    RUNBOOK_EPIC3_PHASE5.md
    REPRO_TRIAGE_EPIC3_PHASE5.md
)

# Runtime benchmark executables remain blocked on the distributed_tensor source PR.
# Until those sources exist, register the metadata scaffold as an IDE-visible target
# so benchmark governance files are versioned and reviewable in normal build flows.
if(NOT TARGET epic3_distributed_tensor_benchmark_scaffold)
    add_custom_target(epic3_distributed_tensor_benchmark_scaffold
        SOURCES
            ${EPIC3_DISTRIBUTED_TENSOR_BENCHMARK_METADATA}
    )
    set_target_properties(epic3_distributed_tensor_benchmark_scaffold PROPERTIES
        FOLDER "benchmarks/epic3_distributed_tensor"
    )
endif()
