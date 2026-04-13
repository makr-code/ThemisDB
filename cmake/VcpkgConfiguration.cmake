# ThemisDB vcpkg Configuration
# Centralized vcpkg setup with binary caching and dependency management

message(STATUS "==========================================")
message(STATUS "vcpkg Configuration")
message(STATUS "==========================================")

# Consume optional cache vars some tooling passes via -D to avoid
# "Manually-specified variables were not used" warnings.
if(DEFINED VCPKG_POWERSHELL_PATH)
    set(ENV{VCPKG_POWERSHELL_PATH} "${VCPKG_POWERSHELL_PATH}")
endif()
if(DEFINED Z_VCPKG_PWSH_PATH)
    set(ENV{Z_VCPKG_PWSH_PATH} "${Z_VCPKG_PWSH_PATH}")
endif()
if(DEFINED Z_VCPKG_POWERSHELL_PATH)
    set(ENV{Z_VCPKG_POWERSHELL_PATH} "${Z_VCPKG_POWERSHELL_PATH}")
endif()

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
    # Multi-tier caching strategy for optimal local builds:
    # 1. Project-local cache (fastest, shared within project)
    # 2. User-level cache (shared across projects)
    # 3. Build-directory fallback
    
    set(_cache_sources "")
    
    # Tier 1: Project-local cache (highest priority)
    set(_project_cache_dir "${CMAKE_SOURCE_DIR}/.vcpkg-cache")
    if(EXISTS "${_project_cache_dir}" OR CMAKE_SOURCE_DIR)
        list(APPEND _cache_sources "files,${_project_cache_dir},readwrite")
        message(STATUS "Binary cache tier 1: Project-local")
        message(STATUS "  Location: ${_project_cache_dir}")
    endif()
    
    # Tier 2: User-level cache (fallback for shared packages)
    set(_user_cache_dir "")
    if(WIN32)
        if(DEFINED ENV{LOCALAPPDATA})
            set(_user_cache_dir "$ENV{LOCALAPPDATA}/vcpkg/archives")
        endif()
    else()
        if(DEFINED ENV{HOME})
            set(_user_cache_dir "$ENV{HOME}/.cache/vcpkg/archives")
        endif()
    endif()
    
    if(_user_cache_dir)
        list(APPEND _cache_sources "files,${_user_cache_dir},readwrite")
        message(STATUS "Binary cache tier 2: User-level")
        message(STATUS "  Location: ${_user_cache_dir}")
    endif()
    
    # Tier 3: Build directory fallback (if nothing else works)
    if(NOT _cache_sources)
        set(_fallback_cache_dir "${CMAKE_BINARY_DIR}/.vcpkg_cache")
        list(APPEND _cache_sources "files,${_fallback_cache_dir},readwrite")
        message(STATUS "Binary cache tier 3: Build directory fallback")
        message(STATUS "  Location: ${_fallback_cache_dir}")
    endif()
    
    # Configure multi-tier cache sources
    if(_cache_sources)
        # Join cache sources with semicolons
        string(REPLACE ";" ";" _cache_sources_joined "${_cache_sources}")
        set(ENV{VCPKG_BINARY_SOURCES} "clear;${_cache_sources_joined}")
        
        message(STATUS "Binary cache: Enabled (multi-tier local cache)")
        message(STATUS "  Mode: Read-Write (all tiers)")
        message(STATUS "  Strategy: Project-local → User-level → Fallback")
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
else()
    # Normalize both paths to forward slashes before comparing (Windows backslash vs forward slash)
    file(TO_CMAKE_PATH "${CMAKE_TOOLCHAIN_FILE}" _current_toolchain_norm)
    file(TO_CMAKE_PATH "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake" _expected_toolchain_norm)
    # Also compare canonicalized absolute paths to tolerate relative/alternate spellings.
    get_filename_component(_current_toolchain_real "${_current_toolchain_norm}" REALPATH)
    get_filename_component(_expected_toolchain_real "${_expected_toolchain_norm}" REALPATH)
    # On Windows paths are case-insensitive – fall back to lowercase comparison
    string(TOLOWER "${_current_toolchain_norm}" _current_toolchain_lower)
    string(TOLOWER "${_expected_toolchain_norm}" _expected_toolchain_lower)
    string(TOLOWER "${_current_toolchain_real}" _current_toolchain_real_lower)
    string(TOLOWER "${_expected_toolchain_real}" _expected_toolchain_real_lower)
    if(_current_toolchain_lower STREQUAL _expected_toolchain_lower)
        message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE} (matches vcpkg root)")
    elseif(_current_toolchain_real_lower STREQUAL _expected_toolchain_real_lower)
        message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE} (matches vcpkg root)")
    elseif(_current_toolchain_norm STREQUAL _expected_toolchain_norm)
        message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE} (matches vcpkg root)")
    else()
        message(WARNING "CMAKE_TOOLCHAIN_FILE already set to: ${CMAKE_TOOLCHAIN_FILE}")
        message(WARNING "  Expected: ${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake")
        message(WARNING "  vcpkg may not be properly configured.")
    endif()
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

# Kafka CDC producer
if(THEMIS_ENABLE_KAFKA)
    list(APPEND VCPKG_MANIFEST_FEATURES "kafka")
    message(STATUS "vcpkg feature: kafka (librdkafka)")
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
