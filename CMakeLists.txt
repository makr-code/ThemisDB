cmake_minimum_required(VERSION 3.20)

# VERSION ist Single-Source-of-Truth: zwingend aus Datei lesen
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
    message(FATAL_ERROR "VERSION file not found at project root. Please create and set semantic version (e.g., 1.2.3).")
endif()
file(READ "${CMAKE_SOURCE_DIR}/VERSION" _ver)
string(STRIP "${_ver}" _ver)
if(NOT _ver MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+(.*)?$")
    message(FATAL_ERROR "Invalid VERSION content: '${_ver}'. Expected semantic version like 1.2.3")
endif()

project(Themis VERSION ${_ver} LANGUAGES CXX)

# Make project version available to all translation units
add_compile_definitions(THEMIS_VERSION_STRING="${PROJECT_VERSION}")

# Include modular build configuration (post-v1.3.0 feature)
# See docs/architecture/MODULARIZATION_PLAN.md for details
include(cmake/ModularBuild.cmake)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable CUDA if requested
if(THEMIS_ENABLE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()
# Prefer CONFIG packages (vcpkg) over FindXXX modules
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Dependencies
find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
    find_package(OpenSSL REQUIRED)
endif()

# Make CURL optional: some environments (CI/dev) may not have dev package installed.
# If found, create CURL::libcurl target; otherwise provide a fallback and continue.
find_package(CURL CONFIG)
if(NOT CURL_FOUND)
    find_package(CURL)
endif()
if(NOT CURL_FOUND)
    message(WARNING "CURL package not found; building without libcurl. Some features may be disabled.")
endif()

# Build options
option(THEMIS_BUILD_TESTS "Build unit tests" ON)
option(THEMIS_BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(THEMIS_ENABLE_GPU "Enable GPU acceleration for vector search" OFF)
option(THEMIS_ENABLE_TRACING "Enable OpenTelemetry distributed tracing" ON)
option(THEMIS_ENABLE_ASAN "Enable AddressSanitizer for debugging" OFF)
option(THEMIS_STRICT_BUILD "Treat warnings as errors" OFF)
option(THEMIS_ENABLE_AVX2 "Enable AVX2/FMA for SIMD kernels in Release builds (MSVC)" ON)
option(THEMIS_QNAP_BUILD "Build for QNAP NAS with baseline x86-64 (no AVX)" OFF)
option(THEMIS_ENABLE_HSM_REAL "Enable real PKCS#11 HSM provider (fallback to stub if OFF)" OFF)
option(THEMIS_STATIC_BUILD "Build fully static binary for maximum portability (e.g., QNAP)" OFF)
option(THEMIS_CORE_SHARED "Build themis_core as a shared library (DLL/.so) instead of static" OFF)
option(THEMIS_ENABLE_CONTENT_PROCESSORS "Include content processors (audio/image/video/geo/cad) in core" OFF)

# For QNAP or fully static builds, force static core library
if(THEMIS_QNAP_BUILD OR THEMIS_STATIC_BUILD)
    set(THEMIS_CORE_SHARED OFF CACHE BOOL "Force static core for QNAP/static builds" FORCE)
    # FORCE STATIC BUILD FOR WINDOWS (LNK1189 Export Limit Workaround - v1.3.0+)
    set(THEMIS_CORE_SHARED OFF CACHE BOOL "Force static core to avoid Windows DLL export limit (LNK1189)" FORCE)
endif()

# LLM Support (v1.3.0+)
# Community Edition: LLM is a core feature (llama.cpp Embedding, Similarity, Inference)
# Enterprise/Hyperscaler: Advanced LLM features (Fine-Tuning, Model Management) available as add-on module
option(THEMIS_ENABLE_LLM "Enable LLM plugin support with llama.cpp (v1.3.0+)" OFF)

# Voice Assistant Support (v1.4.0+) - Enterprise Feature
# Requires: Whisper.cpp (STT), Piper TTS (TTS), llama.cpp (LLM)
option(THEMIS_ENABLE_VOICE_ASSISTANT "Enable Voice Assistant with STT/TTS/LLM (v1.4.0+)" OFF)
option(THEMIS_ENABLE_WHISPER "Enable Whisper.cpp for Speech-to-Text" OFF)
option(THEMIS_ENABLE_PIPER_TTS "Enable Piper TTS for Text-to-Speech" OFF)

# Hardware acceleration backend options
option(THEMIS_ENABLE_CUDA "Enable NVIDIA CUDA acceleration" OFF)
option(THEMIS_ENABLE_HIP "Enable AMD HIP acceleration" OFF)

# Performance Optimization Options (Research-based, see docs/de/research/)
# Phase 1: Quick Wins - Low effort, high impact optimizations (from PR #157)
option(THEMIS_ENABLE_MIMALLOC "Use mimalloc allocator for +10-20% performance" OFF)
option(THEMIS_ENABLE_HUGE_PAGES "Enable huge pages support for +15-30% memory performance" OFF)
option(THEMIS_ENABLE_RCU_INDEX "Use RCU for read-heavy index paths (+200-500% read performance)" OFF)
option(THEMIS_ENABLE_LIRS_CACHE "Use LIRS cache replacement policy (+30-40% hit rate)" OFF)

# Phase 2: Medium-Term - Higher impact, medium effort optimizations (from PR #156/#157 research)
# Medium-term optimizations (3-6 months effort, +100-200% overall performance)
option(THEMIS_ENABLE_WISCKEY "Enable WiscKey key/value separation (+40-60% writes)" OFF)
option(THEMIS_ENABLE_DOSTOEVSKY "Enable Dostoevsky adaptive LSM (+25-35% mixed)" OFF)
option(THEMIS_ENABLE_CICADA "Enable Cicada optimistic CC (+100-150% transactions)" OFF)
option(THEMIS_ENABLE_LIGRA "Enable Ligra graph processing (+200-300% graph)" OFF)
option(THEMIS_ENABLE_RABITQ "Enable RaBitQ vector quantization (16x memory reduction)" OFF)

# Phase 3: Long-Term - Very high impact, high effort optimizations (from PR #156/#157 research)
# Long-term optimizations (6-12 months effort, +200-500% specialized workloads)
option(THEMIS_ENABLE_DISKANN "Enable DiskANN billion-scale vector search (+300-400%)" OFF)
option(THEMIS_ENABLE_BWTREE "Enable Bw-Tree lock-free index (+100-200%)" OFF)
option(THEMIS_ENABLE_SPLINTERDB "Enable SplinterDB concurrent compaction (-70% P99)" OFF)
option(THEMIS_ENABLE_GUNROCK "Enable Gunrock GPU graph analytics (+1000-3000%)" OFF)
option(THEMIS_ENABLE_BAO "Enable Bao ML query optimizer (+30-70%)" OFF)

# Build options
option(THEMIS_BUILD_BENCHMARKS "Build performance benchmarks (requires Google Benchmark)" ON)

# Performance Optimization Dependencies
if(THEMIS_ENABLE_MIMALLOC)
    message(STATUS "Enabling mimalloc allocator optimization (+10-20% performance)")
    find_package(mimalloc CONFIG)
    if(NOT mimalloc_FOUND)
        message(WARNING "mimalloc not found via vcpkg. Install with: vcpkg install mimalloc")
        message(WARNING "Building without mimalloc. Performance optimization disabled.")
        set(THEMIS_ENABLE_MIMALLOC OFF CACHE BOOL "mimalloc not available" FORCE)
    else()
        message(STATUS "Found mimalloc: ${mimalloc_DIR}")
        add_compile_definitions(THEMIS_USE_MIMALLOC)
    endif()
endif()

if(THEMIS_ENABLE_HUGE_PAGES)
    message(STATUS "Enabling huge pages optimization (+15-30% memory performance)")
    
    # Check platform support
    if(UNIX)
        message(STATUS "Huge pages: Linux/Unix support enabled")
        message(STATUS "Configure with: echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages")
        add_compile_definitions(THEMIS_USE_HUGE_PAGES)
    elseif(WIN32)
        message(STATUS "Huge pages: Windows large pages support enabled")
        message(STATUS "Configure via: secpol.msc -> Lock pages in memory")
        add_compile_definitions(THEMIS_USE_HUGE_PAGES)
    else()
        message(WARNING "Huge pages not supported on this platform")
        set(THEMIS_ENABLE_HUGE_PAGES OFF CACHE BOOL "Platform not supported" FORCE)
    endif()
endif()

if(THEMIS_ENABLE_RCU_INDEX)
    message(STATUS "Enabling RCU index optimization (+200-500% read performance)")
    message(STATUS "RCU: Read-Copy-Update for lock-free reads in read-heavy workloads")
    add_compile_definitions(THEMIS_USE_RCU_INDEX)
    
    # RCU requires C++17 or later (already set to C++20)
    # No additional dependencies needed
endif()

if(THEMIS_ENABLE_LIRS_CACHE)
    message(STATUS "Enabling LIRS cache optimization (+30-40% cache hit rate)")
    message(STATUS "LIRS: Low Inter-reference Recency Set for scan-resistant caching")
    add_compile_definitions(THEMIS_USE_LIRS_CACHE)
    
    # LIRS requires C++17 or later (already set to C++20)
    # No additional dependencies needed
endif()

# Static build configuration for QNAP/older systems
if(THEMIS_STATIC_BUILD)
    message(STATUS "Configuring static build for maximum portability")
    
    # Static linking flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        
        # For fully static builds, link everything statically
        # Note: This requires static versions of all libraries
        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
    
    # Prefer static libraries on Linux
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
else()
    # Windows: Use dynamic linking (exe + DLLs)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
endif()
option(THEMIS_ENABLE_ZLUDA "Enable AMD ZLUDA (CUDA compatibility on AMD)" OFF)
option(THEMIS_ENABLE_ROCM "Enable AMD ROCm acceleration" OFF)
option(THEMIS_ENABLE_DIRECTX "Enable DirectX 12 Compute Shaders (Windows)" OFF)
option(THEMIS_ENABLE_VULKAN "Enable Vulkan Compute acceleration" OFF)
option(THEMIS_ENABLE_OPENGL "Enable OpenGL Compute Shaders" OFF)
option(THEMIS_ENABLE_METAL "Enable Apple Metal acceleration" OFF)
option(THEMIS_ENABLE_ONEAPI "Enable Intel OneAPI/SYCL acceleration" OFF)
option(THEMIS_ENABLE_OPENCL "Enable OpenCL acceleration" OFF)
option(THEMIS_ENABLE_WEBGPU "Enable WebGPU acceleration (experimental)" OFF)

# Lokal aktivierbare Code-Qualität (clang-tidy) ohne CI-Kosten.
option(ENABLE_LOCAL_CLANG_TIDY "Enable local clang-tidy analysis (off by default)" OFF)


# v1.1.0 Build Variants (Q1 2026)
option(THEMIS_USE_MIMALLOC "Use mimalloc allocator (v1.1.0 - 20-40% memory boost)" ON)
option(THEMIS_ENABLE_OLAP_VARIANT "Build with DuckDB for OLAP workloads" OFF)
option(THEMIS_EMBEDDED "Build embedded/lightweight variant (reduced dependencies)" OFF)
option(THEMIS_VLLM_COLOCATION "Optimize for vLLM co-location (AI/ML workloads)" OFF)

# Network Protocol Support Options
# Security: All protocols are opt-in (OFF by default) except HTTP/1.1 and protocols already in production
# Note: HTTP/1.1 is always enabled as the base protocol and cannot be disabled
option(THEMIS_ENABLE_HTTP2 "Enable HTTP/2 protocol support" OFF)
option(THEMIS_ENABLE_HTTP3 "Enable HTTP/3 (QUIC) protocol support" OFF)
option(THEMIS_ENABLE_GRPC "Enable gRPC protocol support (inter-shard communication)" ON)  # Already implemented in v1.3.0
option(THEMIS_ENABLE_WEBSOCKET "Enable WebSocket protocol support" OFF)
option(THEMIS_ENABLE_GRAPHQL "Enable GraphQL API support" ON)  # Already implemented
option(THEMIS_ENABLE_SSE "Enable Server-Sent Events (SSE) support" ON)  # Already implemented
option(THEMIS_ENABLE_MCP "Enable Model Context Protocol (MCP) support for LLM integration" OFF)
option(THEMIS_ENABLE_MQTT "Enable MQTT protocol support (IoT)" OFF)
option(THEMIS_ENABLE_POSTGRES_WIRE "Enable PostgreSQL Wire Protocol support" OFF)

# vLLM Co-Location: Automatically enable CUDA (Kernbestand, not Enterprise!)
if(THEMIS_VLLM_COLOCATION)
    set(THEMIS_ENABLE_CUDA ON CACHE BOOL "Force CUDA for vLLM co-location" FORCE)
    message(STATUS "vLLM Co-Location enabled: CUDA forced ON (Kernbestand)")
endif()

# Embedded variant: Disable heavy dependencies
if(THEMIS_EMBEDDED)
    set(THEMIS_ENABLE_TRACING OFF CACHE BOOL "Disable OpenTelemetry for embedded" FORCE)
    set(THEMIS_USE_MIMALLOC OFF CACHE BOOL "Disable mimalloc for embedded (minimal deps)" FORCE)
    message(STATUS "Embedded variant: Lightweight build (TBB/Arrow/OpenTelemetry/CUDA disabled)")
endif()

# Geo feature tiering options (Core vs. Enterprise add-ins)
option(THEMIS_GEO "Enable Geo core feature scaffolding" ON)
option(THEMIS_GEO_SIMD "Enable SIMD kernels for Geo (enterprise add-in)" OFF)
option(THEMIS_GEO_GPU "Enable GPU backend for Geo (enterprise add-in)" OFF)
option(THEMIS_GEO_H3 "Enable H3 integration (enterprise add-in)" OFF)
option(THEMIS_GEO_GEOS_PLUGIN "Enable GEOS prepared geometries plugin (enterprise add-in)" OFF)
option(THEMIS_ENTERPRISE "Enable enterprise capabilities bundle" OFF)
# Multi-Edition Support (v1.3.5+)
# THEMIS_EDITION: One of MINIMAL, COMMUNITY, ENTERPRISE, or HYPERSCALER (compile-time selection)
# This setting determines feature availability, licensing limits, and hardware constraints
# MINIMAL: Core database only (embedded/IoT), COMMUNITY: Full features (default), ENTERPRISE: Scaling+HA, HYPERSCALER: Unlimited
set(THEMIS_EDITION "COMMUNITY" CACHE STRING "Edition: MINIMAL, COMMUNITY, ENTERPRISE, or HYPERSCALER")
set_property(CACHE THEMIS_EDITION PROPERTY STRINGS MINIMAL COMMUNITY ENTERPRISE HYPERSCALER)

# Validate edition selection
if(NOT THEMIS_EDITION MATCHES "^(MINIMAL|COMMUNITY|ENTERPRISE|HYPERSCALER)$")
    message(FATAL_ERROR "THEMIS_EDITION must be MINIMAL, COMMUNITY, ENTERPRISE, or HYPERSCALER (got: ${THEMIS_EDITION})")
endif()

# Edition feature configuration (compile-time feature flags)
# These determine which functionality is available at build time

# GPU Memory Limits: Edition-specific VRAM constraints
# MINIMAL: No GPU, no LLM, no sharding - only core database features
# COMMUNITY: 24 GB (typical gaming GPU, consumer hardware)
# ENTERPRISE: 256 GB (data center grade A100/H100)
# HYPERSCALER: Unlimited (custom, OEM deployments)
if(THEMIS_EDITION STREQUAL "MINIMAL")
    # MINIMAL Edition: Core database features only
    # No LLM, no GPU acceleration, no sharding, no replication, no advanced analytics
    # Smallest footprint, fastest build time, ideal for embedded/edge deployments
    set(THEMIS_GPU_MAX_VRAM_GB 0 CACHE INTERNAL "No GPU support in Minimal Edition")
    set(THEMIS_SHARDING_MAX_NODES 1 CACHE INTERNAL "Single-node only in Minimal Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS OFF CACHE INTERNAL "Enterprise plugins disabled in Minimal")
    set(THEMIS_ENABLE_MULTI_MASTER OFF CACHE INTERNAL "Multi-master replication disabled in Minimal")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION OFF CACHE INTERNAL "Field-level encryption disabled in Minimal")
    set(THEMIS_ENABLE_RBAC OFF CACHE INTERNAL "RBAC disabled in Minimal")
    set(THEMIS_ENABLE_HSM OFF CACHE INTERNAL "HSM disabled in Minimal")
    set(THEMIS_ENABLE_LLM OFF CACHE INTERNAL "LLM disabled in Minimal")
    set(THEMIS_ENABLE_GPU OFF CACHE INTERNAL "GPU disabled in Minimal")
    set(THEMIS_ENABLE_CUDA OFF CACHE INTERNAL "CUDA disabled in Minimal")
    set(THEMIS_ENABLE_HIP OFF CACHE INTERNAL "HIP disabled in Minimal")
    set(THEMIS_ENABLE_VULKAN OFF CACHE INTERNAL "Vulkan disabled in Minimal")
    set(THEMIS_ENABLE_CONTENT_PROCESSORS OFF CACHE INTERNAL "Content processors disabled in Minimal")
    set(THEMIS_ENABLE_TRACING OFF CACHE INTERNAL "OpenTelemetry tracing disabled in Minimal")
    set(THEMIS_ENABLE_VOICE_ASSISTANT OFF CACHE INTERNAL "Voice assistant disabled in Minimal")
    set(THEMIS_ENABLE_WHISPER OFF CACHE INTERNAL "Whisper disabled in Minimal")
    set(THEMIS_ENABLE_PIPER_TTS OFF CACHE INTERNAL "Piper TTS disabled in Minimal")
    set(THEMIS_ENABLE_GRPC OFF CACHE INTERNAL "gRPC disabled in Minimal")
    set(THEMIS_ENABLE_HTTP2 OFF CACHE INTERNAL "HTTP/2 disabled in Minimal")
    set(THEMIS_ENABLE_HTTP3 OFF CACHE INTERNAL "HTTP/3 disabled in Minimal")
    set(THEMIS_ENABLE_WEBSOCKET OFF CACHE INTERNAL "WebSocket disabled in Minimal")
    set(THEMIS_ENABLE_MQTT OFF CACHE INTERNAL "MQTT disabled in Minimal")
    set(THEMIS_ENABLE_POSTGRES_WIRE OFF CACHE INTERNAL "PostgreSQL Wire disabled in Minimal")
    set(THEMIS_ENABLE_MCP OFF CACHE INTERNAL "MCP disabled in Minimal")
    set(THEMIS_BUILD_TESTS OFF CACHE INTERNAL "Tests disabled in Minimal for faster builds")
    set(THEMIS_BUILD_BENCHMARKS OFF CACHE INTERNAL "Benchmarks disabled in Minimal")
    message(STATUS "Edition: MINIMAL - Core database only, no GPU, no LLM, no sharding, no advanced features")
    message(STATUS "  Features: ACID transactions, multi-model storage, secondary indexes, basic queries")
    message(STATUS "  Protocols: HTTP/1.1, REST API only")
    message(STATUS "  Build time: ~5-10 minutes (vs ~30-40 for full build)")
    
elseif(THEMIS_EDITION STREQUAL "COMMUNITY")
    set(THEMIS_GPU_MAX_VRAM_GB 24 CACHE INTERNAL "Max GPU VRAM for Community Edition")
    set(THEMIS_SHARDING_MAX_NODES 1 CACHE INTERNAL "Max sharding nodes for Community Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS OFF CACHE INTERNAL "Enterprise plugins disabled in Community")
    set(THEMIS_ENABLE_MULTI_MASTER OFF CACHE INTERNAL "Multi-master replication disabled in Community")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION OFF CACHE INTERNAL "Field-level encryption disabled in Community")
    set(THEMIS_ENABLE_RBAC OFF CACHE INTERNAL "RBAC disabled in Community")
    set(THEMIS_ENABLE_HSM OFF CACHE INTERNAL "HSM disabled in Community")
    # LLM is a core feature in Community Edition (v1.3.0+) - can be enabled via THEMIS_ENABLE_LLM option
    message(STATUS "Edition: COMMUNITY - GPU limited to ${THEMIS_GPU_MAX_VRAM_GB}GB, single-node")

elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    set(THEMIS_GPU_MAX_VRAM_GB 256 CACHE INTERNAL "Max GPU VRAM for Enterprise Edition")
    set(THEMIS_SHARDING_MAX_NODES 100 CACHE INTERNAL "Max sharding nodes for Enterprise Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    message(STATUS "Edition: ENTERPRISE - GPU up to ${THEMIS_GPU_MAX_VRAM_GB}GB, up to ${THEMIS_SHARDING_MAX_NODES} nodes")

elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    set(THEMIS_GPU_MAX_VRAM_GB 999999 CACHE INTERNAL "Max GPU VRAM for Hyperscaler Edition (unlimited)")
    set(THEMIS_SHARDING_MAX_NODES 999999 CACHE INTERNAL "Max sharding nodes for Hyperscaler Edition (unlimited)")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    message(STATUS "Edition: HYPERSCALER - Unlimited GPU VRAM, unlimited nodes, full custom capabilities")

endif()

# Compile-time edition identifier for runtime checks and telemetry
add_compile_definitions(
    "THEMIS_EDITION_STRING=\"${THEMIS_EDITION}\""
    "THEMIS_GPU_MAX_VRAM_GB=${THEMIS_GPU_MAX_VRAM_GB}"
    "THEMIS_SHARDING_MAX_NODES=${THEMIS_SHARDING_MAX_NODES}"
)

# Detect target architecture
set(THEMIS_TARGET_ARCH "unknown")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|x64)$")
    set(THEMIS_TARGET_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(THEMIS_TARGET_ARCH "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|armv7l)$")
    set(THEMIS_TARGET_ARCH "armv7")
endif()

message(STATUS "Target architecture: ${THEMIS_TARGET_ARCH} (${CMAKE_SYSTEM_PROCESSOR})")

# Compiler warnings and optimizations
if(MSVC)
    add_compile_options(/W4 /utf-8 /wd4701 /bigobj /FS)
    # /FS: Allow parallel PDB writes to avoid C1041 errors
    # Suppress unreachable code warnings from external headers (e.g., nlohmann/json)
    add_compile_options(/wd4702)
    # Suppress AddressSanitizer warnings treated as errors (C5072)
    add_compile_options(/wd5072)
    # Suppress OpenSSL 3.0 deprecation warnings globally (C4996)
    add_compile_options(/wd4996)
    if(THEMIS_STRICT_BUILD)
        add_compile_options(/WX)
    endif()
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0A00)
    # Apply Release-specific settings for single- and multi-config generators
    add_compile_options($<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/GL>)
    # Disable iterator debugging in Release to match vcpkg RocksDB
    add_compile_definitions($<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>)
    if(THEMIS_ENABLE_AVX2 AND THEMIS_TARGET_ARCH STREQUAL "x86_64" AND NOT THEMIS_QNAP_BUILD)
        add_compile_options($<$<CONFIG:Release>:/arch:AVX2>)
    endif()
    # Ensure Link-Time Code Generation to avoid link restart messages
    add_link_options($<$<CONFIG:Release>:/LTCG>)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(THEMIS_STRICT_BUILD)
        add_compile_options(-Werror)
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
        # Architecture-specific optimizations
        if(THEMIS_TARGET_ARCH STREQUAL "x86_64")
            # QNAP build: disable AVX/FMA for old Celeron CPUs (N5095 has AVX2 but no FMA3)
            if(THEMIS_QNAP_BUILD)
                add_compile_options(-march=x86-64 -msse4.2)
                message(STATUS "QNAP build: SSE4.2 only (Celeron compatible, no AVX/FMA)")
            else()
                # Use native optimizations for x86_64 (maximum performance)
                add_compile_options(-march=native)
            endif()
        elseif(THEMIS_TARGET_ARCH STREQUAL "aarch64")
            # ARM64/AArch64 optimizations - enable NEON (standard on AArch64)
            add_compile_options(-march=armv8-a)
            message(STATUS "Enabling ARM64 NEON optimizations")
        elseif(THEMIS_TARGET_ARCH STREQUAL "armv7")
            # ARMv7 optimizations - enable NEON if available
            # Note: Not all ARMv7 CPUs have NEON, but Raspberry Pi 2/3 do
            add_compile_options(-march=armv7-a -mfpu=neon -mfloat-abi=hard)
            message(STATUS "Enabling ARMv7 NEON optimizations")
        else()
            # Generic optimizations for unknown architectures
            message(STATUS "Using generic optimizations for ${THEMIS_TARGET_ARCH}")
        endif()
    endif()
endif()

# AddressSanitizer support
if(THEMIS_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Optionale lokale clang-tidy Aktivierung (nicht in CI standardmäßig).
if(ENABLE_LOCAL_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy aktiviert: ${CLANG_TIDY_EXE}")
        # Keine automatischen Fixes hier; Entwickler nutzen Skript für -fix.
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "ENABLE_LOCAL_CLANG_TIDY=ON aber clang-tidy nicht gefunden.")
    endif()
endif()

# CMake helper function to safely link required targets
# This function checks if a target exists before attempting to link it
# and provides a clear error message if the target is missing.
# Usage: link_required(target_name <PRIVATE|PUBLIC|INTERFACE> dependency_target [additional_targets...])
# Example: link_required(themis_core PRIVATE SomePackage::component)
# Note: This helper is provided for future use. Current code uses explicit target
# checks before target_link_libraries to maintain minimal changes to existing code.
function(link_required target_name visibility dependency_target)
    if(NOT TARGET ${dependency_target})
        message(FATAL_ERROR "Required CMake target '${dependency_target}' not found for '${target_name}'. "
                            "Ensure the corresponding package was found successfully and the target is available.")
    endif()
    # Check additional targets in ARGN if any
    foreach(extra_target IN LISTS ARGN)
        if(NOT TARGET ${extra_target})
            message(FATAL_ERROR "Required CMake target '${extra_target}' not found for '${target_name}'. "
                                "Ensure the corresponding package was found successfully and the target is available.")
        endif()
    endforeach()
    target_link_libraries(${target_name} ${visibility} ${dependency_target} ${ARGN})
endfunction()

# Find required packages
# RocksDB requires zstd::zstd target, so we need to ensure it exists before finding RocksDB
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ZSTD_PKG QUIET libzstd)
    if(ZSTD_PKG_FOUND AND NOT TARGET zstd::zstd)
        add_library(zstd::zstd INTERFACE IMPORTED)
        set_target_properties(zstd::zstd PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_PKG_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${ZSTD_PKG_LIBRARIES}")
        message(STATUS "Created zstd::zstd target from pkg-config")
    endif()
endif()

find_package(RocksDB CONFIG)

# Select appropriate RocksDB target based on build type
# Note: vcpkg's rocksdb-shared only exports the C API, so linking the C++ wrapper
# requires the static library even when themis_core is built as a DLL.
set(THEMIS_ROCKSDB_TARGET "")
if(TARGET RocksDB::rocksdb)
    set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb")
    message(STATUS "RocksDB: Using static library (rocksdb.lib) to satisfy C++ API usage")
elseif(TARGET RocksDB::rocksdb-shared)
    set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb-shared")
    message(STATUS "RocksDB: Using shared C API library (rocksdb-shared.dll)")
endif()

# Fallback: locate system RocksDB if vcpkg didn't provide it
if(NOT TARGET RocksDB::rocksdb AND NOT TARGET RocksDB::rocksdb-shared)
    find_library(ROCKSDB_LIB rocksdb)
    find_path(ROCKSDB_INCLUDE_DIR rocksdb/db.h)
    if(ROCKSDB_LIB AND ROCKSDB_INCLUDE_DIR)
        add_library(RocksDB::rocksdb UNKNOWN IMPORTED)
        set_target_properties(RocksDB::rocksdb PROPERTIES
            IMPORTED_LOCATION "${ROCKSDB_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_INCLUDE_DIR}")
        set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb")
        message(STATUS "Using system RocksDB library at ${ROCKSDB_LIB} and include ${ROCKSDB_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "RocksDB not found. Install via vcpkg or system package (librocksdb-dev).")
    endif()
endif()

# Core packages via vcpkg (or system where available)
find_package(simdjson CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(Arrow CONFIG QUIET)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# yaml-cpp might be found via PkgConfig or CONFIG
find_package(yaml-cpp CONFIG QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found via CONFIG")
    if(NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
        # Some versions export as 'yaml-cpp' instead of 'yaml-cpp::yaml-cpp'
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    endif()
else()
    # Try pkg-config fallback
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(YAMLCPP QUIET yaml-cpp)
        if(YAMLCPP_FOUND)
            message(STATUS "yaml-cpp found via PkgConfig")
            add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
            set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${YAMLCPP_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${YAMLCPP_LIBRARIES}")
        endif()
    endif()
endif()

if(NOT TARGET yaml-cpp::yaml-cpp)
    message(FATAL_ERROR "yaml-cpp not found via CONFIG or PkgConfig")
endif()

# Defensive check: Ensure Boost::system target exists after find_package
if(NOT TARGET Boost::system)
    message(FATAL_ERROR "Required CMake target 'Boost::system' not found. "
                        "Ensure Boost was found and the 'system' component is available.")
endif()
# Optional ZSTD for content compression wrappers
# Note: ZSTD packages may expose different target names depending on the build system
# This defensive logic checks for multiple possible target names
find_package(zstd CONFIG)
if(zstd_FOUND)
    set(THEMIS_ZSTD_TARGET zstd::libzstd_shared)
    if(NOT TARGET zstd::libzstd_shared AND TARGET zstd::libzstd)
        set(THEMIS_ZSTD_TARGET zstd::libzstd)
    endif()
elseif(NOT zstd_FOUND)
    find_package(ZSTD CONFIG)
    if(ZSTD_FOUND)
        set(THEMIS_ZSTD_TARGET ZSTD::ZSTD)
    endif()
endif()

# Optional: HNSW (if not using Faiss)
find_package(hnswlib CONFIG)

# Optional: Faiss for GPU support (graceful degradation if not found)
if(THEMIS_ENABLE_GPU)
    # Hint CMake's FindBLAS towards OpenBLAS on Windows to avoid vendor auto-detection issues
    set(BLA_VENDOR OpenBLAS CACHE STRING "Preferred BLAS vendor for Windows" FORCE)

    # Try to locate OpenBLAS/LAPACK provided by vcpkg (CONFIG mode)
    find_package(OpenBLAS CONFIG QUIET)
    if(OpenBLAS_FOUND)
        message(STATUS "OpenBLAS gefunden – BLAS-Unterstützung aktiv")
    else()
        message(WARNING "OpenBLAS nicht gefunden – installieren mit: vcpkg install openblas")
    endif()

    find_package(LAPACK CONFIG QUIET)
    if(LAPACK_FOUND)
        message(STATUS "LAPACK gefunden")
    endif()

    # FAISS (CONFIG mode via vcpkg). If missing, gracefully disable GPU.
    find_package(faiss CONFIG)
    if(NOT faiss_FOUND)
        message(WARNING "Faiss nicht gefunden – GPU-Unterstützung wird automatisch deaktiviert.")
        set(THEMIS_ENABLE_GPU OFF CACHE BOOL "Enable GPU acceleration for vector search" FORCE)
    endif()
endif()

# Optional: OpenTelemetry for distributed tracing
if(THEMIS_ENABLE_TRACING)
    find_package(opentelemetry-cpp CONFIG REQUIRED)
endif()

# v1.1.0: mimalloc integration (20-40% memory boost, drop-in replacement)
if(THEMIS_USE_MIMALLOC)
    find_package(mimalloc CONFIG)
    if(mimalloc_FOUND)
        message(STATUS "mimalloc found - enabling for 20-40% memory boost")
    else()
        message(WARNING "mimalloc not found - falling back to system allocator. Install with: vcpkg install mimalloc")
        set(THEMIS_USE_MIMALLOC OFF CACHE BOOL "mimalloc not available" FORCE)
    endif()
endif()

# HTTP/2 Protocol Support
if(THEMIS_ENABLE_HTTP2)
    find_package(nghttp2 CONFIG)
    if(nghttp2_FOUND)
        message(STATUS "nghttp2 found - HTTP/2 protocol support enabled")
    else()
        message(WARNING "nghttp2 not found - disabling HTTP/2 support. Install with: vcpkg install nghttp2")
        set(THEMIS_ENABLE_HTTP2 OFF CACHE BOOL "nghttp2 not available" FORCE)
    endif()
endif()

# HTTP/3 Protocol Support (requires both nghttp3 and ngtcp2)
if(THEMIS_ENABLE_HTTP3)
    find_package(nghttp3 CONFIG)
    find_package(ngtcp2 CONFIG)
    if(nghttp3_FOUND AND ngtcp2_FOUND)
        message(STATUS "nghttp3 and ngtcp2 found - HTTP/3 (QUIC) protocol support enabled")
    else()
        if(NOT nghttp3_FOUND)
            message(WARNING "nghttp3 not found - Install with: vcpkg install nghttp3")
        endif()
        if(NOT ngtcp2_FOUND)
            message(WARNING "ngtcp2 not found - Install with: vcpkg install ngtcp2[openssl]")
        endif()
        message(WARNING "Disabling HTTP/3 support due to missing dependencies")
        set(THEMIS_ENABLE_HTTP3 OFF CACHE BOOL "nghttp3/ngtcp2 not available" FORCE)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Copy runtime config files into the build directory so tests and local runs
# can find `config/*` when executed from ${CMAKE_BINARY_DIR} (common when
# running tests via CTest/gtest_discover_tests). This is safe and cheap.
if(EXISTS "${CMAKE_SOURCE_DIR}/config")
    file(COPY "${CMAKE_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# ═══════════════════════════════════════════════════════════
# llama.cpp Integration (v1.3.0+)
# ═══════════════════════════════════════════════════════════
if(THEMIS_ENABLE_LLM)
    message(STATUS "LLM support enabled - integrating llama.cpp")

    # Prefer local, non-committed root clone at ${CMAKE_SOURCE_DIR}/llama.cpp
    # Fallback to legacy path external/llama.cpp if present
    set(LLAMA_SRC_DIR "${CMAKE_SOURCE_DIR}/llama.cpp")
    if(NOT EXISTS "${LLAMA_SRC_DIR}/CMakeLists.txt")
        if(EXISTS "${CMAKE_SOURCE_DIR}/external/llama.cpp/CMakeLists.txt")
            set(LLAMA_SRC_DIR "${CMAKE_SOURCE_DIR}/external/llama.cpp")
            message(WARNING "Using legacy llama.cpp at external/llama.cpp. Prefer root llama.cpp/ (ignored by Git/Docker).")
        else()
            message(FATAL_ERROR
                "llama.cpp source not found!\n"
                "Clone locally (ignored by Git/Docker):\n"
                "  git clone https://github.com/ggerganov/llama.cpp.git llama.cpp\n"
                "Or run: bash scripts/setup-llamacpp.sh")
        endif()
    endif()

    # Configure llama.cpp build options
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama.cpp tests" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama.cpp examples" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama.cpp server" FORCE)
    set(LLAMA_STATIC ON CACHE BOOL "Build llama.cpp as static library" FORCE)
    
    # GPU acceleration options
    if(THEMIS_ENABLE_CUDA)
        message(STATUS "  Enabling CUDA support in llama.cpp")
        set(LLAMA_CUDA ON CACHE BOOL "Enable CUDA in llama.cpp" FORCE)
        set(LLAMA_CUDA_F16 ON CACHE BOOL "Enable FP16 in CUDA" FORCE)
    endif()
    
    if(THEMIS_ENABLE_METAL)
        message(STATUS "  Enabling Metal support in llama.cpp")
        set(LLAMA_METAL ON CACHE BOOL "Enable Metal in llama.cpp" FORCE)
    endif()
    
    if(THEMIS_ENABLE_VULKAN)
        message(STATUS "  Enabling Vulkan support in llama.cpp")
        set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan in llama.cpp" FORCE)
    endif()
    
    if(THEMIS_ENABLE_HIP OR THEMIS_ENABLE_ROCM)
        message(STATUS "  Enabling HIP/ROCm support in llama.cpp")
        set(LLAMA_HIPBLAS ON CACHE BOOL "Enable hipBLAS in llama.cpp" FORCE)
    endif()
    
    # Add llama.cpp subdirectory (resolved via LLAMA_SRC_DIR)
    add_subdirectory(${LLAMA_SRC_DIR} EXCLUDE_FROM_ALL)
    
    # MSVC fix: char8_t string literals in llama-chat.cpp require /Zc:char8_t-
    if(MSVC AND TARGET llama)
        target_compile_options(llama PRIVATE /Zc:char8_t-)
    endif()
    
    # Define LLM enabled for ThemisDB code
    add_compile_definitions(THEMIS_LLM_ENABLED)
    
    message(STATUS "  llama.cpp configured successfully")
endif()

# Core library sources
set(THEMIS_CORE_SOURCES
    src/storage/rocksdb_wrapper.cpp
    src/storage/base_entity.cpp
    src/storage/key_schema.cpp
    src/storage/backup_manager.cpp
    
    src/index/secondary_index.cpp
    src/index/graph_index.cpp
    src/index/temporal_graph.cpp
    src/index/property_graph.cpp
    src/index/edge_types.cpp
    src/index/process_graph.cpp
    src/index/gnn_embeddings.cpp
    src/index/graph_analytics.cpp
    src/index/vector_index.cpp
    src/index/product_quantizer.cpp
    src/index/adaptive_index.cpp
    
    src/security/mock_key_provider.cpp
    src/security/vault_key_provider.cpp
    src/security/key_cache.cpp
    src/security/keyprovider_signing.cpp
    src/security/vault_signing_provider.cpp
    src/security/field_encryption.cpp
    src/security/encrypted_field.cpp
    src/security/malware_scanner.cpp
    
    src/transaction/transaction_manager.cpp
    src/transaction/saga.cpp
    
    src/query/query_engine.cpp
    src/query/query_optimizer.cpp
    src/query/aql_parser.cpp
    src/query/aql_parser_json.cpp
    src/query/aql_translator.cpp
    src/query/aql_runner.cpp
    src/query/let_evaluator.cpp
    src/query/window_evaluator.cpp
    src/query/cte_subquery.cpp
    src/query/cte_cache.cpp

    
    
    src/query/statistical_aggregator.cpp
    src/query/semantic_cache.cpp
    src/query/functions/function_registry.cpp
    
    src/server/http_server.cpp
    $<$<BOOL:${THEMIS_ENABLE_SSE}>:src/server/sse_connection_manager.cpp>
    src/server/audit_api_handler.cpp
    src/server/pki_api_handler.cpp
    src/server/saga_api_handler.cpp
    src/server/pii_api_handler.cpp
    src/server/retention_api_handler.cpp
    src/server/keys_api_handler.cpp
    src/server/classification_api_handler.cpp
    src/server/reports_api_handler.cpp
    src/server/ranger_adapter.cpp
    src/server/policy_engine.cpp
    src/server/auth_middleware.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:src/server/wal_grpc_service.cpp>
    
    # LLM integration (server/API/grpc) is added conditionally below
    src/server/rate_limiter.cpp
    src/server/rate_limiter_v2.cpp
    src/server/load_shedder.cpp
    src/server/update_api_handler.cpp
    src/server/hot_reload_api_handler.cpp
    src/server/export_api_handler.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP2}>:src/server/http2_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:src/server/http3_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:src/server/websocket_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MQTT}>:src/server/mqtt_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:src/server/postgres_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MCP}>:src/server/mcp_server.cpp>
    
    src/content/content_type.cpp
    src/content/content_manager.cpp
    src/content/text_processor.cpp
    src/content/mock_clip_processor.cpp
    src/content/mime_detector.cpp
    src/content/content_policy.cpp
    src/content/content_fs.cpp
    src/content/version_manager.cpp
    
    src/cache/semantic_cache.cpp
    src/cache/embedding_cache.cpp
    # LLM core components are added conditionally below
    src/cdc/changefeed.cpp
    
    # Replication
    src/replication/replication_manager.cpp
    
    # Temporal conflict resolution
    src/temporal/temporal_conflict_resolver.cpp
    
    src/timeseries/timeseries.cpp
    src/timeseries/tsstore.cpp
    src/timeseries/gorilla.cpp
    src/timeseries/retention.cpp
    src/timeseries/continuous_agg.cpp
    src/timeseries/aggregate_scheduler.cpp
    src/timeseries/aggregate_scheduler_helper.cpp
    src/timeseries/query_optimizer.cpp
    
    src/utils/serialization.cpp
    src/utils/logger.cpp
    src/utils/cursor.cpp
    src/utils/tracing.cpp
    src/utils/zstd_codec.cpp
    src/utils/pki_client.cpp
    src/utils/input_validator.cpp
    src/utils/audit_logger.cpp
    src/utils/pii_detection_engine.cpp
    src/utils/regex_detection_engine.cpp
    src/utils/pii_detector.cpp
    src/utils/retention_manager.cpp
    src/utils/hkdf_helper.cpp
    src/utils/hkdf_cache.cpp
    src/utils/saga_logger.cpp
    src/utils/lek_manager.cpp
    src/utils/stemmer.cpp
    src/utils/stopwords.cpp
    src/utils/pii_pseudonymizer.cpp
    src/utils/normalizer.cpp
    src/utils/simd_distance.cpp
    src/utils/update_checker.cpp
    src/utils/http_client_pool.cpp
    
    # Updates (hot-reload)
    src/updates/release_manifest.cpp
    src/updates/manifest_database.cpp
    src/updates/hot_reload_engine.cpp
    src/updates/updates_config.cpp
    
    # Geo utilities
    src/utils/geo/ewkb.cpp
    
    # Spatial index
    src/index/spatial_index.cpp
    
    # Hybrid search (BM25 + Vector)
    src/search/hybrid_search.cpp
    src/security/pki_key_provider.cpp
    src/security/cms_signing.cpp
    src/security/rbac.cpp
    src/security/hsm_provider.cpp
    src/security/hsm_provider_pkcs11.cpp
    src/security/timestamp_authority.cpp
    src/security/timestamp_authority_openssl.cpp
    src/security/vcc_pki_client.cpp
    src/storage/security_signature.cpp
    src/storage/security_signature_manager.cpp
    src/auth/jwt_validator.cpp
    src/governance/policy_engine.cpp
    # Geo scaffolding (headers + stubs; safe no-ops until used)
    src/geo/cpu_backend.cpp
    src/geo/gpu_backend_stub.cpp
    src/geo/boost_cpu_exact_backend.cpp
    src/api/geo_index_hooks.cpp
    
    # Sharding (horizontal scaling)
        src/utils/boost_throw_exception.cpp
    src/sharding/urn.cpp
    src/sharding/consistent_hash.cpp
    src/sharding/shard_topology.cpp
    src/sharding/urn_resolver.cpp
    src/sharding/pki_shard_certificate.cpp
    src/sharding/mtls_client.cpp
    src/sharding/signed_request.cpp
    src/sharding/remote_executor.cpp
    src/sharding/shard_rpc_client.cpp
    src/sharding/shard_router.cpp
    src/sharding/rebalance_operation.cpp
    src/sharding/data_migrator.cpp
    src/sharding/shard_load_detector.cpp
    src/sharding/auto_rebalancer.cpp
    src/sharding/prometheus_metrics.cpp
    src/sharding/metrics_registry.cpp
    src/sharding/health_check.cpp
    src/sharding/admin_api.cpp
    src/sharding/cloud_agent.cpp
    src/sharding/circuit_breaker.cpp
    src/sharding/gossip_protocol.cpp
    src/sharding/raft_configuration.cpp
    src/sharding/raft_log.cpp
    src/sharding/raft_state.cpp
    src/sharding/raft_wal_integration.cpp
    src/sharding/stream_protocol.cpp
    src/sharding/wal_applier.cpp
    src/sharding/wal_manager.cpp
    src/sharding/wal_shipper.cpp
    src/sharding/replication_coordinator.cpp
    src/sharding/replica_topology.cpp
    src/sharding/multi_primary_coordinator.cpp
    src/sharding/health_monitor.cpp
    src/sharding/truetime.cpp
    src/sharding/distributed_transaction.cpp
    src/sharding/redundancy_strategy.cpp
    src/sharding/hot_spare_manager.cpp
    src/sharding/shard_rpc_server.cpp
    src/storage/blob_redundancy_manager.cpp
    src/server/sharding_metrics_handler.cpp
    
    # Hardware acceleration (core abstraction layer - always included)
    src/acceleration/backend_registry.cpp
    src/acceleration/cpu_backend.cpp
    src/acceleration/plugin_loader.cpp
    src/acceleration/plugin_security.cpp
    # Plugin manager (core plugin orchestration)
    src/plugins/plugin_manager.cpp
    
    # Multi-tenancy support
    src/server/tenant_manager.cpp
    
    # Network
    src/network/wire_protocol_server.cpp
    
    # Observability
    src/observability/metrics_collector.cpp
    
    # GraphQL API (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:src/api/graphql.cpp>
    
    # Exporters and Importers
    src/exporters/jsonl_llm_exporter.cpp
    src/importers/postgres_importer.cpp
    
    # OLAP Analytics
    src/analytics/olap.cpp
    src/analytics/process_mining.cpp
)

# Phase 2 Performance Optimizations (PR #156/#157)
# Conditionally add Phase 2 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    src/performance/phase2_feature_flags.cpp
)

if(THEMIS_ENABLE_WISCKEY)
    message(STATUS "Including WiscKey key/value separation (THEMIS_ENABLE_WISCKEY=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/wisckey.cpp)
endif()

if(THEMIS_ENABLE_DOSTOEVSKY)
    message(STATUS "Including Dostoevsky adaptive LSM (THEMIS_ENABLE_DOSTOEVSKY=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/dostoevsky.cpp)
endif()

if(THEMIS_ENABLE_CICADA)
    message(STATUS "Including Cicada optimistic CC (THEMIS_ENABLE_CICADA=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/cicada.cpp)
endif()

if(THEMIS_ENABLE_LIGRA)
    message(STATUS "Including Ligra graph processing (THEMIS_ENABLE_LIGRA=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/ligra.cpp)
endif()

if(THEMIS_ENABLE_RABITQ)
    message(STATUS "Including RaBitQ vector quantization (THEMIS_ENABLE_RABITQ=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/rabitq.cpp)
endif()

# Phase 3 Performance Optimizations (PR #156/#157)
# Conditionally add Phase 3 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    src/performance/phase3/feature_flags.cpp
)

if(THEMIS_ENABLE_DISKANN)
    message(STATUS "Including DiskANN billion-scale vector search (THEMIS_ENABLE_DISKANN=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/phase3/diskann.cpp)
endif()

if(THEMIS_ENABLE_BWTREE)
    message(STATUS "Including Bw-Tree lock-free index (THEMIS_ENABLE_BWTREE=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/phase3/bwtree.cpp)
endif()

if(THEMIS_ENABLE_SPLINTERDB)
    message(STATUS "Including SplinterDB concurrent compaction (THEMIS_ENABLE_SPLINTERDB=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/phase3/splinterdb.cpp)
endif()

if(THEMIS_ENABLE_GUNROCK)
    message(STATUS "Including Gunrock GPU graph analytics (THEMIS_ENABLE_GUNROCK=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/phase3/gunrock.cpp)
endif()

if(THEMIS_ENABLE_BAO)
    message(STATUS "Including Bao ML query optimizer (THEMIS_ENABLE_BAO=ON)")
    list(APPEND THEMIS_CORE_SOURCES src/performance/phase3/bao.cpp)
endif()

# Conditionally add LLM sources (v1.3.0+) only when enabled
if(THEMIS_ENABLE_LLM)
    message(STATUS "Including LLM server and core sources (THEMIS_ENABLE_LLM=ON)")
    list(APPEND THEMIS_CORE_SOURCES
        # Server/API (HTTP + optional gRPC)
        src/server/llm_api_handler.cpp
        src/aql/llm_aql_handler.cpp

        # Optional: gRPC service and generated proto headers (guarded by gRPC)
        # Only add when gRPC is available, otherwise skip to prevent missing headers
    )

    find_package(gRPC CONFIG QUIET)
    if(gRPC_FOUND)
        message(STATUS "gRPC found - enabling LLM gRPC service")
            # TEMP: gRPC proto generation not yet configured - comment out until ready
            # list(APPEND THEMIS_CORE_SOURCES
            #     src/server/llm_grpc_service.cpp
            #     proto/llm_service.proto
            # )
        # Note: Proto generation should be configured in a separate section when needed
    else()
        message(STATUS "gRPC not found - disabling LLM gRPC service and proto generation")
    endif()
        # TEMP: Disable llm_grpc_service until proto generation is configured
        # list(REMOVE_ITEM THEMIS_CORE_SOURCES src/server/llm_grpc_service.cpp)

    # LLM core components
    list(APPEND THEMIS_CORE_SOURCES
        src/llm/llm_interaction_store.cpp
        src/llm/prompt_manager.cpp
        src/llm/block_table.cpp
        src/llm/paged_kv_cache.cpp
    )
endif()

# Ensure minimal LLM components are available when referenced from http_server.
# These have lightweight dependencies and avoid undefined references if LLM is disabled.
if(NOT THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        src/llm/llm_interaction_store.cpp
        src/llm/prompt_manager.cpp
    )
endif()

# Define core library early so subsequent target_* commands are valid
set(_THEMIS_CORE_LIBTYPE STATIC)
if(THEMIS_CORE_SHARED)
    set(_THEMIS_CORE_LIBTYPE SHARED)
endif()
add_library(themis_core ${_THEMIS_CORE_LIBTYPE} ${THEMIS_CORE_SOURCES})

# Per-file compile options for heavy AQL parser translation units (MSVC-only)
if(MSVC)
    set_source_files_properties(src/query/aql_parser.cpp PROPERTIES COMPILE_OPTIONS "/O1;/bigobj;/Bt+")
    set_source_files_properties(src/query/aql_parser_json.cpp PROPERTIES COMPILE_OPTIONS "/O1;/bigobj")
endif()

# On Windows, export all symbols automatically for shared builds to avoid needing __declspec(dllexport)
if(MSVC AND THEMIS_CORE_SHARED)
    # Auto-export needs normal COFF objects; disable /GL (IPO) so __create_def can read symbols
    set_target_properties(themis_core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        INTERPROCEDURAL_OPTIMIZATION FALSE
        VS_GLOBAL_WholeProgramOptimization "false"
    )
    target_compile_options(themis_core PRIVATE
        $<$<CONFIG:Release>:/GL->
        $<$<CONFIG:RelWithDebInfo>:/GL->
        $<$<CONFIG:MinSizeRel>:/GL->
    )
    target_link_options(themis_core PRIVATE
        $<$<CONFIG:Release>:/LTCG:OFF>
        $<$<CONFIG:RelWithDebInfo>:/LTCG:OFF>
        $<$<CONFIG:MinSizeRel>:/LTCG:OFF>
    )
endif()

# Add hardware acceleration backends based on build options
set(THEMIS_ACCELERATION_SOURCES "")

# CUDA backend
if(THEMIS_ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    
    # Add CUDA source files
    list(APPEND THEMIS_ACCELERATION_SOURCES 
        src/acceleration/cuda_backend.cpp
        src/acceleration/cuda/vector_kernels.cu
    )
    
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CUDA)
    target_include_directories(themis_core PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
    target_link_libraries(themis_core PRIVATE CUDA::cudart CUDA::cuda_driver)
    
    message(STATUS "CUDA acceleration enabled")
    message(STATUS "  CUDA Version: ${CUDAToolkit_VERSION}")
    message(STATUS "  CUDA Include: ${CUDAToolkit_INCLUDE_DIRS}")
endif()

# Graphics API backends
if(THEMIS_ENABLE_DIRECTX OR THEMIS_ENABLE_VULKAN OR THEMIS_ENABLE_OPENGL)
    list(APPEND THEMIS_ACCELERATION_SOURCES src/acceleration/graphics_backends.cpp)
    
    if(THEMIS_ENABLE_DIRECTX)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DIRECTX)
        message(STATUS "DirectX acceleration enabled (stub)")
    endif()
    
    if(THEMIS_ENABLE_VULKAN)
        # Try to find Vulkan SDK
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_VULKAN)
            target_include_directories(themis_core PRIVATE ${Vulkan_INCLUDE_DIRS})
            message(STATUS "Vulkan acceleration enabled")
            message(STATUS "  Vulkan Version: ${Vulkan_VERSION}")
        else()
            message(WARNING "Vulkan SDK not found, Vulkan support disabled")
        endif()
    endif()
    
    if(THEMIS_ENABLE_OPENGL)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_OPENGL)
        message(STATUS "OpenGL acceleration enabled (stub)")
    endif()
endif()

# Add acceleration sources to core if any were selected
if(THEMIS_ACCELERATION_SOURCES)
    target_sources(themis_core PRIVATE ${THEMIS_ACCELERATION_SOURCES})
endif()

# Main library already defined above (before feature-specific target modifications)

if(THEMIS_ENABLE_HSM_REAL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HSM_REAL)
    target_sources(themis_core PRIVATE src/security/hsm_provider_pkcs11.cpp)
endif()

# Geo feature flags and Boost.Geometry support
if(THEMIS_GEO)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_ENABLED)
    # Try to find Boost for Boost.Geometry backend
    find_package(Boost QUIET)
    if(Boost_FOUND)
        target_compile_definitions(themis_core PRIVATE THEMIS_GEO_BOOST_BACKEND)
        target_include_directories(themis_core PRIVATE ${Boost_INCLUDE_DIRS})
        message(STATUS "Boost.Geometry enabled for exact geometry checks")
    else()
        message(STATUS "Boost not found - Boost.Geometry backend disabled (will use MBR-only)")
    endif()
endif()

# Defensive selection of Arrow linking target: prefer shared, fall back to static.
if(TARGET Arrow::arrow_shared)
    set(THEMIS_ARROW_TARGET Arrow::arrow_shared)
elseif(TARGET Arrow::arrow_static)
    set(THEMIS_ARROW_TARGET Arrow::arrow_static)
elseif(TARGET arrow_shared)
    set(THEMIS_ARROW_TARGET arrow_shared)
elseif(TARGET arrow_static)
    set(THEMIS_ARROW_TARGET arrow_static)
else()
    set(THEMIS_ARROW_TARGET "")
endif()

# Parquet target (for Arrow parquet writer)
find_package(Parquet CONFIG QUIET)
set(THEMIS_PARQUET_TARGET "")
if(Parquet_FOUND)
    if(TARGET Parquet::parquet_shared)
        set(THEMIS_PARQUET_TARGET Parquet::parquet_shared)
    elseif(TARGET Parquet::parquet_static)
        set(THEMIS_PARQUET_TARGET Parquet::parquet_static)
    elseif(TARGET Parquet::parquet)
        set(THEMIS_PARQUET_TARGET Parquet::parquet)
    endif()
else()
    # Fallbacks some package layouts expose
    if(TARGET parquet)
        set(THEMIS_PARQUET_TARGET parquet)
    elseif(TARGET parquet_static)
        set(THEMIS_PARQUET_TARGET parquet_static)
    elseif(TARGET parquet_shared)
        set(THEMIS_PARQUET_TARGET parquet_shared)
    endif()
endif()

# v1.1.0: Define ARROW_ENABLED if Arrow is available
if(THEMIS_ARROW_TARGET)
    target_compile_definitions(themis_core PUBLIC ARROW_ENABLED)
    message(STATUS "Arrow enabled - Parquet export available")
endif()

target_link_libraries(themis_core
    PUBLIC
        ${THEMIS_ROCKSDB_TARGET}
        simdjson::simdjson
        TBB::tbb
        ${THEMIS_ARROW_TARGET}
        ${THEMIS_PARQUET_TARGET}
        fmt::fmt
        spdlog::spdlog
        Boost::system
        nlohmann_json::nlohmann_json
    $<$<BOOL:${CURL_FOUND}>:CURL::libcurl>
        yaml-cpp::yaml-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    TBB::tbb
)

# Link ZSTD if available and define compile flag
if(DEFINED THEMIS_ZSTD_TARGET)
    target_link_libraries(themis_core PUBLIC ${THEMIS_ZSTD_TARGET})
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_ZSTD)
endif()

# Define THEMIS_ENABLE_CURL if CURL is found
if(CURL_FOUND)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CURL)
endif()

# Enable OpenSSL-based Timestamp Authority (RFC 3161) if both OpenSSL and CURL are available
# OpenSSL is required for cryptographic operations, CURL for HTTP/HTTPS communication with TSA servers
if(OpenSSL_FOUND AND CURL_FOUND)
    target_compile_definitions(themis_core PUBLIC THEMIS_USE_OPENSSL_TSA)
    message(STATUS "Timestamp Authority (RFC 3161): Enabled with OpenSSL and CURL")
else()
    message(STATUS "Timestamp Authority (RFC 3161): Using stub implementation (OpenSSL=${OpenSSL_FOUND}, CURL=${CURL_FOUND})")
endif()

# Define version string for runtime use
target_compile_definitions(themis_core PUBLIC 
    THEMIS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    THEMIS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    THEMIS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    THEMIS_VERSION_STRING="${PROJECT_VERSION}"
)

# Phase 2 Performance Optimizations compile definitions
if(THEMIS_ENABLE_WISCKEY)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_WISCKEY)
endif()
if(THEMIS_ENABLE_DOSTOEVSKY)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DOSTOEVSKY)
endif()
if(THEMIS_ENABLE_CICADA)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CICADA)
endif()
if(THEMIS_ENABLE_LIGRA)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_LIGRA)
endif()
if(THEMIS_ENABLE_RABITQ)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_RABITQ)
endif()

# Phase 3 Performance Optimizations compile definitions
if(THEMIS_ENABLE_DISKANN)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DISKANN)
endif()
if(THEMIS_ENABLE_BWTREE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_BWTREE)
endif()
if(THEMIS_ENABLE_SPLINTERDB)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_SPLINTERDB)
endif()
if(THEMIS_ENABLE_GUNROCK)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GUNROCK)
endif()
if(THEMIS_ENABLE_BAO)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_BAO)
endif()

