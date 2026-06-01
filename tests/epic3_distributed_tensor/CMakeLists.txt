# Documentation-first CMake scaffold for planned tests.

set(EPIC3_DISTRIBUTED_TENSOR_PLANNED_TESTS
    tensor_artifact_classes_test.cc
    artifact_manifest_test.cc
    shard_placement_test.cc
    integrity_verification_test.cc
    recovery_manager_test.cc
    distributed_planner_test.cc
    tensor_infrastructure_test.cc
)

# Test targets will be registered incrementally as implementation files land.
