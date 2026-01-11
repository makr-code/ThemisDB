# ThemisDB External Dependencies Management

# vcpkg Integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Prefer CONFIG packages (vcpkg) over FindXXX modules
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# ============================================================================
# REQUIRED DEPENDENCIES (core functionality)
# ============================================================================

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
    # Fallback to built-in FindOpenSSL when CONFIG package is missing
    find_package(OpenSSL REQUIRED)
endif()
message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")

find_package(ZLIB 1.3.1 REQUIRED)
message(STATUS "ZLIB found: ${ZLIB_VERSION}")

# RocksDB: Prefer CONFIG (vcpkg) and fallback to unofficial target if provided by vcpkg
find_package(RocksDB CONFIG QUIET)
if(RocksDB_FOUND)
    message(STATUS "RocksDB found")
else()
    # vcpkg often provides 'unofficial-rocksdb' with target 'unofficial::rocksdb'
    find_package(unofficial-rocksdb CONFIG QUIET)
    if(unofficial-rocksdb_FOUND)
        add_library(RocksDB::rocksdb ALIAS unofficial::rocksdb)
        message(STATUS "RocksDB found via vcpkg (unofficial)")
    else()
        message(FATAL_ERROR "RocksDB not found. Install via vcpkg (rocksdb) or system package librocksdb-dev.")
    endif()
endif()

find_package(simdjson CONFIG)
if(simdjson_FOUND)
    message(STATUS "simdjson found")
else()
    message(WARNING "simdjson not found - some features may be disabled")
endif()

find_package(TBB CONFIG)
if(TBB_FOUND)
    message(STATUS "TBB found")
else()
    message(WARNING "TBB not found - using fallback threading")
endif()

find_package(fmt REQUIRED CONFIG)
message(STATUS "fmt found")

find_package(spdlog REQUIRED CONFIG)
message(STATUS "spdlog found")

find_package(nlohmann_json REQUIRED CONFIG)
message(STATUS "nlohmann_json found")

# Boost: Try CONFIG first, fall back to MODULE if not found
find_package(Boost 1.70 CONFIG COMPONENTS system filesystem)
if(NOT Boost_FOUND)
    find_package(Boost 1.70 MODULE REQUIRED COMPONENTS system filesystem)
endif()
message(STATUS "Boost found: ${Boost_VERSION}")

find_package(Threads REQUIRED)
message(STATUS "Threads found")

find_package(OpenMP REQUIRED)
message(STATUS "OpenMP found")

# Protobuf (required for gRPC and general serialization)
find_package(Protobuf REQUIRED CONFIG)
message(STATUS "Protobuf found: ${Protobuf_VERSION}")

