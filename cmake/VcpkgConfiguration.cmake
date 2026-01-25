# ThemisDB vcpkg Configuration
# Centralized vcpkg setup with binary caching and dependency management

message(STATUS "==========================================")
message(STATUS "vcpkg Configuration")
message(STATUS "==========================================")

# ============================================================================
# BINARY CACHE CONFIGURATION
# ============================================================================
# Enable binary caching to speed up builds from ~30 minutes to ~2 minutes
# Binary cache can be local filesystem, NuGet, or cloud storage (AWS, Azure, GCS)

# Check if VCPKG_BINARY_SOURCES is set (for CI/CD environments)
if(DEFINED ENV{VCPKG_BINARY_SOURCES})
    message(STATUS "Binary cache: Enabled via VCPKG_BINARY_SOURCES")
    message(STATUS "  Source: $ENV{VCPKG_BINARY_SOURCES}")
else()
    # Set default binary cache location for local builds
    set(_default_cache_dir "")
    
    if(WIN32)
        if(DEFINED ENV{LOCALAPPDATA})
            set(_default_cache_dir "$ENV{LOCALAPPDATA}/vcpkg/archives")
        else()
            set(_default_cache_dir "${CMAKE_BINARY_DIR}/.vcpkg_cache")
        endif()
    else()
        if(DEFINED ENV{HOME})
            set(_default_cache_dir "$ENV{HOME}/.cache/vcpkg/archives")
        else()
            set(_default_cache_dir "${CMAKE_BINARY_DIR}/.vcpkg_cache")
        endif()
    endif()
    
    # Create cache directory if it doesn't exist
    if(_default_cache_dir)
        file(MAKE_DIRECTORY "${_default_cache_dir}")
        
        # Set binary cache environment variable for vcpkg
        set(ENV{VCPKG_BINARY_SOURCES} "clear;files,${_default_cache_dir},readwrite")
        
        message(STATUS "Binary cache: Enabled (default local cache)")
        message(STATUS "  Location: ${_default_cache_dir}")
        message(STATUS "  Mode: Read-Write")
    else()
        message(WARNING "Binary cache: Could not determine cache directory")
    endif()
endif()

# ============================================================================
# VCPKG ROOT DETECTION
# ============================================================================

# Priority order:
# 1. VCPKG_ROOT environment variable
# 2. vcpkg subdirectory in project root
# 3. Error if not found

if(DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_ROOT_DIR "$ENV{VCPKG_ROOT}")
    message(STATUS "vcpkg root: $ENV{VCPKG_ROOT} (from VCPKG_ROOT)")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_ROOT_DIR "${CMAKE_SOURCE_DIR}/vcpkg")
    message(STATUS "vcpkg root: ${CMAKE_SOURCE_DIR}/vcpkg (local)")
else()
    message(WARNING "vcpkg not found. Please set VCPKG_ROOT environment variable or clone vcpkg to ${CMAKE_SOURCE_DIR}/vcpkg")
    message(WARNING "  git clone https://github.com/microsoft/vcpkg.git")
    message(WARNING "  cd vcpkg && ./bootstrap-vcpkg.sh (Linux/macOS) or .\\bootstrap-vcpkg.bat (Windows)")
    return()
endif()

# ============================================================================
# VCPKG TOOLCHAIN FILE
# ============================================================================

# Set CMAKE_TOOLCHAIN_FILE to vcpkg toolchain (BEFORE project() call would be ideal,
# but we're including this after project() for compatibility)
# This must be set as CACHE STRING FORCE to override any previous value

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file" FORCE)
    message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
elseif(NOT CMAKE_TOOLCHAIN_FILE STREQUAL "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake")
    message(WARNING "CMAKE_TOOLCHAIN_FILE already set to: ${CMAKE_TOOLCHAIN_FILE}")
    message(WARNING "  Expected: ${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake")
    message(WARNING "  vcpkg may not be properly configured.")
endif()

# ============================================================================
# VCPKG FEATURE DEPENDENCIES
# ============================================================================
# Map ThemisDB feature flags to vcpkg features

# Build list of vcpkg features to install based on enabled features
set(VCPKG_MANIFEST_FEATURES "")

# GPU acceleration requires FAISS
if(THEMIS_ENABLE_GPU)
    list(APPEND VCPKG_MANIFEST_FEATURES "gpu")
    message(STATUS "vcpkg feature: gpu (FAISS, OpenBLAS, LAPACK)")
