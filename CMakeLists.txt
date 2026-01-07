# ThemisDB Root CMakeLists.txt - New Modular Architecture
# Clean, minimal orchestration of the build system

cmake_minimum_required(VERSION 3.20)

# Read VERSION first (before project())
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    message(FATAL_ERROR "VERSION file not found at ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _ver_raw)
string(STRIP "${_ver_raw}" _ver_raw)

# Parse semantic version
if(NOT _ver_raw MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    message(FATAL_ERROR "Invalid VERSION format: '${_ver_raw}'. Expected: 1.2.3 or 1.2.3-alpha")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ver_numeric "${_ver_raw}")

# Define project with numeric version (CMake requirement)
project(Themis 
    VERSION ${_ver_numeric}
    DESCRIPTION "ThemisDB - Enterprise Distributed Database"
    LANGUAGES CXX
)

# Store full semantic version
set(THEMIS_VERSION_STRING "${_ver_raw}")

# ============================================================================
# BUILD OPTIONS & FEATURE FLAGS
# ============================================================================

set(THEMIS_EDITION "COMMUNITY" CACHE STRING "ThemisDB Edition")
option(THEMIS_BUILD_TESTS "Build unit tests" ON)
option(THEMIS_BUILD_BENCHMARKS "Build benchmarks" ON)
option(THEMIS_STRICT_BUILD "Treat warnings as errors" OFF)
option(THEMIS_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(THEMIS_ENABLE_AVX2 "Enable AVX2 SIMD" ON)
option(THEMIS_QNAP_BUILD "Build for QNAP NAS" OFF)
option(THEMIS_CORE_SHARED "Build themis_core as shared library" OFF)
option(THEMIS_STATIC_BUILD "Build fully static binary" OFF)
option(THEMIS_ENABLE_GRPC "Enable gRPC" ON)
option(THEMIS_ENABLE_TRACING "Enable OpenTelemetry tracing" ON)
option(THEMIS_ENABLE_LLM "Enable LLM plugin" OFF)
option(THEMIS_ENABLE_VISION "Enable CLIP-based vision (LLaVA) support" OFF)
option(THEMIS_ENABLE_GPU "Enable GPU acceleration" OFF)
option(THEMIS_ENABLE_CUDA "Enable CUDA backend" OFF)
option(THEMIS_ENABLE_HIP "Enable HIP backend" OFF)
option(THEMIS_ENABLE_HTTP2 "Enable HTTP/2" OFF)
option(THEMIS_ENABLE_HTTP3 "Enable HTTP/3" OFF)
option(THEMIS_ENABLE_WEBSOCKET "Enable WebSocket" OFF)
option(THEMIS_ENABLE_MQTT "Enable MQTT" OFF)
option(THEMIS_ENABLE_POSTGRES_WIRE "Enable PostgreSQL Wire" OFF)
option(THEMIS_ENABLE_MCP "Enable MCP" OFF)
option(THEMIS_ENABLE_SSE "Enable Server-Sent Events" OFF)
option(THEMIS_ENABLE_GRAPHQL "Enable GraphQL" OFF)
option(THEMIS_ENABLE_MIMALLOC "Enable mimalloc" OFF)
option(THEMIS_ENABLE_HSM_REAL "Enable real HSM provider" OFF)
option(THEMIS_ENABLE_CONTENT_PROCESSORS "Include content processors" OFF)
option(THEMIS_BUILD_DOCKER_RAID_BENCHMARK "Build Docker RAID benchmark" OFF)

# Phase 2 optimization features
option(THEMIS_ENABLE_WISCKEY "Enable WiscKey" OFF)
option(THEMIS_ENABLE_DOSTOEVSKY "Enable Dostoevsky" OFF)
option(THEMIS_ENABLE_CICADA "Enable Cicada" OFF)
option(THEMIS_ENABLE_LIGRA "Enable Ligra" OFF)
option(THEMIS_ENABLE_RABITQ "Enable RaBitQ" OFF)

# Phase 3 optimization features
option(THEMIS_ENABLE_DISKANN "Enable DiskANN" OFF)
option(THEMIS_ENABLE_BWTREE "Enable Bw-Tree" OFF)
option(THEMIS_ENABLE_SPLINTERDB "Enable SplinterDB" OFF)
option(THEMIS_ENABLE_GUNROCK "Enable Gunrock" OFF)
option(THEMIS_ENABLE_BAO "Enable Bao" OFF)

# ============================================================================
# MODULAR CMAKE INCLUDES
# ============================================================================

# Compiler setup
include(cmake/CompilerOptions.cmake)

# Version management
include(cmake/Versions.cmake)

# Dependencies
include(cmake/Dependencies.cmake)

# Main build logic
add_subdirectory(cmake)

# ============================================================================
# CPACK DISTRIBUTION
# ============================================================================

include(CPack)

set(CPACK_PACKAGE_NAME "themisdb")
set(CPACK_PACKAGE_VENDOR "VCC")
set(CPACK_PACKAGE_VERSION "${THEMIS_VERSION_STRING}")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;WIX")
    set(CPACK_PACKAGE_FILE_NAME "ThemisDB-${THEMIS_VERSION_STRING}-Windows-x64")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${THEMIS_VERSION_STRING}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "librocksdb-dev, libgrpc-dev, libgrpc++-dev, libssl-dev, libboost-all-dev")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_RPM_PACKAGE_REQUIRES "rocksdb-devel, grpc-devel, openssl-devel, boost-devel")
endif()

message(STATUS "==========================================")
message(STATUS "ThemisDB ${THEMIS_VERSION} (${THEMIS_VERSION_STRING})")
message(STATUS "Edition: ${THEMIS_EDITION}")
message(STATUS "==========================================")
