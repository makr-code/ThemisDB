cmake_minimum_required(VERSION 3.20)

# VERSION ist Single-Source-of-Truth: zwingend aus Datei lesen
if(NOT EXISTS "${THEMIS_ROOT_DIR}/VERSION")
    message(FATAL_ERROR "VERSION file not found at project root. Please create and set semantic version (e.g., 1.2.3).")
endif()
file(READ "${THEMIS_ROOT_DIR}/VERSION" _ver)
string(STRIP "${_ver}" _ver)
# Accept semantic versions with optional pre-release/build suffixes, but sanitize for CMake's project(VERSION)
if(NOT _ver MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+(.*)?$")
    message(FATAL_ERROR "Invalid VERSION content: '${_ver}'. Expected semantic version like 1.2.3")
endif()
# Extract numeric MAJOR.MINOR.PATCH for CMake's version field
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" _ver_numeric "${_ver}")
if(NOT _ver_numeric)
    message(FATAL_ERROR "Failed to parse numeric version from '${_ver}'")
endif()

project(Themis VERSION ${_ver_numeric} LANGUAGES CXX)

# Make project version available to all translation units
add_compile_definitions(THEMIS_VERSION_STRING="${_ver}")

# Include modular build configuration (post-v1.3.0 feature)
# See docs/architecture/MODULARIZATION_PLAN.md for details
include(${CMAKE_CURRENT_SOURCE_DIR}/ModularBuild.cmake)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Runtime library for static linking on Windows
if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

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

# Office document support (DOCX/XLSX/PPTX)
# Find libzip (needed for both Office and Archive processing)
find_package(libzip CONFIG)
if(NOT libzip_FOUND)
    find_package(libzip)
endif()

if(libzip_FOUND)
    message(STATUS "libzip found - Archive processing available")
else()
    message(WARNING "libzip not found - Install with: vcpkg install libzip")
    message(WARNING "Archive processing will not be available")
endif()

# Office support requires both libzip and pugixml
if(THEMIS_ENABLE_OFFICE)
    find_package(pugixml CONFIG)
    if(NOT pugixml_FOUND)
        find_package(pugixml)
    endif()
    
    if(libzip_FOUND AND pugixml_FOUND)
        message(STATUS "Office support enabled - libzip and pugixml found")
        add_compile_definitions(THEMIS_ENABLE_OFFICE)
    else()
        if(NOT pugixml_FOUND)
            message(WARNING "pugixml not found - Install with: vcpkg install pugixml")
        endif()
        message(WARNING "Disabling Office support due to missing dependencies")
        set(THEMIS_ENABLE_OFFICE OFF CACHE BOOL "Office support disabled - missing dependencies" FORCE)
    endif()
endif()

# Build options
option(THEMIS_BUILD_TESTS "Build unit tests" ON)
option(THEMIS_BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(THEMIS_ENABLE_GPU "Enable GPU acceleration for vector search" ON)
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
option(THEMIS_ENABLE_LLM "Enable LLM plugin support with llama.cpp (v1.3.0+)" ON)

# Voice Assistant Support (v1.4.0+) - Enterprise Feature
# Requires: Whisper.cpp (STT), Piper TTS (TTS), llama.cpp (LLM)
option(THEMIS_ENABLE_VOICE_ASSISTANT "Enable Voice Assistant with STT/TTS/LLM (v1.4.0+)" OFF)
option(THEMIS_ENABLE_WHISPER "Enable Whisper.cpp for Speech-to-Text" OFF)
option(THEMIS_ENABLE_PIPER_TTS "Enable Piper TTS for Text-to-Speech" OFF)

# Hardware acceleration backend options
option(THEMIS_ENABLE_CUDA "Enable NVIDIA CUDA acceleration" OFF)
option(THEMIS_ENABLE_HIP "Enable AMD HIP acceleration" OFF)

# Geospatial Processing (P2 - v1.4.0+)
# Enables GDAL integration for Shapefile and GeoTIFF support
option(THEMIS_ENABLE_GDAL "Enable GDAL integration for geospatial data processing" OFF)

# Office Document Processing (P2 - v1.4.0+)
# Enables libzip and pugixml integration for DOCX/XLSX/PPTX support
option(THEMIS_ENABLE_OFFICE "Enable Office document support (DOCX/XLSX/PPTX) with libzip+pugixml" ON)

# Performance Optimization Options (Research-based, see docs/de/research/)
# Phase 1: Quick Wins - Low effort, high impact optimizations (from PR #157)
# NOTE: These are automatically enabled when building tests/benchmarks for accurate performance measurement
option(THEMIS_ENABLE_MIMALLOC "Use mimalloc allocator for +10-20% performance" OFF)
option(THEMIS_ENABLE_HUGE_PAGES "Enable huge pages support for +15-30% memory performance" OFF)
option(THEMIS_ENABLE_RCU_INDEX "Use RCU for read-heavy index paths (+200-500% read performance)" OFF)
option(THEMIS_ENABLE_LIRS_CACHE "Use LIRS cache replacement policy (+30-40% hit rate)" OFF)

# Auto-enable Phase 1 performance features for tests/benchmarks
if(THEMIS_BUILD_TESTS OR THEMIS_BUILD_BENCHMARKS)
    if(NOT DEFINED THEMIS_PERF_AUTO_ENABLE)
        set(THEMIS_PERF_AUTO_ENABLE ON CACHE BOOL "Auto-enable performance features for tests/benchmarks")
    endif()
    
    if(THEMIS_PERF_AUTO_ENABLE)
        message(STATUS "Auto-enabling Phase 1 performance optimizations for tests/benchmarks")
        message(STATUS "  (Disable with: cmake -DTHEMIS_PERF_AUTO_ENABLE=OFF)")
        
        # Only auto-enable if not explicitly set by user
        if(NOT THEMIS_ENABLE_MIMALLOC)
            set(THEMIS_ENABLE_MIMALLOC ON CACHE BOOL "Auto-enabled for tests/benchmarks" FORCE)
            message(STATUS "  → THEMIS_ENABLE_MIMALLOC=ON (auto)")
        endif()
        
        if(NOT THEMIS_ENABLE_RCU_INDEX)
            set(THEMIS_ENABLE_RCU_INDEX ON CACHE BOOL "Auto-enabled for tests/benchmarks" FORCE)
            message(STATUS "  → THEMIS_ENABLE_RCU_INDEX=ON (auto)")
        endif()
        
        if(NOT THEMIS_ENABLE_LIRS_CACHE)
            set(THEMIS_ENABLE_LIRS_CACHE ON CACHE BOOL "Auto-enabled for tests/benchmarks" FORCE)
            message(STATUS "  → THEMIS_ENABLE_LIRS_CACHE=ON (auto)")
        endif()
        
        # Huge Pages nur auf Linux auto-enablen
        if(NOT WIN32 AND NOT THEMIS_ENABLE_HUGE_PAGES)
            set(THEMIS_ENABLE_HUGE_PAGES ON CACHE BOOL "Auto-enabled for tests/benchmarks (Linux only)" FORCE)
            message(STATUS "  → THEMIS_ENABLE_HUGE_PAGES=ON (auto, Linux)")
        endif()
    endif()
endif()

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
        add_compile_definitions(THEMIS_ENABLE_MIMALLOC)
        
        # On Windows: exclude mimalloc-redirect.dll to prevent DLL-injection crash
        # Use only mimalloc.dll (static override is safer)
        if(WIN32)
            message(STATUS "Windows: Disabling mimalloc-redirect DLL to prevent loader crash")
            # Tell mimalloc to use static override instead of DLL redirection
            add_compile_definitions(MI_DISABLE_REDIRECT=1)
        endif()
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

# Lokal aktivierbare Code-Qualit�t (clang-tidy) ohne CI-Kosten.
option(ENABLE_LOCAL_CLANG_TIDY "Enable local clang-tidy analysis (off by default)" OFF)


# v1.1.0 Build Variants (Q1 2026)
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
    set(THEMIS_SHARDING_MAX_NODES 5 CACHE INTERNAL "Max sharding nodes for Community Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS OFF CACHE INTERNAL "Enterprise plugins disabled in Community")
    set(THEMIS_ENABLE_MULTI_MASTER OFF CACHE INTERNAL "Multi-master replication disabled in Community")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION OFF CACHE INTERNAL "Field-level encryption disabled in Community")
    set(THEMIS_ENABLE_RBAC OFF CACHE INTERNAL "RBAC disabled in Community")
    set(THEMIS_ENABLE_HSM OFF CACHE INTERNAL "HSM disabled in Community")
    
    # GPU acceleration: Vulkan as default (cross-platform, multi-vendor)
    # Vulkan works on NVIDIA, AMD, Intel GPUs across Windows/Linux/macOS/Android
    # CUDA is vendor-specific (NVIDIA only) and must be explicitly enabled
    if(NOT DEFINED THEMIS_ENABLE_VULKAN)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan GPU acceleration (default ON, cross-platform)")
    endif()
    
    # CUDA is OFF by default (NVIDIA-specific, use -DTHEMIS_ENABLE_CUDA=ON to enable)
    # When enabled alongside Vulkan, CUDA takes priority for NVIDIA GPUs
    if(NOT DEFINED THEMIS_ENABLE_CUDA)
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA GPU acceleration (default OFF, NVIDIA-specific)")
    endif()
    
    # LLM support enabled by default in Community Edition (v1.3.0+)
    if(NOT DEFINED THEMIS_ENABLE_LLM)
        set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM support in Community Edition (default ON)")
    endif()
    
    message(STATUS "Edition: COMMUNITY - GPU limited to ${THEMIS_GPU_MAX_VRAM_GB}GB, up to ${THEMIS_SHARDING_MAX_NODES} nodes")
    message(STATUS "  GPU Backend: Vulkan=${THEMIS_ENABLE_VULKAN} (cross-platform), CUDA=${THEMIS_ENABLE_CUDA} (NVIDIA-only)")
    message(STATUS "  LLM Support: ${THEMIS_ENABLE_LLM}")
    if(THEMIS_ENABLE_VULKAN)
        message(STATUS "  Vulkan will provide GPU acceleration on NVIDIA/AMD/Intel GPUs with CPU fallback")
    endif()

elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    set(THEMIS_GPU_MAX_VRAM_GB 256 CACHE INTERNAL "Max GPU VRAM for Enterprise Edition")
    set(THEMIS_SHARDING_MAX_NODES 100 CACHE INTERNAL "Max sharding nodes for Enterprise Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    
    # GPU acceleration: Vulkan as default (cross-platform, multi-vendor)
    if(NOT DEFINED THEMIS_ENABLE_VULKAN)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan GPU acceleration (default ON, cross-platform)")
    endif()
    
    # CUDA OFF by default (enable with -DTHEMIS_ENABLE_CUDA=ON for NVIDIA-specific optimizations)
    if(NOT DEFINED THEMIS_ENABLE_CUDA)
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA GPU acceleration (default OFF, NVIDIA-specific)")
    endif()
    
    # LLM support enabled by default
    if(NOT DEFINED THEMIS_ENABLE_LLM)
        set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM support in Enterprise Edition (default ON)")
    endif()
    
    message(STATUS "Edition: ENTERPRISE - GPU up to ${THEMIS_GPU_MAX_VRAM_GB}GB, up to ${THEMIS_SHARDING_MAX_NODES} nodes")
    message(STATUS "  GPU Backend: Vulkan=${THEMIS_ENABLE_VULKAN} (cross-platform), CUDA=${THEMIS_ENABLE_CUDA} (NVIDIA-only)")
    message(STATUS "  LLM Support: ${THEMIS_ENABLE_LLM}")

elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    set(THEMIS_GPU_MAX_VRAM_GB 999999 CACHE INTERNAL "Max GPU VRAM for Hyperscaler Edition (unlimited)")
    set(THEMIS_SHARDING_MAX_NODES 999999 CACHE INTERNAL "Max sharding nodes for Hyperscaler Edition (unlimited)")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    
    # GPU acceleration: Vulkan as default (cross-platform, multi-vendor)
    if(NOT DEFINED THEMIS_ENABLE_VULKAN)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan GPU acceleration (default ON, cross-platform)")
    endif()
    
    # CUDA OFF by default (enable with -DTHEMIS_ENABLE_CUDA=ON for NVIDIA-specific optimizations)
    if(NOT DEFINED THEMIS_ENABLE_CUDA)
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA GPU acceleration (default OFF, NVIDIA-specific)")
    endif()
    
    # LLM support enabled by default
    if(NOT DEFINED THEMIS_ENABLE_LLM)
        set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM support in Hyperscaler Edition (default ON)")
    endif()
    
    message(STATUS "Edition: HYPERSCALER - Unlimited GPU/VRAM and nodes")
    message(STATUS "  GPU Backend: Vulkan=${THEMIS_ENABLE_VULKAN} (cross-platform), CUDA=${THEMIS_ENABLE_CUDA} (NVIDIA-only)")
    message(STATUS "  LLM Support: ${THEMIS_ENABLE_LLM}")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    message(STATUS "Edition: HYPERSCALER - Unlimited GPU VRAM, unlimited nodes, full custom capabilities")

endif()

# ============================================================================
# LICENSE ENFORCEMENT (v1.4.0+)
# ============================================================================
# Enforce license requirements for ENTERPRISE and HYPERSCALER editions

# ENTERPRISE Edition: License REQUIRED for Release builds only
if(THEMIS_EDITION STREQUAL "ENTERPRISE")
    # Check CMAKE_BUILD_TYPE - if not set, default to Release for safety
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release builds: License is MANDATORY
        if(NOT THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "")
            message(FATAL_ERROR 
                "ENTERPRISE Edition Release builds require an embedded license file.\n"
                "Usage: cmake -B build -S . \\\n"
                "  -DTHEMIS_EDITION=ENTERPRISE \\\n"
                "  -DCMAKE_BUILD_TYPE=Release \\\n"
                "  -DTHEMIS_LICENSE_FILE=/path/to/license.json\n"
                "\n"
                "For development/testing, use Debug build:\n"
                "  cmake -B build -S . -DTHEMIS_EDITION=ENTERPRISE -DCMAKE_BUILD_TYPE=Debug\n")
        endif()
        message(STATUS "ENTERPRISE Edition (Release): License requirement ENFORCED")
    else()
        # Debug/Development builds: License is OPTIONAL
        message(STATUS "ENTERPRISE Edition (${CMAKE_BUILD_TYPE}): License requirement OPTIONAL (dev build)")
    endif()
endif()

# HYPERSCALER Edition: License ALWAYS MANDATORY (no exceptions)
if(THEMIS_EDITION STREQUAL "HYPERSCALER")
    if(NOT THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "")
        message(FATAL_ERROR 
            "HYPERSCALER Edition requires an embedded license file (mandatory for all builds).\n"
            "This edition is only available with volume licensing.\n"
            "\n"
            "Usage: cmake -B build -S . \\\n"
            "  -DTHEMIS_EDITION=HYPERSCALER \\\n"
            "  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json\n"
            "\n"
            "For licensing inquiries, contact: licensing@themisdb.io\n")
    endif()
    message(STATUS "HYPERSCALER Edition: License requirement ENFORCED (mandatory)")
endif()

# Compile-time edition identifier for runtime checks and telemetry
add_compile_definitions(
    "THEMIS_EDITION_STRING=\"${THEMIS_EDITION}\""
    "THEMIS_GPU_MAX_VRAM_GB=${THEMIS_GPU_MAX_VRAM_GB}"
    "THEMIS_SHARDING_MAX_NODES=${THEMIS_SHARDING_MAX_NODES}"
)

# ============================================================================
# LICENSE DATA EMBEDDING (v1.4.0+)
# ============================================================================
# Allows embedding company/license data into the binary at build time.
# This supports both offline deployments and CI/CD pipelines.
# 
# Default Behavior:
#   If no license file is specified, automatically uses the community license
#   from config/license_community_default.json
#
# Usage:
#   cmake -B build -S .  # Uses community license by default
#   cmake -B build -S . -DTHEMIS_LICENSE_FILE=path/to/license.json  # Custom license
#
# The license file should be a JSON file with the following structure:
# {
#   "organization_name": "Company Name",
#   "organization_id": "optional-org-id",
#   "contact_email": "contact@company.com",
#   "license_key": "THEMIS-XXX-XXXXXXXXXXXX",
#   "edition": "ENTERPRISE",
#   "issued_date": "2026-01-01",
#   "expiry_date": "2027-12-31",
#   "max_nodes": 100,
#   "max_cores": -1,
#   "max_storage_tb": -1,
#   "build_id": "optional-build-id",
#   "signature": "optional-rsa-signature"
# }

set(THEMIS_LICENSE_FILE "" CACHE FILEPATH "Path to license JSON file for embedding")

# Auto-detect default license file if none specified
if(NOT THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "")
    set(DEFAULT_LICENSE_FILE "${THEMIS_ROOT_DIR}/config/license_community_default.json")
    if(EXISTS "${DEFAULT_LICENSE_FILE}")
        set(THEMIS_LICENSE_FILE "${DEFAULT_LICENSE_FILE}")
        message(STATUS "No license file specified, using default community license: ${THEMIS_LICENSE_FILE}")
    endif()
endif()

# Default license values (empty/unset if no license file provided)
set(THEMIS_LICENSE_ORG_NAME "")
set(THEMIS_LICENSE_ORG_ID "")
set(THEMIS_LICENSE_CONTACT_EMAIL "")
set(THEMIS_LICENSE_KEY "")
set(THEMIS_LICENSE_EDITION "")
set(THEMIS_LICENSE_ISSUED_DATE "")
set(THEMIS_LICENSE_EXPIRY_DATE "")
set(THEMIS_LICENSE_MAX_NODES -1)
set(THEMIS_LICENSE_MAX_CORES -1)
set(THEMIS_LICENSE_MAX_STORAGE_TB -1)
set(THEMIS_LICENSE_BUILD_ID "")
set(THEMIS_LICENSE_BUILD_TIMESTAMP "")
set(THEMIS_LICENSE_SIGNATURE "")

# If license file is provided or auto-detected, read and parse it
if(THEMIS_LICENSE_FILE AND EXISTS "${THEMIS_LICENSE_FILE}")
    message(STATUS "Embedding license data from: ${THEMIS_LICENSE_FILE}")
    
    # Read the license file
    file(READ "${THEMIS_LICENSE_FILE}" LICENSE_JSON_CONTENT)
    
    # Parse JSON using CMake's built-in JSON support (CMake 3.19+)
    # Note: We need to extract values using string operations since CMake's JSON support is limited
    
    # Helper macro to extract JSON string value
    macro(extract_json_string VAR_NAME JSON_KEY)
        string(REGEX MATCH "\"${JSON_KEY}\"[ \t\n]*:[ \t\n]*\"([^\"]*)\"" _match "${LICENSE_JSON_CONTENT}")
        if(_match)
            set(${VAR_NAME} "${CMAKE_MATCH_1}")
        endif()
    endmacro()
    
    # Helper macro to extract JSON integer value
    macro(extract_json_int VAR_NAME JSON_KEY)
        string(REGEX MATCH "\"${JSON_KEY}\"[ \t\n]*:[ \t\n]*(-?[0-9]+)" _match "${LICENSE_JSON_CONTENT}")
        if(_match)
            set(${VAR_NAME} "${CMAKE_MATCH_1}")
        endif()
    endmacro()
    
    # Extract all license fields
    extract_json_string(THEMIS_LICENSE_ORG_NAME "organization_name")
    extract_json_string(THEMIS_LICENSE_ORG_ID "organization_id")
    extract_json_string(THEMIS_LICENSE_CONTACT_EMAIL "contact_email")
    extract_json_string(THEMIS_LICENSE_KEY "license_key")
    extract_json_string(THEMIS_LICENSE_EDITION "edition")
    extract_json_string(THEMIS_LICENSE_ISSUED_DATE "issued_date")
    extract_json_string(THEMIS_LICENSE_EXPIRY_DATE "expiry_date")
    extract_json_int(THEMIS_LICENSE_MAX_NODES "max_nodes")
    extract_json_int(THEMIS_LICENSE_MAX_CORES "max_cores")
    extract_json_int(THEMIS_LICENSE_MAX_STORAGE_TB "max_storage_tb")
    extract_json_string(THEMIS_LICENSE_BUILD_ID "build_id")
    extract_json_string(THEMIS_LICENSE_SIGNATURE "signature")
    
    # Set build timestamp to current time
    string(TIMESTAMP THEMIS_LICENSE_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
    
    # Display embedded license information
    message(STATUS "  Organization:     ${THEMIS_LICENSE_ORG_NAME}")
    message(STATUS "  License Key:      ${THEMIS_LICENSE_KEY}")
    message(STATUS "  Edition:          ${THEMIS_LICENSE_EDITION}")
    message(STATUS "  Expiry Date:      ${THEMIS_LICENSE_EXPIRY_DATE}")
    message(STATUS "  Max Nodes:        ${THEMIS_LICENSE_MAX_NODES}")
    message(STATUS "  Build Timestamp:  ${THEMIS_LICENSE_BUILD_TIMESTAMP}")
    
    # Validate that license edition matches build edition
    if(THEMIS_LICENSE_EDITION AND NOT THEMIS_LICENSE_EDITION STREQUAL THEMIS_EDITION)
        # For ENTERPRISE and HYPERSCALER, edition mismatch is a FATAL error
        if(THEMIS_EDITION STREQUAL "ENTERPRISE" OR THEMIS_EDITION STREQUAL "HYPERSCALER")
            message(FATAL_ERROR 
                "License edition mismatch: License is for '${THEMIS_LICENSE_EDITION}' but build is '${THEMIS_EDITION}'.\n"
                "You must use a license file that matches your build edition.\n"
                "License file: ${THEMIS_LICENSE_FILE}")
        else()
            # For MINIMAL/COMMUNITY, just warn
            message(WARNING "License edition (${THEMIS_LICENSE_EDITION}) does not match build edition (${THEMIS_EDITION})")
        endif()
    endif()
else()
    if(THEMIS_LICENSE_FILE)
        message(WARNING "License file specified but not found: ${THEMIS_LICENSE_FILE}")
    endif()
    message(STATUS "No license data will be embedded in this build")
endif()

# Add license data as compile definitions
add_compile_definitions(
    "THEMIS_LICENSE_ORG_NAME=\"${THEMIS_LICENSE_ORG_NAME}\""
    "THEMIS_LICENSE_ORG_ID=\"${THEMIS_LICENSE_ORG_ID}\""
    "THEMIS_LICENSE_CONTACT_EMAIL=\"${THEMIS_LICENSE_CONTACT_EMAIL}\""
    "THEMIS_LICENSE_KEY=\"${THEMIS_LICENSE_KEY}\""
    "THEMIS_LICENSE_EDITION=\"${THEMIS_LICENSE_EDITION}\""
    "THEMIS_LICENSE_ISSUED_DATE=\"${THEMIS_LICENSE_ISSUED_DATE}\""
    "THEMIS_LICENSE_EXPIRY_DATE=\"${THEMIS_LICENSE_EXPIRY_DATE}\""
    "THEMIS_LICENSE_MAX_NODES=${THEMIS_LICENSE_MAX_NODES}"
    "THEMIS_LICENSE_MAX_CORES=${THEMIS_LICENSE_MAX_CORES}"
    "THEMIS_LICENSE_MAX_STORAGE_TB=${THEMIS_LICENSE_MAX_STORAGE_TB}"
    "THEMIS_LICENSE_BUILD_ID=\"${THEMIS_LICENSE_BUILD_ID}\""
    "THEMIS_LICENSE_BUILD_TIMESTAMP=\"${THEMIS_LICENSE_BUILD_TIMESTAMP}\""
    "THEMIS_LICENSE_SIGNATURE=\"${THEMIS_LICENSE_SIGNATURE}\""
)

# MSVC: sanitize any accidentally injected '/WX:NO' options (invalid for cl.exe)
if(MSVC)
    get_property(_allTargets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _allTargets)
        foreach(_prop IN ITEMS COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)
            get_target_property(_opts ${_t} ${_prop})
            if(_opts)
                list(TRANSFORM _opts REPLACE "/WX:NO" "")
                list(TRANSFORM _opts REPLACE "/WX:No" "")
                list(TRANSFORM _opts REPLACE "/WX:0"  "")
                set_target_properties(${_t} PROPERTIES ${_prop} "${_opts}")
            endif()
        endforeach()
    endforeach()
endif()

# Note: Architecture detection and optimization moved to cmake/platforms/
# See cmake/platforms/PlatformDetection.cmake and ArchitectureOptimizations.cmake

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
    # Note: AVX2 optimization moved to cmake/platforms/ArchitectureOptimizations.cmake
    # Ensure Link-Time Code Generation to avoid link restart messages
    add_link_options($<$<CONFIG:Release>:/LTCG>)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(THEMIS_STRICT_BUILD)
        add_compile_options(-Werror)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
        # Note: Architecture-specific optimizations moved to cmake/platforms/ArchitectureOptimizations.cmake
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

# Optionale lokale clang-tidy Aktivierung (nicht in CI standardm��ig).
if(ENABLE_LOCAL_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy aktiviert: ${CLANG_TIDY_EXE}")
        # Keine automatischen Fixes hier; Entwickler nutzen Skript f�r -fix.
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
find_package(simdjson CONFIG QUIET)
if(NOT simdjson_FOUND)
    message(WARNING "simdjson not found - some features may be disabled")
endif()

find_package(TBB CONFIG QUIET)
if(NOT TBB_FOUND)
    message(WARNING "TBB not found - will use fallback threading")
endif()

find_package(Arrow CONFIG QUIET)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost QUIET COMPONENTS system)
if(NOT Boost_FOUND)
    message(WARNING "Boost not found - some features may be disabled")
endif()

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
        else()
            message(WARNING "yaml-cpp not found - some configuration features may be limited")
        endif()
    else()
        message(WARNING "yaml-cpp not found - some configuration features may be limited")
    endif()
endif()

if(NOT TARGET yaml-cpp::yaml-cpp)
    message(STATUS "yaml-cpp optional dependency not available - continuing without it")
endif()

# Define optional YAML target for linking
set(THEMIS_YAML_TARGET "")
if(TARGET yaml-cpp::yaml-cpp)
    set(THEMIS_YAML_TARGET yaml-cpp::yaml-cpp)
endif()

# Defensive check: Ensure Boost::system target exists after find_package
if(Boost_FOUND)
    if(NOT TARGET Boost::system)
        message(FATAL_ERROR "Required CMake target 'Boost::system' not found. "
                            "Ensure Boost was found and the 'system' component is available.")
    endif()
else()
    message(WARNING "Boost not available - network features may be disabled")
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
        message(STATUS "OpenBLAS found - BLAS support enabled")
    else()
        message(WARNING "OpenBLAS not found - install with: vcpkg install openblas")
    endif()

    find_package(LAPACK CONFIG QUIET)
    if(LAPACK_FOUND)
        message(STATUS "LAPACK gefunden")
    endif()

    # FAISS (CONFIG mode via vcpkg). If missing, gracefully disable GPU.
    find_package(faiss CONFIG)
    if(NOT faiss_FOUND)
        message(WARNING "Faiss not found - GPU support will be automatically disabled.")
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

# ═══════════════════════════════════════════════════════════
# GDAL Integration (P2 - v1.4.0+)
# ═══════════════════════════════════════════════════════════
if(THEMIS_ENABLE_GDAL)
    message(STATUS "GDAL support enabled - integrating GDAL for geospatial data processing")
    
    # Find GDAL package
    find_package(GDAL CONFIG QUIET)
    if(GDAL_FOUND)
        message(STATUS "GDAL found via CONFIG: ${GDAL_VERSION}")
    else()
        # Fallback to MODULE mode
        find_package(GDAL MODULE)
        if(GDAL_FOUND)
            message(STATUS "GDAL found via MODULE: ${GDAL_VERSION}")
            # Create imported target if not already created
            if(NOT TARGET GDAL::GDAL)
                add_library(GDAL::GDAL UNKNOWN IMPORTED)
                set_target_properties(GDAL::GDAL PROPERTIES
                    IMPORTED_LOCATION "${GDAL_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${GDAL_INCLUDE_DIR}")
            endif()
        else()
            message(WARNING "GDAL not found - Install with: vcpkg install gdal")
            message(WARNING "Disabling GDAL support due to missing dependency")
            set(THEMIS_ENABLE_GDAL OFF CACHE BOOL "GDAL not available" FORCE)
        endif()
    endif()
    
    if(GDAL_FOUND)
        message(STATUS "  GDAL features: Shapefile, GeoTIFF, GeoPackage support enabled")
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
if(EXISTS "${THEMIS_ROOT_DIR}/config")
    file(COPY "${THEMIS_ROOT_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# -----------------------------------------------------------
# llama.cpp Integration (v1.3.0+)
# -----------------------------------------------------------
if(THEMIS_ENABLE_LLM)
    message(STATUS "LLM support enabled - integrating llama.cpp")

    # Prefer local static build of llama.cpp to avoid shared DLL issues
    option(THEMIS_FORCE_LLAMA_SUBDIR "Prefer local static llama.cpp even if a target exists" OFF)

    # Check if llama was already found via find_package (e.g. from installed version)
    if(TARGET llama AND NOT THEMIS_FORCE_LLAMA_SUBDIR)
        message(STATUS "llama.cpp already available as target (from find_package) - skipping add_subdirectory")
    else()
        # Prefer local, non-committed root clone at ${THEMIS_ROOT_DIR}/llama.cpp
        # Fallback to legacy path external/llama.cpp if present
        set(LLAMA_SRC_DIR "${THEMIS_ROOT_DIR}/llama.cpp")
        if(NOT EXISTS "${LLAMA_SRC_DIR}/CMakeLists.txt")
            if(EXISTS "${THEMIS_ROOT_DIR}/external/llama.cpp/CMakeLists.txt")
                set(LLAMA_SRC_DIR "${THEMIS_ROOT_DIR}/external/llama.cpp")
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
    # When LLAMA_SRC_DIR is outside the current source dir (cmake/), CMake requires an explicit binary dir.
    # Guard against double-add since Dependencies.cmake may have already added it
    if(NOT TARGET llama)
        if(THEMIS_FORCE_LLAMA_SUBDIR)
            set(LLAMA_BIN_DIR "${CMAKE_BINARY_DIR}/llama_cpp_forced_build")
        else()
            set(LLAMA_BIN_DIR "${CMAKE_BINARY_DIR}/llama_cpp_build")
        endif()
        add_subdirectory(${LLAMA_SRC_DIR} ${LLAMA_BIN_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    # MSVC fix: char8_t string literals in llama-chat.cpp require /Zc:char8_t-
    if(MSVC AND TARGET llama)
        target_compile_options(llama PRIVATE /Zc:char8_t-)
    endif()
    
    # Define LLM enabled for ThemisDB code
    add_compile_definitions(THEMIS_LLM_ENABLED)
    
    message(STATUS "  llama.cpp configured successfully")
    endif() # end of else block for TARGET llama check
endif()

# Helper: normalize source entries that start with '/' to be rooted at the project source dir
function(_themis_normalize_sources list_var)
    set(_normalized "")
    foreach(_entry IN LISTS ${list_var})
        if(_entry MATCHES "^/")
            list(APPEND _normalized "${THEMIS_ROOT_DIR}${_entry}")
        else()
            list(APPEND _normalized "${_entry}")
        endif()
    endforeach()
    set(${list_var} "${_normalized}" PARENT_SCOPE)
endfunction()

# Core library sources
set(THEMIS_CORE_SOURCES
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/base_entity.cpp
    ../src/storage/key_schema.cpp
    ../src/storage/backup_manager.cpp
    ../src/storage/nlp_metadata_extractor.cpp
    ../src/storage/columnar_format.cpp
    
    ../src/metadata/schema_manager.cpp
    ../src/server/schema_api_handler.cpp
    
    ../src/index/secondary_index.cpp
    ../src/index/graph_index.cpp
    ../src/index/temporal_graph.cpp
    ../src/index/property_graph.cpp
    ../src/index/edge_types.cpp
    ../src/index/process_graph.cpp
    ../src/index/gnn_embeddings.cpp
    ../src/index/graph_analytics.cpp
    ../src/index/vector_index.cpp
    ../src/index/hnsw_layer_optimizer.cpp
    ../src/index/hnsw_parameter_tuner.cpp
    ../src/index/hnsw_production_defaults.cpp
    ../src/index/product_quantizer.cpp
    ../src/index/binary_quantizer.cpp
    ../src/index/learned_quantizer.cpp
    ../src/index/residual_quantizer.cpp
    ../src/index/adaptive_index.cpp
    
    ../src/security/mock_key_provider.cpp
    ../src/security/vault_key_provider.cpp
    ../src/security/key_cache.cpp
    ../src/security/keyprovider_signing.cpp
    ../src/security/vault_signing_provider.cpp
    ../src/security/field_encryption.cpp
    ../src/security/encrypted_field.cpp
    ../src/security/malware_scanner.cpp
    ../src/security/usb_admin_authenticator.cpp
    ../src/security/aql_injection_detector.cpp
    ../src/core/security_initialization.cpp
    
    ../src/transaction/transaction_manager.cpp
    ../src/transaction/saga.cpp
    ../src/transaction/snapshot_manager.cpp
    
    ../src/query/query_engine.cpp
    ../src/query/query_optimizer.cpp
    ../src/query/adaptive_optimizer.cpp
    ../src/graph/graph_query_optimizer.cpp
    ../src/query/optimizer_cost_model.cpp
    ../src/query/aql_parser.cpp
    ../src/query/aql_parser_json.cpp
    ../src/query/aql_translator.cpp
    ../src/query/aql_runner.cpp
    ../src/query/let_evaluator.cpp
    ../src/query/window_evaluator.cpp
    ../src/query/cte_subquery.cpp
    ../src/query/cte_cache.cpp
    ../src/query/result_stream.cpp
    ../src/query/query_cache.cpp

    
    
    ../src/query/statistical_aggregator.cpp
    ../src/query/semantic_cache.cpp
    ../src/query/functions/function_registry.cpp
    
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/transaction_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/auth_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SSE}>:../src/server/sse_connection_manager.cpp>
    ../src/server/cache_api_handler.cpp
    ../src/server/admin_api_handler.cpp
    ../src/server/vector_api_handler.cpp
    ../src/server/rope_api_handler.cpp
    ../src/server/spatial_api_handler.cpp
    ../src/server/monitoring_api_handler.cpp
    ../src/server/query_api_handler.cpp
    ../src/server/policy_api_handler.cpp
    ../src/server/prompt_api_handler.cpp
    ../src/server/graph_api_handler.cpp
    ../src/server/index_api_handler.cpp
    ../src/server/timeseries_api_handler.cpp
    ../src/server/entity_api_handler.cpp
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/server/content_api_handler.cpp>
    ../src/server/changefeed_api_handler.cpp
    ../src/server/wal_api_handler.cpp
    ../src/server/audit_api_handler.cpp
    ../src/server/pki_api_handler.cpp
    ../src/server/saga_api_handler.cpp
    ../src/server/pii_api_handler.cpp
    ../src/server/retention_api_handler.cpp
    ../src/server/keys_api_handler.cpp
    ../src/server/classification_api_handler.cpp
    ../src/server/reports_api_handler.cpp
    ../src/server/ranger_adapter.cpp
    ../src/server/policy_engine.cpp
    ../src/server/auth_middleware.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/wal_grpc_service.cpp>
    
    # LLM integration (server/API/grpc) is added conditionally below
    ../src/server/rate_limiter.cpp
    ../src/server/rate_limiter_v2.cpp
    ../src/server/load_shedder.cpp
    ../src/server/update_api_handler.cpp
    ../src/server/hot_reload_api_handler.cpp
    ../src/server/export_api_handler.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/server/feedback_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/snapshot_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP2}>:../src/server/http2_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/server/websocket_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MQTT}>:../src/server/mqtt_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:../src/server/postgres_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MCP}>:../src/server/mcp_server.cpp>
    
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_type.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/text_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/video_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/audio_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/image_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pdf_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/geo_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/cad_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mock_clip_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mime_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_policy.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_fs.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/version_manager.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_OFFICE}>>:../src/content/office_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/archive_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/async_ingestion_worker.cpp>
    
    # Content Pipeline (GAP-005) - Enhanced with streaming and multi-modal support
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/zstd_compression.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/content_chunker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/bulk_upload_interface.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/async_bulk_uploader.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/multimodal_chunker.cpp>
    
    ../src/cache/semantic_cache.cpp
    ../src/cache/embedding_cache.cpp
    ../src/cache/bounded_lru_cache.cpp
    # LLM core components are added conditionally below
    ../src/cdc/changefeed.cpp
    
    # Replication
    ../src/replication/replication_manager.cpp
    
    # Temporal conflict resolution
    ../src/temporal/temporal_conflict_resolver.cpp
    
    ../src/timeseries/timeseries.cpp
    ../src/timeseries/timeseries_metrics.cpp
    ../src/timeseries/tsstore.cpp
    ../src/timeseries/gorilla.cpp
    ../src/timeseries/retention.cpp
    ../src/timeseries/continuous_agg.cpp
    ../src/timeseries/aggregate_scheduler.cpp
    ../src/timeseries/aggregate_scheduler_helper.cpp
    ../src/timeseries/query_optimizer.cpp
    
    ../src/utils/serialization.cpp
    ../src/utils/logger.cpp
    ../src/utils/cursor.cpp
    ../src/utils/tracing.cpp
    
    # Cross-cutting concerns abstraction layer
    ../src/core/concerns/i_logger.cpp
    ../src/core/concerns/concerns_context.cpp
    ../src/utils/zstd_codec.cpp
    ../src/utils/pki_client.cpp
    ../src/utils/input_validator.cpp
    ../src/utils/audit_logger.cpp
    ../src/utils/pii_detection_engine.cpp
    ../src/utils/regex_detection_engine.cpp
    ../src/utils/pii_detector.cpp
    ../src/utils/retention_manager.cpp
    ../src/utils/hkdf_helper.cpp
    ../src/utils/hkdf_cache.cpp
    ../src/utils/saga_logger.cpp
    ../src/utils/lek_manager.cpp
    ../src/utils/stemmer.cpp
    ../src/utils/stopwords.cpp
    ../src/utils/pii_pseudonymizer.cpp
    ../src/utils/normalizer.cpp
    ../src/utils/simd_distance.cpp
    ../src/utils/update_checker.cpp
    ../src/utils/http_client_pool.cpp
    ../src/utils/grpc_channel_pool.cpp
    ../src/utils/thread_pool_manager.cpp
    ../src/utils/build_info.cpp
    ../src/utils/license_info.cpp
    ../src/utils/error_registry.cpp
    ../src/utils/memory/pool_allocator.cpp
    
    # Updates (hot-reload)
    ../src/updates/release_manifest.cpp
    ../src/updates/manifest_database.cpp
    ../src/updates/hot_reload_engine.cpp
    ../src/updates/updates_config.cpp
    
    # Geo utilities
    ../src/utils/geo/ewkb.cpp
    
    # Spatial index
    ../src/index/spatial_index.cpp
    
    # Hybrid search (BM25 + Vector)
    ../src/search/hybrid_search.cpp
    ../src/security/pki_key_provider.cpp
    ../src/security/cms_signing.cpp
    ../src/security/rbac.cpp
    ../src/security/access_control_manager.cpp
    ../src/security/access_control.cpp
    ../src/security/user_registration_plugin.cpp
    ../src/security/arrow_user_registration_plugin.cpp
    ../src/security/webdav_user_registration_plugin.cpp
    ../src/security/embedded_user_registration_plugin.cpp
    ../src/security/hsm_provider.cpp
    ../src/security/hsm_provider_pkcs11.cpp
    ../src/security/hsm_key_provider_adapter.cpp
    ../src/security/timestamp_authority.cpp
    ../src/security/timestamp_authority_openssl.cpp
    ../src/security/vram_secure_clear.cpp
    ../src/security/vcc_pki_client.cpp
    ../src/storage/security_signature.cpp
    ../src/storage/security_signature_manager.cpp
    ../src/security/binary_manifest.cpp
    ../src/security/manifest_signer.cpp
    ../src/auth/jwt_validator.cpp
    ../src/auth/gssapi_authenticator.cpp
    ../src/governance/policy_engine.cpp
    ../src/governance/policy_manager.cpp
    ../src/governance/policy_template.cpp
    ../src/governance/policy_coordinator.cpp
    ../src/governance/compliance_reporting.cpp
    # Geo scaffolding (headers + stubs; safe no-ops until used)
    ../src/geo/cpu_backend.cpp
    ../src/geo/gpu_backend_stub.cpp
    ../src/geo/boost_cpu_exact_backend.cpp
    ../src/api/geo_index_hooks.cpp
    
    # Sharding (horizontal scaling)
    ../src/utils/boost_throw_exception.cpp
    ../src/sharding/urn.cpp
    ../src/sharding/consistent_hash.cpp
    ../src/sharding/shard_topology.cpp
    ../src/sharding/metadata_shard.cpp
    ../src/sharding/urn_resolver.cpp
    ../src/sharding/pki_shard_certificate.cpp
    ../src/sharding/mtls_client.cpp
    ../src/sharding/mtls_connection_pool.cpp
    ../src/sharding/signed_request.cpp
    ../src/sharding/remote_executor.cpp
    ../src/sharding/shard_rpc_client.cpp
    ../src/utils/file_utils.cpp
    ../src/stubs.cpp
    ../src/sharding/shard_router.cpp
    ../src/sharding/rebalance_operation.cpp
    ../src/sharding/data_migrator.cpp
    ../src/sharding/shard_load_detector.cpp
    ../src/sharding/auto_rebalancer.cpp
    ../src/sharding/prometheus_metrics.cpp
    ../src/sharding/metrics_registry.cpp
    ../src/sharding/health_check.cpp
    ../src/sharding/admin_api.cpp
    ../src/sharding/cloud_agent.cpp
    ../src/sharding/circuit_breaker.cpp
    ../src/sharding/gossip_protocol.cpp
    ../src/sharding/gossip_config_manager.cpp
    ../src/sharding/distributed_coordinator.cpp
    ../src/sharding/shard_resource_manager.cpp
    ../src/sharding/locality_aware_router.cpp
    ../src/sharding/raft_configuration.cpp
    ../src/sharding/raft_log.cpp
    ../src/sharding/raft_state.cpp
    ../src/sharding/raft_wal_integration.cpp
    ../src/sharding/raft_consensus.cpp
    ../src/sharding/quorum_manager.cpp
    ../src/sharding/partition_detector.cpp
    ../src/sharding/replica_consistency.cpp
    ../src/sharding/stream_protocol.cpp
    ../src/sharding/wal_applier.cpp
    ../src/sharding/wal_manager.cpp
    ../src/sharding/wal_shipper.cpp
    ../src/sharding/replication_coordinator.cpp
    ../src/sharding/replica_topology.cpp
    ../src/sharding/multi_primary_coordinator.cpp
    ../src/sharding/health_monitor.cpp
    ../src/sharding/shard_durability.cpp
    ../src/sharding/truetime.cpp
    ../src/sharding/distributed_transaction.cpp
    ../src/sharding/consensus_factory.cpp
    ../src/sharding/raft_consensus_adapter.cpp
    ../src/sharding/gossip_consensus_adapter.cpp
    ../src/sharding/paxos_consensus.cpp
    ../src/sharding/cross_shard_transaction.cpp
    ../src/sharding/redundancy_strategy.cpp
    ../src/sharding/hot_spare_manager.cpp
    ../src/sharding/predictive_detector.cpp
    ../src/sharding/shard_rpc_server.cpp
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:/src/sharding/gpu_erasure_coder.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:/src/sharding/gpu_erasure_coder.cu>
    $<$<BOOL:${THEMIS_ENABLE_OPENCL}>:/src/sharding/gpu_erasure_coder_opencl.cpp>
    ../src/storage/blob_redundancy_manager.cpp
    ../src/server/sharding_metrics_handler.cpp
    
    # Hardware acceleration (core abstraction layer - always included)
    ../src/acceleration/backend_registry.cpp
    ../src/acceleration/cpu_backend.cpp
    ../src/acceleration/plugin_loader.cpp
    ../src/acceleration/plugin_security.cpp
    # Plugin manager (core plugin orchestration)
    ../src/plugins/plugin_manager.cpp
    ../src/plugins/plugin_hot_plug_monitor.cpp
    ../src/plugins/plugin_registry.cpp
    ../src/plugins/plugin_metrics.cpp
    
    # Multi-tenancy support
    ../src/server/tenant_manager.cpp
    
    # Network
    ../src/network/wire_protocol_server.cpp
    
    # Observability (GAP-008: Alertmanager integration)
    ../src/observability/metrics_collector.cpp
    ../src/observability/alertmanager.cpp
    
    # GraphQL API (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/api/graphql.cpp>
    
    # Exporters and Importers
    ../src/exporters/jsonl_llm_exporter.cpp
    ../src/importers/postgres_importer.cpp
    
    # OLAP Analytics
    ../src/analytics/olap.cpp
    ../src/analytics/process_mining.cpp
    ../src/analytics/nlp_text_analyzer.cpp
    ../src/analytics/arrow_export.cpp
    ../src/analytics/analytics_export.cpp
    
    # Git-like MVCC Features
    # Phase 1: Named Snapshots
    ../src/transaction/snapshot_manager.cpp
    ../src/storage/pitr_manager.cpp
    
    # Phase 2: Diff Engine
    ../src/analytics/diff_engine.cpp
)

# HTTP Server API handlers (only if HTTP server enabled)
if(THEMIS_ENABLE_HTTP_SERVER)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/server/diff_api_handler.cpp
    )
endif()

# Phase 2 Performance Optimizations (PR #156/#157)
# Conditionally add Phase 2 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    ../src/performance/phase2_feature_flags.cpp
)