if(THEMIS_ENABLE_GPU)
    target_link_libraries(themis_core PUBLIC faiss)
    target_compile_definitions(themis_core PUBLIC THEMIS_GPU_ENABLED)
elseif(hnswlib_FOUND)
    target_link_libraries(themis_core PUBLIC hnswlib::hnswlib)
    target_compile_definitions(themis_core PUBLIC THEMIS_HNSW_ENABLED)
endif()

# v1.1.0: Link mimalloc (20-40% memory boost, drop-in replacement)
if(THEMIS_USE_MIMALLOC AND mimalloc_FOUND)
    target_link_libraries(themis_core PUBLIC mimalloc-static)
    target_compile_definitions(themis_core PUBLIC THEMIS_USE_MIMALLOC)
    message(STATUS "mimalloc linked to themis_core")
endif()

# HTTP/2 Protocol Support
if(THEMIS_ENABLE_HTTP2 AND nghttp2_FOUND)
    target_link_libraries(themis_core PUBLIC nghttp2::nghttp2)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HTTP2)
    message(STATUS "HTTP/2 support enabled and linked to themis_core")
endif()

# HTTP/3 Protocol Support
if(THEMIS_ENABLE_HTTP3 AND nghttp3_FOUND AND ngtcp2_FOUND)
    target_link_libraries(themis_core PUBLIC nghttp3::nghttp3 ngtcp2::ngtcp2)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HTTP3)
    message(STATUS "HTTP/3 support enabled and linked to themis_core")