endif()

# LLM integration (llama.cpp is external, but we may need ONNX Runtime)
if(THEMIS_ENABLE_LLM)
    list(APPEND VCPKG_MANIFEST_FEATURES "llm")
    message(STATUS "vcpkg feature: llm")
endif()

# RPC framework (gRPC)
if(THEMIS_ENABLE_GRPC)
    list(APPEND VCPKG_MANIFEST_FEATURES "rpc")
    message(STATUS "vcpkg feature: rpc (gRPC, protobuf)")
endif()

# CUDA support (does not add vcpkg dependencies, but enables CUDA features)
if(THEMIS_ENABLE_CUDA)
    list(APPEND VCPKG_MANIFEST_FEATURES "cuda")
    message(STATUS "vcpkg feature: cuda (requires CUDA Toolkit installed separately)")
endif()

# HTTP/2 protocol
if(THEMIS_ENABLE_HTTP2)
    list(APPEND VCPKG_MANIFEST_FEATURES "http2")
    message(STATUS "vcpkg feature: http2 (nghttp2)")
endif()

# HTTP/3 protocol (experimental)
if(THEMIS_ENABLE_HTTP3)
    list(APPEND VCPKG_MANIFEST_FEATURES "http3")
    message(STATUS "vcpkg feature: http3 (nghttp3, ngtcp2)")
endif()

# WebSocket protocol (uses Boost.Beast, no extra dependencies)
if(THEMIS_ENABLE_WEBSOCKET)
    list(APPEND VCPKG_MANIFEST_FEATURES "websocket")
    message(STATUS "vcpkg feature: websocket")
endif()

# MCP protocol
if(THEMIS_ENABLE_MCP)
    list(APPEND VCPKG_MANIFEST_FEATURES "mcp")
    message(STATUS "vcpkg feature: mcp")
endif()

# MQTT protocol
if(THEMIS_ENABLE_MQTT)
    list(APPEND VCPKG_MANIFEST_FEATURES "mqtt")
    message(STATUS "vcpkg feature: mqtt (paho-mqttpp3)")
endif()

# PostgreSQL wire protocol (no extra dependencies)
if(THEMIS_ENABLE_POSTGRES_WIRE)
    list(APPEND VCPKG_MANIFEST_FEATURES "postgres-wire")
    message(STATUS "vcpkg feature: postgres-wire")
endif()

# GDAL for geospatial support
if(THEMIS_ENABLE_GDAL)
    list(APPEND VCPKG_MANIFEST_FEATURES "gdal")
    message(STATUS "vcpkg feature: gdal")
endif()

# Set VCPKG_MANIFEST_FEATURES for vcpkg to use
if(VCPKG_MANIFEST_FEATURES)
    set(VCPKG_MANIFEST_FEATURES "${VCPKG_MANIFEST_FEATURES}" CACHE STRING "vcpkg manifest features" FORCE)
    message(STATUS "Active vcpkg features: ${VCPKG_MANIFEST_FEATURES}")
else()
    message(STATUS "No optional vcpkg features enabled (minimal build)")
endif()

# ============================================================================
# DEPENDENCY VALIDATION
# ============================================================================

# Validate that required packages will be available
# This is a pre-check before find_package() calls in Dependencies.cmake

message(STATUS "Validating dependency requirements...")

# Check if vcpkg.json exists
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json")
    message(WARNING "vcpkg.json not found at ${CMAKE_SOURCE_DIR}/vcpkg.json")
    message(WARNING "  vcpkg manifest mode may not work correctly.")
endif()

# GPU features require FAISS
if(THEMIS_ENABLE_GPU AND NOT "gpu" IN_LIST VCPKG_MANIFEST_FEATURES)
    message(WARNING "GPU acceleration enabled but 'gpu' vcpkg feature not in manifest.")
    message(WARNING "  FAISS library may not be available.")
endif()

# gRPC features require gRPC
if(THEMIS_ENABLE_GRPC AND NOT "rpc" IN_LIST VCPKG_MANIFEST_FEATURES)
    message(WARNING "gRPC enabled but 'rpc' vcpkg feature not in manifest.")
    message(WARNING "  gRPC library may not be available.")
endif()

message(STATUS "==========================================")
message(STATUS "vcpkg configuration complete")
message(STATUS "==========================================")
