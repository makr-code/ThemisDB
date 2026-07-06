# EPIC 3 — Distributed Tensor Artifacts
# Distributed tensor retrieval and manifesting support

set(DISTRIBUTED_TENSOR_PUBLIC_HEADERS
    include/distributed_planner.h
)

set(DISTRIBUTED_TENSOR_SOURCES
    src/distributed_planner.cc
)

add_library(themis_distributed_tensor STATIC ${DISTRIBUTED_TENSOR_SOURCES})
target_include_directories(themis_distributed_tensor 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

# Link against dependencies
target_link_libraries(themis_distributed_tensor
    PUBLIC themis_tensor
    PUBLIC themis_rag
)

# Install headers
install(FILES ${DISTRIBUTED_TENSOR_PUBLIC_HEADERS}
    DESTINATION include/distributed_tensor
)