if(THEMIS_ENABLE_WISCKEY)
    message(STATUS "Including WiscKey key/value separation (THEMIS_ENABLE_WISCKEY=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/wisckey.cpp)
endif()

if(THEMIS_ENABLE_DOSTOEVSKY)
    message(STATUS "Including Dostoevsky adaptive LSM (THEMIS_ENABLE_DOSTOEVSKY=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/dostoevsky.cpp)
endif()

if(THEMIS_ENABLE_CICADA)
    message(STATUS "Including Cicada optimistic CC (THEMIS_ENABLE_CICADA=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/cicada.cpp)
endif()

if(THEMIS_ENABLE_LIGRA)
    message(STATUS "Including Ligra graph processing (THEMIS_ENABLE_LIGRA=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/ligra.cpp)
endif()

if(THEMIS_ENABLE_RABITQ)
    message(STATUS "Including RaBitQ vector quantization (THEMIS_ENABLE_RABITQ=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/rabitq.cpp)
endif()

# Phase 3 Performance Optimizations (PR #156/#157)
# Conditionally add Phase 3 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    ../src/performance/phase3/feature_flags.cpp
)

if(THEMIS_ENABLE_DISKANN)
    message(STATUS "Including DiskANN billion-scale vector search (THEMIS_ENABLE_DISKANN=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/diskann.cpp)
endif()

if(THEMIS_ENABLE_BWTREE)
    message(STATUS "Including Bw-Tree lock-free index (THEMIS_ENABLE_BWTREE=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/bwtree.cpp)
endif()

if(THEMIS_ENABLE_SPLINTERDB)
    message(STATUS "Including SplinterDB concurrent compaction (THEMIS_ENABLE_SPLINTERDB=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/splinterdb.cpp)
endif()

if(THEMIS_ENABLE_GUNROCK)
    message(STATUS "Including Gunrock GPU graph analytics (THEMIS_ENABLE_GUNROCK=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/gunrock.cpp)
endif()

if(THEMIS_ENABLE_BAO)
    message(STATUS "Including Bao ML query optimizer (THEMIS_ENABLE_BAO=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/bao.cpp)
endif()

# ============================================================================
# MODULAR SOURCE FILES (organized by feature/component)
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/AccelerationBackends.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ContentProcessors.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/LLMIntegration.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/DistributedTraining.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/BlobStorage.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/BufferManagement.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ErrorHealthServices.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/IndexQueryEnhancements.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/EditionFeatures.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/StorageEnhancements.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/RPCServices.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/MiscellaneousFeatures.cmake)

# Conditionally add LLM sources (v1.3.0+) only when enabled
if(THEMIS_ENABLE_LLM)
    message(STATUS "Including LLM server and core sources (THEMIS_ENABLE_LLM=ON)")
    list(APPEND THEMIS_CORE_SOURCES
        # Server/API (HTTP + optional gRPC)
    ../src/server/llm_api_handler.cpp
    ../src/aql/llm_aql_handler.cpp

        # Optional: gRPC service and generated proto headers (guarded by gRPC)
        # Only add when gRPC is available, otherwise skip to prevent missing headers
    )

    find_package(gRPC CONFIG QUIET)
    if(gRPC_FOUND)
        message(STATUS "gRPC found - enabling LLM gRPC service")
            # TEMP: gRPC proto generation not yet configured - comment out until ready
            # list(APPEND THEMIS_CORE_SOURCES
            #     ../src/server/llm_grpc_service.cpp
            #     /proto/llm_service.proto
            # )
        # Note: Proto generation should be configured in a separate section when needed
    else()
        message(STATUS "gRPC not found - disabling LLM gRPC service and proto generation")
    endif()
        # TEMP: Disable llm_grpc_service until proto generation is configured
        # list(REMOVE_ITEM THEMIS_CORE_SOURCES ../src/server/llm_grpc_service.cpp)

    # LLM core components
    list(APPEND THEMIS_CORE_SOURCES
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/prompt_manager.cpp
    ../src/llm/block_table.cpp
    ../src/llm/paged_kv_cache.cpp
    ../src/llm/adapter_load_balancer.cpp
    ../src/llm/lora_framework/resource_profiler.cpp
    ../src/llm/lora_framework/model_compatibility.cpp
    ../src/llm/lora_framework/embedding_provider.cpp
    ../src/llm/llm_model_audit_logger.cpp
    ../src/llm/llama_lora_adapter.cpp
    # GPU VRAM Allocation (vLLM-inspired)
    ../src/llm/adaptive_vram_allocator.cpp
    ../src/llm/multi_gpu_memory_coordinator.cpp
    ../src/llm/paged_kv_cache_manager.cpp
    ../src/llm/mixed_precision_inference.cpp
    )
    
    # RAG enhancement modules (Phase 1: Knowledge Gap Detector)
    list(APPEND THEMIS_CORE_SOURCES
    ../src/rag/knowledge_gap_detector.cpp
    )

    # Test utilities linked for LLM integration tests
    list(APPEND THEMIS_CORE_SOURCES
    ../tests/utils/raid_simulator.cpp
    ../tests/utils/shard_failure_injector.cpp
    )
    # Note: rag_judge.cpp will be added in a future phase
endif()

# Ensure minimal LLM components are available when referenced from http_server.
# These have lightweight dependencies and avoid undefined references if LLM is disabled.
if(NOT THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/prompt_manager.cpp
    )
endif()

_themis_normalize_sources(THEMIS_CORE_SOURCES)

# ============================================================================
# BUILD MODE SELECTION: Modular vs Monolithic
# ============================================================================
# Check if modular build is enabled (v1.4.0+ feature)
if(THEMIS_BUILD_MODULAR)
    message(STATUS "==============================================")
    message(STATUS "MODULAR BUILD MODE ENABLED")
    message(STATUS "Building ThemisDB as separate module libraries")
    message(STATUS "==============================================")
    
    # Call the modular build function to create all module targets
    themis_build_modular()
    
    # Create themis_core as an interface library that links all modules
    # This maintains compatibility with existing code that links to themis_core
    add_library(themis_core INTERFACE)
    target_link_libraries(themis_core INTERFACE ${THEMIS_ALL_MODULES})
    
    message(STATUS "Created themis_core interface library linking: ${THEMIS_ALL_MODULES}")
    
else()
    # Traditional monolithic build (default for v1.3.x and earlier)
    message(STATUS "Building ThemisDB as monolithic themis_core library")

# Define core library early so subsequent target_* commands are valid
set(_THEMIS_CORE_LIBTYPE STATIC)
if(THEMIS_CORE_SHARED)
    set(_THEMIS_CORE_LIBTYPE SHARED)
endif()
add_library(themis_core ${_THEMIS_CORE_LIBTYPE} ${THEMIS_CORE_SOURCES})

# Base include directories for core headers
# Needed to resolve includes like "sharding/...", "llm/...", etc.
target_include_directories(themis_core
    PUBLIC
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${CMAKE_BINARY_DIR}/proto_generated
)

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
    ../src/acceleration/cuda_backend.cpp
    ../src/acceleration/cuda/vector_kernels.cu
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
    list(APPEND THEMIS_ACCELERATION_SOURCES ../src/acceleration/graphics_backends.cpp)
    
    if(THEMIS_ENABLE_DIRECTX)
        # Add DirectX 12 compute pipeline sources
        list(APPEND THEMIS_ACCELERATION_SOURCES
            ../src/llm/lora_framework/directx_context.cpp
            ../src/llm/lora_framework/directx_buffer.cpp
            ../src/llm/lora_framework/directx_descriptors.cpp
            ../src/llm/lora_framework/directx_shader.cpp
            ../src/llm/lora_framework/directx_pipeline.cpp
            ../src/llm/lora_framework/kernels/directx_kernels.cpp
        )
        
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DIRECTX)
        
        # Link DirectX 12 libraries (Windows only)
        if(WIN32)
            target_link_libraries(themis_core PRIVATE d3d12 dxgi)
            
            # Find DXC shader compiler (part of Windows SDK or DirectX Agility SDK)
            find_program(DXC_COMPILER dxc
                HINTS
                    "$ENV{WindowsSdkDir}bin/${CMAKE_VS_PLATFORM_NAME}"
                    "$ENV{WindowsSdkDir}bin/x64"
                    "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.22621.0/x64"
                    "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.19041.0/x64"
            )
            
            if(DXC_COMPILER)
                message(STATUS "DirectX 12 acceleration enabled")
                message(STATUS "  DirectX compute pipeline integrated")
                message(STATUS "  DXC compiler found: ${DXC_COMPILER}")
                
                # Set shader output directory
                set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/lora")
                file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
                
                # Compile HLSL shaders to DXIL bytecode
                set(SHADER_SOURCE_DIR "${THEMIS_ROOT_DIR}/src/acceleration/directx/shaders/lora")
                
                # Matmul shader
                add_custom_command(
                    OUTPUT ${SHADER_OUTPUT_DIR}/matmul.cso
                    COMMAND ${DXC_COMPILER} -T cs_6_0 -E main
                            ${SHADER_SOURCE_DIR}/matmul.hlsl
                            -Fo ${SHADER_OUTPUT_DIR}/matmul.cso
                    DEPENDS ${SHADER_SOURCE_DIR}/matmul.hlsl
                    COMMENT "Compiling matmul.hlsl to DXIL bytecode"
                    VERBATIM
                )
                
                # Elementwise shader
                add_custom_command(
                    OUTPUT ${SHADER_OUTPUT_DIR}/elementwise.cso
                    COMMAND ${DXC_COMPILER} -T cs_6_0 -E main
                            ${SHADER_SOURCE_DIR}/elementwise.hlsl
                            -Fo ${SHADER_OUTPUT_DIR}/elementwise.cso
                    DEPENDS ${SHADER_SOURCE_DIR}/elementwise.hlsl
                    COMMENT "Compiling elementwise.hlsl to DXIL bytecode"
                    VERBATIM
                )
                
                # Gradient shader
                add_custom_command(
                    OUTPUT ${SHADER_OUTPUT_DIR}/gradient.cso
                    COMMAND ${DXC_COMPILER} -T cs_6_0 -E main
                            ${SHADER_SOURCE_DIR}/gradient.hlsl
                            -Fo ${SHADER_OUTPUT_DIR}/gradient.cso
                    DEPENDS ${SHADER_SOURCE_DIR}/gradient.hlsl
                    COMMENT "Compiling gradient.hlsl to DXIL bytecode"
                    VERBATIM
                )
                
                # Custom target to compile all shaders
                add_custom_target(directx_shaders ALL
                    DEPENDS
                        ${SHADER_OUTPUT_DIR}/matmul.cso
                        ${SHADER_OUTPUT_DIR}/elementwise.cso
                        ${SHADER_OUTPUT_DIR}/gradient.cso
                    COMMENT "Building DirectX shaders"
                )
                
                # Install shaders to output directory
                install(FILES
                    ${SHADER_OUTPUT_DIR}/matmul.cso
                    ${SHADER_OUTPUT_DIR}/elementwise.cso
                    ${SHADER_OUTPUT_DIR}/gradient.cso
                    DESTINATION bin/shaders/lora
                    COMPONENT shaders
                )
                
                # Make themis_core depend on shader compilation
                add_dependencies(themis_core directx_shaders)
                
                message(STATUS "  Shaders: matmul.hlsl, elementwise.hlsl, gradient.hlsl → .cso")
            else()
                message(WARNING "DXC compiler not found - DirectX shaders will not be compiled")
                message(WARNING "  Install Windows 10 SDK 10.0.19041.0+ or DirectX Agility SDK")
                message(WARNING "  DirectX backend will require pre-compiled shaders or fail at runtime")
            endif()
        else()
            message(WARNING "DirectX is only available on Windows")
        endif()
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
_themis_normalize_sources(THEMIS_ACCELERATION_SOURCES)
if(THEMIS_ACCELERATION_SOURCES)
    target_sources(themis_core PRIVATE ${THEMIS_ACCELERATION_SOURCES})
endif()

# Main library already defined above (before feature-specific target modifications)

if(THEMIS_ENABLE_HSM_REAL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HSM_REAL)
    target_sources(themis_core PRIVATE ../src/security/hsm_provider_pkcs11.cpp)
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
        ${THEMIS_YAML_TARGET}
    OpenSSL::SSL
    OpenSSL::Crypto
    TBB::tbb
    $<$<BOOL:${prometheus-cpp_FOUND}>:prometheus-cpp::core>
)

if(WIN32)
    target_link_libraries(themis_core PUBLIC Secur32)
endif()

# Link libzip (for archive and office processing)
if(libzip_FOUND)
    # libzip may export as libzip::zip or libzip::libzip depending on version
    if(TARGET libzip::zip)
        target_link_libraries(themis_core PUBLIC libzip::zip)
    elseif(TARGET libzip::libzip)
        target_link_libraries(themis_core PUBLIC libzip::libzip)
    else()
        message(WARNING "libzip target not found - please check libzip installation")
    endif()
endif()

# Link pugixml (for office processing) if enabled
if(THEMIS_ENABLE_OFFICE AND pugixml_FOUND)
    # pugixml should export as pugixml::pugixml or pugixml::static
    if(TARGET pugixml::pugixml)
        target_link_libraries(themis_core PUBLIC pugixml::pugixml)
    elseif(TARGET pugixml::static)
        target_link_libraries(themis_core PUBLIC pugixml::static)
    elseif(TARGET pugixml)
        target_link_libraries(themis_core PUBLIC pugixml)
    else()
        message(WARNING "pugixml target not found - please check pugixml installation")
    endif()
    
    message(STATUS "Office support linked: libzip and pugixml")
endif()

# Link Kerberos/GSSAPI if enabled
if(THEMIS_ENABLE_KERBEROS)
    if(TARGET KRB5::krb5)
        target_link_libraries(themis_core PUBLIC KRB5::krb5)
        message(STATUS "Kerberos support linked: KRB5::krb5")
    endif()
    if(TARGET KRB5::gssapi)
        target_link_libraries(themis_core PUBLIC KRB5::gssapi)
        message(STATUS "Kerberos support linked: KRB5::gssapi")
    endif()
    if(TARGET Kerberos::Kerberos)
        target_link_libraries(themis_core PUBLIC Kerberos::Kerberos)
        message(STATUS "Kerberos support linked: Kerberos::Kerberos")
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_KERBEROS)
    message(STATUS "Kerberos/GSSAPI authentication enabled")
endif()

# Link ZSTD if available and define compile flag
if(DEFINED THEMIS_ZSTD_TARGET)
    target_link_libraries(themis_core PUBLIC ${THEMIS_ZSTD_TARGET})
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_ZSTD)
endif()

# Define THEMIS_ENABLE_CURL if CURL is found
if(CURL_FOUND)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CURL)
endif()

# RFC 3161 Timestamp Authority (TSA) - OpenSSL implementation
# OpenSSL is required for cryptographic operations, CURL for HTTP/HTTPS communication with TSA servers
if(THEMIS_USE_OPENSSL_TSA AND OpenSSL_FOUND AND CURL_FOUND)
    target_compile_definitions(themis_core PUBLIC THEMIS_USE_OPENSSL_TSA)
    message(STATUS "Timestamp Authority (RFC 3161): OpenSSL implementation enabled")
else()
    if(THEMIS_USE_OPENSSL_TSA AND NOT OpenSSL_FOUND)
        message(WARNING "RFC 3161 TSA: OpenSSL mode requested but OpenSSL not found - falling back to stub mode")
    endif()
    if(THEMIS_USE_OPENSSL_TSA AND NOT CURL_FOUND)
        message(WARNING "RFC 3161 TSA: OpenSSL mode requested but CURL not found - falling back to stub mode")
    endif()
    message(STATUS "Timestamp Authority (RFC 3161): Using stub implementation (development only - NOT for production)")
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
if(THEMIS_ENABLE_MIMALLOC AND mimalloc_FOUND)
    target_link_libraries(themis_core PUBLIC mimalloc-static)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MIMALLOC)
    message(STATUS "mimalloc linked to themis_core")
endif()

# FFmpeg support for video processing (optional)
if(FFMPEG_FOUND)
    target_link_libraries(themis_core PUBLIC FFmpeg::avformat)
    message(STATUS "FFmpeg linked to themis_core - real video processing enabled")
endif()

# ============================================================================
# PHASE 1 PERFORMANCE FEATURES
# ============================================================================

# RCU Index (Read-Copy-Update for lock-free reads, +200-500% for read-heavy workloads)
if(THEMIS_ENABLE_RCU_INDEX)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_RCU_INDEX)
    message(STATUS "RCU Index enabled (+200-500% read performance)")
endif()

# LIRS Cache (Low Inter-reference Recency Set, +30-40% cache hit rate)
if(THEMIS_ENABLE_LIRS_CACHE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_LIRS_CACHE)
    message(STATUS "LIRS Cache enabled (+30-40% hit rate improvement)")
endif()

# Huge Pages (Linux-only, Windows uses automatic large pages)
if(THEMIS_ENABLE_HUGE_PAGES)
    if(NOT WIN32)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HUGE_PAGES)
        message(STATUS "Huge pages enabled")
    else()
        message(STATUS "Huge pages disabled on Windows (automatic large pages used)")
    endif()
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

