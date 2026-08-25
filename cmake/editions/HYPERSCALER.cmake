# ThemisDB HYPERSCALER Edition Configuration
# Maximum performance edition with all features enabled

message(STATUS "Configuring HYPERSCALER edition...")

if(NOT DEFINED THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "" OR NOT EXISTS "${THEMIS_LICENSE_FILE}")
    message(FATAL_ERROR
        "=============================================================\n"
        "HYPERSCALER Edition REQUIRES embedded license file\n"
        "=============================================================\n"
        "\n"
        "HYPERSCALER Edition cannot be configured without a valid\n"
        "license file.\n"
        "\n"
        "Usage:\n"
        "  cmake -B build -S . \\\n"
        "    -DTHEMIS_EDITION=HYPERSCALER \\\n"
        "    -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json \\\n"
        "    -DCMAKE_BUILD_TYPE=Release\n"
        "\n"
        "=============================================================\n"
    )
endif()

message(STATUS "HYPERSCALER Edition: License requirement ENFORCED")

# Hardware limits - no limits
set(THEMIS_GPU_MAX_VRAM_GB 0 CACHE STRING "GPU VRAM limit (GB) - 0 = unlimited" FORCE)
set(THEMIS_SHARDING_MAX_NODES 0 CACHE STRING "Maximum sharding nodes - 0 = unlimited" FORCE)
set(THEMIS_MAX_CACHE_SIZE_MB 0 CACHE STRING "Maximum cache size (MB) - 0 = unlimited" FORCE)

# Feature defaults for HYPERSCALER edition
# All features enabled (skipped in CI mode so runners without heavy deps can build)
if(NOT THEMIS_CI_MODE)
    set(THEMIS_ENABLE_LLM ON CACHE BOOL "LLM enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_GRPC ON CACHE BOOL "gRPC enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_TRACING ON CACHE BOOL "Tracing enabled for HYPERSCALER edition" FORCE)
    set(THEMIS_ENABLE_DISTRIBUTED_TRAINING ON CACHE BOOL "Distributed training enabled for HYPERSCALER edition" FORCE)
else()
    message(STATUS "  HYPERSCALER CI mode: heavyweight features not force-enabled (LLM/GPU/gRPC/Tracing/DistTraining)")
endif()

# All optional features available (user can disable if needed)
if(NOT DEFINED THEMIS_ENABLE_HSM_REAL)
    set(THEMIS_ENABLE_HSM_REAL ON CACHE BOOL "Real HSM available in HYPERSCALER edition")
endif()

# Edition-specific compile definitions
add_compile_definitions(THEMIS_HYPERSCALER_EDITION)
add_compile_definitions(THEMIS_SHARDING_MAX_NODES=0)

message(STATUS "  Hardware limits: Unlimited GPU VRAM, unlimited nodes, unlimited cache")
message(STATUS "  Features: All features enabled")
