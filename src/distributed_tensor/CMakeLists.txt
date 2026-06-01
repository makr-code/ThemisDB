# Documentation-first CMake scaffold for planned epic work.

set(DISTRIBUTED_TENSOR_PLANNED_PUBLIC_HEADERS
    include/tensor_artifact_classes.h
    include/artifact_manifest.h
    include/shard_placement.h
    include/integrity_verification.h
    include/recovery_manager.h
    include/distributed_planner.h
    include/tensor_infrastructure.h
)

set(DISTRIBUTED_TENSOR_PLANNED_SOURCES
    src/tensor_artifact_classes.cc
    src/artifact_manifest.cc
    src/shard_placement.cc
    src/integrity_verification.cc
    src/recovery_manager.cc
    src/distributed_planner.cc
    src/tensor_infrastructure.cc
)

# Targets intentionally remain disabled until interface review and implementation approval are complete.