# GDAL Geospatial Processing Support
if(THEMIS_ENABLE_GDAL AND GDAL_FOUND)
    target_link_libraries(themis_core PUBLIC GDAL::GDAL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GDAL)
    message(STATUS "GDAL geospatial processing enabled and linked to themis_core")
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
        if(NOT TARGET gRPC::grpc++_reflection)
            # vcpkg static triplet may not export reflection target; try to create an imported target
            find_library(GRPCPP_REFLECTION_LIB NAMES grpc++_reflection
                PATHS
                    ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib
                    ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib
                NO_DEFAULT_PATH)
            if(GRPCPP_REFLECTION_LIB)
                add_library(gRPC::grpc++_reflection UNKNOWN IMPORTED)
                set_target_properties(gRPC::grpc++_reflection PROPERTIES
                    IMPORTED_LOCATION ${GRPCPP_REFLECTION_LIB}
                    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
                message(STATUS "gRPC reflection library found manually: ${GRPCPP_REFLECTION_LIB}")
            else()
                message(WARNING "gRPC reflection library not found; proceeding without reflection")
            endif()
        endif()

        if(TARGET gRPC::grpc++_reflection)
            target_link_libraries(themis_core PUBLIC gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf)
            message(STATUS "gRPC and Protobuf found - linking to themis_core (with reflection)")
        else()
            target_link_libraries(themis_core PUBLIC gRPC::grpc++ protobuf::libprotobuf)
            message(WARNING "gRPC::grpc++_reflection not found; linking without reflection to themis_core")
        endif()

        if(COMMAND protobuf_generate)
            # Generate shard RPC stubs (ApplyWalBatch, etc.)
            set(SHARD_PROTO ${THEMIS_ROOT_DIR}/proto/sharding/shard_rpc.proto)
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
                     -I ${THEMIS_ROOT_DIR}/proto/sharding
                     ${SHARD_PROTO}
                DEPENDS ${SHARD_PROTO} protobuf::protoc gRPC::grpc_cpp_plugin
                COMMENT "Generating protobuf and gRPC sources for shard_rpc.proto"
                VERBATIM
            )

            add_library(themis_shard_proto STATIC ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS})
            target_include_directories(themis_shard_proto PUBLIC ${SHARD_PROTO_GEN_DIR})
            target_link_libraries(themis_shard_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

            # Suppress warnings in generated protobuf files
            if(MSVC)
                target_compile_options(themis_shard_proto PRIVATE /wd4996 /wd4018 /wd4244 /wd4267)
            else()
                target_compile_options(themis_shard_proto PRIVATE -Wno-unused-parameter -Wno-deprecated-declarations -Wno-conversion)
            endif()

            target_link_libraries(themis_core PUBLIC themis_shard_proto)
        else()
            message(WARNING "protobuf_generate() not available; skipping shard RPC stub generation")
        endif()
    else()
        message(WARNING "gRPC/Protobuf not found - WAL gRPC service will be disabled at runtime")
    endif()
    # Fallback: if CONFIG packages not found, try pkg-config for apt-installed gRPC
    if(NOT gRPC_FOUND OR NOT Protobuf_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(GRPC QUIET grpc++)
            pkg_check_modules(PROTOBUF QUIET protobuf)
            if(GRPC_FOUND AND PROTOBUF_FOUND)
                target_link_libraries(themis_core PUBLIC ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
                target_include_directories(themis_core PUBLIC ${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})
                message(STATUS "gRPC and Protobuf found via pkg-config - linking to themis_core")
            endif()
        endif()
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
        ../src/llm/llama_wrapper.cpp
        ../src/llm/embedded_llm.cpp          # Embedded LLM facade for system-wide use
        ../src/llm/llm_plugin_manager.cpp
        ../src/llm/model_loader.cpp          # Ollama-style lazy loading
        ../src/llm/multi_lora_manager.cpp    # vLLM-style multi-LoRA
        ../src/llm/async_inference_engine.cpp # Async threading (independent from DB)
        # v1.3.1: Cache Reuse Integration (Phase 2)
        ../src/llm/model_metadata_cache.cpp  # Model metadata caching with ConcurrentCache
        ../src/llm/lora_metadata_cache.cpp   # LoRA metadata caching with ConcurrentCache
        
        # LoRA Adapter Framework (Issue #319) - RE-ENABLED WITH FIXED APIs
        ../src/llm/lora_framework/lora_orchestrator.cpp       # LoRA CRUD orchestrator
        ../src/llm/lora_framework/lora_adapter_manager.cpp    # Adapter lifecycle & caching
        # Note: lora_storage_service.cpp removed to avoid duplicate symbol errors
        # Only using lora_storage_service_themisdb.cpp which has complete ThemisDB + FileSystem implementations
        ../src/llm/lora_framework/lora_storage_service_themisdb.cpp  # ThemisDB storage backend (complete implementation)
        ../src/llm/lora_framework/lora_training_service.cpp   # Training pipeline (re-enabled for distributed training integration)
        ../src/llm/lora_framework/training_service_registry.cpp  # Service registry for dependency injection
        ../src/llm/lora_framework/lora_layers.cpp             # LoRA layer implementations
        ../src/llm/lora_framework/lora_audit_logger.cpp       # Audit logging
        ../src/llm/lora_framework/feedback_plugin.cpp         # Feedback plugin system
        ../src/llm/lora_framework/lora_feedback_storage.cpp   # Feedback storage service
        ../src/llm/lora_framework/lora_training_config.cpp    # YAML training configuration
        ../src/llm/lora_framework/lora_layers.cpp             # LoRA layers and tensor operations (Phase 1)
        ../src/llm/lora_framework/base_model_adapter.cpp      # Base model loading and LoRA injection (Phase 2)
        ../src/llm/lora_framework/data_loader.cpp             # Text data loading and tokenization (Phase 2)
        ../src/llm/lora_framework/llama_tokenizer.cpp         # llama.cpp native tokenizer integration
        # Production training features
        ../src/llm/lora_framework/mixed_precision.cpp         # Mixed precision training (FP16/BF16)
        ../src/llm/lora_framework/lr_scheduler.cpp            # Learning rate schedulers
        ../src/llm/lora_framework/gradient_utils.cpp          # Gradient clipping & accumulation
        ../src/llm/lora_framework/distributed_trainer.cpp     # Multi-GPU distributed training
        # QLoRA quantization support
        ../src/llm/lora_framework/quantization.cpp            # NF4/INT8 quantization (CPU)
        ../src/llm/lora_framework/quantized_model.cpp         # Quantized model management
        ../src/llm/distributed_training_coordinator.cpp       # Coordinator implementation (link fixes)
        ../src/llm/applications/themis_help_lora.cpp          # Themis Help LoRA app (link fixes)
        ../src/llm/security/signature_verifier.cpp            # LLM signature verification
        
        # Dynamic batch size adaptation (Issue #39)
        ../src/llm/lora_framework/adaptive_batcher.cpp        # Adaptive batch sizing for optimal GPU utilization
        ../src/llm/lora_framework/sequence_packer.cpp         # Sequence packing to reduce padding waste
        ../src/llm/lora_framework/gpu_utilization_monitor.cpp # GPU utilization monitoring (NVML/ROCm)
        
        ../src/llm/paged_block_manager.cpp   # Block management for PagedAttention (v1.4.0 prep)
        ../src/llm/kv_cache_buffer.cpp       # KV cache batching with VectorAutoBuffer (Phase 2.2)
        ../src/llm/llm_response_cache.cpp    # Response caching with SemanticCache (Phase 2.3)
        
        # Phase 3.2: Grammar-Constrained Generation
        ../src/llm/grammar.cpp               # Grammar parsing and compilation
        ../src/llm/grammar_cache.cpp         # Grammar caching for performance
        
        # Phase 4+5 Week 4-6: llama.cpp Integration
        ../src/llm/gguf_loader.cpp           # GGUF model file loader with memory-mapping
        ../src/llm/llamacpp_inference_engine.cpp # llama.cpp inference engine with PagedAttention
        ../src/llm/llm_model_storage.cpp     # LLM model storage and metadata management
        
        # Week 13-14: Production Validation & Testing (P1 Enterprise Features)
        ../src/llm/production_validator.cpp  # Production-grade LLM validation and benchmarking
        
        # P1: Enterprise Features - LLM Inference Engine Improvements
        ../src/llm/continuous_batch_scheduler.cpp  # vLLM-style continuous batching
        ../src/llm/llm_prefix_cache.cpp           # Context caching with prefix reuse
        ../src/llm/grafana_metrics.cpp            # Prometheus/Grafana metrics integration
        ../src/llm/inference_engine_enhanced.cpp  # Enhanced inference engine with P1 features
        
        # AI Decision Auditing and Explainability (EU AI Act, GDPR Compliance)
        ../src/llm/ai_decision_auditor.cpp        # AI decision auditing with cryptographic signing
        ../src/llm/explanation_generator.cpp      # Human-readable explanation generation
        ../src/llm/ethical_guidelines_manager.cpp # Ethical guidelines management
        
        # RAG Enhancement: Quality Evaluation and Gap Detection
        ../src/rag/llm_meta_analyzer.cpp          # Base class for LLM-based meta-analysis
        ../src/rag/rag_judge.cpp                  # LLM-as-Judge for RAG quality evaluation
        ../src/rag/knowledge_gap_detector.cpp     # Knowledge gap detection
        
        # P3: Vision Support (Multi-Modal)
        # vision_encoder.cpp is added conditionally below via THEMIS_ENABLE_VISION
        ../src/llm/gpu_memory_manager.cpp         # GPU memory management (CPU fallback when CUDA disabled)
        ../src/llm/kernel_fusion.cpp              # Kernel fusion with CPU fallback
    )

    if(THEMIS_ENABLE_GPU)
        target_sources(themis_core PRIVATE
            ../src/llm/lora_framework/gpu_lora_layers.cpp         # GPU LoRA layer implementations
            ../src/llm/lora_framework/gpu_tensor.cpp              # GPU tensor operations
            ../src/llm/lora_framework/gpu_memory.cpp              # GPU memory management
            ../src/llm/lora_framework/vram_allocator.cpp          # VRAM allocation and tracking
            ../src/llm/lora_framework/gpu_data_loader.cpp         # GPU data loader with async transfers
            ../src/llm/lora_framework/gpu_training_loop.cpp       # GPU training loop implementation
            ../src/llm/lora_framework/gpu_embedding_layer.cpp     # GPU embedding layer for real embeddings
        )
        message(STATUS "Including LoRA GPU sources (THEMIS_ENABLE_GPU=ON)")
    else()
        message(STATUS "Skipping LoRA GPU sources (THEMIS_ENABLE_GPU=OFF)")
    endif()

    # Add CLIP vision encoder only when explicitly enabled
    if(THEMIS_ENABLE_VISION)
        target_sources(themis_core PRIVATE
            ../src/llm/vision_encoder.cpp
        )
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_VISION=1)
        message(STATUS "LLM Vision: CLIP vision encoder enabled")
    else()
        message(STATUS "LLM Vision: disabled (set -DTHEMIS_ENABLE_VISION=ON to enable)")
    endif()
    
    # Add CUDA kernel fusion sources when CUDA is enabled
    if(THEMIS_ENABLE_CUDA)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/kernel_fusion.cu                        # Flash Attention CUDA kernels
            ../src/llm/lora_framework/kernels/cuda_kernels.cu  # LoRA CUDA kernels
            ../src/llm/lora_framework/kernels/quantization_kernels.cu  # QLoRA quantization GPU kernels
            ../src/llm/lora_framework/kernels/cuda_fused_kernels.cu  # LoRA fused CUDA kernels (Phase 10.3)
            ../src/llm/lora_framework/kernels/cuda_fp16_kernels.cu  # FP16 mixed precision kernels (Phase 10.4)
            ../src/llm/lora_framework/kernels/cuda_bf16_kernels.cu  # BF16 mixed precision kernels (Phase 10.4)
            ../src/llm/lora_framework/kernels/cuda_flash_lora_kernels.cu  # FlashLoRA memory-efficient kernels
        )
        message(STATUS "LLM: CUDA kernel fusion enabled (Flash Attention + LoRA)")
    endif()
    
    # Add HIP kernel sources when HIP is enabled
    if(THEMIS_ENABLE_HIP)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/lora_framework/kernels/hip_kernels.cpp  # LoRA HIP kernels (AMD)
        )
        message(STATUS "LoRA: HIP kernels enabled (AMD ROCm support)")
    endif()
    
    # Add Vulkan kernel sources when Vulkan is enabled
    if(THEMIS_ENABLE_VULKAN)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/lora_framework/kernels/vulkan_kernels.cpp  # LoRA Vulkan compute
        )
        message(STATUS "LoRA: Vulkan compute kernels enabled (cross-platform)")
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
            # Access internal llama grammar headers when needed
            target_include_directories(themis_core PRIVATE ${LLAMA_SRC_DIR}/src)
        endif()
        message(STATUS "LLM Plugin: Linked llama.cpp library")
    else()
        message(WARNING "LLM Plugin: llama.cpp target not found - plugin will be built without llama.cpp")
    endif()
    
    # LLM is already defined via THEMIS_LLM_ENABLED in earlier section
    message(STATUS "LLM Plugin: Source files added to themis_core")
    message(STATUS "  - Ollama-style lazy model loading enabled")
    message(STATUS "  - vLLM-style multi-LoRA management enabled")
    message(STATUS "  - LoRA Adapter Framework enabled (Issue #319)")
    message(STATUS "    * Full CRUD operations with BaseEntity storage")
    message(STATUS "    * Enterprise security (AES-256-GCM + Ed25519)")
    message(STATUS "    * Async job management for training/loading")
    message(STATUS "    * Complete audit trail for compliance")
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