# gRPC (inter-shard communication)
# Priority: CONFIG, then pkg-config, then fallback
find_package(gRPC QUIET CONFIG)
if(NOT gRPC_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(gRPC QUIET grpc++ grpc)
        if(gRPC_FOUND)
            message(STATUS "gRPC found via pkg-config")
        endif()
    endif()
endif()

if(NOT gRPC_FOUND)
    message(FATAL_ERROR "gRPC not found. Install grpc-devel or configure VCPKG_ROOT")
endif()

message(STATUS "gRPC found")

# GTest (unit testing framework - required for tests)
if(THEMIS_BUILD_TESTS)
    find_package(GTest QUIET CONFIG)
    if(GTest_FOUND)
        message(STATUS "GTest found - tests enabled")
        add_compile_definitions(THEMIS_HAS_GTEST=1)
    else()
        message(WARNING "GTest not found - tests will not be built")
        message(WARNING "Install with: vcpkg install gtest OR apt-get install libgtest-dev")
        set(THEMIS_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    endif()
else()
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
endif()

# Google Benchmark (performance testing - required for benchmarks)
if(THEMIS_BUILD_BENCHMARKS)
    find_package(benchmark QUIET CONFIG)
    if(benchmark_FOUND)
        message(STATUS "Google Benchmark found - benchmarks enabled")
        add_compile_definitions(THEMIS_HAS_BENCHMARK=1)
    else()
        message(WARNING "Google Benchmark not found - benchmarks will not be built")
        message(WARNING "Install with: vcpkg install benchmark OR apt-get install libbenchmark-dev")
        set(THEMIS_BUILD_BENCHMARKS OFF CACHE BOOL "Build benchmarks" FORCE)
    endif()
else()
    message(STATUS "Benchmarks disabled (THEMIS_BUILD_BENCHMARKS=OFF)")
endif()

# Prometheus C++ Client (metrics - optional for LoRA framework)
find_package(prometheus-cpp QUIET CONFIG)
if(prometheus-cpp_FOUND)
    message(STATUS "Prometheus C++ client found - metrics enabled")
    add_compile_definitions(THEMIS_HAS_PROMETHEUS=1)
else()
    message(STATUS "Prometheus C++ client not found - metrics collection disabled")
    message(STATUS "Install with: vcpkg install prometheus-cpp (optional)")
endif()

# ============================================================================
# OPTIONAL DEPENDENCIES (features)
# ============================================================================

# CURL (HTTP client, optional - some features disabled if missing)
find_package(CURL QUIET CONFIG)
if(NOT CURL_FOUND)
    find_package(CURL QUIET)
endif()

if(CURL_FOUND)
    message(STATUS "CURL found - enabling HTTP client features")
    add_compile_definitions(THEMIS_HAS_CURL=1)
else()
    message(WARNING "CURL not found - some HTTP features will be disabled")
endif()

# Arrow + Parquet (Parquet export support)
find_package(Arrow QUIET CONFIG)
find_package(Parquet QUIET CONFIG)

if(Arrow_FOUND)
    message(STATUS "Arrow found - enabling Parquet export")
    add_compile_definitions(THEMIS_HAS_ARROW=1)
    if(Parquet_FOUND)
        add_compile_definitions(THEMIS_HAS_PARQUET=1)
    endif()
else()
    message(STATUS "Arrow not found - Parquet export disabled")
endif()

# YAML (configuration parsing)
find_package(yaml-cpp QUIET CONFIG)
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found")
else()
    message(WARNING "yaml-cpp not found - configuration features may be limited")
endif()

# zstd (compression codec)
find_package(zstd QUIET CONFIG)
if(zstd_FOUND)
    message(STATUS "zstd found - enabling Zstandard compression")
    add_compile_definitions(THEMIS_HAS_ZSTD=1)
else()
    message(STATUS "zstd not found - using fallback compression")
endif()

# HNSW library (vector indexing)
find_package(hnswlib QUIET CONFIG)
if(hnswlib_FOUND AND NOT THEMIS_ENABLE_GPU)
    message(STATUS "hnswlib found - enabling HNSW vector search")
else()
    if(THEMIS_ENABLE_GPU)
        message(STATUS "GPU enabled - using GPU vector search, HNSW optional")
    else()
        message(STATUS "hnswlib not found - using fallback vector search")
    endif()
endif()

# mimalloc (fast memory allocator, optional)
if(THEMIS_ENABLE_MIMALLOC)
    find_package(mimalloc QUIET CONFIG)
    if(mimalloc_FOUND)
        message(STATUS "mimalloc found - enabling high-performance memory allocation")
        add_compile_definitions(THEMIS_HAS_MIMALLOC=1)
    else()
        message(FATAL_ERROR "THEMIS_ENABLE_MIMALLOC=ON but mimalloc not found")
    endif()
endif()

# OpenTelemetry (distributed tracing and observability)
if(THEMIS_ENABLE_TRACING)
    find_package(opentelemetry-cpp REQUIRED CONFIG)
    message(STATUS "OpenTelemetry-cpp found - enabling distributed tracing")
    add_compile_definitions(THEMIS_ENABLE_TRACING=1)
endif()

# ============================================================================
# PROTOCOL-SPECIFIC DEPENDENCIES
# ============================================================================

# HTTP/2 support
if(THEMIS_ENABLE_HTTP2)
    find_package(nghttp2 QUIET)
    if(nghttp2_FOUND)
        message(STATUS "nghttp2 found - enabling HTTP/2 support")
        add_compile_definitions(THEMIS_ENABLE_HTTP2=1)
    else()
        message(FATAL_ERROR "THEMIS_ENABLE_HTTP2=ON but nghttp2 not found")
    endif()
endif()

# HTTP/3 support
if(THEMIS_ENABLE_HTTP3)
    find_package(nghttp3 QUIET)
    find_package(ngtcp2 QUIET)
    if(nghttp3_FOUND AND ngtcp2_FOUND)
        message(STATUS "nghttp3 + ngtcp2 found - enabling HTTP/3 support")
        add_compile_definitions(THEMIS_ENABLE_HTTP3=1)
    else()
        message(FATAL_ERROR "THEMIS_ENABLE_HTTP3=ON but nghttp3 or ngtcp2 not found")
    endif()
endif()

# ============================================================================
# HARDWARE ACCELERATION DEPENDENCIES
# ============================================================================

# CUDA (GPU acceleration)
if(THEMIS_ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "CUDA Toolkit found: ${CUDAToolkit_VERSION}")
    add_compile_definitions(THEMIS_ENABLE_CUDA=1)
    
    # Optional: FAISS for GPU-accelerated vector search
    find_package(faiss QUIET)
    if(faiss_FOUND)
        message(STATUS "FAISS found - enabling GPU vector search")
        add_compile_definitions(THEMIS_HAS_FAISS=1)
    else()
        message(STATUS "FAISS not found - using CuBLAS for vector operations")
    endif()
endif()

# HIP (AMD GPU acceleration) - optional alternative to CUDA
if(THEMIS_ENABLE_HIP)
    find_package(HIP REQUIRED)
    message(STATUS "HIP found - enabling AMD GPU support")
    add_compile_definitions(THEMIS_ENABLE_HIP=1)
endif()

# ============================================================================
# LLM DEPENDENCIES
# ============================================================================

if(THEMIS_ENABLE_LLM)
    # Ensure C language is enabled so OpenMP::OpenMP_C target exists
    enable_language(C)
    # OpenMP MUST be found before llama.cpp configuration
    # because ggml-config.cmake references OpenMP::OpenMP_C target
    find_package(OpenMP REQUIRED)
    message(STATUS "OpenMP found for LLM support")
    
    # Check if llama.cpp is available as a target or library
    find_package(llama QUIET CONFIG)
    
    if(NOT llama_FOUND)
        # Try to find via pkg-config
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(llama QUIET llama)
        endif()
    endif()
    
    if(NOT llama_FOUND)
        message(FATAL_ERROR 
            "THEMIS_ENABLE_LLM=ON but llama.cpp not found.\n"
            "Please install llama.cpp or set llama_DIR to its build directory")
    endif()
    
    message(STATUS "llama.cpp found - enabling LLM plugin support")
    add_compile_definitions(THEMIS_ENABLE_LLM=1)
    
    # Voice assistant support (requires Whisper, Piper)
    if(THEMIS_ENABLE_VOICE_ASSISTANT)
        if(THEMIS_ENABLE_WHISPER)
            find_package(whisper QUIET CONFIG)
            if(whisper_FOUND)
                message(STATUS "Whisper.cpp found - enabling Speech-to-Text")
                add_compile_definitions(THEMIS_ENABLE_WHISPER=1)
            else()
                message(FATAL_ERROR "THEMIS_ENABLE_WHISPER=ON but whisper.cpp not found")
            endif()
        endif()
        
        if(THEMIS_ENABLE_PIPER_TTS)
            find_package(piper-phoneme-ids QUIET CONFIG)
            if(piper-phoneme-ids_FOUND)
                message(STATUS "Piper TTS found - enabling Text-to-Speech")
                add_compile_definitions(THEMIS_ENABLE_PIPER_TTS=1)
            else()
                message(FATAL_ERROR "THEMIS_ENABLE_PIPER_TTS=ON but Piper TTS not found")
            endif()
        endif()
    endif()
endif()

# ============================================================================
# BENCHMARK-SPECIFIC DEPENDENCIES
# ============================================================================

if(THEMIS_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED CONFIG)
    message(STATUS "Google Benchmark found")
    
    # Docker RAID benchmark extras
    if(THEMIS_BUILD_DOCKER_RAID_BENCHMARK)
        find_package(prometheus-cpp QUIET CONFIG)
        if(prometheus-cpp_FOUND)
            message(STATUS "prometheus-cpp found - enabling RAID benchmark metrics")
            add_compile_definitions(THEMIS_HAS_PROMETHEUS=1)
        endif()
    endif()
endif()

# ============================================================================
# SUMMARY
# ============================================================================

message(STATUS "============================================")
message(STATUS "ThemisDB Dependencies Summary")
message(STATUS "============================================")
message(STATUS "Required: OpenSSL, RocksDB, gRPC, Protobuf, GTest")
message(STATUS "Optional: CURL, Arrow, Parquet, mimalloc, OpenTelemetry")
message(STATUS "Protocols: HTTP/2=${THEMIS_ENABLE_HTTP2}, HTTP/3=${THEMIS_ENABLE_HTTP3}")
message(STATUS "Features: LLM=${THEMIS_ENABLE_LLM}, GPU=${THEMIS_ENABLE_GPU}, CUDA=${THEMIS_ENABLE_CUDA}")
message(STATUS "============================================")