endif()

# GraphQL Protocol Support
if(THEMIS_ENABLE_GRAPHQL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GRAPHQL)
    message(STATUS "GraphQL API support enabled")
endif()

# SSE (Server-Sent Events) Support
if(THEMIS_ENABLE_SSE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_SSE)
    message(STATUS "Server-Sent Events (SSE) support enabled")
endif()

# WebSocket Protocol Support
if(THEMIS_ENABLE_WEBSOCKET)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_WEBSOCKET)
    message(STATUS "WebSocket protocol support enabled")
endif()

# MCP (Model Context Protocol) Support
if(THEMIS_ENABLE_MCP)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MCP)
    message(STATUS "Model Context Protocol (MCP) support enabled")
    message(STATUS "  MCP Transports: stdio, SSE, WebSocket")
    message(STATUS "  MCP Components: Tools, Resources, Prompts")
endif()

# MQTT Protocol Support
if(THEMIS_ENABLE_MQTT)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MQTT)
    message(STATUS "MQTT protocol support enabled")
endif()

# PostgreSQL Wire Protocol Support
if(THEMIS_ENABLE_POSTGRES_WIRE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_POSTGRES_WIRE)
    message(STATUS "PostgreSQL Wire Protocol support enabled")
endif()

# gRPC Protocol Support (inter-shard communication - implemented in v1.3.0)
if(THEMIS_ENABLE_GRPC)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GRPC)
    message(STATUS "gRPC support enabled (inter-shard communication)")
    find_package(gRPC CONFIG)
    find_package(Protobuf CONFIG)
    if(gRPC_FOUND AND Protobuf_FOUND)
        target_link_libraries(themis_core PUBLIC gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf)
        message(STATUS "gRPC and Protobuf found - linking to themis_core")

        if(COMMAND protobuf_generate)
            # Generate shard RPC stubs (ApplyWalBatch, etc.)
            set(SHARD_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/proto/sharding/shard_rpc.proto)
            set(SHARD_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto_generated)
            file(MAKE_DIRECTORY ${SHARD_PROTO_GEN_DIR})

            # Manually generate protobuf files
            set(SHARD_PROTO_SRCS
                ${SHARD_PROTO_GEN_DIR}/shard_rpc.pb.cc
                ${SHARD_PROTO_GEN_DIR}/shard_rpc.grpc.pb.cc
            )
            set(SHARD_PROTO_HDRS
                ${SHARD_PROTO_GEN_DIR}/shard_rpc.pb.h
                ${SHARD_PROTO_GEN_DIR}/shard_rpc.grpc.pb.h
            )

            add_custom_command(
                OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                COMMAND $<TARGET_FILE:protobuf::protoc>
                ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                     --grpc_out ${SHARD_PROTO_GEN_DIR}
                     --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                     -I ${CMAKE_CURRENT_SOURCE_DIR}/proto/sharding
                     ${SHARD_PROTO}
                DEPENDS ${SHARD_PROTO} protobuf::protoc gRPC::grpc_cpp_plugin
                COMMENT "Generating protobuf and gRPC sources for shard_rpc.proto"
                VERBATIM
            )

            add_library(themis_shard_proto STATIC ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS})
            target_include_directories(themis_shard_proto PUBLIC ${SHARD_PROTO_GEN_DIR})
            target_link_libraries(themis_shard_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

            target_link_libraries(themis_core PUBLIC themis_shard_proto)
        else()
            message(WARNING "protobuf_generate() not available; skipping shard RPC stub generation")
        endif()
    else()
        message(WARNING "gRPC/Protobuf not found - WAL gRPC service will be disabled at runtime")
    endif()
endif()

# WebSocket Protocol Support (placeholder for future implementation)
if(THEMIS_ENABLE_WEBSOCKET)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_WEBSOCKET)
    message(STATUS "WebSocket support enabled (Boost.Beast has native support)")
    # Note: Boost.Beast already provides WebSocket support, just needs integration
