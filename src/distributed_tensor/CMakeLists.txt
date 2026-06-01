# EPIC 3 — Distributed Tensor Artifacts: skeleton headers and sources are in place.
# Enable library targets after interface review and acceptance-test sign-off.

set(DISTRIBUTED_TENSOR_PUBLIC_HEADERS
    include/tensor_artifact_classes.h
    include/artifact_manifest.h
    include/shard_placement.h
    include/integrity_verification.h
    include/recovery_manager.h
    include/distributed_planner.h
    include/tensor_infrastructure.h
)

set(DISTRIBUTED_TENSOR_SOURCES
    src/tensor_artifact_classes.cc
    src/artifact_manifest.cc
    src/shard_placement.cc
    src/integrity_verification.cc
    src/recovery_manager.cc
    src/distributed_planner.cc
    src/tensor_infrastructure.cc
)

# Uncomment when build targets are approved for activation:
# add_library(themis_distributed_tensor STATIC ${DISTRIBUTED_TENSOR_SOURCES})
# target_include_directories(themis_distributed_tensor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
