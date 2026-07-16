# EPIC 3 — Distributed Tensor Artifacts: Implementation scaffolds.
# Source files (*.cc, *.h) provide production logic for distributed tensor handling.

# Define source files for the distributed tensor library
set(DISTRIBUTED_TENSOR_PUBLIC_HEADERS
    include/tensor_artifact_classes.h
    include/artifact_manifest.h
    include/shard_placement.h
    include/tensor_delta_log.h
    include/manifest_store.h
    include/snapshot_update_worker.h
    include/artifact_invalidation.h
    include/exact_graph_fallback.h
)

set(DISTRIBUTED_TENSOR_SOURCES
    src/tensor_artifact_classes.cc
    src/artifact_manifest.cc
    src/shard_placement.cc
    src/tensor_delta_log.cc
    src/manifest_store.cc
    src/snapshot_update_worker.cc
    src/artifact_invalidation.cc
    src/exact_graph_fallback.cc
)

# Create the distributed tensor library
add_library(themis_distributed_tensor STATIC ${DISTRIBUTED_TENSOR_SOURCES})

# Set include directories
target_include_directories(themis_distributed_tensor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Enable C++17
target_compile_features(themis_distributed_tensor PRIVATE cxx_std_17)

