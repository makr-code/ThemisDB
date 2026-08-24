# ThemisDB COMMUNITY Edition Configuration
# Standard edition with optional LLM, gRPC, and basic protocols

message(STATUS "Configuring COMMUNITY edition...")

# Hardware limits
set(THEMIS_GPU_MAX_VRAM_GB 16 CACHE STRING "GPU VRAM limit (GB)" FORCE)
set(THEMIS_SHARDING_MAX_NODES 5 CACHE STRING "Maximum sharding nodes" FORCE)
set(THEMIS_MAX_CACHE_SIZE_MB 1024 CACHE STRING "Maximum cache size (MB)" FORCE)

# Feature defaults for COMMUNITY edition
# Core features enabled, optional LLM/gRPC/protocols (user can enable/disable)
# GPU/LLM/protocols default to OFF but can be enabled by user
if(NOT DEFINED THEMIS_ENABLE_LLM)
    set(THEMIS_ENABLE_LLM ON CACHE BOOL "LLM available in COMMUNITY edition")
endif()
if(NOT DEFINED THEMIS_ENABLE_GRPC)
    set(THEMIS_ENABLE_GRPC ON CACHE BOOL "gRPC available in COMMUNITY edition")
endif()
if(NOT DEFINED THEMIS_ENABLE_GPU)
    set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU available in COMMUNITY edition")
endif()

# Advanced features disabled by default
set(THEMIS_ENABLE_TRACING OFF CACHE BOOL "Tracing not available in COMMUNITY edition" FORCE)
set(THEMIS_ENABLE_HSM_REAL OFF CACHE BOOL "Real HSM not available in COMMUNITY edition" FORCE)
set(THEMIS_ENABLE_DISTRIBUTED_TRAINING OFF CACHE BOOL "Distributed training not available in COMMUNITY edition" FORCE)

# Edition-specific compile definitions
add_compile_definitions(THEMIS_COMMUNITY_EDITION)
add_compile_definitions(THEMIS_SHARDING_MAX_NODES=5)

message(STATUS "  Hardware limits: Up to 16 GB GPU VRAM (1× Tesla T4), 5 nodes, 1 GB cache")
message(STATUS "  Features: Core + optional LLM/gRPC/GPU")