# End of monolithic build configuration
endif() # THEMIS_BUILD_MODULAR

# ============================================================================
# EXECUTABLES AND TESTS (work with both modular and monolithic builds)
# ============================================================================

# Main executable (demo)
add_executable(themis_demo
    ../src/main.cpp
)

target_link_libraries(themis_demo
    PRIVATE
        themis_core
)

# Encryption demo executable
add_executable(themis_demo_encryption
    ../src/demo_encryption.cpp
)

target_link_libraries(themis_demo_encryption
    PRIVATE
        themis_core
)

# API Server executable
add_executable(themis_server
    ../src/main_server.cpp
)

target_link_libraries(themis_server
    PRIVATE
        themis_core
)

# Ensure themis_server does not directly link mimalloc; rely on themis_core
if(WIN32 AND THEMIS_ENABLE_MIMALLOC)
    get_target_property(_srv_libs themis_server LINK_LIBRARIES)
    if(_srv_libs)
        list(REMOVE_ITEM _srv_libs mimalloc mimalloc-debug mimalloc-static)
        set_target_properties(themis_server PROPERTIES LINK_LIBRARIES "${_srv_libs}")
        message(STATUS "Pruned mimalloc libs from themis_server link libraries")
    endif()
endif()

# Tests - Delegated to tests/CMakeLists.txt
if(THEMIS_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(${THEMIS_ROOT_DIR}/tests tests)

    # Leichtgewichtiger Test-Target: Nur SecondaryIndex-Tests
    add_executable(themis_secidx_tests
        ${THEMIS_ROOT_DIR}/tests/test_secondary_index.cpp
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
        ${THEMIS_ROOT_DIR}/tests/test_lib_rocksdb_integration.cpp
        ${THEMIS_ROOT_DIR}/tests/test_lib_hnsw_integration.cpp
        ${THEMIS_ROOT_DIR}/tests/test_lib_boost_integration.cpp
        ${THEMIS_ROOT_DIR}/tests/test_secondary_index.cpp
        ${THEMIS_ROOT_DIR}/tests/test_thread_pool_manager.cpp
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
        # LLM tests are now handled in tests/CMakeLists.txt via file(GLOB_RECURSE)

        # Lightweight LLM-only test target to avoid unrelated suite breakages
        # Disabled: test_llamacpp_integration.cpp does not exist
        # add_executable(themis_llm_tests
        #     ${THEMIS_ROOT_DIR}/tests/test_llamacpp_integration.cpp
        # )
        # target_link_libraries(themis_llm_tests
        #     PRIVATE
        #         themis_core
        #         GTest::gtest
        #         GTest::gtest_main
        #         Boost::system
        # )
        # if(TARGET llama)
        #     add_custom_command(TARGET themis_llm_tests POST_BUILD
        #         COMMAND ${CMAKE_COMMAND} -E copy_if_different
        #                 $<TARGET_FILE:llama>
        #                 $<TARGET_FILE_DIR:themis_llm_tests>
        #     )
        #     add_custom_command(TARGET themis_llm_tests POST_BUILD
        #         COMMAND ${CMAKE_COMMAND} -E copy_directory
        #                 $<TARGET_FILE_DIR:llama>
        #                 $<TARGET_FILE_DIR:themis_llm_tests>
        #     )
        # endif()
        # include(GoogleTest)
        # gtest_discover_tests(themis_llm_tests
        #     DISCOVERY_MODE PRE_TEST
        #     WORKING_DIRECTORY $<TARGET_FILE_DIR:themis_llm_tests>
        #     EXTRA_ARGS --gtest_color=yes
        #     CONFIGURATIONS Release Debug
        # )
        
        # Phase 1 Feature Tests (Flash Attention, KV-Cache Reuse, Embeddings)
        # Flash Attention Tests
        add_executable(test_phase1_flash_attention
            ${THEMIS_ROOT_DIR}/tests/test_phase1_flash_attention.cpp
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
            ${THEMIS_ROOT_DIR}/tests/test_phase1_kv_cache_reuse.cpp
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

    # Note: themis_tests discovery is now handled in tests/CMakeLists.txt
    
    # Phase 3 Performance Optimization Tests (separate executables)
    if(THEMIS_ENABLE_DISKANN)
        add_executable(test_diskann
            ${THEMIS_ROOT_DIR}/tests/performance/phase3/test_diskann.cpp
        )
        target_link_libraries(test_diskann
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_diskann
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_diskann>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_BWTREE)
        add_executable(test_bwtree
            ${THEMIS_ROOT_DIR}/tests/performance/phase3/test_bwtree.cpp
        )
        target_link_libraries(test_bwtree
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_bwtree
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_bwtree>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_SPLINTERDB)
        add_executable(test_splinterdb
            ${THEMIS_ROOT_DIR}/tests/performance/phase3/test_splinterdb.cpp
        )
        target_link_libraries(test_splinterdb
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_splinterdb
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_splinterdb>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_GUNROCK)
        add_executable(test_gunrock
            ${THEMIS_ROOT_DIR}/tests/performance/phase3/test_gunrock.cpp
        )
        target_link_libraries(test_gunrock
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_gunrock
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_gunrock>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    if(THEMIS_ENABLE_BAO)
        add_executable(test_bao
            ${THEMIS_ROOT_DIR}/tests/performance/phase3/test_bao.cpp
        )
        target_link_libraries(test_bao
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_bao
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_bao>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
    
    # GPU Vector Index Test
    if(THEMIS_ENABLE_GPU AND THEMIS_ENABLE_VECTOR_SEARCH)
        add_executable(test_gpu_vector_index
            ${THEMIS_ROOT_DIR}/tests/test_gpu_vector_index.cpp
        )
        target_link_libraries(test_gpu_vector_index
            PRIVATE
                themis_core
                GTest::gtest
                GTest::gtest_main
        )
        gtest_discover_tests(test_gpu_vector_index
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:test_gpu_vector_index>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
    endif()
endif()

# Benchmarks
option(THEMIS_BUILD_DOCKER_RAID_BENCHMARK "Build Docker RAID comprehensive benchmark" OFF)

if(THEMIS_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
    
    # Modern performance benchmarks (all core areas)
    add_executable(bench_core_performance
        ${THEMIS_ROOT_DIR}/benchmarks/bench_core_performance.cpp
    )
    
    target_link_libraries(bench_core_performance
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Comprehensive benchmark suite (simple/complex, LLM, AQL, binary, stress tests)
    add_executable(bench_comprehensive
        ${THEMIS_ROOT_DIR}/benchmarks/bench_comprehensive.cpp
    )
    
    target_link_libraries(bench_comprehensive
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Advanced patterns: Read/Write ratios, Parallelity, Self-Protection, Best-Practices, Gap-Analysis
    if(NOT TARGET bench_advanced_patterns)
        add_executable(bench_advanced_patterns
            ${THEMIS_ROOT_DIR}/benchmarks/bench_advanced_patterns.cpp
        )

        target_link_libraries(bench_advanced_patterns
            PRIVATE
                themis_core
                benchmark::benchmark
                benchmark::benchmark_main
        )
    endif()

    add_executable(bench_hotspots_micro
        ${THEMIS_ROOT_DIR}/benchmarks/bench_hotspots_micro.cpp
    )

    target_link_libraries(bench_hotspots_micro
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    add_executable(bench_lock_contention
        ${THEMIS_ROOT_DIR}/benchmarks/bench_lock_contention.cpp
    )
    target_link_libraries(bench_lock_contention
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    add_executable(bench_wal_stress
        ${THEMIS_ROOT_DIR}/benchmarks/bench_wal_stress.cpp
    )
    target_link_libraries(bench_wal_stress
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(themis_benchmarks
        ${THEMIS_ROOT_DIR}/benchmarks/bench_crud.cpp
        ${THEMIS_ROOT_DIR}/benchmarks/bench_query.cpp
        ${THEMIS_ROOT_DIR}/benchmarks/bench_vector_search.cpp
        ${THEMIS_ROOT_DIR}/benchmarks/bench_wal_apply_grpc.cpp
    )

    # Many legacy benchmark sources embed their own BENCHMARK_MAIN / int main().
    # Linking them all into the unified themis_benchmarks target produced duplicate
    # entry points on MSVC. Until those files are refactored to be main-free, keep
    # the unified binary minimal to avoid linker collisions.
    
    target_link_libraries(themis_benchmarks
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    if(TARGET llama)
        add_custom_command(TARGET themis_benchmarks POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:llama>
                    $<TARGET_FILE_DIR:themis_benchmarks>
        )
        add_custom_command(TARGET themis_benchmarks POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    $<TARGET_FILE_DIR:llama>
                    $<TARGET_FILE_DIR:themis_benchmarks>
        )
    endif()
    
    # Compression benchmark (standalone)
    add_executable(bench_compression
        ${THEMIS_ROOT_DIR}/benchmarks/bench_compression.cpp
    )
    
    target_link_libraries(bench_compression
        PRIVATE
            themis_core
            benchmark::benchmark
    )
    
    # Index rebuild benchmark (standalone)
    add_executable(bench_index_rebuild
        ${THEMIS_ROOT_DIR}/benchmarks/bench_index_rebuild.cpp
    )
    
    target_link_libraries(bench_index_rebuild
        PRIVATE
            themis_core
            benchmark::benchmark
    )
    
    # MVCC performance benchmark (standalone)
    add_executable(bench_mvcc
        ${THEMIS_ROOT_DIR}/benchmarks/bench_mvcc.cpp
    )
    
    target_link_libraries(bench_mvcc
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Encryption performance benchmark (standalone)
    add_executable(bench_encryption
        ${THEMIS_ROOT_DIR}/benchmarks/bench_encryption.cpp
    )
    
    target_link_libraries(bench_encryption
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # OLAP Analytics benchmark (v1.3.0 Phase 2)
    add_executable(bench_olap_analytics
        ${THEMIS_ROOT_DIR}/benchmarks/bench_olap_analytics.cpp
    )
    
    target_link_libraries(bench_olap_analytics
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Video Processor benchmark (v1.3.0 Phase 2)
    add_executable(bench_video_processor
        ${THEMIS_ROOT_DIR}/benchmarks/bench_video_processor.cpp
    )
    
    target_link_libraries(bench_video_processor
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Stream Protocol benchmark (v1.3.0 Phase 2)
    add_executable(bench_stream_protocol
        ${THEMIS_ROOT_DIR}/benchmarks/bench_stream_protocol.cpp
    )
    
    target_link_libraries(bench_stream_protocol
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Process Mining benchmark (v1.3.0 Phase 2)
    add_executable(bench_process_mining
        ${THEMIS_ROOT_DIR}/benchmarks/bench_process_mining.cpp
    )
    
    target_link_libraries(bench_process_mining
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Async I/O MultiScan benchmark (v1.3.0 Phase 2)
    add_executable(bench_async_io_multiscan
        ${THEMIS_ROOT_DIR}/benchmarks/bench_async_io_multiscan.cpp
    )
    
    target_link_libraries(bench_async_io_multiscan
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Hybrid AQL Syntax Sugar benchmark (Phase 2.5)
    add_executable(bench_hybrid_aql_sugar
        ${THEMIS_ROOT_DIR}/benchmarks/bench_hybrid_aql_sugar.cpp
    )
    
    target_link_libraries(bench_hybrid_aql_sugar
        PRIVATE
            themis_core
            benchmark::benchmark
    )

    # HSM Provider performance benchmark (standalone)
    add_executable(bench_hsm_provider
        ${THEMIS_ROOT_DIR}/benchmarks/bench_hsm_provider.cpp
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
        ${THEMIS_ROOT_DIR}/benchmarks/bench_shard_routing.cpp
    )
    
    target_link_libraries(bench_shard_routing
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Transaction Throughput benchmark
    add_executable(bench_transaction_throughput
        ${THEMIS_ROOT_DIR}/benchmarks/bench_transaction_throughput.cpp
    )
    
    target_link_libraries(bench_transaction_throughput
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Changefeed Throughput benchmark
    add_executable(bench_changefeed_throughput
        ${THEMIS_ROOT_DIR}/benchmarks/bench_changefeed_throughput.cpp
    )
    
    target_link_libraries(bench_changefeed_throughput
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # Timeseries Ingestion benchmark
    add_executable(bench_timeseries_ingestion
        ${THEMIS_ROOT_DIR}/benchmarks/bench_timeseries_ingestion.cpp
    )
    
    target_link_libraries(bench_timeseries_ingestion
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # SAGA Compensation benchmark
    add_executable(bench_saga_compensation
        ${THEMIS_ROOT_DIR}/benchmarks/bench_saga_compensation.cpp
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
        ${THEMIS_ROOT_DIR}/benchmarks/bench_phase1_flash_attention.cpp
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
        ${THEMIS_ROOT_DIR}/benchmarks/bench_graph_traversal.cpp
    )
    
    target_link_libraries(bench_graph_traversal
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Graph Query Optimizer benchmark
    add_executable(bench_graph_query_optimizer
        ${THEMIS_ROOT_DIR}/benchmarks/bench_graph_query_optimizer.cpp
    )
    
    target_link_libraries(bench_graph_query_optimizer
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # PageRank benchmark
    add_executable(bench_pagerank
        ${THEMIS_ROOT_DIR}/benchmarks/bench_pagerank.cpp
    )
    
    target_link_libraries(bench_pagerank
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # GPU Backends benchmark
    add_executable(bench_gpu_backends
        ${THEMIS_ROOT_DIR}/benchmarks/bench_gpu_backends.cpp
    )
    
    target_link_libraries(bench_gpu_backends
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # GPU Vector Index benchmark
    if(THEMIS_ENABLE_GPU AND THEMIS_ENABLE_VECTOR_SEARCH)
        add_executable(bench_gpu_vector_index
            ${THEMIS_ROOT_DIR}/benchmarks/bench_gpu_vector_index.cpp
        )
        
        target_link_libraries(bench_gpu_vector_index
            PRIVATE
                themis_core
                benchmark::benchmark
                benchmark::benchmark_main
        )
    endif()

    # GNN Embeddings benchmark
    add_executable(bench_gnn_embeddings
        ${THEMIS_ROOT_DIR}/benchmarks/bench_gnn_embeddings.cpp
    )
    
    target_link_libraries(bench_gnn_embeddings
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Phase 3: Content Management & Governance Benchmarks
    add_executable(bench_content_versioning
        ${THEMIS_ROOT_DIR}/benchmarks/bench_content_versioning.cpp
    )
    
    target_link_libraries(bench_content_versioning
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_text_extraction
        ${THEMIS_ROOT_DIR}/benchmarks/bench_text_extraction.cpp
    )
    
    target_link_libraries(bench_text_extraction
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_image_analysis
        ${THEMIS_ROOT_DIR}/benchmarks/bench_image_analysis.cpp
    )
    
    target_link_libraries(bench_image_analysis
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_image_analysis_latency
        ${THEMIS_ROOT_DIR}/benchmarks/bench_image_analysis_latency.cpp
    )
    
    target_link_libraries(bench_image_analysis_latency
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    add_executable(bench_policy_evaluation
        ${THEMIS_ROOT_DIR}/benchmarks/bench_policy_evaluation.cpp
    )
    
    target_link_libraries(bench_policy_evaluation
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    # Image Analysis Plugin Benchmark
    add_executable(benchmark_image_analysis
        ${THEMIS_ROOT_DIR}/benchmarks/benchmark_image_analysis.cpp
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
        ${THEMIS_ROOT_DIR}/benchmarks/bench_tpcc.cpp
    )
    
    target_link_libraries(bench_tpcc
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # YCSB: Yahoo Cloud Serving Benchmark (Workloads A-F)
    add_executable(bench_ycsb
        ${THEMIS_ROOT_DIR}/benchmarks/bench_ycsb.cpp
    )
    
    target_link_libraries(bench_ycsb
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # MMDB-E: Multi-Modal Database Benchmark with Embeddings (Industry-First)
    add_executable(bench_mmdb
        ${THEMIS_ROOT_DIR}/benchmarks/bench_mmdb.cpp
    )
    
    target_link_libraries(bench_mmdb
        PRIVATE
            themis_core
            benchmark::benchmark
            benchmark::benchmark_main
    )

    # TPC-H: Decision Support (OLAP) Benchmark - 22 Analytical Queries
    add_executable(bench_tpch
        ${THEMIS_ROOT_DIR}/benchmarks/bench_tpch.cpp
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
        ${THEMIS_ROOT_DIR}/benchmarks/bench_aql_functions.cpp
    )
    target_link_libraries(bench_aql_functions PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # ARM Memory Benchmark
    add_executable(bench_arm_memory
        ${THEMIS_ROOT_DIR}/benchmarks/bench_arm_memory.cpp
    )
    target_link_libraries(bench_arm_memory PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # ARM SIMD Benchmark
    add_executable(bench_arm_simd
        ${THEMIS_ROOT_DIR}/benchmarks/bench_arm_simd.cpp
    )
    target_link_libraries(bench_arm_simd PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Auto Buffers Benchmark
    add_executable(bench_auto_buffers
        ${THEMIS_ROOT_DIR}/benchmarks/bench_auto_buffers.cpp
    )
    target_link_libraries(bench_auto_buffers PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Blob ZSTD Compression Benchmark
    add_executable(bench_blob_zstd
        ${THEMIS_ROOT_DIR}/benchmarks/bench_blob_zstd.cpp
    )
    target_link_libraries(bench_blob_zstd PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # HNSW Prefilter Minimal Benchmark
    add_executable(bench_hnsw_prefilter_minimal
        ${THEMIS_ROOT_DIR}/benchmarks/bench_hnsw_prefilter_minimal.cpp
    )
    target_link_libraries(bench_hnsw_prefilter_minimal PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Hybrid Vector+Geo Benchmark
    add_executable(bench_hybrid_vector_geo
        ${THEMIS_ROOT_DIR}/benchmarks/bench_hybrid_vector_geo.cpp
    )
    target_link_libraries(bench_hybrid_vector_geo PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Lossy vs Lossless Compression Benchmark
    add_executable(bench_lossy_vs_lossless
        ${THEMIS_ROOT_DIR}/benchmarks/bench_lossy_vs_lossless.cpp
    )
    target_link_libraries(bench_lossy_vs_lossless PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Product Quantization Benchmark
    add_executable(bench_product_quantization
        ${THEMIS_ROOT_DIR}/benchmarks/bench_product_quantization.cpp
    )
    target_link_libraries(bench_product_quantization PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Sanity Tests Benchmark
    add_executable(bench_sanity
        ${THEMIS_ROOT_DIR}/benchmarks/bench_sanity.cpp
    )
    target_link_libraries(bench_sanity PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Sharding Performance Benchmark
    add_executable(bench_sharding_performance
        ${THEMIS_ROOT_DIR}/benchmarks/bench_sharding_performance.cpp
    )
    target_link_libraries(bench_sharding_performance PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # SIMD Distance Benchmark
    add_executable(bench_simd_distance
        ${THEMIS_ROOT_DIR}/benchmarks/bench_simd_distance.cpp
    )
    target_link_libraries(bench_simd_distance PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # Spatial Index Benchmark
    add_executable(bench_spatial_index
        ${THEMIS_ROOT_DIR}/benchmarks/bench_spatial_index.cpp
    )
    target_link_libraries(bench_spatial_index PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)

    # V1.3.0 Features Benchmark (Critical for Release Testing)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_v1_3_0_features.cpp")
        add_executable(bench_v1_3_0_features
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_v1_3_0_features.cpp
        )
        target_link_libraries(bench_v1_3_0_features PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # Vector Compression Lossless Benchmark
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_vector_compression_lossless.cpp")
        add_executable(bench_vector_compression_lossless
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_vector_compression_lossless.cpp
        )
        target_link_libraries(bench_vector_compression_lossless PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # Vector Prefilter Benchmark
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_vector_prefilter.cpp")
        add_executable(bench_vector_prefilter
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_vector_prefilter.cpp
        )
        target_link_libraries(bench_vector_prefilter PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # Phase 2 Performance Optimization Benchmarks
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/performance_optimizations/phase2/benchmark_phase2.cpp")
        add_executable(bench_phase2
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/performance_optimizations/phase2/benchmark_phase2.cpp
        )
        target_link_libraries(bench_phase2 PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # v1.3.4 Optimization Benchmarks
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_v1_3_4_optimizations.cpp")
        add_executable(bench_v1_3_4_optimizations
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_v1_3_4_optimizations.cpp
        )
        target_link_libraries(bench_v1_3_4_optimizations PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # v1.3.4 Batch Insert Benchmark (optional if source exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_batch_insert.cpp")
        add_executable(bench_batch_insert
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_batch_insert.cpp
        )
        target_link_libraries(bench_batch_insert PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # Simple insert test for debugging (optional if source exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_simple_insert_test.cpp")
        add_executable(bench_simple_insert_test
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_simple_insert_test.cpp
        )
        target_link_libraries(bench_simple_insert_test PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()

    # RAID and LoRA Adapter Benchmark (optional if source exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_raid_lora.cpp")
        add_executable(bench_raid_lora
            ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_raid_lora.cpp
        )
        target_link_libraries(bench_raid_lora PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
    endif()
    
    # QLoRA GPU Kernels Benchmark (only when CUDA is enabled)
    if(THEMIS_ENABLE_CUDA AND THEMIS_ENABLE_LLM)
        add_executable(bench_qlora_gpu_kernels
            ${THEMIS_ROOT_DIR}/benchmarks/bench_qlora_gpu_kernels.cpp
        )
        target_link_libraries(bench_qlora_gpu_kernels PRIVATE themis_core benchmark::benchmark benchmark::benchmark_main)
        message(STATUS "QLoRA GPU Kernels Benchmark: Enabled")
    endif()
    
    # Plugin Hot-Plug Monitoring Benchmark
    if(EXISTS "${THEMIS_ROOT_DIR}/benchmarks/bench_plugin_hot_plug.cpp")
        add_executable(bench_plugin_hot_plug
            ${THEMIS_ROOT_DIR}/benchmarks/bench_plugin_hot_plug.cpp
        )
        target_link_libraries(bench_plugin_hot_plug PRIVATE 
            themis_core 
            benchmark::benchmark 
            benchmark::benchmark_main
        )
        message(STATUS "Plugin Hot-Plug Monitoring Benchmark: Enabled")
    endif()

endif()

if(THEMIS_BUILD_DOCKER_RAID_BENCHMARK)
    find_package(benchmark CONFIG REQUIRED)
    find_package(prometheus-cpp CONFIG REQUIRED)
    # Docker RAID Comprehensive Benchmark (v1.3.4+)
    # 1+ hour benchmark suite for Docker multi-container RAID testing
    # Tests RAID0, RAID1, RAID5, RAID6, RAID10 configurations
    # See: ${THEMIS_ROOT_DIR}/benchmarks/DOCKER_RAID_BENCHMARK_SUITE_README.md
    add_executable(bench_docker_raid_comprehensive
        ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_docker_raid_comprehensive.cpp
    )
    target_link_libraries(bench_docker_raid_comprehensive PRIVATE 
        themis_core 
        benchmark::benchmark 
        benchmark::benchmark_main
        prometheus-cpp::pull
        prometheus-cpp::core
    )
    
    # GPU Erasure Coding Benchmark (conditionally compiled if CUDA is enabled)
    if(THEMIS_ENABLE_CUDA)
        add_executable(bench_gpu_erasure
            ${THEMIS_ROOT_DIR}/benchmarks/bench_gpu_erasure.cpp
        )
        target_link_libraries(bench_gpu_erasure PRIVATE 
            themis_core 
            benchmark::benchmark 
            benchmark::benchmark_main
        )
    endif()
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

install(DIRECTORY /include/
    DESTINATION include
)

# Performance Optimization Benchmarks
if(THEMIS_BUILD_BENCHMARKS)
    # add_subdirectory(benchmarks/performance_optimizations) # Directory does not exist
endif()


# Final MSVC cleanup: strip any invalid '/WX:NO' that might have been injected
if(MSVC)
    get_property(_allTargets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _allTargets)
        foreach(_prop IN ITEMS COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)
            get_target_property(_opts ${_t} ${_prop})
            if(_opts)
                list(TRANSFORM _opts REPLACE "/WX:NO" "")
                list(TRANSFORM _opts REPLACE "/WX:No" "")
                list(TRANSFORM _opts REPLACE "/WX:0"  "")
                set_target_properties(${_t} PROPERTIES ${_prop} "${_opts}")
            endif()
        endforeach()
    endforeach()
endif()





