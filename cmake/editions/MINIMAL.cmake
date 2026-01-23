# ThemisDB MINIMAL Edition Configuration
# Lightweight edition for embedded systems, IoT, and edge devices

message(STATUS "Configuring MINIMAL edition...")

# Hardware limits
set(THEMIS_GPU_MAX_VRAM_GB 0 CACHE STRING "GPU VRAM limit (GB) - 0 = no GPU" FORCE)
set(THEMIS_SHARDING_MAX_NODES 1 CACHE STRING "Maximum sharding nodes" FORCE)
set(THEMIS_MAX_CACHE_SIZE_MB 128 CACHE STRING "Maximum cache size (MB)" FORCE)

# Feature defaults for MINIMAL edition
# Core database only - no LLM, no gRPC, no GPU, no protocols
set(THEMIS_ENABLE_LLM OFF CACHE BOOL "LLM disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_GRPC OFF CACHE BOOL "gRPC disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_GPU OFF CACHE BOOL "GPU disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_TRACING OFF CACHE BOOL "Tracing disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_HTTP2 OFF CACHE BOOL "HTTP/2 disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_HTTP3 OFF CACHE BOOL "HTTP/3 disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_WEBSOCKET OFF CACHE BOOL "WebSocket disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_MQTT OFF CACHE BOOL "MQTT disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_POSTGRES_WIRE OFF CACHE BOOL "PostgreSQL Wire disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_MCP OFF CACHE BOOL "MCP disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_SSE OFF CACHE BOOL "SSE disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_GRAPHQL OFF CACHE BOOL "GraphQL disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_VISION OFF CACHE BOOL "Vision disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_S3 OFF CACHE BOOL "S3 disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_AZURE OFF CACHE BOOL "Azure disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_WEBDAV OFF CACHE BOOL "WebDAV disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_DISTRIBUTED_TRAINING OFF CACHE BOOL "Distributed training disabled for MINIMAL edition" FORCE)
set(THEMIS_ENABLE_HSM_REAL OFF CACHE BOOL "Real HSM disabled for MINIMAL edition" FORCE)

# Edition-specific compile definitions
add_compile_definitions(THEMIS_MINIMAL_EDITION)
add_compile_definitions(THEMIS_GPU_MAX_VRAM_GB=0)
add_compile_definitions(THEMIS_SHARDING_MAX_NODES=1)

message(STATUS "  Hardware limits: No GPU, 1 node, 128 MB cache")
message(STATUS "  Features: Core database only")
