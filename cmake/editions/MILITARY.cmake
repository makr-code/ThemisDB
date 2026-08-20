# ThemisDB MILITARY Edition Configuration
# Hardened, air-gapped capable edition for classified and high-security deployments.
# Strict security controls with no external cloud dependencies.

message(STATUS "Configuring MILITARY edition...")

if(NOT DEFINED THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "" OR NOT EXISTS "${THEMIS_LICENSE_FILE}")
    message(FATAL_ERROR
        "=============================================================\n"
        "MILITARY Edition REQUIRES embedded license file\n"
        "=============================================================\n"
        "\n"
        "MILITARY Edition cannot be configured without a valid\n"
        "license file.\n"
        "\n"
        "Usage:\n"
        "  cmake -B build -S . \\\n"
        "    -DTHEMIS_EDITION=MILITARY \\\n"
        "    -DTHEMIS_LICENSE_FILE=/path/to/military-license.json \\\n"
        "    -DCMAKE_BUILD_TYPE=Release\n"
        "\n"
        "=============================================================\n"
    )
endif()

message(STATUS "MILITARY Edition: License requirement ENFORCED")

# Hardware limits - conservative for secure, controlled deployments
set(THEMIS_GPU_MAX_VRAM_GB 16 CACHE STRING "GPU VRAM limit (GB)" FORCE)
set(THEMIS_SHARDING_MAX_NODES 50 CACHE STRING "Maximum sharding nodes" FORCE)
set(THEMIS_MAX_CACHE_SIZE_MB 2048 CACHE STRING "Maximum cache size (MB)" FORCE)

# Feature defaults for MILITARY edition
# Air-gapped operation: LLM remains local (llama.cpp) with CPU fallback.
set(THEMIS_ENABLE_LLM ON CACHE BOOL "LLM enabled for MILITARY edition (local/offline inference)" FORCE)
set(THEMIS_ENABLE_DISTRIBUTED_TRAINING OFF CACHE BOOL "Distributed training disabled for MILITARY edition" FORCE)

# gRPC enabled for secure inter-node comms (required, skipped in CI mode)
if(NOT THEMIS_CI_MODE)
    set(THEMIS_ENABLE_GRPC ON CACHE BOOL "gRPC required for MILITARY edition secure communications" FORCE)
else()
    message(STATUS "  MILITARY CI mode: gRPC not force-enabled")
endif()

# Real HSM required for key management
set(THEMIS_ENABLE_HSM_REAL ON CACHE BOOL "Real HSM required for MILITARY edition" FORCE)

# GPU enabled by default; runtime can fall back to CPU-only execution
set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU enabled for MILITARY edition (runtime CPU fallback)" FORCE)

# Tracing disabled by default (operational security - minimise side channels)
if(NOT DEFINED THEMIS_ENABLE_TRACING)
    set(THEMIS_ENABLE_TRACING OFF CACHE BOOL "Tracing optional in MILITARY edition")
endif()

# Edition-specific compile definitions
add_compile_definitions(THEMIS_MILITARY_EDITION)
add_compile_definitions(THEMIS_SHARDING_MAX_NODES=50)

message(STATUS "  Hardware limits: Up to 16 GB GPU VRAM, 50 nodes, 2 GB cache")
message(STATUS "  Features: Hardened security + HSM + gRPC + local LLM/GPU (CPU fallback)")