endif()

# MQTT Protocol Support (placeholder for future implementation)
if(THEMIS_ENABLE_MQTT)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MQTT)
    message(STATUS "MQTT support enabled (stub - full implementation pending)")
    # TODO: Add MQTT dependencies when implemented
    # find_package(PahoMqttCpp CONFIG)
endif()

# PostgreSQL Wire Protocol Support (placeholder for future implementation)
if(THEMIS_ENABLE_POSTGRES_WIRE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_POSTGRES_WIRE)
    message(STATUS "PostgreSQL Wire Protocol support enabled (stub - full implementation pending)")
endif()

# v1.1.0: vLLM Co-Location compile definitions
if(THEMIS_VLLM_COLOCATION)
    target_compile_definitions(themis_core PRIVATE
        THEMIS_VLLM_COLOCATION=1
        THEMIS_MAX_GPU_VRAM_MB=2048        # 2 GB per GPU (vLLM has rest)
        THEMIS_MAX_VECTOR_BATCH_SIZE=1024  # Small batches
        THEMIS_GPU_LOW_PRIORITY=1          # Lower than vLLM
        THEMIS_CPU_CORES_RESERVED=50       # 50 of 64 cores
        THEMIS_RAM_GB_ALLOCATED=200        # 200 of 256 GB
    )
    message(STATUS "vLLM Co-Location: Resource limits configured")
