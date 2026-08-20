# ThemisDB ENTERPRISE Edition Configuration
# Advanced edition with enhanced features, HSM, and multi-shard support

message(STATUS "Configuring ENTERPRISE edition...")

if(NOT DEFINED THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "" OR NOT EXISTS "${THEMIS_LICENSE_FILE}")
    message(FATAL_ERROR
        "=============================================================\n"
        "ENTERPRISE Edition REQUIRES embedded license file\n"
        "=============================================================\n"
        "\n"
        "ENTERPRISE Edition cannot be configured without a valid\n"
        "license file.\n"
        "\n"
        "Usage:\n"
        "  cmake -B build -S . \\\n"
        "    -DTHEMIS_EDITION=ENTERPRISE \\\n"
        "    -DTHEMIS_LICENSE_FILE=/path/to/enterprise-license.json \\\n"
        "    -DCMAKE_BUILD_TYPE=Release\n"
        "\n"
        "=============================================================\n"
    )
endif()

message(STATUS "ENTERPRISE Edition: License requirement ENFORCED")

# Hardware limits
set(THEMIS_GPU_MAX_VRAM_GB 24 CACHE STRING "GPU VRAM limit (GB)" FORCE)
set(THEMIS_SHARDING_MAX_NODES 100 CACHE STRING "Maximum sharding nodes" FORCE)
set(THEMIS_MAX_CACHE_SIZE_MB 4096 CACHE STRING "Maximum cache size (MB)" FORCE)

# Feature defaults for ENTERPRISE edition
# Advanced features enabled, gRPC required (skipped in CI mode)
if(NOT THEMIS_CI_MODE)
    set(THEMIS_ENABLE_GRPC ON CACHE BOOL "gRPC required for ENTERPRISE edition" FORCE)
else()
    message(STATUS "  ENTERPRISE CI mode: gRPC not force-enabled")
endif()
if(NOT DEFINED THEMIS_ENABLE_LLM)
    set(THEMIS_ENABLE_LLM ON CACHE BOOL "LLM available in ENTERPRISE edition")
endif()
if(NOT DEFINED THEMIS_ENABLE_GPU)
    set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU available in ENTERPRISE edition")
endif()

# Enterprise-specific features
set(THEMIS_ENABLE_HSM_REAL ON CACHE BOOL "Real HSM available in ENTERPRISE edition")

# Tracing disabled by default but can be enabled
if(NOT DEFINED THEMIS_ENABLE_TRACING)
    set(THEMIS_ENABLE_TRACING OFF CACHE BOOL "Tracing available in ENTERPRISE edition")
endif()

# Distributed training disabled by default
set(THEMIS_ENABLE_DISTRIBUTED_TRAINING OFF CACHE BOOL "Distributed training not available in ENTERPRISE edition" FORCE)

# Edition-specific compile definitions
add_compile_definitions(THEMIS_ENTERPRISE_EDITION)
add_compile_definitions(THEMIS_SHARDING_MAX_NODES=100)

message(STATUS "  Hardware limits: Up to 24 GB GPU VRAM, 100 nodes, 4 GB cache")
message(STATUS "  Features: Advanced features + HSM + multi-shard")
