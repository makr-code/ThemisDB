# EPIC 3 — Distributed Tensor Artifacts: Implementation scaffolds.
# Source files (*.cc, *.h) provide production logic for distributed tensor handling.

# Define source files for the distributed tensor library
set(DISTRIBUTED_TENSOR_PUBLIC_HEADERS
    include/tensor_artifact_classes.h
    include/artifact_manifest.h
    include/shard_placement.h
)

set(DISTRIBUTED_TENSOR_SOURCES
    src/tensor_artifact_classes.cc
    src/artifact_manifest.cc
    src/shard_placement.cc
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