endif()

# v1.3.0: LLM Plugin Support with llama.cpp
if(THEMIS_ENABLE_LLM)
    # Add LLM plugin source files
    target_sources(themis_core PRIVATE
        src/llm/llama_wrapper.cpp
        src/llm/embedded_llm.cpp          # Embedded LLM facade for system-wide use
        src/llm/llm_plugin_manager.cpp
        src/llm/model_loader.cpp          # Ollama-style lazy loading
        src/llm/multi_lora_manager.cpp    # vLLM-style multi-LoRA
        src/llm/async_inference_engine.cpp # Async threading (independent from DB)
        # v1.3.1: Cache Reuse Integration (Phase 2)
        src/llm/model_metadata_cache.cpp  # Model metadata caching with ConcurrentCache
        src/llm/lora_metadata_cache.cpp   # LoRA metadata caching with ConcurrentCache
        src/llm/paged_block_manager.cpp   # Block management for PagedAttention (v1.4.0 prep)
        src/llm/kv_cache_buffer.cpp       # KV cache batching with VectorAutoBuffer (Phase 2.2)
        src/llm/llm_response_cache.cpp    # Response caching with SemanticCache (Phase 2.3)
        
        # Phase 4+5 Week 4-6: llama.cpp Integration
        src/llm/gguf_loader.cpp           # GGUF model file loader with memory-mapping
        src/llm/llamacpp_inference_engine.cpp # llama.cpp inference engine with PagedAttention
        
        # Week 13-14: Production Validation & Testing (P1 Enterprise Features)
        src/llm/production_validator.cpp  # Production-grade LLM validation and benchmarking
        
        # P1: Enterprise Features - LLM Inference Engine Improvements
        src/llm/continuous_batch_scheduler.cpp  # vLLM-style continuous batching
        src/llm/llm_prefix_cache.cpp           # Context caching with prefix reuse
        src/llm/grafana_metrics.cpp            # Prometheus/Grafana metrics integration
        src/llm/inference_engine_enhanced.cpp  # Enhanced inference engine with P1 features
        
        # P3: Vision Support (Multi-Modal)
        src/llm/vision_encoder.cpp             # CLIP vision encoder for LLaVA
        src/llm/gpu_memory_manager.cpp         # GPU memory management (CPU fallback when CUDA disabled)
        src/llm/kernel_fusion.cpp              # Kernel fusion with CPU fallback
    )
    
    # Add CUDA kernel fusion sources when CUDA is enabled
    if(THEMIS_ENABLE_CUDA)
        list(APPEND THEMIS_CORE_SOURCES
            src/llm/kernel_fusion.cu           # Flash Attention CUDA kernels
        )
        message(STATUS "LLM: CUDA kernel fusion enabled (Flash Attention)")
    endif()
    
    # Threading support required for async inference
    find_package(Threads REQUIRED)
    target_link_libraries(themis_core PRIVATE Threads::Threads)
    
    # Link llama.cpp library
    if(TARGET llama)
        target_link_libraries(themis_core PRIVATE llama)
        # Include headers for llama.cpp
        if(DEFINED LLAMA_SRC_DIR)
            target_include_directories(themis_core PRIVATE ${LLAMA_SRC_DIR}/include)
        endif()
        message(STATUS "LLM Plugin: Linked llama.cpp library")
    else()
        message(WARNING "LLM Plugin: llama.cpp target not found - plugin will be built without llama.cpp")
    endif()
    
    # LLM is already defined via THEMIS_LLM_ENABLED in earlier section
    message(STATUS "LLM Plugin: Source files added to themis_core")
    message(STATUS "  - Ollama-style lazy model loading enabled")
    message(STATUS "  - vLLM-style multi-LoRA management enabled")
    message(STATUS "  - Async inference engine enabled (independent threading)")
    message(STATUS "  - ThemisDB cache reuse integration enabled (Phases 2.1-2.3)")
    message(STATUS "    * ModelMetadataCache (ConcurrentCache)")
    message(STATUS "    * LoRAMetadataCache (ConcurrentCache)")
    message(STATUS "    * PagedBlockManager (foundation for PagedAttention v1.4.0)")
    message(STATUS "    * KVCacheBuffer (VectorAutoBuffer for KV cache batching - Phase 2.2)")
    message(STATUS "    * LLMResponseCache (SemanticCache for response caching - Phase 2.3 - 75x faster)")
