# src/geo/CMakeLists.txt
# Geo module build orchestration with GPU conditional compilation
# Target: geo_object_lib (linked into themis_core by parent build)

# ============================================================================
# Core geo sources (always compiled)
# ============================================================================

set(GEO_CORE_SOURCES
    geo_json_geometry.cpp
    raster_query_interface.cpp
    rtree_cursor.cpp
    spatial_join_filter.cpp
    temporal_spatial_query_builder.cpp
    cpu_backend.cpp
    device_detector.cpp
    boost_cpu_exact_backend.cpp
    geo_rtree.cpp
    geo_clustering.cpp
    spatial_join.cpp
    raster.cpp
    temporal_spatial_query.cpp
    tile_server.cpp
    geo_backend_dispatch.cpp
    geo_policy.cpp
    phase2_phase3_hardening.cpp
    geo_faiss_knn.cpp
    geo_phase2_phase3_integration.cpp
)

# ============================================================================
# GPU-conditional sources
# ============================================================================

set(GEO_GPU_SOURCES)

# CUDA backend (preferred if available)
if(THEMIS_GEO_CUDA)
    list(APPEND GEO_GPU_SOURCES gpu_backend_cuda.cu)
    message(STATUS "[geo] CUDA kernel dispatch enabled (THEMIS_GEO_CUDA=ON)")
# HIP backend (AMD ROCm alternative)
elseif(THEMIS_GEO_HIP)
    list(APPEND GEO_GPU_SOURCES gpu_backend_hip.cpp)
    message(STATUS "[geo] HIP kernel dispatch enabled (THEMIS_GEO_HIP=ON)")
# Fallback: CPU-only dispatcher
else()
    list(APPEND GEO_GPU_SOURCES gpu_kernel_dispatcher_cpu.cpp)
    message(STATUS "[geo] GPU dispatch disabled; CPU fallback active")
endif()

# Always include stub for graceful fallback
list(APPEND GEO_GPU_SOURCES gpu_backend_stub.cpp)
list(APPEND GEO_GPU_SOURCES gpu_backend_production.cpp)

# ============================================================================
# Combine all geo sources
# ============================================================================

set(GEO_ALL_SOURCES ${GEO_CORE_SOURCES} ${GEO_GPU_SOURCES})

# Prepend source directory prefix for root-level include
set(GEO_SOURCES_WITH_PREFIX)
foreach(src IN LISTS GEO_ALL_SOURCES)
    list(APPEND GEO_SOURCES_WITH_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

# ============================================================================
# Geo object library (linked by parent project)
# ============================================================================

# Create object library so geo sources can be linked into themis_core
# without creating a separate shared/static library
add_library(geo_object_lib OBJECT IMPORTED)

# Store the list of geo sources for parent to link
set_target_properties(geo_object_lib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/geo_object_lib.a"
)

# Expose geo sources to parent via cache variable
# Parent project uses: target_sources(themis_core PRIVATE ${GEO_SOURCES_FULL_PATH})
set(GEO_SOURCES_FULL_PATH ${GEO_SOURCES_WITH_PREFIX} PARENT_SCOPE)

# ============================================================================
# GPU-specific compiler and linker settings
# ============================================================================

if(THEMIS_GEO_CUDA)
    # CUDA settings propagated to parent project
    set(GEO_CUDA_ENABLED TRUE PARENT_SCOPE)
    message(STATUS "[geo] CUDA compilation flags will be applied by parent")
elseif(THEMIS_GEO_HIP)
    # HIP settings propagated to parent project
    set(GEO_HIP_ENABLED TRUE PARENT_SCOPE)
    message(STATUS "[geo] HIP compilation flags will be applied by parent")
endif()

# ============================================================================
# Module exports for parent build
# ============================================================================

# Export variables for parent project to use
set(GEO_MODULE_SOURCES ${GEO_SOURCES_FULL_PATH} PARENT_SCOPE)
set(GEO_MODULE_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."  # root/include access
    PARENT_SCOPE
)

# Log compilation status
message(STATUS "[geo] Core sources: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "[geo] Total files: ${GEO_CORE_SOURCES} + ${GEO_GPU_SOURCES}")