endif()


# Propagate Geo feature tiering compile definitions
if(THEMIS_GEO)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_ENABLED)
endif()
if(THEMIS_GEO_SIMD)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_SIMD_ENABLED)
endif()
if(THEMIS_GEO_GPU)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_GPU_ENABLED)
endif()
if(THEMIS_GEO_H3)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_H3_ENABLED)
endif()
if(THEMIS_GEO_GEOS_PLUGIN)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_GEOS_PLUGIN_ENABLED)
endif()
if(THEMIS_ENTERPRISE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENTERPRISE_ENABLED)
endif()

if(THEMIS_ENABLE_TRACING)
    # Defensive checks: Ensure OpenTelemetry targets exist before linking
    if(NOT TARGET opentelemetry-cpp::trace)
        message(FATAL_ERROR "Required CMake target 'opentelemetry-cpp::trace' not found. "
                            "Ensure opentelemetry-cpp was found successfully.")
    endif()
    if(NOT TARGET opentelemetry-cpp::otlp_http_exporter)
        message(FATAL_ERROR "Required CMake target 'opentelemetry-cpp::otlp_http_exporter' not found. "
                            "Ensure opentelemetry-cpp was found successfully with otlp-http feature.")
    endif()
    
    target_link_libraries(themis_core PUBLIC
        opentelemetry-cpp::trace
        opentelemetry-cpp::otlp_http_exporter
    )
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_TRACING)
endif()

# Main executable (demo)
add_executable(themis_demo
    src/main.cpp
)

target_link_libraries(themis_demo
    PRIVATE
        themis_core
)

# Encryption demo executable
add_executable(themis_demo_encryption
    src/demo_encryption.cpp
)

target_link_libraries(themis_demo_encryption
    PRIVATE
        themis_core
)

# API Server executable
add_executable(themis_server
    src/main_server.cpp
)

target_link_libraries(themis_server
    PRIVATE
        themis_core
)

# Link performance optimization libraries
if(THEMIS_ENABLE_MIMALLOC AND mimalloc_FOUND)
    target_link_libraries(themis_server PRIVATE mimalloc)
    message(STATUS "Linking themis_server with mimalloc")
endif()

# Tests
if(THEMIS_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    
    add_executable(themis_tests
        tests/test_env.cpp
        tests/test_base_entity.cpp
        tests/test_rocksdb_high_parallel_tuning.cpp
        tests/test_rocksdb_stats_json.cpp
        # Library integration tests (systematic coverage)
        tests/test_lib_rocksdb_integration.cpp
        tests/test_lib_openssl_integration.cpp
        tests/test_lib_json_integration.cpp
        tests/test_lib_hnsw_integration.cpp
        tests/test_lib_boost_integration.cpp
        tests/test_lib_tbb_integration.cpp
        tests/test_secondary_index.cpp
        tests/test_composite_index.cpp
        tests/test_unique_index.cpp
        tests/test_range_index.cpp
        tests/test_sparse_geo_index.cpp
        tests/test_ttl_fulltext_index.cpp
        tests/test_index_stats.cpp
        tests/test_graph_index.cpp
        tests/test_vector_index.cpp
        tests/test_query_engine.cpp
        tests/test_query_or.cpp
        tests/test_stats_api.cpp
        tests/test_http_range_index.cpp
              tests/test_stemming.cpp
              tests/test_stopwords.cpp
              tests/test_normalization.cpp
              tests/test_phrase_search.cpp
              tests/test_aql_fulltext.cpp
              tests/test_aql_or.cpp
              tests/test_aql_fulltext_hybrid.cpp
              tests/test_aql_bm25.cpp
              tests/test_aql_similarity.cpp
              tests/test_aql_similarity_dispatch.cpp
              tests/test_aql_proximity.cpp
              tests/test_aql_proximity_dispatch.cpp
        tests/test_metrics_api.cpp
        tests/test_http_query_range.cpp
        tests/test_http_aql.cpp
        tests/test_http_aql_fulltext_score.cpp
        tests/test_http_aql_fulltext_or.cpp
    tests/test_http_aql_join.cpp
    tests/test_http_aql_let.cpp
        tests/test_http_aql_collect.cpp
        tests/test_http_config.cpp
        tests/test_http_aql_graph.cpp
        tests/test_crash_recovery_direct.cpp
        tests/test_http_aql_collect.cpp
        tests/test_http_vector.cpp
        tests/test_http_vector_largescale.cpp
        tests/test_http_content.cpp
        tests/test_video_processor_extended.cpp
        tests/test_http_changefeed.cpp
        tests/test_http_changefeed_sse.cpp
        tests/test_http_timeseries.cpp
    tests/test_stemming.cpp
        tests/test_stopwords.cpp
        tests/test_http_governance.cpp
        
        # Geo tests
        tests/geo/test_geo_ewkb.cpp
        tests/geo/test_spatial_index.cpp
        tests/geo/test_aql_st_functions.cpp
        tests/geo/test_aql_st_queryengine.cpp
        tests/geo/test_geo_3d_functions.cpp
        tests/test_geo_index_integration.cpp
        tests/test_aql_let_st.cpp
        tests/test_hybrid_queries.cpp
        tests/test_http_audit.cpp
        tests/test_transaction_manager.cpp
        tests/test_transaction_isolation.cpp
        tests/test_changefeed_ordering.cpp
        tests/test_gorilla_codec_edge_cases.cpp
        tests/test_saga_concurrent_execution.cpp
        tests/test_consistent_hash_distribution.cpp
        tests/test_vulkan_backend.cpp
        tests/test_postgres_importer_datatypes.cpp
        tests/test_plugin_lifecycle.cpp
        tests/test_mvcc.cpp
        tests/test_aql_parser.cpp
        tests/test_aql_with_clause.cpp
        tests/test_aql_subqueries.cpp
        tests/test_cte_cache.cpp
        tests/test_aql_translator.cpp
        tests/test_text_processor.cpp
    tests/test_prompt_manager.cpp
    tests/test_mock_clip.cpp
        tests/test_image_analysis_interface.cpp
        tests/test_image_analysis_quality.cpp
        tests/test_backup_restore.cpp
        tests/test_raid5_backup.cpp
        tests/test_cursor.cpp
        tests/test_temporal_graph.cpp
        tests/test_temporal_aggregation.cpp
    tests/test_temporal_aggregation_property.cpp
        tests/temporal/test_temporal_conflict_resolver.cpp
        tests/test_recursive_path_query.cpp
        tests/test_timerange_query.cpp
        tests/test_property_graph.cpp
        tests/test_graph_type_filtering.cpp
        tests/test_gnn_embeddings.cpp
        tests/test_semantic_cache.cpp
        tests/test_tsstore.cpp
        tests/test_gorilla.cpp
    tests/test_gorilla_probe.cpp
        tests/test_timeseries_retention.cpp
        tests/test_continuous_agg.cpp
        tests/test_adaptive_index.cpp
        tests/test_http_adaptive_index.cpp
        tests/test_encryption.cpp
    tests/test_hkdf_cache.cpp
    tests/test_field_encryption_batch.cpp
    tests/test_pki_eidas.cpp
    tests/test_pki_api_handler.cpp
    tests/test_wal_archiving.cpp
        tests/test_encryption_e2e.cpp
        tests/test_vault_key_provider.cpp
        tests/test_vault_key_provider_retry.cpp
    tests/test_wal_backup_manager.cpp
    tests/test_wal_grpc_apply.cpp
        tests/test_wal_replication_integration.cpp
        tests/test_audit_logger.cpp
        tests/test_pii_detector.cpp
        tests/test_retention_manager.cpp
        tests/test_saga_logger.cpp
        tests/test_http_retention_api.cpp
        tests/test_policy_yaml.cpp
        tests/test_pki_client.cpp
    tests/test_cms_signing.cpp
        tests/test_keyprovider_signing.cpp
    tests/test_vault_signing_provider.cpp
        # Enterprise tests (only if enterprise source is available)
        $<$<AND:$<BOOL:${THEMIS_BUILD_ENTERPRISE}>,$<TARGET_EXISTS:themis_enterprise_common>>:tests/test_enterprise_scalability.cpp>
        tests/test_http_policies_export.cpp
        tests/test_pii_soft_delete.cpp
        tests/test_http_pii_lazy_init.cpp
    tests/test_http_pii_manager_new.cpp
    tests/test_jwt_validator.cpp
    tests/test_jwt_integration.cpp
        tests/test_jwt_rotation_unit.cpp
        tests/test_llm_api_handler_jwt.cpp
        tests/test_pki_client_rest.cpp
        tests/test_group_dek.cpp
        tests/test_input_validator.cpp
        tests/test_sharding_core.cpp
        tests/test_raid_redundancy.cpp
        tests/test_hot_spare.cpp
        tests/test_lora_adapter.cpp
        tests/test_raid_integration.cpp
        tests/test_stream_protocol_extended.cpp
        tests/test_cloud_agent.cpp
        tests/test_pki_shard_certificate.cpp
        tests/test_mtls_client.cpp
        tests/test_signed_request.cpp
        tests/test_shard_communication.cpp
        tests/test_rebalance_migration.cpp
        tests/test_monitoring_ops.cpp
        tests/update_checker_test.cpp
        
        # New feature tests
        tests/test_tenant_manager.cpp
        tests/test_graphql.cpp
        tests/test_olap.cpp
        tests/test_olap_extended.cpp
        tests/test_process_mining_extended.cpp
        tests/test_async_io_multiscan.cpp  # v1.3.0 Phase 2: Async I/O MultiScan tests
        
        # Phase 1 Performance Optimization Tests (from PR #157)
        tests/test_performance_feature_flags.cpp
        tests/test_performance_allocator.cpp
        tests/test_huge_pages.cpp
        tests/test_rcu_index.cpp
        tests/test_lirs_cache.cpp
        
        # Phase 2 Performance Optimization Tests (from this PR)
        tests/test_phase2_optimizations.cpp
        
        # Protocol tests (conditional based on build flags)
        $<$<BOOL:${THEMIS_ENABLE_HTTP2}>:tests/test_http2_protocol.cpp>
        $<$<BOOL:${THEMIS_ENABLE_HTTP2}>:tests/test_http2_server_push.cpp>
        $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:tests/test_http3_protocol.cpp>
        $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:tests/test_websocket_cdc.cpp>
        $<$<BOOL:${THEMIS_ENABLE_MQTT}>:tests/test_mqtt_protocol.cpp>
        $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:tests/test_postgres_wire.cpp>
        $<$<BOOL:${THEMIS_ENABLE_MCP}>:tests/test_mcp_protocol.cpp>
        
        # Vector encryption integration tests
        tests/test_vector_encryption_integration.cpp
        
        # LLM plugin tests (v1.3.0+) are added conditionally below
    )
    
    target_link_libraries(themis_tests
        PRIVATE
            themis_core
            GTest::gtest
            GTest::gtest_main
            Boost::system
    )

    # Leichtgewichtiger Test-Target: Nur SecondaryIndex-Tests
    add_executable(themis_secidx_tests
        tests/test_secondary_index.cpp
    )
    target_link_libraries(themis_secidx_tests
        PRIVATE
            themis_core
            GTest::gtest
            GTest::gtest_main
            Boost::system
    )
    include(GoogleTest)
    gtest_discover_tests(themis_secidx_tests
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY $<TARGET_FILE_DIR:themis_secidx_tests>
        EXTRA_ARGS --gtest_color=yes
        CONFIGURATIONS Release Debug
    )

    # CRITICAL TESTS ONLY (for fast validation)
    add_executable(themis_tests_critical
        tests/test_lib_rocksdb_integration.cpp
        tests/test_lib_hnsw_integration.cpp
        tests/test_lib_boost_integration.cpp
        tests/test_secondary_index.cpp
    )
    target_link_libraries(themis_tests_critical
        PRIVATE
            themis_core
            GTest::gtest
            GTest::gtest_main
            Boost::system
    )
    include(GoogleTest)
    gtest_discover_tests(themis_tests_critical
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY $<TARGET_FILE_DIR:themis_tests_critical>
        EXTRA_ARGS --gtest_color=yes
        CONFIGURATIONS Release Debug
    )

    # Conditionally add LLM tests only when LLM is enabled
    if(THEMIS_ENABLE_LLM)
        target_sources(themis_tests PRIVATE
            tests/test_llm_plugin.cpp
            tests/test_llamacpp_integration.cpp
            tests/test_llm_caching.cpp
            tests/test_kv_cache_buffer.cpp
            tests/test_llm_response_cache.cpp
            tests/test_paged_attention.cpp
            tests/test_continuous_batch_scheduler.cpp
            tests/test_production_validator.cpp  # P1 Enterprise: Production validation tests
            tests/test_llm_grafana_metrics.cpp   # P1 Enterprise: Grafana metrics integration tests
            tests/test_llm_response_cache_metrics.cpp  # P1 Enterprise: Response cache metrics tests
            tests/test_inference_engine_enhanced.cpp  # P1: Enhanced inference engine tests
        )

        # Lightweight LLM-only test target to avoid unrelated suite breakages
        add_executable(themis_llm_tests
            tests/test_llamacpp_integration.cpp
        )
        target_link_libraries(themis_llm_tests
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
                Boost::system
        )
            include(GoogleTest)
        gtest_discover_tests(themis_llm_tests
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:themis_llm_tests>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
        
        # Phase 1 Feature Tests (Flash Attention, KV-Cache Reuse, Embeddings)
        # Flash Attention Tests
        add_executable(test_phase1_flash_attention
            tests/test_phase1_flash_attention.cpp
        )
        target_link_libraries(test_phase1_flash_attention
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_phase1_flash_attention
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_phase1_flash_attention>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
        
        # KV-Cache Reuse Tests
        add_executable(test_phase1_kv_cache_reuse
            tests/test_phase1_kv_cache_reuse.cpp
        )
        target_link_libraries(test_phase1_kv_cache_reuse
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_phase1_kv_cache_reuse
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_phase1_kv_cache_reuse>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
    endif()

    # Discover individual GoogleTest tests for better CTest integration
    include(GoogleTest)
    # On multi-config generators (MSVC), ensure correct working dir for runtime assets
    if(CMAKE_CONFIGURATION_TYPES)
        set(_gtest_workdir "$<TARGET_FILE_DIR:themis_tests>")
    else()
        set(_gtest_workdir ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    gtest_discover_tests(themis_tests
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY ${_gtest_workdir}
        EXTRA_ARGS --gtest_color=yes
    )

    # Note: Do not add an aggregate test that runs the entire suite again.
    # gtest_discover_tests already registers each test case with CTest. Adding
    # an extra `add_test(NAME ThemisTests COMMAND themis_tests ...)` would run
    # the full suite a second time and can make CI appear to hang on the last
    # test (previously "Test #619: ThemisTests").
    
    # Phase 3 Performance Optimization Tests (separate executables)
    if(THEMIS_ENABLE_DISKANN)
        add_executable(test_diskann
            tests/performance/phase3/test_diskann.cpp
        )
        target_link_libraries(test_diskann
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_diskann
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY ${_gtest_workdir}
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_BWTREE)
        add_executable(test_bwtree
            tests/performance/phase3/test_bwtree.cpp
        )
        target_link_libraries(test_bwtree
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_bwtree
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY ${_gtest_workdir}
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_SPLINTERDB)
        add_executable(test_splinterdb
            tests/performance/phase3/test_splinterdb.cpp
        )
        target_link_libraries(test_splinterdb
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_splinterdb
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY ${_gtest_workdir}
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_GUNROCK)
        add_executable(test_gunrock
            tests/performance/phase3/test_gunrock.cpp
        )
        target_link_libraries(test_gunrock
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_gunrock
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY ${_gtest_workdir}
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_BAO)
        add_executable(test_bao
            tests/performance/phase3/test_bao.cpp
        )
        target_link_libraries(test_bao
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_bao
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY ${_gtest_workdir}
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
endif()

# Benchmarks
option(THEMIS_BUILD_DOCKER_RAID_BENCHMARK "Build Docker RAID comprehensive benchmark" OFF)

if(THEMIS_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
    
    # Modern performance benchmarks (all core areas)
    add_executable(bench_core_performance
        benchmarks/bench_core_performance.cpp
    )
    
    target_link_libraries(bench_core_performance
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Comprehensive benchmark suite (simple/complex, LLM, AQL, binary, stress tests)
    add_executable(bench_comprehensive
        benchmarks/bench_comprehensive.cpp
    )
    
    target_link_libraries(bench_comprehensive
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Advanced patterns: Read/Write ratios, Parallelity, Self-Protection, Best-Practices, Gap-Analysis
    add_executable(bench_advanced_patterns
        benchmarks/bench_advanced_patterns.cpp
    )
    
    target_link_libraries(bench_advanced_patterns
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    add_executable(bench_hotspots_micro
        benchmarks/bench_hotspots_micro.cpp
    )

    target_link_libraries(bench_hotspots_micro
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    add_executable(bench_lock_contention
        benchmarks/bench_lock_contention.cpp
    )
    target_link_libraries(bench_lock_contention
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    add_executable(bench_wal_stress
        benchmarks/bench_wal_stress.cpp
    )
    target_link_libraries(bench_wal_stress
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(themis_benchmarks
        benchmarks/bench_crud.cpp
        benchmarks/bench_query.cpp
        benchmarks/bench_vector_search.cpp
        benchmarks/bench_wal_apply_grpc.cpp
    )
    
    target_link_libraries(themis_benchmarks
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Compression benchmark (standalone)
    add_executable(bench_compression
        benchmarks/bench_compression.cpp
    )
    
    target_link_libraries(bench_compression
        PRIVATE
            themis_core
            benchmark::benchmark
    )
    
    # Index rebuild benchmark (standalone)
    add_executable(bench_index_rebuild
        benchmarks/bench_index_rebuild.cpp
    )
    
    target_link_libraries(bench_index_rebuild
        PRIVATE
            themis_core
            benchmark::benchmark
    )
    
    # MVCC performance benchmark (standalone)
    add_executable(bench_mvcc
        benchmarks/bench_mvcc.cpp
    )
    
    target_link_libraries(bench_mvcc
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Encryption performance benchmark (standalone)
    add_executable(bench_encryption
        benchmarks/bench_encryption.cpp
    )
    
    target_link_libraries(bench_encryption
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # OLAP Analytics benchmark (v1.3.0 Phase 2)
    add_executable(bench_olap_analytics
        benchmarks/bench_olap_analytics.cpp
    )
    
    target_link_libraries(bench_olap_analytics
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Video Processor benchmark (v1.3.0 Phase 2)
    add_executable(bench_video_processor
        benchmarks/bench_video_processor.cpp
    )
    
    target_link_libraries(bench_video_processor
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Stream Protocol benchmark (v1.3.0 Phase 2)
    add_executable(bench_stream_protocol
        benchmarks/bench_stream_protocol.cpp
    )
    
    target_link_libraries(bench_stream_protocol
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Process Mining benchmark (v1.3.0 Phase 2)
    add_executable(bench_process_mining
        benchmarks/bench_process_mining.cpp
    )
    
    target_link_libraries(bench_process_mining
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Async I/O MultiScan benchmark (v1.3.0 Phase 2)
    add_executable(bench_async_io_multiscan
        benchmarks/bench_async_io_multiscan.cpp
    )
    
    target_link_libraries(bench_async_io_multiscan
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Hybrid AQL Syntax Sugar benchmark (Phase 2.5)
    add_executable(bench_hybrid_aql_sugar
        benchmarks/bench_hybrid_aql_sugar.cpp
    )
    
    target_link_libraries(bench_hybrid_aql_sugar
        PRIVATE
            themis_core
            benchmark::benchmark
    )

    # HSM Provider performance benchmark (standalone)
    add_executable(bench_hsm_provider
        benchmarks/bench_hsm_provider.cpp
    )
    
    target_link_libraries(bench_hsm_provider
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Phase 1 Benchmarks - Critical Performance Testing

    # Shard Routing benchmark
    add_executable(bench_shard_routing
        benchmarks/bench_shard_routing.cpp
    )
    
    target_link_libraries(bench_shard_routing
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Transaction Throughput benchmark
    add_executable(bench_transaction_throughput
        benchmarks/bench_transaction_throughput.cpp
    )
    
    target_link_libraries(bench_transaction_throughput
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Changefeed Throughput benchmark
    add_executable(bench_changefeed_throughput
        benchmarks/bench_changefeed_throughput.cpp
    )
    
    target_link_libraries(bench_changefeed_throughput
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Timeseries Ingestion benchmark
    add_executable(bench_timeseries_ingestion
        benchmarks/bench_timeseries_ingestion.cpp
    )
    
    target_link_libraries(bench_timeseries_ingestion
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # SAGA Compensation benchmark
    add_executable(bench_saga_compensation
        benchmarks/bench_saga_compensation.cpp
    )
    
    target_link_libraries(bench_saga_compensation
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Phase 1 Benchmarks - LLM Optimizations (Flash Attention, KV-Cache, Embeddings)
    
    # Flash Attention Benchmark
    add_executable(bench_phase1_flash_attention
        benchmarks/bench_phase1_flash_attention.cpp
    )
    
    target_link_libraries(bench_phase1_flash_attention
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Phase 2 Benchmarks - GPU Backends and Graph Analytics

    # Graph Traversal benchmark
    add_executable(bench_graph_traversal
        benchmarks/bench_graph_traversal.cpp
    )
    
    target_link_libraries(bench_graph_traversal
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # PageRank benchmark
    add_executable(bench_pagerank
        benchmarks/bench_pagerank.cpp
    )
    
    target_link_libraries(bench_pagerank
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # GPU Backends benchmark
    add_executable(bench_gpu_backends
        benchmarks/bench_gpu_backends.cpp
    )
    
    target_link_libraries(bench_gpu_backends
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # GNN Embeddings benchmark
    add_executable(bench_gnn_embeddings
        benchmarks/bench_gnn_embeddings.cpp
    )
    
    target_link_libraries(bench_gnn_embeddings
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Phase 3: Content Management & Governance Benchmarks
    add_executable(bench_content_versioning
        benchmarks/bench_content_versioning.cpp
    )
    
    target_link_libraries(bench_content_versioning
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_text_extraction
        benchmarks/bench_text_extraction.cpp
    )
    
    target_link_libraries(bench_text_extraction
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_image_analysis
        benchmarks/bench_image_analysis.cpp
    )
    
    target_link_libraries(bench_image_analysis
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_image_analysis_latency
        benchmarks/bench_image_analysis_latency.cpp
    )
    
    target_link_libraries(bench_image_analysis_latency
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_policy_evaluation
        benchmarks/bench_policy_evaluation.cpp
    )
    
    target_link_libraries(bench_policy_evaluation
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Image Analysis Plugin Benchmark
    add_executable(benchmark_image_analysis
        benchmarks/benchmark_image_analysis.cpp
    )
    
    target_link_libraries(benchmark_image_analysis
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Scientific Standard Benchmarks (Phase 2-4)
    # TPC-C: OLTP benchmark (Transaction Processing Council Standard)
    add_executable(bench_tpcc
        benchmarks/bench_tpcc.cpp
    )
    
    target_link_libraries(bench_tpcc
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # YCSB: Yahoo Cloud Serving Benchmark (Workloads A-F)
    add_executable(bench_ycsb
        benchmarks/bench_ycsb.cpp
    )
    
    target_link_libraries(bench_ycsb
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # MMDB-E: Multi-Modal Database Benchmark with Embeddings (Industry-First)
    add_executable(bench_mmdb
        benchmarks/bench_mmdb.cpp
    )
    
    target_link_libraries(bench_mmdb
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # TPC-H: Decision Support (OLAP) Benchmark - 22 Analytical Queries
    add_executable(bench_tpch
        benchmarks/bench_tpch.cpp
    )
    
    target_link_libraries(bench_tpch
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # V1.3.0 Additional Benchmarks (previously missing)
    
    # AQL Functions Benchmark
    add_executable(bench_aql_functions
        benchmarks/bench_aql_functions.cpp
    )
    target_link_libraries(bench_aql_functions PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # ARM Memory Benchmark
    add_executable(bench_arm_memory
        benchmarks/bench_arm_memory.cpp
    )
    target_link_libraries(bench_arm_memory PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # ARM SIMD Benchmark
    add_executable(bench_arm_simd
        benchmarks/bench_arm_simd.cpp
    )
    target_link_libraries(bench_arm_simd PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Auto Buffers Benchmark
    add_executable(bench_auto_buffers
        benchmarks/bench_auto_buffers.cpp
    )
    target_link_libraries(bench_auto_buffers PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Blob ZSTD Compression Benchmark
    add_executable(bench_blob_zstd
        benchmarks/bench_blob_zstd.cpp
    )
    target_link_libraries(bench_blob_zstd PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # HNSW Prefilter Minimal Benchmark
    add_executable(bench_hnsw_prefilter_minimal
        benchmarks/bench_hnsw_prefilter_minimal.cpp
    )
    target_link_libraries(bench_hnsw_prefilter_minimal PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Hybrid Vector+Geo Benchmark
    add_executable(bench_hybrid_vector_geo
        benchmarks/bench_hybrid_vector_geo.cpp
    )
    target_link_libraries(bench_hybrid_vector_geo PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Lossy vs Lossless Compression Benchmark
    add_executable(bench_lossy_vs_lossless
        benchmarks/bench_lossy_vs_lossless.cpp
    )
    target_link_libraries(bench_lossy_vs_lossless PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Product Quantization Benchmark
    add_executable(bench_product_quantization
        benchmarks/bench_product_quantization.cpp
    )
    target_link_libraries(bench_product_quantization PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Sanity Tests Benchmark
    add_executable(bench_sanity
        benchmarks/bench_sanity.cpp
    )
    target_link_libraries(bench_sanity PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Sharding Performance Benchmark
    add_executable(bench_sharding_performance
        benchmarks/bench_sharding_performance.cpp
    )
    target_link_libraries(bench_sharding_performance PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # SIMD Distance Benchmark
    add_executable(bench_simd_distance
        benchmarks/bench_simd_distance.cpp
    )
    target_link_libraries(bench_simd_distance PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Spatial Index Benchmark
    add_executable(bench_spatial_index
        benchmarks/bench_spatial_index.cpp
    )
    target_link_libraries(bench_spatial_index PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # V1.3.0 Features Benchmark (Critical for Release Testing)
    add_executable(bench_v1_3_0_features
        benchmarks/bench_v1_3_0_features.cpp
    )
    target_link_libraries(bench_v1_3_0_features PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Vector Compression Lossless Benchmark
    add_executable(bench_vector_compression_lossless
        benchmarks/bench_vector_compression_lossless.cpp
    )
    target_link_libraries(bench_vector_compression_lossless PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Vector Prefilter Benchmark
    add_executable(bench_vector_prefilter
        benchmarks/bench_vector_prefilter.cpp
    )
    target_link_libraries(bench_vector_prefilter PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Phase 2 Performance Optimization Benchmarks
    add_executable(bench_phase2
        benchmarks/performance_optimizations/phase2/benchmark_phase2.cpp
    )
    target_link_libraries(bench_phase2 PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # v1.3.4 Optimization Benchmarks
    add_executable(bench_v1_3_4_optimizations
        benchmarks/bench_v1_3_4_optimizations.cpp
    )
    target_link_libraries(bench_v1_3_4_optimizations PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # v1.3.4 Batch Insert Benchmark
    add_executable(bench_batch_insert
        benchmarks/bench_batch_insert.cpp
    )
    target_link_libraries(bench_batch_insert PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Simple insert test for debugging
    add_executable(bench_simple_insert_test
        benchmarks/bench_simple_insert_test.cpp
    )
    target_link_libraries(bench_simple_insert_test PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # RAID and LoRA Adapter Benchmark
    add_executable(bench_raid_lora
        benchmarks/bench_raid_lora.cpp
    )
    target_link_libraries(bench_raid_lora PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

endif()

if(THEMIS_BUILD_DOCKER_RAID_BENCHMARK)
    find_package(benchmark CONFIG REQUIRED)
    find_package(prometheus-cpp CONFIG REQUIRED)
    # Docker RAID Comprehensive Benchmark (v1.3.4+)
    # 1+ hour benchmark suite for Docker multi-container RAID testing
    # Tests RAID0, RAID1, RAID5, RAID6, RAID10 configurations
    # See: benchmarks/DOCKER_RAID_BENCHMARK_SUITE_README.md
    add_executable(bench_docker_raid_comprehensive
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/bench_docker_raid_comprehensive.cpp
    )
    target_link_libraries(bench_docker_raid_comprehensive PRIVATE 
        themis_core 
        benchmark::benchmark 
        benchmark::benchmark_main
        prometheus-cpp::pull
        prometheus-cpp::core
    )
endif()

# Admin Tools (.NET)
option(THEMIS_BUILD_ADMIN_TOOLS "Build .NET admin tools (requires .NET SDK 8.0+)" OFF)
if(THEMIS_BUILD_ADMIN_TOOLS)
    add_subdirectory(tools)
endif()

# Enterprise Plugins
option(THEMIS_BUILD_ENTERPRISE_PLUGINS "Build enterprise plugins" ON)
if(THEMIS_BUILD_ENTERPRISE_PLUGINS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins")
    add_subdirectory(plugins)
endif()

# Enterprise Modules (new modular DLL architecture)
option(THEMIS_BUILD_ENTERPRISE "Enable enterprise module builds" OFF)
if(THEMIS_BUILD_ENTERPRISE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/enterprise")
        add_subdirectory(src/enterprise)
    else()
        message(WARNING "Enterprise source code not found. Enterprise features require a commercial license. Contact sales@themisdb.com")
        set(THEMIS_BUILD_ENTERPRISE OFF CACHE BOOL "Enterprise source not available" FORCE)
    endif()
endif()

# Installation
install(TARGETS themis_server themis_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)

# Performance Optimization Benchmarks
if(THEMIS_BUILD_BENCHMARKS)
    # add_subdirectory(benchmarks/performance_optimizations) # Directory does not exist
endif()

