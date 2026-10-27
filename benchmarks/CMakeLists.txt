# ThemisDB Benchmarks CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building benchmarks
if(NOT THEMIS_BUILD_BENCHMARKS)
    message(STATUS "Benchmarks disabled (THEMIS_BUILD_BENCHMARKS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB benchmark suite")

# Per team policy: suppress warnings for benchmarks only.
# Core/library targets keep their warning policy unchanged.
if(MSVC)
    add_compile_options(/W0)
    message(STATUS "MSVC: compiler warnings are suppressed for benchmarks")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-w)
    message(STATUS "Compiler warnings are suppressed for benchmarks")
endif()

function(themis_apply_benchmark_release_flags target_name)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /O2
            /DNDEBUG
        )
        return()
    endif()

    set(_themis_benchmark_arch_flags "${BENCHMARK_ARCH_FLAGS}")
    if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
        set(_themis_benchmark_arch_flags "-march=native")
    endif()

    target_compile_options(${target_name} PRIVATE
        -O3
        ${_themis_benchmark_arch_flags}
        -DNDEBUG
    )
endfunction()

# ============================================================================
# Standalone Benchmarks (no Google Benchmark required – custom main())
# These benchmarks use synthetic data and have no external dependencies beyond
# the packages already required for the importer build.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/importers/bench_importer_throughput.cpp")
    message(STATUS "Adding Importer Throughput benchmarks (standalone, no live broker)")

    add_executable(bench_importer_throughput
        importers/bench_importer_throughput.cpp
    )

    target_include_directories(bench_importer_throughput PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_importer_throughput PRIVATE
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_importer_throughput PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_importer_throughput)

    install(TARGETS bench_importer_throughput
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Importer Throughput: PostgreSQL COPY/INSERT, SQLite, MongoDB NDJSON, Kafka mock-import (AC-8/AC-9)")
endif()

# Find Google Benchmark
find_package(benchmark QUIET CONFIG)
if(NOT benchmark_FOUND)
    message(WARNING "Google Benchmark not found - benchmarks will not be built")
    message(WARNING "Install benchmark or use vcpkg: vcpkg install benchmark")
    return()
endif()

message(STATUS "Google Benchmark found - building benchmarks")

set(THEMIS_BENCHMARK_ROOT "${THEMIS_ROOT_DIR}")
if(NOT THEMIS_BENCHMARK_ROOT)
    get_filename_component(THEMIS_BENCHMARK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

include("${THEMIS_BENCHMARK_ROOT}/cmake/SuiteGating.cmake")
themis_log_suite_context("Benchmarks")

# Canonical CTest registration helper for benchmark smoke entries.
# Must be included before any add_subdirectory() that uses
# themis_register_benchmark_ctest().
include("${CMAKE_CURRENT_LIST_DIR}/cmake/BenchmarkPolicy.cmake")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/proto_generated
)

# Common benchmark libraries
set(BENCHMARK_LIBS
    benchmark::benchmark
    benchmark::benchmark_main
)

function(themis_add_standard_benchmark target_name source_file)
    set(_benchmark_source "${source_file}")
    if(IS_ABSOLUTE "${source_file}")
        set(_benchmark_source "${source_file}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
        set(_benchmark_source "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
    elseif(EXISTS "${source_file}")
        set(_benchmark_source "${source_file}")
    endif()

    if(NOT EXISTS "${_benchmark_source}")
        message(STATUS "Benchmark source not found at ${source_file}")
        return()
    endif()

    if(TARGET ${target_name})
        return()
    endif()

    add_executable(${target_name}
            "${_benchmark_source}"
    )

    target_link_libraries(${target_name} PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    # Propagate module-specific deps for benchmarks that rely on plugin headers
    # (e.g. user_storage_encrypted uses include/user_storage_encrypted/*.hpp).
    # If the matching module target exists, link it privately so its include
    # directories and compile definitions are available to the benchmark.
    if(TARGET themis_user_storage_encrypted)
        target_link_libraries(${target_name} PRIVATE themis_user_storage_encrypted)
    endif()

    target_compile_definitions(${target_name} PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(${target_name})

    install(TARGETS ${target_name}
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
endfunction()

# Module-style benchmark layout (analogous to tests/<module>/CMakeLists.txt).
# Modules that provide their own benchmark CMake registration are wired here.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/access_model/CMakeLists.txt")
    add_subdirectory(access_model)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ai/CMakeLists.txt")
    add_subdirectory(ai)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/CMakeLists.txt")
    add_subdirectory(analytics)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/aql/CMakeLists.txt")
    add_subdirectory(aql)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/CMakeLists.txt")
    add_subdirectory(core)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/epic1_retrieval/CMakeLists.txt")
    add_subdirectory(epic1_retrieval)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/epic2_evaluation/CMakeLists.txt")
    add_subdirectory(epic2_evaluation)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/epic3_distributed_tensor/CMakeLists.txt")
    add_subdirectory(epic3_distributed_tensor)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/performance_optimizations/CMakeLists.txt")
    add_subdirectory(performance_optimizations)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/query/CMakeLists.txt")
    add_subdirectory(query)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rag/CMakeLists.txt")
    add_subdirectory(rag)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm_wiki/CMakeLists.txt")
    add_subdirectory(llm_wiki)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/server/CMakeLists.txt")
    add_subdirectory(server)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/transaction/CMakeLists.txt")
    add_subdirectory(transaction)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wave8/CMakeLists.txt")
    add_subdirectory(wave8)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wave7/CMakeLists.txt")
    add_subdirectory(wave7)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wave5/CMakeLists.txt")
    add_subdirectory(wave5)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wave4/CMakeLists.txt")
    add_subdirectory(wave4)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wave2/CMakeLists.txt")
    add_subdirectory(wave2)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/phase0/CMakeLists.txt")
    add_subdirectory(phase0)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/document/CMakeLists.txt")
    add_subdirectory(document)
endif()

# Wave 3D module benchmark subdirectories (contract + release-gate benchmarks)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/performance/CMakeLists.txt")
    add_subdirectory(performance)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/governance/CMakeLists.txt")
    add_subdirectory(governance)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/metadata/CMakeLists.txt")
    add_subdirectory(metadata)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/content/CMakeLists.txt")
    add_subdirectory(content)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/utils/CMakeLists.txt")
    add_subdirectory(utils)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/updates/CMakeLists.txt")
    add_subdirectory(updates)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/projects/CMakeLists.txt")
    add_subdirectory(projects)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/toolbox/CMakeLists.txt")
    add_subdirectory(toolbox)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/process/CMakeLists.txt")
    add_subdirectory(process)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/themis/CMakeLists.txt")
    add_subdirectory(themis)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/search/CMakeLists.txt")
    add_subdirectory(search)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vector_search/CMakeLists.txt")
    add_subdirectory(vector_search)
endif()
# Wave 2 module benchmark subdirectories (security/network/storage/geo)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/CMakeLists.txt")
    add_subdirectory(security)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/network/CMakeLists.txt")
    add_subdirectory(network)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/storage/CMakeLists.txt")
    add_subdirectory(storage)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/geo/CMakeLists.txt")
    add_subdirectory(geo)
endif()
# Wave 3B module benchmark subdirectories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/failover/CMakeLists.txt")
    add_subdirectory(failover)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/observability/CMakeLists.txt")
    add_subdirectory(observability)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/distributed_knowledge/CMakeLists.txt")
    add_subdirectory(distributed_knowledge)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/CMakeLists.txt")
    add_subdirectory(exporters)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/importers/CMakeLists.txt")
    add_subdirectory(importers)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ingestion/CMakeLists.txt")
    add_subdirectory(ingestion)
endif()
# Wave 3A module benchmark subdirectories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/replication/CMakeLists.txt")
    add_subdirectory(replication)
endif()
# Wave D module benchmark subdirectories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sharding/CMakeLists.txt")
    add_subdirectory(sharding)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graph/CMakeLists.txt")
    add_subdirectory(graph)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temporal/CMakeLists.txt")
    add_subdirectory(temporal)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/timeseries/CMakeLists.txt")
    add_subdirectory(timeseries)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tensor/CMakeLists.txt")
    add_subdirectory(tensor)
endif()

# Wave D dedicated benchmark gates
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/training/CMakeLists.txt")
    add_subdirectory(training)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/acceleration/CMakeLists.txt")
    add_subdirectory(acceleration)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/CMakeLists.txt")
    add_subdirectory(gpu)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/distributed_tensor/CMakeLists.txt")
    add_subdirectory(distributed_tensor)
endif()

themis_add_standard_benchmark(bench_ycsb ycsb/bench_ycsb.cpp)

# ============================================================================
# Phase 3 Optimization Benchmarks (P3-05)
# ============================================================================

    themis_add_standard_benchmark(bench_phase3_optimization performance_optimizations/bench_phase3_optimization.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_gpu_module.cpp" AND NOT TARGET bench_gpu_module)
    message(STATUS "Adding GPU Module benchmarks")

    add_executable(bench_gpu_module
        gpu/bench_gpu_module.cpp
    )

    target_link_libraries(bench_gpu_module PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_gpu_module PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(THEMIS_ENABLE_GPU)
        target_compile_definitions(bench_gpu_module PRIVATE THEMIS_ENABLE_GPU=1)
    endif()

    themis_apply_benchmark_release_flags(bench_gpu_module)

    install(TARGETS bench_gpu_module
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
endif()

# ============================================================================
# LoRA Framework Benchmarks (NEW)
# ============================================================================

# Check for LoRA framework benchmark file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_lora_framework.cpp")
    message(STATUS "Adding LoRA framework benchmarks")
    
    add_executable(bench_lora_framework
        llm/bench_lora_framework.cpp
    )
    
    target_link_libraries(bench_lora_framework PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_lora_framework PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_lora_framework)
    
    # Install benchmark
    install(TARGETS bench_lora_framework
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
else()
    message(STATUS "LoRA framework benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_lora_framework.cpp")
endif()

# Multi-LoRA Fusion Benchmarks (v1.5.0)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_multi_lora_fusion.cpp")
    message(STATUS "Adding Multi-LoRA fusion benchmarks")
    
    add_executable(bench_multi_lora_fusion
        llm/bench_multi_lora_fusion.cpp
    )
    
    target_link_libraries(bench_multi_lora_fusion PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_multi_lora_fusion PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_multi_lora_fusion)
    
    # Install benchmark
    install(TARGETS bench_multi_lora_fusion
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    # Custom target to run Multi-LoRA fusion benchmarks
    add_custom_target(run_benchmark_multi_lora_fusion
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bench_multi_lora_fusion
        DEPENDS bench_multi_lora_fusion
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running Multi-LoRA fusion benchmarks..."
    )
else()
    message(STATUS "Multi-LoRA fusion benchmarks skipped (LLM disabled or file missing)")
endif()

# LoRA Auto-Binding Benchmarks
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_lora_auto_binding.cpp")
    message(STATUS "Adding LoRA auto-binding benchmarks")
    
    add_executable(bench_lora_auto_binding
        llm/bench_lora_auto_binding.cpp
    )
    
    target_link_libraries(bench_lora_auto_binding PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_lora_auto_binding PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_lora_auto_binding)
    
    # Install benchmark
    install(TARGETS bench_lora_auto_binding
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    # Custom target to run LoRA auto-binding benchmarks
    add_custom_target(run_benchmark_lora_auto_binding
        COMMAND bench_lora_auto_binding --benchmark_min_time=0.5
        DEPENDS bench_lora_auto_binding
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running LoRA auto-binding benchmarks..."
    )
    
    message(STATUS "  LoRA auto-binding: Auto-binding, context switching, caching, eviction")
else()
    message(STATUS "LoRA auto-binding benchmarks skipped (LLM disabled or file missing)")
endif()

# ============================================================================
# Fused Kernels Benchmarks (Phase 10.3)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_fused_kernels.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Fused Kernels benchmarks (CUDA/HIP)")
    
    add_executable(bench_fused_kernels
        gpu/bench_fused_kernels.cpp
    )
    
    target_link_libraries(bench_fused_kernels PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_fused_kernels PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_fused_kernels PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_fused_kernels PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_fused_kernels)
    
    # Install benchmark
    install(TARGETS bench_fused_kernels
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Benchmark targets: Forward/Backward/Optimizer (Fused vs Unfused)")
    message(STATUS "  Expected speedup: Forward 1.5-1.8x, Backward 1.7-2.0x, Overall 1.5-2x")
else()
    if(NOT (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
        message(STATUS "Fused kernels benchmarks skipped (CUDA/HIP not enabled)")
    else()
        message(STATUS "Fused kernels benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_fused_kernels.cpp")
    endif()
endif()

# Fused LoRA Kernels Benchmarks (Issue #36)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_fused_lora_kernels.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Fused LoRA Kernels benchmarks (CUDA/HIP)")
    
    add_executable(bench_fused_lora_kernels
        gpu/bench_fused_lora_kernels.cpp
    )
    
    target_link_libraries(bench_fused_lora_kernels PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_fused_lora_kernels PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_fused_lora_kernels PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_fused_lora_kernels PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_fused_lora_kernels)
    
    # Install benchmark
    install(TARGETS bench_fused_lora_kernels
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  LoRA Benchmark configurations: Varying batch sizes, ranks, dimensions")
    message(STATUS "  Expected speedup: Forward 1.5-3x, Backward 1.7-3x, Memory bandwidth reduction ~66%")
else()
    if(NOT (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
        message(STATUS "Fused LoRA kernels benchmarks skipped (CUDA/HIP not enabled)")
    else()
        message(STATUS "Fused LoRA kernels benchmarks skipped (LLM disabled or file missing)")
    endif()
endif()

# ============================================================================
# Snapshot Manager Benchmarks (Named Snapshots Feature)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maintenance/bench_snapshot_manager.cpp")
    message(STATUS "Adding Snapshot Manager benchmarks")
    
    add_executable(bench_snapshot_manager
        maintenance/bench_snapshot_manager.cpp
    )
    
    target_link_libraries(bench_snapshot_manager PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_snapshot_manager PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_snapshot_manager)
    
    # Install benchmark
    install(TARGETS bench_snapshot_manager
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
else()
    message(STATUS "Snapshot Manager benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_snapshot_manager.cpp")
endif()

# ============================================================================
# GPU Training Benchmarks (PR #2)
# ============================================================================

# 1. End-to-End Training Cycle Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_gpu_training_cycle.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding GPU Training Cycle benchmarks")
    
    add_executable(bench_gpu_training_cycle
        gpu/bench_gpu_training_cycle.cpp
    )
    
    target_link_libraries(bench_gpu_training_cycle PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_gpu_training_cycle PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_gpu_training_cycle PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_gpu_training_cycle PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_gpu_training_cycle)
    
    install(TARGETS bench_gpu_training_cycle
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  GPU Training Cycle: forward + backward + optimizer steps")
endif()

# 2. Multi-GPU Scaling Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_multi_gpu_scaling.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Multi-GPU Scaling benchmarks")
    
    add_executable(bench_multi_gpu_scaling
        gpu/bench_multi_gpu_scaling.cpp
    )
    
    target_link_libraries(bench_multi_gpu_scaling PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_multi_gpu_scaling PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_multi_gpu_scaling PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_multi_gpu_scaling PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_multi_gpu_scaling)
    
    install(TARGETS bench_multi_gpu_scaling
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Multi-GPU: data parallelism scaling efficiency")
endif()

# 2.1 Multi-GPU LoRA Advanced Benchmarks (v1.5.0)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_multi_gpu_lora_advanced.cpp")
    message(STATUS "Adding Multi-GPU LoRA Advanced benchmarks")
    
    add_executable(bench_multi_gpu_lora_advanced
        gpu/bench_multi_gpu_lora_advanced.cpp
    )
    
    target_link_libraries(bench_multi_gpu_lora_advanced PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_multi_gpu_lora_advanced PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_multi_gpu_lora_advanced)
    
    install(TARGETS bench_multi_gpu_lora_advanced
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Multi-GPU LoRA: resource-aware eviction, dynamic scheduling, migration")
else()
    message(STATUS "Multi-GPU LoRA Advanced benchmarks skipped (LLM disabled or file missing)")
endif()

# 3. Mixed Precision Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_mixed_precision_perf.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Mixed Precision Performance benchmarks")
    
    add_executable(bench_mixed_precision_perf
        gpu/bench_mixed_precision_perf.cpp
    )
    
    target_link_libraries(bench_mixed_precision_perf PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_mixed_precision_perf PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_mixed_precision_perf)
    
    install(TARGETS bench_mixed_precision_perf
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Mixed Precision: FP32 vs FP16 throughput comparison")
endif()

# 4. Data Transfer Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_data_transfer.cpp" AND THEMIS_ENABLE_LLM)
    message(STATUS "Adding Data Transfer benchmarks")
    
    add_executable(bench_data_transfer
        gpu/bench_data_transfer.cpp
    )
    
    target_link_libraries(bench_data_transfer PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        themis_llm
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_data_transfer PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_data_transfer)
    
    install(TARGETS bench_data_transfer
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Data Transfer: CPU↔GPU bandwidth and async prefetching")
endif()

# 5. Backend Comparison Benchmarks
if(THEMIS_ENABLE_LLM AND THEMIS_ENABLE_GPU AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_backend_comparison.cpp")
    message(STATUS "Adding Backend Comparison benchmarks")
    
    add_executable(bench_backend_comparison
        gpu/bench_backend_comparison.cpp
    )
    
    target_link_libraries(bench_backend_comparison PRIVATE
        themis_core
        themis_llm

        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_backend_comparison PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_backend_comparison PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_backend_comparison PRIVATE THEMIS_ENABLE_HIP=1)
    endif()
    if(THEMIS_ENABLE_VULKAN)
        target_compile_definitions(bench_backend_comparison PRIVATE THEMIS_ENABLE_VULKAN=1)
    endif()

    themis_apply_benchmark_release_flags(bench_backend_comparison)
    
    install(TARGETS bench_backend_comparison
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Backend Comparison: CUDA vs HIP vs Vulkan vs CPU")
else()
    message(STATUS "Backend Comparison benchmarks skipped (GPU disabled or file missing)")
endif()

# ============================================================================
# GPU Backends Benchmark (Issue #5 — M05 GPU matrix)
# gpu/bench_gpu_backends.cpp uses per-backend #ifdef guards (THEMIS_ENABLE_CUDA,
# THEMIS_ENABLE_HIP, THEMIS_ENABLE_VULKAN) — always builds; GPU sections
# activate at compile time when the matching flag is ON.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_gpu_backends.cpp")
    message(STATUS "Adding GPU Backends benchmarks (CPU always + GPU per-backend flags)")

    add_executable(bench_gpu_backends
        gpu/bench_gpu_backends.cpp
    )

    target_link_libraries(bench_gpu_backends PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_gpu_backends PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_gpu_backends PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_gpu_backends PRIVATE THEMIS_ENABLE_HIP=1)
    endif()
    if(THEMIS_ENABLE_VULKAN)
        target_compile_definitions(bench_gpu_backends PRIVATE THEMIS_ENABLE_VULKAN=1)
    endif()

    themis_apply_benchmark_release_flags(bench_gpu_backends)

    install(TARGETS bench_gpu_backends
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    if(THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP OR THEMIS_ENABLE_VULKAN)
        message(STATUS "  GPU Backends: real CUDA/HIP/Vulkan distance computation benchmarks enabled")
    else()
        message(STATUS "  GPU Backends: CPU-only path (no GPU flags set); GPU sections will skip at runtime")
    endif()
endif()

# ============================================================================
# Vulkan LoRA Benchmark (Issue #5 — M05 GPU matrix)
# Guards with #ifndef THEMIS_ENABLE_GPU — real benchmarks require
# THEMIS_ENABLE_GPU=ON (and ideally THEMIS_ENABLE_VULKAN for Vulkan path).
# ============================================================================

set(_themis_vulkan_sdk_available OFF)
if(TARGET Vulkan::Vulkan OR Vulkan_FOUND)
    set(_themis_vulkan_sdk_available ON)
endif()

if(THEMIS_ENABLE_GPU AND THEMIS_ENABLE_VULKAN AND _themis_vulkan_sdk_available AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_vulkan_lora.cpp")
    message(STATUS "Adding Vulkan LoRA benchmarks (THEMIS_ENABLE_GPU=ON, Vulkan SDK found)")

    add_executable(bench_vulkan_lora
        gpu/bench_vulkan_lora.cpp
    )

    target_link_libraries(bench_vulkan_lora PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_vulkan_lora PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(NOT TARGET vulkan_lora_shaders)
        set(_themis_glslc_hints
            "${Vulkan_GLSLC_EXECUTABLE}"
            "$ENV{VULKAN_SDK}/Bin"
            "$ENV{VULKAN_SDK}/bin"
        )

        find_program(_themis_glslc glslc HINTS ${_themis_glslc_hints})
        if(_themis_glslc)
            set(_themis_vulkan_lora_shader_src_dir "${THEMIS_BENCHMARK_ROOT}/src/acceleration/vulkan/shaders/lora")
            set(_themis_vulkan_lora_shader_out_dir "${CMAKE_BINARY_DIR}/shaders/lora")
            file(MAKE_DIRECTORY "${_themis_vulkan_lora_shader_out_dir}")

            set(_themis_vulkan_lora_shader_outputs)
            foreach(_themis_shader IN ITEMS matmul elementwise gradient)
                set(_themis_shader_src "${_themis_vulkan_lora_shader_src_dir}/${_themis_shader}.comp")
                set(_themis_shader_out "${_themis_vulkan_lora_shader_out_dir}/${_themis_shader}.comp.spv")
                add_custom_command(
                    OUTPUT "${_themis_shader_out}"
                    COMMAND "${_themis_glslc}" -o "${_themis_shader_out}" "${_themis_shader_src}"
                    DEPENDS "${_themis_shader_src}"
                    COMMENT "Compiling Vulkan LoRA shader: ${_themis_shader}.comp"
                    VERBATIM
                )
                list(APPEND _themis_vulkan_lora_shader_outputs "${_themis_shader_out}")
            endforeach()

            add_custom_target(vulkan_lora_shaders
                DEPENDS ${_themis_vulkan_lora_shader_outputs}
                COMMENT "Building Vulkan LoRA shaders"
            )
        else()
            message(WARNING "glslc not found - bench_vulkan_lora will require pre-compiled shaders")
        endif()
    endif()

    if(THEMIS_ENABLE_VULKAN)
        target_compile_definitions(bench_vulkan_lora PRIVATE THEMIS_ENABLE_VULKAN=1)
    endif()
    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_vulkan_lora PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_vulkan_lora PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_vulkan_lora)

    if(TARGET vulkan_lora_shaders)
        add_dependencies(bench_vulkan_lora vulkan_lora_shaders)
    endif()

    install(TARGETS bench_vulkan_lora
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Vulkan LoRA: forward pass, memory bandwidth, pipeline throughput")
else()
    message(STATUS "Vulkan LoRA benchmarks skipped (GPU/Vulkan disabled, Vulkan SDK missing, or file missing)")
endif()

# ============================================================================
# LoRA GPU Benchmark (Issue #5 — M05 GPU matrix)
# Guards with #ifndef THEMIS_ENABLE_GPU — real benchmarks require
# THEMIS_ENABLE_GPU=ON.
# ============================================================================

if(THEMIS_ENABLE_GPU AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_lora_gpu.cpp")
    message(STATUS "Adding LoRA GPU benchmarks (THEMIS_ENABLE_GPU=ON)")

    add_executable(bench_lora_gpu
        gpu/bench_lora_gpu.cpp
    )

    target_link_libraries(bench_lora_gpu PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_lora_gpu PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_GPU=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_lora_gpu PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_lora_gpu PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_lora_gpu)

    install(TARGETS bench_lora_gpu
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LoRA GPU: inference latency, batch throughput, memory bandwidth")
else()
    message(STATUS "LoRA GPU benchmarks skipped (THEMIS_ENABLE_GPU=OFF or file missing)")
endif()

# ============================================================================
# RAG Ethics Benchmarks
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_ethics.cpp")
    message(STATUS "Adding RAG Ethics benchmarks")
    
    add_executable(bench_rag_ethics
        rag/bench_rag_ethics.cpp
    )
    
    target_link_libraries(bench_rag_ethics PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        themis_llm
        spdlog::spdlog
    )
    
    target_compile_definitions(bench_rag_ethics PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_rag_ethics)
    
    # Install benchmark
    install(TARGETS bench_rag_ethics
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
else()
    message(STATUS "RAG Ethics benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_ethics.cpp")
endif()

# ============================================================================
# Benchmark Utilities
# ============================================================================

# Custom target to run all benchmarks (LLM-only when enabled)
if(THEMIS_ENABLE_LLM)
    add_custom_target(run_benchmarks
        COMMAND bench_lora_framework --benchmark_out=benchmark_results.json --benchmark_out_format=json
        DEPENDS bench_lora_framework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all ThemisDB benchmarks..."
    )
endif()

# Custom target to run benchmarks with various filters (LLM-only when enabled)
if(THEMIS_ENABLE_LLM)
    add_custom_target(run_benchmarks_quick
        COMMAND bench_lora_framework --benchmark_filter="Storage|Manager" --benchmark_min_time=0.1
        DEPENDS bench_lora_framework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running quick benchmarks..."
    )
endif()

if(THEMIS_ENABLE_LLM)
    add_custom_target(run_benchmarks_detailed
        COMMAND bench_lora_framework --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
        DEPENDS bench_lora_framework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running detailed benchmarks with statistics..."
    )
endif()

# ============================================================================
# Knowledge Gap Detector Phase 2 Benchmarks (NEW)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rag/bench_knowledge_gap_detector_phase2.cpp")
    message(STATUS "Adding Knowledge Gap Detector Phase 2 benchmarks")
    
    add_executable(bench_knowledge_gap_detector_phase2
        rag/bench_knowledge_gap_detector_phase2.cpp
    )
    
    target_link_libraries(bench_knowledge_gap_detector_phase2 PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_knowledge_gap_detector_phase2 PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_knowledge_gap_detector_phase2)
    
    # Install benchmark
    install(TARGETS bench_knowledge_gap_detector_phase2
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    # Custom target to run Knowledge Gap Detector Phase 2 benchmarks
    add_custom_target(run_benchmark_knowledge_gap_phase2
        COMMAND bench_knowledge_gap_detector_phase2 --benchmark_min_time=0.5
        DEPENDS bench_knowledge_gap_detector_phase2
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running Knowledge Gap Detector Phase 2 benchmarks..."
    )
    
    message(STATUS "  Knowledge Gap Detector Phase 2: Token probability, perplexity, self-consistency, FLARE")
else()
    message(STATUS "Knowledge Gap Detector Phase 2 benchmarks not found")
endif()

# ============================================================================
# RAG Hybrid Retriever Benchmarks (Phase 3: BM25 + vector, configurable RRF)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_hybrid_retriever.cpp")
    message(STATUS "Adding RAG Hybrid Retriever benchmarks")

    add_executable(bench_rag_hybrid_retriever
        rag/bench_rag_hybrid_retriever.cpp
    )

    target_link_libraries(bench_rag_hybrid_retriever PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_rag_hybrid_retriever PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_rag_hybrid_retriever)

    install(TARGETS bench_rag_hybrid_retriever
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    add_custom_target(run_benchmark_rag_hybrid_retriever
        COMMAND bench_rag_hybrid_retriever --benchmark_min_time=0.5
        DEPENDS bench_rag_hybrid_retriever
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running RAG Hybrid Retriever benchmarks..."
    )

    message(STATUS "  RAG Hybrid Retriever: RRF fusion, linear combination, disjoint candidates, factory")
else()
    message(STATUS "RAG Hybrid Retriever benchmarks not found")
endif()

# ============================================================================
# RAG Evaluation Benchmarks (recall@K, latency per mode, distributed, security)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_evaluation.cpp")
    message(STATUS "Adding RAG Evaluation benchmarks")

    add_executable(bench_rag_evaluation
        rag/bench_rag_evaluation.cpp
    )

    target_link_libraries(bench_rag_evaluation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        themis_llm
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_rag_evaluation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_rag_evaluation)

    install(TARGETS bench_rag_evaluation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    add_custom_target(run_benchmark_rag_evaluation
        COMMAND bench_rag_evaluation --benchmark_min_time=0.5
        DEPENDS bench_rag_evaluation
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running RAG Evaluation benchmarks (recall@K, latency, security)..."
    )

    message(STATUS "  RAG Evaluation: recall@K, FAST/BALANCED/THOROUGH latency, distributed, injection scan")
else()
    message(STATUS "RAG Evaluation benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_evaluation.cpp")
endif()

# ============================================================================
# Advanced Patterns Benchmarks (Sharding Strategy)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/bench_advanced_patterns.cpp")
    message(STATUS "Adding Advanced Patterns benchmarks")
    
    add_executable(bench_advanced_patterns
        base/bench_advanced_patterns.cpp
    )
    
    target_link_libraries(bench_advanced_patterns PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_advanced_patterns PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_advanced_patterns)
    
    # Install benchmark
    install(TARGETS bench_advanced_patterns
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    # Custom target to run Advanced Patterns benchmarks
    add_custom_target(run_benchmark_advanced_patterns
        COMMAND bench_advanced_patterns --benchmark_min_time=0.5
        DEPENDS bench_advanced_patterns
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running Advanced Patterns benchmarks..."
    )
    
    message(STATUS "  Advanced Patterns: Parallel scalability, sharding strategy, read/write ratios")
else()
    message(STATUS "Advanced Patterns benchmarks not found")
endif()

# ============================================================================
# Distributed Coordinator Benchmarks
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sharding/bench_distributed_coordinator.cpp" AND NOT TARGET bench_distributed_coordinator)
    message(STATUS "Adding Distributed Coordinator benchmarks")
    
    add_executable(bench_distributed_coordinator
        sharding/bench_distributed_coordinator.cpp
    )
    
    target_link_libraries(bench_distributed_coordinator PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_distributed_coordinator PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_distributed_coordinator)
    
    # Install benchmark
    install(TARGETS bench_distributed_coordinator
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    # Custom target to run Distributed Coordinator benchmarks
    add_custom_target(run_benchmark_distributed_coordinator
        COMMAND bench_distributed_coordinator --benchmark_min_time=0.5
        DEPENDS bench_distributed_coordinator
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running Distributed Coordinator benchmarks..."
    )
    
    message(STATUS "  Distributed Coordinator: Leader election, task scheduling, failover")
else()
    message(STATUS "Distributed Coordinator benchmarks not found")
endif()

# ============================================================================
# Graph Traversal Benchmarks (parallel multi-source BFS/DFS, graph analytics)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_graph_traversal.cpp")
    message(STATUS "Adding Graph Traversal benchmarks")

    add_executable(bench_graph_traversal
        graph/bench_graph_traversal.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(bench_graph_traversal PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_graph_traversal PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    themis_apply_benchmark_release_flags(bench_graph_traversal)

    install(TARGETS bench_graph_traversal
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Graph Traversal: BFS/DFS, parallel multi-source traversal, fan-out threshold, graph analytics")
endif()

# TensorFingerprintGraph Benchmarks
# Targets: insert ≤ 10 ms / 100 K nodes, findSimilar ≤ 50 ms / 100 K nodes
# (include/graph/tensor_fingerprint_graph.h §Performance, Phase 4, Q2 2027)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_tensor_fingerprint_graph.cpp")
    message(STATUS "Adding TensorFingerprintGraph benchmarks")

    add_executable(bench_tensor_fingerprint_graph
        tensor/bench_tensor_fingerprint_graph.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/tensor_fingerprint_graph.cpp
    )

    target_include_directories(bench_tensor_fingerprint_graph PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_tensor_fingerprint_graph PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_tensor_fingerprint_graph PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    target_compile_features(bench_tensor_fingerprint_graph PRIVATE cxx_std_17)

    themis_apply_benchmark_release_flags(bench_tensor_fingerprint_graph)

    install(TARGETS bench_tensor_fingerprint_graph
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  TensorFingerprintGraph: insert throughput, findSimilar latency, concurrent reads")
endif()

# Graph Query Optimizer Benchmarks (cost-based algorithm selection, plan caching)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_graph_query_optimizer.cpp")
    message(STATUS "Adding Graph Query Optimizer benchmarks")

    add_executable(bench_graph_query_optimizer
        graph/bench_graph_query_optimizer.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/graph_query_optimizer.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/path_constraints.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(bench_graph_query_optimizer PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_graph_query_optimizer PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    themis_apply_benchmark_release_flags(bench_graph_query_optimizer)

    install(TARGETS bench_graph_query_optimizer
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Graph Query Optimizer: BFS/DFS/Dijkstra/A*, plan caching, parallel execution, adaptive cost model")
endif()

# ============================================================================
# Tensor Fingerprint Graph Benchmarks (Phase 8.5 acceptance criteria)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_tensor_fingerprint.cpp")
    message(STATUS "Adding Tensor Fingerprint Graph benchmarks (Phase 8.5)")

    add_executable(bench_tensor_fingerprint
        tensor/bench_tensor_fingerprint.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/tensor_fingerprint_graph.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/tensor_deduplication_manager.cpp
    )

    target_link_libraries(bench_tensor_fingerprint PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        themis_graph
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_tensor_fingerprint PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_tensor_fingerprint)

    install(TARGETS bench_tensor_fingerprint
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Tensor Fingerprint: insert ≤10ms, findSimilar ≤50ms/100K nodes, ≥40% storage reduction")
endif()

# ============================================================================
# Real Performance Benchmarks (Issue: Implement Real Google Benchmark Tests)
# ============================================================================

# Storage Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/storage/bench_storage_performance.cpp")
    message(STATUS "Adding Storage Performance benchmarks")
    
    add_executable(bench_storage_performance
        storage/bench_storage_performance.cpp
    )
    
    target_link_libraries(bench_storage_performance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(bench_storage_performance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_storage_performance)
    
    install(TARGETS bench_storage_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Storage: mimalloc, huge pages, RCU index")
endif()

# Storage Hotspot CRUD Benchmarks (Wave-2 P1)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/bench_hotspots_micro.cpp")
    message(STATUS "Adding Storage Hotspot CRUD benchmarks")

    add_executable(bench_hotspots_micro
        base/bench_hotspots_micro.cpp
    )

    target_link_libraries(bench_hotspots_micro PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_hotspots_micro PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_hotspots_micro)

    install(TARGETS bench_hotspots_micro
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Storage Hotspots: raw write, mixed read/write, secondary index; owner target for Wave-2 CRUD cases")
endif()

# Random Access with Prefetch Benchmarks (v1.4.1)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/performance/bench_random_access_prefetch.cpp")
    message(STATUS "Adding Random Access Prefetch benchmarks")
    
    add_executable(bench_random_access_prefetch
        performance/bench_random_access_prefetch.cpp
    )
    
    target_link_libraries(bench_random_access_prefetch PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(bench_random_access_prefetch PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_random_access_prefetch)
    
    install(TARGETS bench_random_access_prefetch
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Random Access: CPU prefetch hints, data locality")
endif()

# OLAP Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/bench_olap_performance.cpp")
    message(STATUS "Adding OLAP Performance benchmarks")
    
    add_executable(bench_olap_performance
        analytics/bench_olap_performance.cpp
    )
    
    target_link_libraries(bench_olap_performance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(bench_olap_performance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_olap_performance)
    
    install(TARGETS bench_olap_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  OLAP: Aggregations, GROUP BY, filters at scale")
endif()

# OLAP Analytics Benchmarks (≥4 productive cases, Maßnahme #2 / AN goals)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/bench_olap_analytics.cpp")
    message(STATUS "Adding OLAP Analytics benchmarks")

    add_executable(bench_olap_analytics
        analytics/bench_olap_analytics.cpp
    )

    target_link_libraries(bench_olap_analytics PRIVATE
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_olap_analytics PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_olap_analytics)

    install(TARGETS bench_olap_analytics
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  OLAP Analytics: GroupBy, WindowFunction, MultiJoin, TopN (AN-GB/WIN/JOIN/TOPN)")
endif()

# Embedding Cache Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cache/bench_embedding_cache_performance.cpp")
    message(STATUS "Adding Embedding Cache Performance benchmarks")
    
    add_executable(bench_embedding_cache_performance
        cache/bench_embedding_cache_performance.cpp
    )
    
    target_link_libraries(bench_embedding_cache_performance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(bench_embedding_cache_performance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_embedding_cache_performance)
    
    install(TARGETS bench_embedding_cache_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Embedding Cache: HNSW, hit/miss, batch operations")
endif()

# Adaptive Query Cache Performance Benchmarks (Issue #1597)
# Covers the Production Readiness Checklist item "Performance benchmarks" for
# the multi-level L1/L2/L3 adaptive query cache (src/cache/).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/query/bench_adaptive_query_cache.cpp")
    message(STATUS "Adding Adaptive Query Cache performance benchmarks")

    add_executable(bench_adaptive_query_cache
        query/bench_adaptive_query_cache.cpp
    )

    target_link_libraries(bench_adaptive_query_cache PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_adaptive_query_cache PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_adaptive_query_cache)

    install(TARGETS bench_adaptive_query_cache
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Adaptive Query Cache: L1/L2 put/get, miss, tenant isolation, circuit breaker, concurrent access, invalidation")
endif()

# Cache Eviction Coordinator Integration Benchmarks (Phase 3: Q4 2026)
# Measures latency of cache→coordinator→storage event flow
# Gates: L1→L2 promotion ≤50µs, cache→storage feedback ≤100µs
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cache/bench_cache_eviction_coordinator.cpp")
   message(STATUS "Adding Cache Eviction Coordinator benchmarks (Phase 3)")

   add_executable(bench_cache_eviction_coordinator
       cache/bench_cache_eviction_coordinator.cpp
   )

   target_include_directories(bench_cache_eviction_coordinator PRIVATE
       ${CMAKE_SOURCE_DIR}/include
       ${CMAKE_SOURCE_DIR}/src
   )

   target_link_libraries(bench_cache_eviction_coordinator PRIVATE
       ${BENCHMARK_LIBS}
       themis_core
       spdlog::spdlog
       Threads::Threads
   )

   target_compile_definitions(bench_cache_eviction_coordinator PRIVATE
       THEMIS_BENCHMARK_BUILD=1
   )

   themis_apply_benchmark_release_flags(bench_cache_eviction_coordinator)

   install(TARGETS bench_cache_eviction_coordinator
       RUNTIME DESTINATION bin/benchmarks
       COMPONENT benchmarks
   )

   message(STATUS "  Cache Eviction Coordinator: event emission, listener delivery, promotion/demotion latency (GATE-CAP-01..06)")
endif()

# LLM Speculative Decoding & Inference Engine Benchmarks
# Runs automatically on every PR touching async_inference_engine.cpp or
# inference_engine_enhanced.cpp.  Acceptance criterion: tokens/sec regression < 5 %.
    if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/llm_bench.cpp")
    message(STATUS "Adding LLM speculative-decoding benchmarks")

    add_executable(llm_bench
        llm/llm_bench.cpp
        ${CMAKE_SOURCE_DIR}/src/llama_cpp/llama_cpp_plugin.cpp
        ${CMAKE_SOURCE_DIR}/src/llama_cpp/llama_cpp_registrar.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/rag_context_assembler.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/targ_retrieval.cpp
    )

    target_link_libraries(llm_bench PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    if(TARGET themis_llm_api)
        target_link_libraries(llm_bench PRIVATE themis_llm_api)
    endif()
    if(TARGET themis_sharding)
        target_link_libraries(llm_bench PRIVATE themis_sharding)
    endif()
    if(TARGET themis_geo)
        target_link_libraries(llm_bench PRIVATE themis_geo)
    endif()
    if(TARGET CURL::libcurl)
        target_link_libraries(llm_bench PRIVATE CURL::libcurl)
    endif()
    if(TARGET pugixml::pugixml)
        target_link_libraries(llm_bench PRIVATE pugixml::pugixml)
    elseif(TARGET pugixml::static)
        target_link_libraries(llm_bench PRIVATE pugixml::static)
    elseif(TARGET pugixml)
        target_link_libraries(llm_bench PRIVATE pugixml)
    endif()

    target_compile_definitions(llm_bench PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(llm_bench)

    install(TARGETS llm_bench
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LLM: Speculative decoding verify(), InferenceEngineEnhanced throughput")
endif()

# LLM Inference Performance Benchmarks
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_llm_inference_performance.cpp")
    message(STATUS "Adding LLM Inference Performance benchmarks")
    
    add_executable(bench_llm_inference_performance
        llm/bench_llm_inference_performance.cpp
    )
    
    target_link_libraries(bench_llm_inference_performance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(bench_llm_inference_performance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_llm_inference_performance)
    
    install(TARGETS bench_llm_inference_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    themis_register_benchmark_ctest(
        NAME         "bench_llm_inference_performance_smoke"
        TARGET       "bench_llm_inference_performance"
        MODULE       "llm"
        KIND         "wave_a"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_a_benchmark_server_llm wave_a_benchmark_search_index_tensor_graph_llm benchmark-baseline
    )
    
    message(STATUS "  LLM: Inference, LoRA adapters, throughput")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_llm_hotpaths.cpp" AND NOT TARGET bench_llm_hotpaths)
    add_executable(bench_llm_hotpaths
        llm/bench_llm_hotpaths.cpp
    )
    target_include_directories(bench_llm_hotpaths PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(bench_llm_hotpaths PRIVATE
        themis_core ${BENCHMARK_LIBS} OpenSSL::SSL OpenSSL::Crypto
        spdlog::spdlog nlohmann_json::nlohmann_json RocksDB::rocksdb Threads::Threads
    )
    themis_apply_benchmark_release_flags(bench_llm_hotpaths)
    install(TARGETS bench_llm_hotpaths
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
endif()

# LLM Model Loading from ThemisDB Benchmarks
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_SOURCE_DIR}/tests/llm/bench_model_loading_from_themisdb.cpp")
    message(STATUS "Adding LLM Model Loading from ThemisDB benchmarks")

    add_executable(bench_llm_model_loading_from_themisdb
        ${CMAKE_SOURCE_DIR}/tests/llm/bench_model_loading_from_themisdb.cpp
    )

    target_include_directories(bench_llm_model_loading_from_themisdb PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_llm_model_loading_from_themisdb PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(bench_llm_model_loading_from_themisdb PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_LLM_EXPORTS
    )

    themis_apply_benchmark_release_flags(bench_llm_model_loading_from_themisdb)

    install(TARGETS bench_llm_model_loading_from_themisdb
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LLM: Model store/load latency (1KB/1MB/10MB), SHA256 verification, temp-file cleanup")
endif()

# ActiveVRAMAllocator Benchmarks (LLM-MISSING-001)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_active_vram_allocator.cpp")
    message(STATUS "Adding ActiveVRAMAllocator benchmarks")

    add_executable(bench_active_vram_allocator
        gpu/bench_active_vram_allocator.cpp
    )

    target_include_directories(bench_active_vram_allocator PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(bench_active_vram_allocator PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_active_vram_allocator PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_LLM_EXPORTS
    )

    themis_apply_benchmark_release_flags(bench_active_vram_allocator)

    install(TARGETS bench_active_vram_allocator
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ActiveVRAMAllocator: alloc/free at 4KB/1MB/64MB, batch alloc, evictLRU, defragment, spillToCPU, getStats")
endif()

# ============================================================================
# Rotary Embeddings Benchmarks (NEW)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_rotary_embeddings.cpp")
    message(STATUS "Adding Rotary Embeddings benchmarks")
    
    add_executable(bench_rotary_embeddings
        llm/bench_rotary_embeddings.cpp
    )
    
    target_link_libraries(bench_rotary_embeddings PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_rotary_embeddings PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_rotary_embeddings)
    
    install(TARGETS bench_rotary_embeddings
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Rotary Embeddings: Single rotation, batch, relational, VectorIndex integration")
endif()

# ============================================================================
# GPU VRAM Allocation Benchmarks (NEW - vLLM-inspired)
# ============================================================================

if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_gpu_vram_allocation.cpp")
    message(STATUS "Adding GPU VRAM Allocation benchmarks")
    
    add_executable(bench_gpu_vram_allocation
        gpu/bench_gpu_vram_allocation.cpp
    )
    
    target_link_libraries(bench_gpu_vram_allocation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        RocksDB::rocksdb
        Threads::Threads
    )
    if(TARGET themis_llm)
        target_link_libraries(bench_gpu_vram_allocation PRIVATE themis_llm)
    endif()
    
    target_compile_definitions(bench_gpu_vram_allocation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_gpu_vram_allocation)
    
    install(TARGETS bench_gpu_vram_allocation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  GPU VRAM: Allocation planning, paged KV-cache, multi-GPU, mixed precision")
else()
    message(STATUS "GPU VRAM allocation benchmarks skipped (LLM disabled or file missing)")
endif()

message(STATUS "Benchmarks configured successfully")
message(STATUS "  - Build LoRA benchmarks with: cmake --build . --target bench_lora_auto_binding")

# ============================================================================
# HotReloadManager Benchmarks
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maintenance/bench_hot_reload_manager.cpp")
    message(STATUS "Adding HotReloadManager benchmarks")

    add_executable(bench_hot_reload_manager
        maintenance/bench_hot_reload_manager.cpp
    )

    target_link_libraries(bench_hot_reload_manager PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_hot_reload_manager PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_hot_reload_manager)

    install(TARGETS bench_hot_reload_manager
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  HotReloadManager: registration, reload fast-fail, callback dispatch, concurrency")
endif()
message(STATUS "  - Run LoRA benchmarks with: ./benchmarks/bench_lora_auto_binding")
message(STATUS "  - Or use: make run_benchmark_lora_auto_binding")
message(STATUS "  - Build all benchmarks with: cmake --build . --target bench_lora_framework")
message(STATUS "  - Run all benchmarks with: ./benchmarks/bench_lora_framework")
message(STATUS "  - Or use: make run_benchmarks")
message(STATUS "  - Performance benchmarks: bench_storage_performance, bench_olap_performance,")
message(STATUS "    bench_embedding_cache_performance, bench_adaptive_query_cache, bench_llm_inference_performance, bench_rotary_embeddings, bench_gpu_vram_allocation")

# ============================================================================
# CUDA vs CPU Acceleration Benchmarks (Issue #1375 / Q3 2026)
# Compares CPU and CUDA kernels for ANN (L2, Cosine, Inner-Product, Top-K)
# and geospatial (Haversine distance, Point-in-polygon) operations.
# CPU benchmarks always build; CUDA benchmarks activate when
# THEMIS_ENABLE_CUDA is set and skip gracefully at runtime when no GPU
# is present.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_cuda_vs_cpu.cpp")
    message(STATUS "Adding CUDA vs CPU acceleration benchmarks")

    add_executable(bench_cuda_vs_cpu
        gpu/bench_cuda_vs_cpu.cpp
    )

    target_link_libraries(bench_cuda_vs_cpu PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_cuda_vs_cpu PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_cuda_vs_cpu PRIVATE
            THEMIS_ENABLE_CUDA=1
        )
    endif()

    themis_apply_benchmark_release_flags(bench_cuda_vs_cpu)

    install(TARGETS bench_cuda_vs_cpu
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  CUDA vs CPU: ANN L2/Cosine/InnerProduct/TopK, Geo Haversine/PointInPolygon")
    if(THEMIS_ENABLE_CUDA)
        message(STATUS "  CUDA benchmarks: ENABLED (THEMIS_ENABLE_CUDA is set)")
    else()
        message(STATUS "  CUDA benchmarks: DISABLED (THEMIS_ENABLE_CUDA not set; CPU-only benchmarks active)")
    endif()
endif()

# ============================================================================
# DI Overhead and Logging Throughput Benchmarks (Issue #1420)
# Measures ConcernsContext creation cost, NoOpLogger/SpdlogLoggerAdapter
# throughput, structured log and trace-context injection overhead, and
# convenience wrapper dispatch cost.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/observability/bench_di_logging.cpp")
    message(STATUS "Adding DI overhead and logging throughput benchmarks")

    add_executable(bench_di_logging
        observability/bench_di_logging.cpp
    )

    target_link_libraries(bench_di_logging PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_di_logging PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_di_logging)

    install(TARGETS bench_di_logging
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  DI overhead: ConcernsContext creation (NoOp, custom)")
    message(STATUS "  Logging throughput: NoOpLogger, SpdlogAdapter (plain-text, JSON), logStructured, logWithContext")
endif()

# ============================================================================
# Auth Token Validation Latency Benchmarks (Issue #1551)
# Measures per-operation latency for JWT RS256 validation (cached JWKS),
# JTI blacklist lookup, TOTP computation, and static API token lookup.
# Targets: JWT < 5 ms (cached), TOTP < 10 ms.
# ============================================================================


# ============================================================================
# Governance Policy Evaluation Latency Benchmarks (Issue #1779)
# Measures per-operation latency of PolicyEngine::evaluate() and
# PolicyEngine::checkQueryPermission() at query time.
# Test matrix: heuristic fallback vs YAML-loaded profiles, all four VS
# classification levels, CCPA opt-out overhead, field-masking overhead,
# high-volume throughput (1/10/100/1 000 requests per batch).
# Targets: evaluate() p99 < 0.5 ms, checkQueryPermission() p99 < 0.5 ms.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/governance/bench_governance_policy_latency.cpp")
    message(STATUS "Adding governance policy evaluation latency benchmarks")

    if(NOT TARGET bench_governance_policy_latency)
        add_executable(bench_governance_policy_latency
            governance/bench_governance_policy_latency.cpp
        )
    endif()

    target_link_libraries(bench_governance_policy_latency PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        $<$<TARGET_EXISTS:yaml-cpp::yaml-cpp>:yaml-cpp::yaml-cpp>
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_governance_policy_latency PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_governance_policy_latency)

    install(TARGETS bench_governance_policy_latency
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Governance Policy Latency: policy evaluation, hot-swap latency, cache hit rates")
else()
    message(STATUS "Governance Policy Latency benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/governance/bench_governance_policy_latency.cpp")
endif()

# Compliance / Security / Governance Performance Benchmarks
# Measures evaluation latency for GDPR, HIPAA, SOC 2, PCI-DSS, and CCPA/CPRA
# compliance rule paths, data masking throughput, and cross-regulation conflict
# detection overhead.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/bench_compliance_security_governance.cpp")
    message(STATUS "Adding compliance/security/governance benchmarks")

    add_executable(bench_compliance_security_governance
        security/bench_compliance_security_governance.cpp
    )

    target_link_libraries(bench_compliance_security_governance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        $<$<TARGET_EXISTS:yaml-cpp::yaml-cpp>:yaml-cpp::yaml-cpp>
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(bench_compliance_security_governance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    message(STATUS "  Compliance/Security/Governance: GDPR/HIPAA/SOC2/PCI-DSS/CCPA rule evaluation, data masking throughput")
else()
    message(STATUS "Compliance/Security/Governance benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/security/bench_compliance_security_governance.cpp")
endif()

# ============================================================================
# Security Module Benchmarks
# Measures the cryptographic and policy hot-paths mandated by
# src/security/FUTURE_ENHANCEMENTS.md performance targets:
#   - AES-256-GCM encrypt/decrypt throughput  : ≥ 1 GB/s (AES-NI, single core)
#   - RBAC policy evaluation (100 roles)       : p99 ≤ 0.5 ms
#   - Kyber-1024 key encapsulation             : ≥ 2 000 ops/s
#   - Dilithium-5 signing                      : ≥ 1 000 ops/s
#   - FIPS algorithm-list validation           : overhead characterisation
#   - AQL injection detection throughput       : throughput characterisation
#   - Audit log tamper-evident append          : p99 ≤ 2 ms per entry
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/bench_security.cpp")
    message(STATUS "Adding security module benchmarks")

    add_executable(bench_security
        security/bench_security.cpp
    )

    target_include_directories(bench_security PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(bench_security PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_security PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_security)

    install(TARGETS bench_security
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Security: AES-256-GCM throughput, RBAC 100-role check, Kyber-1024/Dilithium-5, FIPS validation, AQL injection, audit-log append")
else()
    message(STATUS "Security benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_security.cpp")
endif()

# ============================================================================
# Auth Hot-Path Release-Gate Benchmarks (Phase 5 — Q3 2026)
# Covers token/session/revocation hot paths with hard release gates
# (GATE-AHP-01..06).  Feeds into src/auth/ROADMAP.md Phase 5 sign-off.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auth/bench_auth_hotpaths.cpp" AND NOT TARGET bench_auth_hotpaths)
    message(STATUS "Adding auth hot-path release-gate benchmarks (AHP-01..08)")

    add_executable(bench_auth_hotpaths
        auth/bench_auth_hotpaths.cpp
    )

    target_include_directories(bench_auth_hotpaths PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(bench_auth_hotpaths PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_auth_hotpaths PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_auth_hotpaths)

    install(TARGETS bench_auth_hotpaths
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Auth hot-paths: blacklist hit/miss p99, session create/validate p99, "
                   "distributed revocation RocksDB add/isRevoked, federation realm-lookup overhead "
                   "(GATE-AHP-01..06)")
else()
    if(NOT TARGET bench_auth_hotpaths)
        message(STATUS "Auth hot-path benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/auth/bench_auth_hotpaths.cpp")
    endif()
endif()


# ============================================================================
# Policy Evaluation Benchmarks (Issue #1779)
# Measures RBAC/ABAC rule evaluation performance with varying policy set sizes.
# Uses a self-contained mock PolicyEngine (no external library dependencies).
#
# Test matrix: simple rules, nested rules, large policy sets (10/50/100),
# conflict detection, and evaluation throughput.
# Targets: evaluate() p99 < 0.5 ms (consistent with bench_security RBAC target).
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/governance/bench_policy_evaluation.cpp")
    message(STATUS "Adding policy evaluation benchmarks")

    add_executable(bench_policy_evaluation
        governance/bench_policy_evaluation.cpp
    )

    target_link_libraries(bench_policy_evaluation PRIVATE
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_policy_evaluation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_policy_evaluation PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_policy_evaluation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Policy Evaluation: simple/nested/large rule sets, RBAC/ABAC throughput, conflict detection")
else()
    message(STATUS "Policy evaluation benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/governance/bench_policy_evaluation.cpp")
endif()

# ConfigPathResolver Performance Benchmarks (Issue #1675)
# Measures cache hit rate and resolution latency for config path resolution.
#
# Targets (src/config/FUTURE_ENHANCEMENTS.md):
#   - resolve() cache hit:                 < 1 µs
#   - resolve() cache miss (mapped path):  < 200 µs
#   - mapLegacyToNew() lookup:             < 1 µs
#   - DeprecationAggregator hot path:      < 50 ns
#   - Prometheus metrics scrape:           < 1 ms
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/bench_config_path_resolver.cpp")
    message(STATUS "Adding ConfigPathResolver cache hit rate and resolution latency benchmarks")

    add_executable(bench_config_path_resolver
        config/bench_config_path_resolver.cpp
        ../src/config/config_path_resolver.cpp
    )

    target_link_libraries(bench_config_path_resolver PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_config_path_resolver PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_config_path_resolver)

    install(TARGETS bench_config_path_resolver
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Config benchmarks: cache hit/miss latency, mapLegacyToNew, deprecation aggregator, metrics scrape, concurrent hit rate")
else()
    message(STATUS "ConfigPathResolver benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_config_path_resolver.cpp")
endif()

# ============================================================================
# Config Migration Scanner Benchmarks
# Performance target: CLI scanner 10K files < 5 s
# (ref: src/config/FUTURE_ENHANCEMENTS.md, Performance Targets)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/bench_config_migration_scanner.cpp")
    message(STATUS "Adding ConfigMigrationScanner throughput benchmarks")

    add_executable(bench_config_migration_scanner
        config/bench_config_migration_scanner.cpp
    )

    target_link_libraries(bench_config_migration_scanner PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_config_migration_scanner PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_config_migration_scanner)

    install(TARGETS bench_config_migration_scanner
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Migration scanner benchmarks: shouldScanFile, scanFile (no/with match), 1K-tree, 10K-tree")
else()
    message(STATUS "ConfigMigrationScanner benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_config_migration_scanner.cpp")
endif()

# ============================================================================
# Acceleration Dispatch Benchmarks
# CPUVectorBackend::computeDistances, batchKnnSearch;
# CPUGeoBackend::haversineDistance, batchHaversineDistances, pointInPolygon
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/acceleration/bench_acceleration_dispatch.cpp")
    message(STATUS "Adding Acceleration Dispatch benchmarks")

    add_executable(bench_acceleration_dispatch
        acceleration/bench_acceleration_dispatch.cpp
    )

    target_link_libraries(bench_acceleration_dispatch PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_acceleration_dispatch PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_acceleration_dispatch)

    install(TARGETS bench_acceleration_dispatch
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Acceleration: computeDistances (L2/cosine), batchKnnSearch, haversineDistance, batchHaversineDistances, pointInPolygon")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/acceleration/bench_acceleration_cuda_gates.cpp")
    message(STATUS "Adding Acceleration CUDA gate benchmarks")

    add_executable(bench_acceleration_cuda_gates
        acceleration/bench_acceleration_cuda_gates.cpp
    )

    target_link_libraries(bench_acceleration_cuda_gates PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_acceleration_cuda_gates PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_acceleration_cuda_gates)

    install(TARGETS bench_acceleration_cuda_gates
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Acceleration CUDA gates: L2/cosine/inner-product speedup vs CPU (>=8x)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/acceleration/bench_category_b_gpu_cpu_parity.cpp")
    message(STATUS "Adding Category-B GPU/CPU parity benchmark gate")

    add_executable(bench_category_b_gpu_cpu_parity
        acceleration/bench_category_b_gpu_cpu_parity.cpp
    )

    target_link_libraries(bench_category_b_gpu_cpu_parity PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_category_b_gpu_cpu_parity PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_category_b_gpu_cpu_parity)

    install(TARGETS bench_category_b_gpu_cpu_parity
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Category-B parity gate: BFS/Dijkstra/Geo ST_CONTAINS CUDA vs CPU")
endif()

# ============================================================================
# CDC Pipeline Benchmarks
# ConsumerGroupManager: createGroup/deleteGroup, fetchEventsAtLeastOnce,
# acknowledgeEvents, concurrent fan-out
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cdc/bench_cdc_pipeline.cpp")
    message(STATUS "Adding CDC Pipeline benchmarks")

    add_executable(bench_cdc_pipeline
        cdc/bench_cdc_pipeline.cpp
    )

    target_link_libraries(bench_cdc_pipeline PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_cdc_pipeline PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_cdc_pipeline)

    install(TARGETS bench_cdc_pipeline
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  CDC Pipeline: createGroup, deleteGroup, fetchEventsAtLeastOnce, acknowledgeEvents, concurrent fetch")
endif()

# ============================================================================
# CDC Delivery Gates Benchmarks
# DeliveryTracker, ConsumerGroupManager, InMemoryReplayController hot paths
# CDG-01..06 / GATE-CDC-01..06
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cdc/bench_cdc_delivery_gates.cpp")
    message(STATUS "Adding CDC Delivery Gates benchmarks")

    add_executable(bench_cdc_delivery_gates
        cdc/bench_cdc_delivery_gates.cpp
    )

    target_link_libraries(bench_cdc_delivery_gates PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_cdc_delivery_gates PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_cdc_delivery_gates)

    install(TARGETS bench_cdc_delivery_gates
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  CDC Delivery Gates: trackDelivery, acknowledge, acknowledgeUpTo, createGroup/deleteGroup, fetchEventsAtLeastOnce, replayFromSequence (CDG-01..06)")
endif()

# ============================================================================
# Temporal Queries Benchmarks
# BiTemporalTable: insertWithValidTime, queryBiTemporal,
# queryCurrentByValidTime, updateForValidTime, deleteForValidTime, getHistory
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_temporal_queries.cpp")
    message(STATUS "Adding Temporal Queries benchmarks")

    add_executable(bench_temporal_queries
        temporal/bench_temporal_queries.cpp
    )

    target_link_libraries(bench_temporal_queries PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_temporal_queries PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_temporal_queries)

    install(TARGETS bench_temporal_queries
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Temporal: insertWithValidTime, queryBiTemporal, queryCurrentByValidTime, updateForValidTime, deleteForValidTime, getHistory")
endif()

# ============================================================================
# Continuous Query Engine Benchmarks (Phase 8.5 — CQ-PERF-01/02)
# ContinuousQueryEngine: injectTuple throughput (CQ-PERF-01),
# SynopsisStore/IncrementalAgg window tick overhead (CQ-PERF-02)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/query/bench_continuous_query.cpp")
    message(STATUS "Adding Continuous Query Engine benchmarks (CQ-PERF-01/02)")

    add_executable(bench_continuous_query
        query/bench_continuous_query.cpp
        ${CMAKE_SOURCE_DIR}/src/query/continuous_query_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/query/continuous_query_planner.cpp
        ${CMAKE_SOURCE_DIR}/src/query/cq_watermark.cpp
        ${CMAKE_SOURCE_DIR}/src/query/synopsis_store.cpp
        ${CMAKE_SOURCE_DIR}/src/query/incremental_agg.cpp
    )

    target_link_libraries(bench_continuous_query PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_continuous_query PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_continuous_query)

    install(TARGETS bench_continuous_query
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  CQ: BM_ContinuousQuery_Throughput (CQ-PERF-01), BM_ContinuousQuery_WindowTick (CQ-PERF-02)")
endif()

# ============================================================================
# Task Scheduler Benchmarks
# TaskScheduler: registerTask, unregisterTask, executeTaskNow, listTasks,
# getStats, concurrent registerTask
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scheduler/bench_task_scheduler.cpp")
    message(STATUS "Adding Task Scheduler benchmarks")

    add_executable(bench_task_scheduler
        scheduler/bench_task_scheduler.cpp
    )

    target_link_libraries(bench_task_scheduler PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_task_scheduler PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_task_scheduler)

    install(TARGETS bench_task_scheduler
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Scheduler: registerTask, unregisterTask, executeTaskNow, listTasks, getStats, concurrent register")
endif()

# ============================================================================
# Replication Throughput Benchmarks
# WALManager: append, readFrom; WALEntry: serialize/deserialize;
# ReplicationManager: initialize
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_replication_throughput.cpp")
    message(STATUS "Adding Replication Throughput benchmarks")

    add_executable(bench_replication_throughput
        replication/bench_replication_throughput.cpp
    )

    target_link_libraries(bench_replication_throughput PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_replication_throughput PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_replication_throughput)

    install(TARGETS bench_replication_throughput
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Replication: WAL append, WAL readFrom, WALEntry serialize/deserialize, ReplicationManager initialize")
endif()

# ============================================================================
# Update Pipeline Benchmarks
# UpdateStateMachine: transition, currentState;
# ReleaseManifest: toJson/fromJson;
# DeltaUpdateEngine: generatePatch, applyPatch
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maintenance/bench_update_pipeline.cpp")
    message(STATUS "Adding Update Pipeline benchmarks")

    add_executable(bench_update_pipeline
        maintenance/bench_update_pipeline.cpp
    )

    target_link_libraries(bench_update_pipeline PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_update_pipeline PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_update_pipeline)

    install(TARGETS bench_update_pipeline
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Updates: state machine transitions, rollback path, manifest JSON round-trip, generatePatch, applyPatch")
endif()

# ============================================================================
# Observability: MetricsCollector Performance Benchmarks
# Measures metrics recording, aggregation, Prometheus text export,
# and concurrent access performance.
# Performance target: < 1% CPU overhead at steady-state 1,000 req/s workload.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maintenance/bench_metrics_collector.cpp")
    message(STATUS "Adding Observability: MetricsCollector performance benchmarks")

    add_executable(bench_metrics_collector
        maintenance/bench_metrics_collector.cpp
    )

    target_include_directories(bench_metrics_collector PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_metrics_collector PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_metrics_collector PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_metrics_collector)

    install(TARGETS bench_metrics_collector
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  MetricsCollector: counter/gauge/histogram recording, Prometheus export, contention behaviour")
endif()

# ============================================================================
# Observability: 1:1 Goal-Coverage Benchmarks (Issue #4549)
# Provides dedicated primary benchmark cases for every OBS goal ID:
#   OBS-1  Metrics collection overhead  < 1 % CPU @ 1 000 req/s
#   OBS-2  Adaptive span sampling        <= 1 % CPU overhead @ > 10 000 spans/s
#   OBS-3  Concurrent Prometheus scrape  >= 3x vs exclusive-mutex baseline
# Results include overhead, latency, and scaling data and are scientifically
# reproducible (config labels, hardware info, google/benchmark JSON output).
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_observability_goals.cpp")
    message(STATUS "Adding Observability: 1:1 goal-coverage benchmarks (OBS-1, OBS-2, OBS-3)")

    add_executable(bench_observability_goals
        observability/bench_observability_goals.cpp
    )

    target_include_directories(bench_observability_goals PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_observability_goals PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_observability_goals PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_observability_goals)

    install(TARGETS bench_observability_goals
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  OBS-1: metrics overhead, OBS-2: adaptive span sampling, OBS-3: shared-mutex scrape speedup")
endif()

# Index Module Benchmarks (Issue: #1884 — Performance benchmarks: QPS, recall, memory)
# These benchmarks cover vector search, quantization, spatial indexing, and
# approximate radius search.  All CPU-only benchmarks run without GPU hardware.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_vector_search.cpp")
    message(STATUS "Adding Vector Search benchmarks (Issue #1884)")

    add_executable(bench_vector_search
        ann/bench_vector_search.cpp
    )

    target_link_libraries(bench_vector_search PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_vector_search PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_vector_search)

    install(TARGETS bench_vector_search
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  VectorSearch: HNSW insert QPS, k-NN search QPS (L2/Cosine), recall vs QPS trade-off")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_index_rebuild.cpp")
    message(STATUS "Adding Index Rebuild benchmarks (Issue #1884)")

    add_executable(bench_index_rebuild
        index/bench_index_rebuild.cpp
    )

    target_link_libraries(bench_index_rebuild PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_index_rebuild PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_index_rebuild)

    install(TARGETS bench_index_rebuild
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  IndexRebuild: secondary index build time, online rebuild read-QPS impact, WriteBatch throughput")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/geo/bench_spatial_index.cpp")
    message(STATUS "Adding Spatial Index benchmarks (Issue #1884)")

    add_executable(bench_spatial_index
        geo/bench_spatial_index.cpp
    )

    target_link_libraries(bench_spatial_index PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    if(TARGET themis_geo)
        target_link_libraries(bench_spatial_index PRIVATE themis_geo)
    endif()

    target_compile_definitions(bench_spatial_index PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_spatial_index)

    install(TARGETS bench_spatial_index
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  SpatialIndex: R-tree insert/search QPS, Z-order range query, KNN spatial queries")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/search/bench_approximate_radius_search.cpp")
    message(STATUS "Adding Approximate Radius Search benchmarks (Issue #1884)")

    add_executable(bench_approximate_radius_search
        search/bench_approximate_radius_search.cpp
    )

    target_link_libraries(bench_approximate_radius_search PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_approximate_radius_search PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_approximate_radius_search)

    install(TARGETS bench_approximate_radius_search
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ApproximateRadiusSearch: radius search QPS, recall vs radius, batch search throughput")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_binary_quantization.cpp")
    message(STATUS "Adding Binary Quantization benchmarks (Issue #1884)")

    add_executable(bench_binary_quantization
        ann/bench_binary_quantization.cpp
    )

    target_link_libraries(bench_binary_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_binary_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_binary_quantization)

    install(TARGETS bench_binary_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  BinaryQuantization: encode throughput, Hamming distance QPS, memory reduction factor")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_product_quantization.cpp")
    message(STATUS "Adding Product Quantization benchmarks (Issue #1884)")

    add_executable(bench_product_quantization
        ann/bench_product_quantization.cpp
    )

    target_link_libraries(bench_product_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_product_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_product_quantization)

    install(TARGETS bench_product_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ProductQuantization: train throughput, encode QPS, ADC distance table, memory footprint")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_residual_quantization.cpp")
    message(STATUS "Adding Residual Quantization benchmarks (Issue #1884)")

    add_executable(bench_residual_quantization
        ann/bench_residual_quantization.cpp
    )

    target_link_libraries(bench_residual_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_residual_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_residual_quantization)

    install(TARGETS bench_residual_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ResidualQuantization: multi-stage encode throughput, reconstruction error, cascade QPS")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_learned_quantization.cpp")
    message(STATUS "Adding Learned Quantization benchmarks (Issue #1884)")

    add_executable(bench_learned_quantization
        ann/bench_learned_quantization.cpp
    )

    target_link_libraries(bench_learned_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_learned_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_learned_quantization)

    install(TARGETS bench_learned_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LearnedQuantization: neural codebook training, encode throughput, distortion vs compression rate")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_hnsw_prefilter_minimal.cpp")
    if(TARGET hnswlib::hnswlib)
        message(STATUS "Adding HNSW Pre-filter benchmarks (Issue #1884)")

        add_executable(bench_hnsw_prefilter_minimal
            ann/bench_hnsw_prefilter_minimal.cpp
        )

        target_link_libraries(bench_hnsw_prefilter_minimal PRIVATE
            ${BENCHMARK_LIBS}
            themis_core
            Threads::Threads
        )

        target_link_libraries(bench_hnsw_prefilter_minimal PRIVATE hnswlib::hnswlib)

        target_compile_definitions(bench_hnsw_prefilter_minimal PRIVATE
            THEMIS_BENCHMARK_BUILD=1
        )

        themis_apply_benchmark_release_flags(bench_hnsw_prefilter_minimal)

        install(TARGETS bench_hnsw_prefilter_minimal
            RUNTIME DESTINATION bin/benchmarks
            COMPONENT benchmarks
        )

        message(STATUS "  HnswPrefilterMinimal: HNSW pre-filter vs post-filter recall/QPS, selective-filter throughput")
    else()
        message(STATUS "Skipping HNSW pre-filter benchmark: hnswlib dependency not available")
    endif()
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_hybrid_vector_geo.cpp")
    message(STATUS "Adding Hybrid Vector+Geo benchmarks (Issue #1884)")

    add_executable(bench_hybrid_vector_geo
        ann/bench_hybrid_vector_geo.cpp
    )

    target_link_libraries(bench_hybrid_vector_geo PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_hybrid_vector_geo PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_hybrid_vector_geo)

    install(TARGETS bench_hybrid_vector_geo
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  HybridVectorGeo: combined vector+geo search QPS, geo-filter overhead, fused query latency")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_vector_prefilter.cpp")
    message(STATUS "Adding Vector Prefilter benchmarks (Issue #1884)")

    add_executable(bench_vector_prefilter
        ann/bench_vector_prefilter.cpp
    )

    target_link_libraries(bench_vector_prefilter PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        RocksDB::rocksdb
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_vector_prefilter PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_vector_prefilter)

    install(TARGETS bench_vector_prefilter
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  VectorPrefilter: pre-filter selectivity vs recall, attribute filter overhead, ANN+filter QPS")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_vector_compression_lossless.cpp")
    message(STATUS "Adding Vector Compression (Lossless) benchmarks (Issue #1884)")

    add_executable(bench_vector_compression_lossless
        ann/bench_vector_compression_lossless.cpp
    )

    target_link_libraries(bench_vector_compression_lossless PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_vector_compression_lossless PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_vector_compression_lossless)

    install(TARGETS bench_vector_compression_lossless
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  VectorCompressionLossless: SIMD delta/varint encode throughput, compression ratio, decompress latency")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_gpu_vector_index.cpp")
    if(THEMIS_ENABLE_GPU)
        message(STATUS "Adding GPU Vector Index benchmarks (THEMIS_ENABLE_GPU=ON, Issue #1884)")

        add_executable(bench_gpu_vector_index
            gpu/bench_gpu_vector_index.cpp
        )

        target_link_libraries(bench_gpu_vector_index PRIVATE
            ${BENCHMARK_LIBS}
            themis_core
            Threads::Threads
        )

        if(TARGET CUDA::cudart)
            target_link_libraries(bench_gpu_vector_index PRIVATE CUDA::cudart)
        endif()

        target_compile_definitions(bench_gpu_vector_index PRIVATE
            THEMIS_BENCHMARK_BUILD=1
            THEMIS_ENABLE_GPU
        )

        if(THEMIS_ENABLE_VULKAN)
            target_compile_definitions(bench_gpu_vector_index PRIVATE THEMIS_ENABLE_VULKAN=1)

            if(NOT TARGET vulkan_vector_index_shaders)
                set(_themis_glslc_hints
                    "${Vulkan_GLSLC_EXECUTABLE}"
                    "$ENV{VULKAN_SDK}/Bin"
                    "$ENV{VULKAN_SDK}/bin"
                )

                find_program(_themis_glslc glslc HINTS ${_themis_glslc_hints})
                if(_themis_glslc)
                    set(_themis_vulkan_vector_shader_src_dir "${THEMIS_BENCHMARK_ROOT}/src/acceleration/vulkan/shaders")
                    set(_themis_vulkan_vector_shader_out_dir "${CMAKE_BINARY_DIR}/shaders/vector_index")
                    file(MAKE_DIRECTORY "${_themis_vulkan_vector_shader_out_dir}")

                    set(_themis_vulkan_vector_shader_outputs)
                    foreach(_themis_shader IN ITEMS l2_distance cosine_distance inner_product_distance batch_search topk_selection)
                        set(_themis_shader_src "${_themis_vulkan_vector_shader_src_dir}/${_themis_shader}.comp")
                        set(_themis_shader_out "${_themis_vulkan_vector_shader_out_dir}/${_themis_shader}.comp.spv")
                        add_custom_command(
                            OUTPUT "${_themis_shader_out}"
                            COMMAND "${_themis_glslc}" -o "${_themis_shader_out}" "${_themis_shader_src}"
                            DEPENDS "${_themis_shader_src}"
                            COMMENT "Compiling Vulkan Vector Index shader: ${_themis_shader}.comp"
                            VERBATIM
                        )
                        list(APPEND _themis_vulkan_vector_shader_outputs "${_themis_shader_out}")
                    endforeach()

                    add_custom_target(vulkan_vector_index_shaders
                        DEPENDS ${_themis_vulkan_vector_shader_outputs}
                        COMMENT "Building Vulkan Vector Index shaders"
                    )
                else()
                    message(WARNING "glslc not found - bench_gpu_vector_index Vulkan path requires pre-compiled shaders")
                endif()
            endif()
        endif()

        themis_apply_benchmark_release_flags(bench_gpu_vector_index)

        if(TARGET vulkan_vector_index_shaders)
            add_dependencies(bench_gpu_vector_index vulkan_vector_index_shaders)
        endif()

        install(TARGETS bench_gpu_vector_index
            RUNTIME DESTINATION bin/benchmarks
            COMPONENT benchmarks
        )

        message(STATUS "  GpuVectorIndex: GPU insert/search QPS vs CPU, memory bandwidth utilisation, multi-GPU fan-out")
    else()
        message(STATUS "Skipping GPU Vector Index benchmarks (THEMIS_ENABLE_GPU=OFF)")
    endif()
endif()

# ============================================================================
# Ingestion Module Benchmarks (Issue: #1911 — Performance benchmarks: docs/sec, MB/sec)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_ingestion_kv.cpp")
    message(STATUS "Adding Ingestion KV benchmarks (Issue #1911)")

    add_executable(bench_ingestion_kv
        ingestion/bench_ingestion_kv.cpp
    )

    target_link_libraries(bench_ingestion_kv PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_ingestion_kv PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_ingestion_kv)

    install(TARGETS bench_ingestion_kv
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  IngestionKV: bulk-insert throughput (docs/sec, MB/sec), WriteBatch sizing, checkpoint overhead")
endif()

# ============================================================================
# Voice Assistant Benchmarks (Issue: #2357 — Performance benchmarks: STT latency, TTS speed)
# ============================================================================

if(THEMIS_ENABLE_VOICE_ASSISTANT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_voice_assistant.cpp")
    message(STATUS "Adding Voice Assistant benchmarks (Issue #2356)")

    add_executable(bench_voice_assistant
        voice/bench_voice_assistant.cpp
    )

    target_link_libraries(bench_voice_assistant PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_voice_assistant PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_assistant)

    install(TARGETS bench_voice_assistant
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  VoiceAssistant: STT latency, TTS generation speed, command pipeline throughput, wake-word detection QPS")
endif()

# ============================================================================
# Prompt Engineering Performance Benchmarks (ROADMAP Production Readiness)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_prompt_engineering.cpp")
    message(STATUS "Adding prompt engineering performance benchmarks")

    add_executable(bench_prompt_engineering
        prompt_engineering/bench_prompt_engineering.cpp
    )

    target_link_libraries(bench_prompt_engineering PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        TBB::tbb
        Threads::Threads
    )

    target_compile_definitions(bench_prompt_engineering PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_prompt_engineering)

    install(TARGETS bench_prompt_engineering
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  PromptManager: createTemplate/getTemplate/injectContext/validateTemplate")
    message(STATUS "  PromptVersionControl: commit/getHistory")
    message(STATUS "  PromptOptimizer/Evaluator: optimize and evaluateSingle latency")
endif()

# ============================================================================
# Exporters Throughput Benchmarks (Issue #1730)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/bench_exporters.cpp")
    message(STATUS "Adding Exporters throughput benchmarks")

    if(NOT TARGET bench_exporters)
        add_executable(bench_exporters
            exporters/bench_exporters.cpp
        )
    endif()

    target_link_libraries(bench_exporters PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_exporters PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_exporters)

    install(TARGETS bench_exporters
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  JSONL export throughput and incremental delta speedup")
endif()

# ============================================================================
# Analytics Export Benchmarks (AN-3 / AN-4)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/bench_parquet_export.cpp")
    message(STATUS "Adding Parquet export benchmark (AN-3)")

    add_executable(bench_parquet_export
        exporters/bench_parquet_export.cpp
    )

    target_link_libraries(bench_parquet_export PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_parquet_export PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_parquet_export)

    install(TARGETS bench_parquet_export
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/bench_csv_export.cpp")
    message(STATUS "Adding CSV export benchmark (AN-4)")

    add_executable(bench_csv_export
        exporters/bench_csv_export.cpp
    )

    target_link_libraries(bench_csv_export PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_csv_export PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_csv_export)

    install(TARGETS bench_csv_export
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
endif()

# ============================================================================
# Metadata Cache Benchmarks (META-MISSING-001 — cache hit rate + scan latency)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_metadata_cache.cpp")
    message(STATUS "Adding Metadata Cache performance benchmarks (META-MISSING-001)")

    add_executable(bench_metadata_cache
        metadata/bench_metadata_cache.cpp
    )

    target_link_libraries(bench_metadata_cache PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_metadata_cache PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_metadata_cache)

    install(TARGETS bench_metadata_cache
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  MetadataCache: cold scan, warm hit, hit/miss comparison, TTL variants, adaptive TTL, concurrent reads, RocksDB direct scan")
endif()

# Content module benchmarks are registered in benchmarks/content/CMakeLists.txt.
# Keep only the non-content benchmarks in this top-level file.

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_geo_cpu_gpu.cpp")
    message(STATUS "Adding Geo CPU/GPU parity benchmarks")

    add_executable(bench_geo_cpu_gpu
        geo/bench_geo_cpu_gpu.cpp
    )

    target_link_libraries(bench_geo_cpu_gpu PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    if(TARGET themis_geo)
        target_link_libraries(bench_geo_cpu_gpu PRIVATE themis_geo)
    endif()

    target_compile_definitions(bench_geo_cpu_gpu PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_geo_cpu_gpu)

    install(TARGETS bench_geo_cpu_gpu
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  GeoParity: CPU/GPU throughput and fallback parity for intersects, distance, ST_BUFFER")
endif()

# ============================================================================
# Process retrieval benchmarks
# Validates PROC-OPEN-01/02/03:
#   - Deterministic embedding generation on import (auto-embed hook)
#   - State-change re-embedding (ProcessInstance transitions)
#   - BM25 full-text search over process descriptions
#   - HNSW-style cosine top-k retrieval (findSimilar baseline)
# Self-contained — no Themis runtime headers required.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_process_retrieval.cpp")
    message(STATUS "Adding Process retrieval benchmarks (embedding + HNSW + full-text)")

    add_executable(bench_process_retrieval
        process/bench_process_retrieval.cpp
    )

    target_link_libraries(bench_process_retrieval PRIVATE
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_process_retrieval PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_process_retrieval)

    install(TARGETS bench_process_retrieval
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ProcessRetrieval: embedding generation, HNSW top-k, BM25 full-text, state-change re-embed")
endif()

# ============================================================================
# GPU Hardware Capability benchmarks
# Beta-exit evidence for:
#   Issue #1800 — GPU P2P transfer manager (GPUP2PTransferManager, CPU path)
#   Issue #1802 — NVLink topology-aware scheduling (TOPOLOGY_AWARE strategy)
#   Issue #1805 — GPU module test coverage > 80 %
# Self-contained — no CUDA/HIP hardware required; CPU simulation paths used.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu/bench_gpu_hardware_capability.cpp")
    message(STATUS "Adding GPU hardware capability benchmarks (P2P, NVLink, load-balancer)")

    add_executable(bench_gpu_hardware_capability
        gpu/bench_gpu_hardware_capability.cpp
    )

    target_link_libraries(bench_gpu_hardware_capability PRIVATE
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_gpu_hardware_capability PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_gpu_hardware_capability)

    install(TARGETS bench_gpu_hardware_capability
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  GPUCapability: device probe, P2P CPU-fallback, NVLink topology, TOPOLOGY_AWARE scheduling")
endif()

# ============================================================================
# Sharding benchmarks
# Validates sharding beta-exit evidence:
#   - cross-shard routing throughput
#   - scatter/gather and cross-shard join latency
#   - operational scalability across shard counts
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sharding/bench_shard_routing.cpp" AND NOT TARGET bench_shard_routing)
    message(STATUS "Adding Shard routing benchmarks")

    add_executable(bench_shard_routing
        sharding/bench_shard_routing.cpp
    )

    target_link_libraries(bench_shard_routing PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_shard_routing PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_shard_routing)

    install(TARGETS bench_shard_routing
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ShardRouting: single-shard lookup, consistent-hash, batch routing, distribution quality")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sharding/bench_sharding_performance.cpp" AND NOT TARGET bench_sharding_performance)
    message(STATUS "Adding Sharding performance benchmarks")

    add_executable(bench_sharding_performance
        sharding/bench_sharding_performance.cpp
    )

    target_link_libraries(bench_sharding_performance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_sharding_performance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_sharding_performance PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_OPENCL)
        target_compile_definitions(bench_sharding_performance PRIVATE THEMIS_ENABLE_OPENCL=1)
    endif()

    themis_apply_benchmark_release_flags(bench_sharding_performance)

    install(TARGETS bench_sharding_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ShardingPerformance: scatter-gather latency, cross-shard joins, migration and gossip overhead")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sharding/bench_sharding_release_gates.cpp" AND NOT TARGET bench_sharding_release_gates)
    add_executable(bench_sharding_release_gates
        sharding/bench_sharding_release_gates.cpp
    )
    target_include_directories(bench_sharding_release_gates PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(bench_sharding_release_gates PRIVATE
        ${BENCHMARK_LIBS}
    )
    themis_apply_benchmark_release_flags(bench_sharding_release_gates)
    install(TARGETS bench_sharding_release_gates
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    themis_register_benchmark_ctest(
        NAME         "bench_sharding_release_gates_smoke"
        TARGET       "bench_sharding_release_gates"
        MODULE       "sharding"
        KIND         "wave_a"
        TIMEOUT      180
        MIN_TIME     "0.01s"
        EXTRA_LABELS wave_a_benchmark_sharding_transaction benchmark-baseline release-gates
    )
endif()

# ============================================================================
# AdaptiveQueryCompiler benchmarks (v1.8.0, Issue #86)
# Validates:
#   - Cold path (interpreted) vs. hot path (compiled) throughput
#   - Compilation overhead (<100ms target)
#   - Pre-warmed cache execute(CompiledQuery, params)
#   - Multi-fingerprint cache performance
#   - Differential correctness (cold == hot result sizes)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/query/bench_adaptive_query_compilation.cpp")
    message(STATUS "Adding AdaptiveQueryCompiler benchmarks (performance module, Issue #86)")

    add_executable(bench_adaptive_query_compilation
        query/bench_adaptive_query_compilation.cpp
        ${CMAKE_SOURCE_DIR}/src/performance/adaptive_query_compiler.cpp
    )

    target_include_directories(bench_adaptive_query_compilation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(bench_adaptive_query_compilation PRIVATE
        ${BENCHMARK_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_adaptive_query_compilation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_adaptive_query_compilation)

    install(TARGETS bench_adaptive_query_compilation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  AdaptiveQueryCompiler: cold/hot filter/agg/join throughput, compilation overhead, differential correctness")
endif()

# ============================================================================
# PIIStreamScanner benchmarks (v0.9.0)
# Validates acceptance criteria:
#   - Streaming PII scan throughput: >100 MB/s per core for English legal text
#   - Scan+pseudonymise overhead: <2× vs. scan-only
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/utils/bench_pii_stream_scanner.cpp")
    message(STATUS "Adding PIIStreamScanner benchmarks (utils module, v0.9.0)")

    add_executable(bench_pii_stream_scanner
        utils/bench_pii_stream_scanner.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/pii_stream_scanner.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/regex_detection_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/pii_detection_engine.cpp
    )

    target_include_directories(bench_pii_stream_scanner PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(bench_pii_stream_scanner PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_pii_stream_scanner PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_pii_stream_scanner)

    install(TARGETS bench_pii_stream_scanner
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  PIIStreamScanner: 100MB scan-only and scan+pseudonymise throughput at 4KB/64KB/1MB chunks")
endif()

# ============================================================================
# Timeseries Ingestion & Downsampling benchmarks (TS-6, Run-Plan 17)
# Validates acceptance criteria:
#   - Gorilla compression round-trip throughput
#   - Time-range query performance at various range sizes
#   - DownsamplingThroughput: 1-min bucket aggregation over 3600-pt 1-hour dataset
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_timeseries_ingestion.cpp")
    message(STATUS "Adding Timeseries Ingestion & Downsampling benchmarks (TS-6)")

    add_executable(bench_timeseries_ingestion
        timeseries/bench_timeseries_ingestion.cpp
    )

    target_include_directories(bench_timeseries_ingestion PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(bench_timeseries_ingestion PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_timeseries_ingestion PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_timeseries_ingestion)

    install(TARGETS bench_timeseries_ingestion
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  DownsamplingThroughput: 1-min bucketed aggregation, 3600 pts/iter, throughput in pts/s")
endif()

# ============================================================================
# Timeseries Adaptive Flush benchmarks (PERF-D1)
# Validates acceptance criteria:
#   - Sustained ≥500k pts/s single- and multi-threaded TSAutoBuffer::add()
#   - P99 write latency <100µs per add() call
#   - FlushController adaptive batch-size convergence
#   - Buffer statistics and overdue-flush metrics exposure
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_timeseries_adaptive_flush.cpp")
    message(STATUS "Adding Timeseries Adaptive Flush benchmarks (PERF-D1)")

    add_executable(bench_timeseries_adaptive_flush
        timeseries/bench_timeseries_adaptive_flush.cpp
    )

    target_include_directories(bench_timeseries_adaptive_flush PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(bench_timeseries_adaptive_flush PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_timeseries_adaptive_flush PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_timeseries_adaptive_flush)

    install(TARGETS bench_timeseries_adaptive_flush
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  AdaptiveFlush: >=500k pts/s single/multi-threaded, P99 <100us, FlushController convergence, buffer stats")
endif()

# ============================================================================
# bench_query_lazy_eval (PERF-D7, v1.0.0)
# Validates acceptance criteria for Issue [PERF-D7]:
#   - Query engine lazy evaluation / predicate pushdown via columnar SIMD
#   - Filter-only throughput ≥ 200 M items/s (Release build, x86-64)
#   - Filter+Project late-materialization throughput ≥ 150 M items/s
#   - Multi-predicate AND filter throughput ≥ 100 M items/s
#   - Filter+Aggregate (SUM) throughput ≥ 80 M items/s
#   - Full pipeline (Filter → Project → Sort) ≥ 50 M items/s
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_query_lazy_eval.cpp")
    message(STATUS "Adding Query Engine Lazy Evaluation benchmarks (PERF-D7, v1.0.0)")

    add_executable(bench_query_lazy_eval
        query/bench_query_lazy_eval.cpp
    )

    target_link_libraries(bench_query_lazy_eval PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_query_lazy_eval PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_query_lazy_eval)

    install(TARGETS bench_query_lazy_eval
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  bench_query_lazy_eval: filter/project/multi-pred/aggregate/full-pipeline throughput, batch-size sweep, selectivity sweep, SIMD vs scalar")
endif()

# ============================================================================
# Thread Pool Saturation Benchmarks (utils module — Issue #1575 analogue)
# Validates: load tests under queue saturation, concurrent producers
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scheduler/bench_thread_pool_saturation.cpp")
    message(STATUS "Adding ThreadPoolManager saturation benchmarks")

    add_executable(bench_thread_pool_saturation
        scheduler/bench_thread_pool_saturation.cpp
    )

    target_include_directories(bench_thread_pool_saturation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_thread_pool_saturation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_thread_pool_saturation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_thread_pool_saturation)

    install(TARGETS bench_thread_pool_saturation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ThreadPool: submit throughput, saturated-queue drop rate, priority ordering, concurrent producers, shutdown latency")
endif()

# ============================================================================
# Voice Wake-Word and Batch-Processor Benchmarks
# Validates: wake-word latency <200 ms, batch linear scaling to 16 threads
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_voice_wake_word_batch.cpp")
    message(STATUS "Adding Voice wake-word and batch-processor benchmarks")

    add_executable(bench_voice_wake_word_batch
        voice/bench_voice_wake_word_batch.cpp
    )

    target_include_directories(bench_voice_wake_word_batch PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_wake_word_batch PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_voice_wake_word_batch PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_wake_word_batch)

    install(TARGETS bench_voice_wake_word_batch
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice: wake-word latency (silent/energy/rolling), batch throughput 1..16 threads, runLoadTest overhead")
endif()

# ============================================================================
# Phase 5: Voice Module Performance Benchmarks and SLA Gates
# ============================================================================

# Task 5.1: STT/TTS Latency Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/bench_voice_stt_tts_latency.cpp")
    message(STATUS "Adding Voice STT/TTS Latency benchmarks (Phase 5 Task 5.1)")

    add_executable(bench_voice_stt_tts_latency
        voice/bench_voice_stt_tts_latency.cpp
    )

    target_include_directories(bench_voice_stt_tts_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/benchmarks/voice
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_stt_tts_latency PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
    )

    target_compile_definitions(bench_voice_stt_tts_latency PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_stt_tts_latency)

    install(TARGETS bench_voice_stt_tts_latency
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice Phase 5.1: STT latency (100ms, 5s, 60s, noisy audio), TTS latency (short, medium, long, rapid-fire)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/bench_voice_a8_baselines.cpp")
    message(STATUS "Adding Voice Wave A closure baseline benchmarks")

    add_executable(bench_voice_a8_baselines
        voice/bench_voice_a8_baselines.cpp
    )

    target_include_directories(bench_voice_a8_baselines PRIVATE
        ${CMAKE_SOURCE_DIR}/benchmarks/voice
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_a8_baselines PRIVATE
        themis_core
        benchmark::benchmark
        benchmark::benchmark_main
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_voice_a8_baselines PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_a8_baselines)

    install(TARGETS bench_voice_a8_baselines
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice Wave A: stream validation, liveness, and session-overhead p95/p99 baselines")
endif()

# Task 5.2: Streaming Throughput Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/bench_voice_streaming_throughput.cpp")
    message(STATUS "Adding Voice Streaming Throughput benchmarks (Phase 5 Task 5.2)")

    add_executable(bench_voice_streaming_throughput
        voice/bench_voice_streaming_throughput.cpp
    )

    target_include_directories(bench_voice_streaming_throughput PRIVATE
        ${CMAKE_SOURCE_DIR}/benchmarks/voice
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_streaming_throughput PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
    )

    target_compile_definitions(bench_voice_streaming_throughput PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_streaming_throughput)

    install(TARGETS bench_voice_streaming_throughput
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice Phase 5.2: Streaming throughput, latency, buffer overhead, multi-stream, rebalancing, recovery")
endif()

# Task 5.3: Session Lifecycle Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/bench_voice_session_lifecycle.cpp")
    message(STATUS "Adding Voice Session Lifecycle benchmarks (Phase 5 Task 5.3)")

    add_executable(bench_voice_session_lifecycle
        voice/bench_voice_session_lifecycle.cpp
    )

    target_include_directories(bench_voice_session_lifecycle PRIVATE
        ${CMAKE_SOURCE_DIR}/benchmarks/voice
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_session_lifecycle PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
    )

    target_compile_definitions(bench_voice_session_lifecycle PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_session_lifecycle)

    install(TARGETS bench_voice_session_lifecycle
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice Phase 5.3: Session lifecycle (create, delete, scaling 100/1000), memory, cleanup")
endif()

# Task 5.4: Audio Preprocessing Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/bench_voice_audio_preprocessing.cpp")
    message(STATUS "Adding Voice Audio Preprocessing benchmarks (Phase 5 Task 5.4)")

    add_executable(bench_voice_audio_preprocessing
        voice/bench_voice_audio_preprocessing.cpp
    )

    target_include_directories(bench_voice_audio_preprocessing PRIVATE
        ${CMAKE_SOURCE_DIR}/benchmarks/voice
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_audio_preprocessing PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
    )

    target_compile_definitions(bench_voice_audio_preprocessing PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_audio_preprocessing)

    install(TARGETS bench_voice_audio_preprocessing
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice Phase 5.4: Audio preprocessing (validate, preprocess chain, wake-word, intent, emotion)")
endif()

# Task 5.5: Assistant Latency Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/bench_voice_assistant_latency.cpp")
    message(STATUS "Adding Voice Assistant Latency benchmarks (Phase 5 Task 5.5)")

    add_executable(bench_voice_assistant_latency
        voice/bench_voice_assistant_latency.cpp
    )

    target_include_directories(bench_voice_assistant_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/benchmarks/voice
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_voice_assistant_latency PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
    )

    target_compile_definitions(bench_voice_assistant_latency PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_voice_assistant_latency)

    install(TARGETS bench_voice_assistant_latency
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Voice Phase 5.5: Assistant latency (LLM, TTS, E2E, command parse/response)")
endif()

# ============================================================================
# llama.cpp Inference Benchmarks
# Validates: LlamaCppPlugin stub/real latency, batch throughput, concurrency
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_llama_cpp_inference.cpp")
    message(STATUS "Adding llama.cpp inference benchmarks")

    add_executable(bench_llama_cpp_inference
        llama_cpp/bench_llama_cpp_inference.cpp
        ${CMAKE_SOURCE_DIR}/src/llama_cpp/llama_cpp_plugin.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/rag_context_assembler.cpp
    )

    target_include_directories(bench_llama_cpp_inference PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_llama_cpp_inference PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_llama_cpp_inference PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_PLUGIN_EXPORTS
    )

    if(DEFINED ENV{THEMIS_BENCH_LLAMA_MODEL_PATH})
        target_compile_definitions(bench_llama_cpp_inference PRIVATE
            THEMIS_BENCH_LLAMA_MODEL_PATH="$ENV{THEMIS_BENCH_LLAMA_MODEL_PATH}"
        )
    endif()

    if(THEMIS_ENABLE_VULKAN)
        target_compile_definitions(bench_llama_cpp_inference PRIVATE THEMIS_ENABLE_VULKAN=1)
    endif()
    if(THEMIS_ENABLE_CUDA)
        target_compile_definitions(bench_llama_cpp_inference PRIVATE THEMIS_ENABLE_CUDA=1)
    endif()
    if(THEMIS_ENABLE_HIP)
        target_compile_definitions(bench_llama_cpp_inference PRIVATE THEMIS_ENABLE_HIP=1)
    endif()

    themis_apply_benchmark_release_flags(bench_llama_cpp_inference)

    install(TARGETS bench_llama_cpp_inference
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LlamaCpp: generate/batch/embed/stream latency, concurrent threads, stats query (stub + real model)")
endif()

# ============================================================================
# Whisper Transcription Benchmarks
# Validates: transcribe() latency, file-size limits, CLI parity budget
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_whisper_transcription.cpp" AND NOT WIN32)
    message(STATUS "Adding Whisper transcription benchmarks")

    add_executable(bench_whisper_transcription
        whisper/bench_whisper_transcription.cpp
        ${CMAKE_SOURCE_DIR}/src/whisper/whisper_plugin.cpp
        ${CMAKE_SOURCE_DIR}/src/whisper/whisper_config.cpp
        ${CMAKE_SOURCE_DIR}/src/whisper/audio_chunk_reader.cpp
        ${CMAKE_SOURCE_DIR}/src/whisper/whisper_transcriber.cpp
    )

    target_include_directories(bench_whisper_transcription PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_whisper_transcription PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_whisper_transcription PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_PLUGIN_EXPORTS
    )

    if(DEFINED ENV{THEMIS_BENCH_WHISPER_MODEL_PATH})
        target_compile_definitions(bench_whisper_transcription PRIVATE
            THEMIS_BENCH_WHISPER_MODEL_PATH="$ENV{THEMIS_BENCH_WHISPER_MODEL_PATH}"
        )
    endif()

    themis_apply_benchmark_release_flags(bench_whisper_transcription)

    install(TARGETS bench_whisper_transcription
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Whisper: transcribe 1s/5s/30s/8kHz, detectLanguage, file fast-fail, stub dispatch, buffer-size sweep")
endif()

# ============================================================================
# User Storage Encrypted Mount-Latency Benchmarks
# Validates: Pipe vs. File key delivery, mount/unmount cycle overhead
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user_storage_encrypted/bench_user_storage_mount_latency.cpp" AND NOT WIN32)
    message(STATUS "Adding user_storage_encrypted mount-latency benchmarks")

    add_executable(bench_user_storage_mount_latency
        user_storage_encrypted/bench_user_storage_mount_latency.cpp
        ${CMAKE_SOURCE_DIR}/src/user_storage_encrypted/gocryptfs_backend.cpp
    )

    target_include_directories(bench_user_storage_mount_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_user_storage_mount_latency PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_user_storage_mount_latency PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_user_storage_mount_latency)

    install(TARGETS bench_user_storage_mount_latency
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  UserStorageEncrypted: initialize, checkAvailability, isMounted, mount/unmount fast-fail, pipe vs. no-KDF dispatch")
endif()

if(TARGET bench_user_storage_mount_latency)
    target_sources(bench_user_storage_mount_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/src/user_storage_encrypted/gocryptfs_backend.cpp
    )
    target_include_directories(bench_user_storage_mount_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/src/user_storage_encrypted
        ${CMAKE_SOURCE_DIR}/include/user_storage_encrypted
    )
endif()

# ============================================================================
# Base Module Load and Hot-Reload Benchmarks (Issue #1575)
# Validates: module load/unload overhead, hot-reload cycles, concurrent scaling
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/bench_module_load_hot_reload.cpp")
    message(STATUS "Adding base module-load and hot-reload benchmarks")

    add_executable(bench_module_load_hot_reload
        base/bench_module_load_hot_reload.cpp
    )

    target_include_directories(bench_module_load_hot_reload PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_module_load_hot_reload PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_module_load_hot_reload PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_module_load_hot_reload)

    install(TARGETS bench_module_load_hot_reload
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Base: loadModule fast-fail, unload, isLoaded, loadAllModules empty dir, getMetrics, audit trail, concurrent reload 1..16 threads")
endif()

# ============================================================================
# Base Module Hot-Path Release Gate Benchmarks (Issue #5631)
# Validates: GATE-BASE-01..06 — isModuleLoaded, getMetrics, registeredModules,
#            isRollbackAvailable, buildFromResolver 100-node, reloadModule fast-fail
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/bench_base_hot_paths.cpp")
    message(STATUS "Adding base module hot-path release gate benchmarks (GATE-BASE-01..06)")

    add_executable(bench_base_hot_paths
        base/bench_base_hot_paths.cpp
    )

    target_include_directories(bench_base_hot_paths PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_base_hot_paths PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_base_hot_paths PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_base_hot_paths)

    install(TARGETS bench_base_hot_paths
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Base hot paths: GATE-BASE-01 isModuleLoaded ≥500k ops/s, GATE-BASE-02 getMetrics ≥100k ops/s, GATE-BASE-03 registeredModules p99≤5µs, GATE-BASE-04 isRollbackAvailable p99≤1µs, GATE-BASE-05 buildFromResolver 100-node ≤1ms, GATE-BASE-06 reloadModule fast-fail ≤50µs")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/bench_base_wasm_sandbox.cpp")
    message(STATUS "Adding base module wasm/sandbox dedicated benchmarks (GATE-BASE-07..12)")

    add_executable(bench_base_wasm_sandbox
        base/bench_base_wasm_sandbox.cpp
    )

    target_include_directories(bench_base_wasm_sandbox PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_base_wasm_sandbox PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_base_wasm_sandbox PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_base_wasm_sandbox)

    install(TARGETS bench_base_wasm_sandbox
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Base wasm/sandbox: GATE-BASE-07 loadFromBytes valid, GATE-BASE-08 loadFromBytes invalid, GATE-BASE-09 AbiChecker checkVersions, GATE-BASE-10 resolveDescription, GATE-BASE-11 isKnownCode, GATE-BASE-12 sandbox stats")
endif()

# ============================================================================
# Themis Core SLO Benchmarks (TH-1..TH-5)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_themis_core.cpp")
    message(STATUS "Adding Themis core SLO benchmarks (TH-1..TH-5)")

    add_executable(bench_themis_core
        themis/bench_themis_core.cpp
    )

    target_include_directories(bench_themis_core PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_themis_core PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_themis_core PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_themis_core)

    install(TARGETS bench_themis_core
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Themis: module load/hash verify, build info cold/warm, license signature, wire dispatch, edition hot path")
endif()

# ============================================================================
# Ingestion Extraction Throughput and LLM-Latency Benchmarks
# Validates: DeonticExtractor regex throughput, LLM adapter latency, MIME detection
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_ingestion_extraction.cpp")
    message(STATUS "Adding ingestion extraction and LLM-latency benchmarks")

    add_executable(bench_ingestion_extraction
        ingestion/bench_ingestion_extraction.cpp
    )

    target_include_directories(bench_ingestion_extraction PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_ingestion_extraction PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_ingestion_extraction PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(DEFINED ENV{THEMIS_BENCH_LLM_MODEL_PATH})
        target_compile_definitions(bench_ingestion_extraction PRIVATE
            THEMIS_BENCH_LLM_MODEL_PATH="$ENV{THEMIS_BENCH_LLM_MODEL_PATH}"
        )
    endif()

    themis_apply_benchmark_release_flags(bench_ingestion_extraction)

    install(TARGETS bench_ingestion_extraction
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Ingestion: DeonticExtractor regex throughput, LegalLlmAdapter fn call, MIME detection, checkpoint store")
endif()

# ============================================================================
# Ingestion Phase 7 – IngestionQualityJudge Benchmarks
# Validates: QJ01..QJ11 — evaluate() fast paths, observer dispatch, config
#            mutation, ctor/dtor cost, CRAG-style feedback loop simulation
# Scientific foundation:
#   Zheng et al. (NeurIPS 2023) — LLM-as-judge
#   Es et al. (EACL 2024) — RAGAS quality dimensions
#   Yan et al. (ICLR 2024) — CRAG re-ingestion loop
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_ingestion_quality_judge.cpp")
    message(STATUS "Adding IngestionQualityJudge benchmarks (Phase 7)")

    add_executable(bench_ingestion_quality_judge
        ingestion/bench_ingestion_quality_judge.cpp
    )

    target_include_directories(bench_ingestion_quality_judge PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_ingestion_quality_judge PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        themis_ingestion
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_ingestion_quality_judge PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_ingestion_quality_judge)

    install(TARGETS bench_ingestion_quality_judge
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  QualityJudge: fail-open path, single/all-dims eval, entity scaling, bullet-list parsing, observer dispatch, config mutation, ctor/dtor, CRAG feedback loop")
endif()

# ============================================================================
# Process Module Import, Retrieval and Prompt-Size Benchmarks
# Validates: BPMN/EPK import latency, RocksDB CRUD, LlmProcessDescriptor size
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_process_import_retrieval.cpp")
    message(STATUS "Adding process module import/retrieval/prompt-size benchmarks")

    add_executable(bench_process_import_retrieval
        process/bench_process_import_retrieval.cpp
    )

    target_include_directories(bench_process_import_retrieval PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_process_import_retrieval PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_process_import_retrieval PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_process_import_retrieval)

    install(TARGETS bench_process_import_retrieval
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Process: BPMN/EPK import, RocksDB save/load/list, LlmDescriptor generate/prompt/conformance")
endif()

# ============================================================================
# Process Mining Performance Benchmarks
# Validates: discovery algorithm throughput, DFG construction, conformance,
#            BPMN/PNML export, variant clustering at scale
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_process_mining.cpp")
    message(STATUS "Adding Process Mining benchmarks")

    add_executable(bench_process_mining
        process/bench_process_mining.cpp
    )

    target_include_directories(bench_process_mining PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_process_mining PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(bench_process_mining PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_process_mining)

    install(TARGETS bench_process_mining
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ProcessMining: Alpha/Heuristic/Inductive discovery, DFG, conformance, export")
endif()

# ============================================================================
# Chaos Framework Extended Stress Benchmarks
# Validates: FaultInjector throughput, concurrent stress, scheduler overhead
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_chaos_stress.cpp")
    message(STATUS "Adding chaos framework extended stress benchmarks")

    add_executable(bench_chaos_stress
        chaos/bench_chaos_stress.cpp
        ${CMAKE_SOURCE_DIR}/src/chaos/chaos_framework.cpp
    )

    target_include_directories(bench_chaos_stress PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_chaos_stress PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_chaos_stress PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_chaos_stress)

    install(TARGETS bench_chaos_stress
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Chaos: FaultInjector inject/query/recover throughput, expired pruning, callback dispatch, concurrent stress 1..16 threads, ChaosScheduler")
endif()

# ============================================================================
# Chaos Framework Release Gate Benchmarks (Phase 5 hardening)
# Validates: GATE-CHS-01..06 release gates for scheduler/concurrency hot paths
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/chaos/bench_chaos_release_gates.cpp")
    message(STATUS "Adding chaos framework release gate benchmarks (Phase 5)")

    add_executable(bench_chaos_release_gates
        chaos/bench_chaos_release_gates.cpp
        ${CMAKE_SOURCE_DIR}/src/chaos/chaos_framework.cpp
    )

    target_include_directories(bench_chaos_release_gates PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bench_chaos_release_gates PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_chaos_release_gates PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_chaos_release_gates)

    install(TARGETS bench_chaos_release_gates
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Chaos gates: GATE-CHS-01 inject throughput, GATE-CHS-02 query latency, GATE-CHS-03 recover throughput, GATE-CHS-04 concurrent ops, GATE-CHS-05 schedule throughput, GATE-CHS-06 callback overhead")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/chaos/bench_chaos_microbenchmarks.cpp")
    message(STATUS "Adding chaos microbenchmarks (Wave D mid-term — CHAOS-MB-01..03)")

    add_executable(bench_chaos_microbenchmarks
        chaos/bench_chaos_microbenchmarks.cpp
        ${CMAKE_SOURCE_DIR}/src/chaos/chaos_framework.cpp
    )

    target_include_directories(bench_chaos_microbenchmarks PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(bench_chaos_microbenchmarks PRIVATE
        benchmark::benchmark_main
        themis_core
        Threads::Threads
    )
    target_compile_definitions(bench_chaos_microbenchmarks PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    themis_apply_benchmark_release_flags(bench_chaos_microbenchmarks)

    install(TARGETS bench_chaos_microbenchmarks
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Chaos microbenchmarks: CHAOS-MB-01 inject p99, CHAOS-MB-02 recover p99, CHAOS-MB-03 concurrent throughput")
endif()

# ============================================================================
# Auto-registration for eligible benchmark sources not yet manually wired
#
# Goal: keep benchmark coverage aligned with current edition/feature switches
# without requiring immediate per-file hand wiring.
#
# Notes:
# - Targets are EXCLUDE_FROM_ALL, so regular default builds stay stable.
# - Targets are still discoverable and can be built explicitly.
# ============================================================================

option(THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS
    "Auto-register missing benchmark targets that are eligible in current edition/feature configuration"
    OFF)

if(THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS)
    file(GLOB _all_benchmark_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/bench*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/llm/llm_bench.cpp")

    set(_auto_registered_bench_targets "")
    foreach(_bench_source ${_all_benchmark_sources})
        get_filename_component(_bench_target "${_bench_source}" NAME_WE)

        if(TARGET ${_bench_target})
            continue()
        endif()

        themis_is_suite_enabled(_suite_enabled "${_bench_target}")
        if(NOT _suite_enabled)
            continue()
        endif()

        add_executable(${_bench_target} EXCLUDE_FROM_ALL
            ${_bench_source}
        )

        target_link_libraries(${_bench_target} PRIVATE
            ${BENCHMARK_LIBS}
            themis_core
            Threads::Threads
        )

        target_compile_definitions(${_bench_target} PRIVATE
            THEMIS_BENCHMARK_BUILD=1
        )

        themis_apply_benchmark_release_flags(${_bench_target})

        install(TARGETS ${_bench_target}
            RUNTIME DESTINATION bin/benchmarks
            COMPONENT benchmarks
        )

        themis_suite_group_for_name(_suite_group "${_bench_target}")
        set_property(TARGET ${_bench_target} PROPERTY FOLDER "benchmarks/auto/${_suite_group}")

        list(APPEND _auto_registered_bench_targets ${_bench_target})
    endforeach()

    list(LENGTH _auto_registered_bench_targets _auto_registered_count)
    message(STATUS "Auto-registered eligible benchmark targets: ${_auto_registered_count}")
endif()

# Some auto-registered benchmarks reference sharding protobuf message types
# directly and require explicit linkage to the generated shard proto target.
if(TARGET bench_gossip_config AND TARGET themis_shard_proto)
    target_link_libraries(bench_gossip_config PRIVATE themis_shard_proto)
endif()

# Aggregate build target for all benchmark executables that are currently
# eligible under edition/feature gating.
set(_eligible_benchmark_targets "")
get_property(_benchmark_dir_targets DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
foreach(_candidate_target ${_benchmark_dir_targets})
    if(_candidate_target MATCHES "^bench_" OR _candidate_target MATCHES "^benchmark_" OR _candidate_target STREQUAL "llm_bench")
        themis_is_suite_enabled(_candidate_enabled "${_candidate_target}")
        if(_candidate_enabled)
            list(APPEND _eligible_benchmark_targets ${_candidate_target})
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES _eligible_benchmark_targets)

add_custom_target(themis_benchmarks_all_eligible)
if(_eligible_benchmark_targets)
    add_dependencies(themis_benchmarks_all_eligible ${_eligible_benchmark_targets})
endif()

get_property(_wave_a_server_llm_benchmark_targets GLOBAL PROPERTY THEMIS_WAVE_A_BENCHMARK_SERVER_LLM_TARGETS)
if(_wave_a_server_llm_benchmark_targets)
    list(REMOVE_DUPLICATES _wave_a_server_llm_benchmark_targets)
    add_custom_target(themis_wave_a_server_llm_benchmarks
        DEPENDS ${_wave_a_server_llm_benchmark_targets})
    message(STATUS "themis_wave_a_server_llm_benchmarks aggregate target covers: ${_wave_a_server_llm_benchmark_targets}")
else()
    add_custom_target(themis_wave_a_server_llm_benchmarks)
    message(WARNING "themis_wave_a_server_llm_benchmarks: no targets collected — check Wave A benchmark labels")
endif()

get_property(_wave_a_sqst_benchmark_targets GLOBAL PROPERTY THEMIS_WAVE_A_BENCHMARK_SERVER_QUERY_STORAGE_TRANSACTION_TARGETS)
if(_wave_a_sqst_benchmark_targets)
    list(REMOVE_DUPLICATES _wave_a_sqst_benchmark_targets)
    add_custom_target(themis_wave_a_server_query_storage_transaction_benchmarks
        DEPENDS ${_wave_a_sqst_benchmark_targets})
    message(STATUS "themis_wave_a_server_query_storage_transaction_benchmarks aggregate target covers: ${_wave_a_sqst_benchmark_targets}")
else()
    add_custom_target(themis_wave_a_server_query_storage_transaction_benchmarks)
    message(WARNING "themis_wave_a_server_query_storage_transaction_benchmarks: no targets collected — check Wave A benchmark labels")
endif()

get_property(_wave_a_sharding_txn_benchmark_targets GLOBAL PROPERTY THEMIS_WAVE_A_BENCHMARK_SHARDING_TRANSACTION_TARGETS)
if(_wave_a_sharding_txn_benchmark_targets)
    list(REMOVE_DUPLICATES _wave_a_sharding_txn_benchmark_targets)
    add_custom_target(themis_wave_a_sharding_transaction_benchmarks
        DEPENDS ${_wave_a_sharding_txn_benchmark_targets})
    message(STATUS "themis_wave_a_sharding_transaction_benchmarks aggregate target covers: ${_wave_a_sharding_txn_benchmark_targets}")
else()
    add_custom_target(themis_wave_a_sharding_transaction_benchmarks)
    message(WARNING "themis_wave_a_sharding_transaction_benchmarks: no targets collected — check Wave A benchmark labels")
endif()

get_property(_wave_a_search_stack_benchmark_targets GLOBAL PROPERTY THEMIS_WAVE_A_BENCHMARK_SEARCH_INDEX_TENSOR_GRAPH_LLM_TARGETS)
if(_wave_a_search_stack_benchmark_targets)
    list(REMOVE_DUPLICATES _wave_a_search_stack_benchmark_targets)
    add_custom_target(themis_wave_a_search_index_tensor_graph_llm_benchmarks
        DEPENDS ${_wave_a_search_stack_benchmark_targets})
    message(STATUS "themis_wave_a_search_index_tensor_graph_llm_benchmarks aggregate target covers: ${_wave_a_search_stack_benchmark_targets}")
else()
    add_custom_target(themis_wave_a_search_index_tensor_graph_llm_benchmarks)
    message(WARNING "themis_wave_a_search_index_tensor_graph_llm_benchmarks: no targets collected — check Wave A benchmark labels")
endif()

list(LENGTH _eligible_benchmark_targets _eligible_benchmark_count)
message(STATUS "Aggregate benchmark target 'themis_benchmarks_all_eligible' includes ${_eligible_benchmark_count} currently eligible benchmark executables")

if(WIN32)
    set(_themis_vcpkg_runtime_bin_dir "")
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_themis_vcpkg_runtime_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    endif()

    function(themis_sync_runtime_dlls_for_benchmark_target target_name)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DBIN_DIR=${CMAKE_BINARY_DIR}/bin
                -DVCPKG_BIN_DIR=${_themis_vcpkg_runtime_bin_dir}
                -DDST_DIR=$<TARGET_FILE_DIR:${target_name}>
                -P ${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake
            COMMENT "Syncing core runtime DLLs for ${target_name}"
        )

        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DBIN_DIR=$<TARGET_FILE_DIR:${target_name}>
                -DVCPKG_BIN_DIR=${CMAKE_BINARY_DIR}/bin
                -DDST_DIR=${CMAKE_BINARY_DIR}/bin
                -P ${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake
            COMMENT "Syncing benchmark runtime DLLs for ${target_name} into build/bin"
        )
    endfunction()

    add_custom_target(sync_benchmark_runtime_dlls
        COMMAND ${CMAKE_COMMAND}
            -DBIN_DIR=${CMAKE_BINARY_DIR}/bin
            -DVCPKG_BIN_DIR=${_themis_vcpkg_runtime_bin_dir}
            -DDST_DIR=${CMAKE_BINARY_DIR}/cmake/benchmarks
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake
        COMMAND ${CMAKE_COMMAND}
            -DBIN_DIR=${CMAKE_BINARY_DIR}/cmake/benchmarks
            -DVCPKG_BIN_DIR=${CMAKE_BINARY_DIR}/bin
            -DDST_DIR=${CMAKE_BINARY_DIR}/bin
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake
        COMMENT "Syncing benchmark runtime DLLs between cmake/benchmarks and build/bin"
    )

    foreach(_benchmark_runtime_target ${_eligible_benchmark_targets})
        if(TARGET ${_benchmark_runtime_target})
            get_target_property(_benchmark_runtime_type ${_benchmark_runtime_target} TYPE)
            if(_benchmark_runtime_type STREQUAL "EXECUTABLE")
                themis_sync_runtime_dlls_for_benchmark_target(${_benchmark_runtime_target})
                add_dependencies(sync_benchmark_runtime_dlls ${_benchmark_runtime_target})
            endif()
        endif()
    endforeach()
endif()

themis_report_benchmark_coverage("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_LIST_FILE}")

# ============================================================================
# IntervalTreeIndex Erase Benchmarks (v1.7.0+)
# ITEB-01: single-key erase on varying tree sizes
# ITEB-02: bulk erase all keys
# ITEB-03: rebuild baseline comparison
# ITEB-04: mixed read/erase under concurrency
# ITEB-05: large-payload erase
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temporal/bench_interval_tree_erase.cpp" AND NOT TARGET bench_interval_tree_erase)
    message(STATUS "Adding IntervalTreeIndex Erase benchmarks (ITEB-01..05)")

    add_executable(bench_interval_tree_erase
        temporal/bench_interval_tree_erase.cpp
    )

    target_link_libraries(bench_interval_tree_erase PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_interval_tree_erase PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_interval_tree_erase)

    install(TARGETS bench_interval_tree_erase
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  IntervalTreeIndex: erase vs rebuild, bulk erase, mixed read/erase, large payload")
endif()

# ============================================================================
# Distributed Knowledge Federation Benchmarks (DK-8 / Session 9)
# BM_TriggerAggregation_N64, BM_FederatedRAGMerge_N16x50,
# BM_FeedbackDedup_Throughput, BM_PublishFeedback_Latency
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_distributed_knowledge.cpp" AND NOT TARGET bench_distributed_knowledge)
    message(STATUS "Adding Distributed Knowledge Federation benchmarks (DK-8, Session 9)")

    add_executable(bench_distributed_knowledge
        distributed_knowledge/bench_distributed_knowledge.cpp
    )

    target_include_directories(bench_distributed_knowledge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(bench_distributed_knowledge PRIVATE
        themis_distributed_knowledge
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_distributed_knowledge PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_distributed_knowledge)

    install(TARGETS bench_distributed_knowledge
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  DK-BM-01: triggerAggregation N=64 shards, 100 keys — target ≤ 500 ms")
    message(STATUS "  DK-BM-02: FederatedRAGMerge N=16 shards × 50 docs — target ≤ 20 ms")
    message(STATUS "  DK-BM-03: CrossShardFeedbackSync dedup throughput — target ≥ 10k msg/s")
    message(STATUS "  DK-BM-04: publishFeedback end-to-end latency — target ≤ 1 ms")
endif()

# ============================================================================
# Distributed Knowledge Operational Resilience Benchmarks (DK-OR / Session 11)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_distributed_knowledge_or.cpp" AND NOT TARGET bench_distributed_knowledge_or)
    message(STATUS "Adding Distributed Knowledge OR benchmarks (DK-OR, Session 11)")

    add_executable(bench_distributed_knowledge_or
        distributed_knowledge/bench_distributed_knowledge_or.cpp
    )

    target_include_directories(bench_distributed_knowledge_or PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(bench_distributed_knowledge_or PRIVATE
        themis_distributed_knowledge
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_distributed_knowledge_or PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_distributed_knowledge_or)

    install(TARGETS bench_distributed_knowledge_or
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  DK-OR-BM-01: triggerAggregation timeout overhead — target ≤ 2 ms vs no-timeout")
    message(STATUS "  DK-OR-BM-02: publishFeedback backpressure skip — target ≤ 0.5 µs/call")
endif()


# ============================================================================
# LLM+RAID Pipeline Benchmarks (Phase 3 + Phase 6 Distributed Inference Cases)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_llm_raid_pipeline.cpp" AND NOT TARGET bench_llm_raid_pipeline)
    message(STATUS "Adding LLM+RAID pipeline benchmarks (Phase 3 LoRA/RAID + Phase 6 Distributed Inference)")

    add_executable(bench_llm_raid_pipeline
        llm/bench_llm_raid_pipeline.cpp
    )

    target_include_directories(bench_llm_raid_pipeline PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(bench_llm_raid_pipeline PRIVATE
        themis_core
        themis_llm
        ${BENCHMARK_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_llm_raid_pipeline PRIVATE
        THEMIS_BENCHMARK_BUILD=1
        THEMIS_ENABLE_LLM=1
    )

    themis_apply_benchmark_release_flags(bench_llm_raid_pipeline)

    install(TARGETS bench_llm_raid_pipeline
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  BM_ModelLoadSingleShard, BM_LoRA_LoadSingleAdapter, BM_LoRA_LoadMultipleAdapters")
    message(STATUS "  BM_LoRA_SwitchingLatency, BM_Inference_WithoutLoRA, BM_Inference_WithSingleLoRA")
    message(STATUS "  BM_MultiShard_ConcurrentInference, BM_DataDistribution_RAIDStriping")
    message(STATUS "  BM_CompletePipeline_EndToEnd")
    message(STATUS "  BM_DomainRouting_OverheadPerRequest (Phase 6 distributed inference)")
    message(STATUS "  BM_BatchFanOut_LatencyScaling (Phase 6 distributed inference)")
endif()

# ============================================================================
# Batch-wired benchmarks (standard Google Benchmark targets)
# ============================================================================

    themis_add_standard_benchmark(bench_ai_plugin_generator    ai/bench_ai_plugin_generator.cpp)
themis_add_standard_benchmark(bench_ann_cpu_gpu_dispatch   ann/bench_ann_cpu_gpu_dispatch.cpp)
themis_add_standard_benchmark(bench_arm_memory             base/bench_arm_memory.cpp)
themis_add_standard_benchmark(bench_arm_simd               base/bench_arm_simd.cpp)
themis_add_standard_benchmark(bench_async_io_multiscan     base/bench_async_io_multiscan.cpp)
themis_add_standard_benchmark(bench_batch_insert           storage/bench_batch_insert.cpp)
themis_add_standard_benchmark(bench_blob_zstd              storage/bench_blob_zstd.cpp)
themis_add_standard_benchmark(bench_changefeed_throughput  cdc/bench_changefeed_throughput.cpp)
themis_add_standard_benchmark(bench_comprehensive          base/bench_comprehensive.cpp)
themis_add_standard_benchmark(bench_compression            base/bench_compression.cpp)
themis_add_standard_benchmark(bench_cross_functional_end_to_end process/bench_cross_functional_end_to_end.cpp)
themis_add_standard_benchmark(bench_crud                   storage/bench_crud.cpp)
themis_add_standard_benchmark(bench_edge_cases_comprehensive base/bench_edge_cases_comprehensive.cpp)
themis_add_standard_benchmark(bench_embedded_llm           llm/bench_embedded_llm.cpp)
themis_add_standard_benchmark(bench_encryption             security/bench_encryption.cpp)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_ethics_ai_plugin.cpp")
    if(NOT TARGET bench_ethics_ai_plugin)
        add_executable(bench_ethics_ai_plugin ethics_ai/bench_ethics_ai_plugin.cpp)
        install(TARGETS bench_ethics_ai_plugin RUNTIME DESTINATION bin/benchmarks)
    endif()

if(TARGET bench_ethics_ai_plugin)
    target_sources(bench_ethics_ai_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/ethics_ai_types.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/philosophy_loader.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/argument_store.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/rag_context_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/discourse_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/ethics_evaluator.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/chain_visualizer.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/ethics_profile_registry.cpp
        ${CMAKE_SOURCE_DIR}/src/ethics_ai/ethics_selection_router.cpp
    )
    target_link_libraries(bench_ethics_ai_plugin PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_compile_definitions(bench_ethics_ai_plugin PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    target_include_directories(bench_ethics_ai_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/ethics_ai
    )
    themis_apply_benchmark_release_flags(bench_ethics_ai_plugin)

    if(TARGET yaml-cpp::yaml-cpp)
        target_link_libraries(bench_ethics_ai_plugin PRIVATE yaml-cpp::yaml-cpp)
        target_compile_definitions(bench_ethics_ai_plugin PRIVATE HAVE_YAML_CPP)
    endif()
endif()
endif()

# LDM benchmark suite (bench_ethics_ai_ldm)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ethics_ai/CMakeLists.txt")
    add_subdirectory(ethics_ai ${CMAKE_CURRENT_BINARY_DIR}/ethics_ai)
endif()
themis_add_standard_benchmark(bench_extended_context       llm/bench_extended_context.cpp)
themis_add_standard_benchmark(bench_flash_attention        llm/bench_flash_attention.cpp)
themis_add_standard_benchmark(bench_gnn_embeddings         graph/bench_gnn_embeddings.cpp)
themis_add_standard_benchmark(bench_aql_geo_filter         aql/bench_aql_geo_filter.cpp)
themis_add_standard_benchmark(bench_geo_dbscan             geo/bench_geo_dbscan.cpp)
themis_add_standard_benchmark(bench_geojson_parse          geo/bench_geojson_parse.cpp)
themis_add_standard_benchmark(bench_gorilla_codec          base/bench_gorilla_codec.cpp)
themis_add_standard_benchmark(bench_gossip_config          sharding/bench_gossip_config.cpp)
themis_add_standard_benchmark(bench_gpu_erasure            gpu/bench_gpu_erasure.cpp)
themis_add_standard_benchmark(bench_gpu_a8_baselines      gpu/bench_gpu_a8_baselines.cpp)
themis_add_standard_benchmark(bench_gpu_phase2_phase3_gates gpu/bench_gpu_phase2_phase3_gates.cpp)
themis_add_standard_benchmark(bench_gpu_phase_c_gates      gpu/bench_gpu_phase_c_gates.cpp)
themis_add_standard_benchmark(bench_hsm_provider           security/bench_hsm_provider.cpp)
themis_add_standard_benchmark(bench_hybrid_aql_sugar       aql/bench_hybrid_aql_sugar.cpp)
themis_add_standard_benchmark(bench_image_analysis         image_analysis/bench_image_analysis.cpp)
themis_add_standard_benchmark(bench_image_analysis_latency image_analysis/bench_image_analysis_latency.cpp)
themis_add_standard_benchmark(bench_insert_profiling       storage/bench_insert_profiling.cpp)
themis_add_standard_benchmark(bench_latency_comprehensive  base/bench_latency_comprehensive.cpp)
themis_add_standard_benchmark(bench_legal_lora_pipeline    llm/bench_legal_lora_pipeline.cpp)
themis_add_standard_benchmark(bench_llm_infrastructure     llm/bench_llm_infrastructure.cpp)
themis_add_standard_benchmark(bench_llm_judge_integration  llm/bench_llm_judge_integration.cpp)
themis_add_standard_benchmark(bench_llm_real_models        llm/bench_llm_real_models.cpp)
themis_add_standard_benchmark(bench_llm_response_cache     llm/bench_llm_response_cache.cpp)
themis_add_standard_benchmark(bench_locality_aware_router  sharding/bench_locality_aware_router.cpp)
themis_add_standard_benchmark(bench_lock_contention        maintenance/bench_lock_contention.cpp)
themis_add_standard_benchmark(bench_lora_inline            llm/bench_lora_inline.cpp)
themis_add_standard_benchmark(bench_lora_training          llm/bench_lora_training.cpp)
themis_add_standard_benchmark(bench_multitask_lora_training llm/bench_multitask_lora_training.cpp)
themis_add_standard_benchmark(bench_mmdb                   mmdb/bench_mmdb.cpp)
themis_add_standard_benchmark(bench_multithreading_comprehensive base/bench_multithreading_comprehensive.cpp)
themis_add_standard_benchmark(bench_mvcc                   storage/bench_mvcc.cpp)
themis_add_standard_benchmark(bench_pagerank               graph/bench_pagerank.cpp)
themis_add_standard_benchmark(bench_phase1_flash_attention llm/bench_phase1_flash_attention.cpp)
themis_add_standard_benchmark(bench_plugin_hot_plug        plugins/bench_plugin_hot_plug.cpp)
themis_add_standard_benchmark(bench_ssm_phase0_baseline     storage/bench_ssm_phase0_baseline.cpp)
themis_add_standard_benchmark(bench_plugin_system           plugins/bench_plugin_system.cpp)
themis_add_standard_benchmark(bench_qlora_gpu_kernels      gpu/bench_qlora_gpu_kernels.cpp)
    themis_add_standard_benchmark(bench_raid_lora              llm/bench_raid_lora.cpp)
themis_add_standard_benchmark(bench_saga_compensation      maintenance/bench_saga_compensation.cpp)
themis_add_standard_benchmark(bench_sanity                 maintenance/bench_sanity.cpp)
themis_add_standard_benchmark(bench_scalability_comprehensive base/bench_scalability_comprehensive.cpp)
themis_add_standard_benchmark(bench_shard_resource_manager sharding/bench_shard_resource_manager.cpp)
themis_add_standard_benchmark(bench_simd_distance          base/bench_simd_distance.cpp)
themis_add_standard_benchmark(bench_simple_insert_test     storage/bench_simple_insert_test.cpp)
themis_add_standard_benchmark(bench_tensor_cpu_gpu_dispatch gpu/bench_tensor_cpu_gpu_dispatch.cpp)
themis_add_standard_benchmark(bench_tensor_deduplication_manager bench_tensor_deduplication_manager.cpp)
themis_add_standard_benchmark(bench_tensor_integration_baseline bench_tensor_integration_baseline.cpp)
themis_add_standard_benchmark(bench_tensor_compression_routing tensor/bench_tensor_compression_routing.cpp)
themis_add_standard_benchmark(bench_tensor_deduplication_manager tensor/bench_tensor_deduplication_manager.cpp)
themis_add_standard_benchmark(bench_tensor_integration_baseline tensor/bench_tensor_integration_baseline.cpp)

# ── Dynamic Tensor Update benchmarks — sub-issue #5472 (BDU-01..10) ─────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tensor/bench_tensor_dynamic_update.cpp" AND NOT TARGET bench_tensor_dynamic_update)
    add_executable(bench_tensor_dynamic_update
        tensor/bench_tensor_dynamic_update.cpp
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/stale_artifact_detector.cc
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/error_recovery_handler.cc
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/distributed_lock_manager.cc
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/src/artifact_invalidation.cc
    )
    target_include_directories(bench_tensor_dynamic_update PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/distributed_tensor/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(bench_tensor_dynamic_update PRIVATE
        ${BENCHMARK_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_compile_features(bench_tensor_dynamic_update PRIVATE cxx_std_17)
    themis_apply_benchmark_release_flags(bench_tensor_dynamic_update)
    message(STATUS "Adding benchmark: bench_tensor_dynamic_update (BDU-01..10, sub-issue #5472)")
endif()

themis_add_standard_benchmark(bench_tpcc                   tpc/bench_tpcc.cpp)
themis_add_standard_benchmark(bench_tpch                   tpc/bench_tpch.cpp)
    themis_add_standard_benchmark(bench_v1_3_0_features        performance_optimizations/bench_v1_3_0_features.cpp)
    themis_add_standard_benchmark(bench_v1_3_4_optimizations   performance_optimizations/bench_v1_3_4_optimizations.cpp)
themis_add_standard_benchmark(bench_video_processor        process/bench_video_processor.cpp)
themis_add_standard_benchmark(bench_wal_stress             storage/bench_wal_stress.cpp)
    themis_add_standard_benchmark(benchmark_distributed_hybrid_search search/benchmark_distributed_hybrid_search.cpp)
    themis_add_standard_benchmark(benchmark_hybrid_search      search/benchmark_hybrid_search.cpp)
    themis_add_standard_benchmark(benchmark_image_analysis     image_analysis/benchmark_image_analysis.cpp)
    themis_add_standard_benchmark(kernel_block_size_bench      gpu/kernel_block_size_bench.cpp)

# ============================================================================
# Benchmarks with custom main() (plain executables, not Google Benchmark)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/monitoring/bench_cycle_metrics.cpp" AND NOT TARGET bench_cycle_metrics)
    add_executable(bench_cycle_metrics monitoring/bench_cycle_metrics.cpp)
    target_include_directories(bench_cycle_metrics PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(bench_cycle_metrics PRIVATE themis_core Threads::Threads)
    themis_apply_benchmark_release_flags(bench_cycle_metrics)
    install(TARGETS bench_cycle_metrics RUNTIME DESTINATION bin/benchmarks COMPONENT benchmarks)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/bench_auto_buffers.cpp" AND NOT TARGET bench_auto_buffers)
    add_executable(bench_auto_buffers base/bench_auto_buffers.cpp)
    target_include_directories(bench_auto_buffers PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(bench_auto_buffers PRIVATE themis_core Threads::Threads)
    themis_apply_benchmark_release_flags(bench_auto_buffers)
    install(TARGETS bench_auto_buffers RUNTIME DESTINATION bin/benchmarks COMPONENT benchmarks)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maintenance/bench_docker_raid_comprehensive.cpp" AND NOT TARGET bench_docker_raid_comprehensive)
    add_executable(bench_docker_raid_comprehensive maintenance/bench_docker_raid_comprehensive.cpp)
    target_include_directories(bench_docker_raid_comprehensive PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(bench_docker_raid_comprehensive PRIVATE themis_core Threads::Threads)
    themis_apply_benchmark_release_flags(bench_docker_raid_comprehensive)
    install(TARGETS bench_docker_raid_comprehensive RUNTIME DESTINATION bin/benchmarks COMPONENT benchmarks)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ann/bench_lossy_vs_lossless.cpp" AND NOT TARGET bench_lossy_vs_lossless)
    add_executable(bench_lossy_vs_lossless ann/bench_lossy_vs_lossless.cpp)
    target_include_directories(bench_lossy_vs_lossless PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(bench_lossy_vs_lossless PRIVATE themis_core Threads::Threads)
    themis_apply_benchmark_release_flags(bench_lossy_vs_lossless)
    install(TARGETS bench_lossy_vs_lossless RUNTIME DESTINATION bin/benchmarks COMPONENT benchmarks)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/geo/bench_spatial_join.cpp" AND NOT TARGET bench_spatial_join)
    add_executable(bench_spatial_join geo/bench_spatial_join.cpp)
    target_include_directories(bench_spatial_join PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(bench_spatial_join PRIVATE themis_core Threads::Threads)
    themis_apply_benchmark_release_flags(bench_spatial_join)
    install(TARGETS bench_spatial_join RUNTIME DESTINATION bin/benchmarks COMPONENT benchmarks)
endif()

# Ensure late-registered standard benchmark targets get required implementation
# units even when platform-gated explicit blocks above are skipped.
if(TARGET bench_user_storage_mount_latency)
    target_sources(bench_user_storage_mount_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/src/user_storage_encrypted/gocryptfs_backend.cpp
    )
    target_include_directories(bench_user_storage_mount_latency PRIVATE
        ${CMAKE_SOURCE_DIR}/src/user_storage_encrypted
        ${CMAKE_SOURCE_DIR}/include/user_storage_encrypted
    )
endif()

if(TARGET bench_whisper_transcription)
    target_sources(bench_whisper_transcription PRIVATE
        ${CMAKE_SOURCE_DIR}/src/whisper/whisper_plugin.cpp
        ${CMAKE_SOURCE_DIR}/src/whisper/whisper_config.cpp
        ${CMAKE_SOURCE_DIR}/src/whisper/audio_chunk_reader.cpp
    )
    target_include_directories(bench_whisper_transcription PRIVATE
        ${CMAKE_SOURCE_DIR}/include/whisper
        ${CMAKE_SOURCE_DIR}/src/whisper
    )
    target_compile_definitions(bench_whisper_transcription PRIVATE
        THEMIS_PLUGIN_EXPORTS
    )
endif()

# ============================================================================
# API Module Benchmarks (Q3 2026 transport + Phase 5 release gates)
# ============================================================================

# Q3 2026 transport hot-path benchmarks
themis_add_standard_benchmark(bench_api_transport api/bench_api_transport.cpp)

# Phase 5 release-gate benchmarks (GATE-API-01..06 + p95/p99 envelopes)
themis_add_standard_benchmark(bench_api_release_gates api/bench_api_release_gates.cpp)

# ============================================================================
# Wave 3C — Infrastructure Module Release-Gate Benchmarks (Phase 5)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maintenance/CMakeLists.txt")
    add_subdirectory(maintenance)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/CMakeLists.txt")
    add_subdirectory(plugins)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stable_diffusion/CMakeLists.txt")
    add_subdirectory(stable_diffusion)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rpc_grpc/CMakeLists.txt")
    add_subdirectory(rpc_grpc)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scheduler/CMakeLists.txt")
    add_subdirectory(scheduler)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/execution/CMakeLists.txt")
    add_subdirectory(execution)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scraper/CMakeLists.txt")
    add_subdirectory(scraper)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user_storage_encrypted/CMakeLists.txt")
    add_subdirectory(user_storage_encrypted)
endif()

# Wave D: voice and llm_streaming dedicated benchmark gates
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/CMakeLists.txt")
    add_subdirectory(voice)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm_streaming/CMakeLists.txt")
    add_subdirectory(llm_streaming)
endif()

# ===== UNREGISTERED BENCHMARK SOURCES — BATCH REGISTRATION =====
# Executed: 2026-10-11 Phase 2, Task 2
# Count: 58 sources (4 base/ sources fixed separately in Task 1)


# Module: acceleration
themis_add_standard_benchmark(bench_acceleration_dedicated_gates            acceleration/bench_acceleration_dedicated_gates.cpp)

# Module: analytics
themis_add_standard_benchmark(bench_analytics_critical_paths_focused        analytics/bench_analytics_critical_paths_focused.cpp)

# Module: chimera
themis_add_standard_benchmark(bench_chimera_adapter                         chimera/bench_chimera_adapter.cpp)
themis_add_standard_benchmark(bench_chimera_dedicated_gates                 chimera/bench_chimera_dedicated_gates.cpp)

# Module: config
themis_add_standard_benchmark(bench_config_release_gates                    config/bench_config_release_gates.cpp)

# Module: content
themis_add_standard_benchmark(bench_text_extraction                         content/bench_text_extraction.cpp)

# Module: distributed_tensor
themis_add_standard_benchmark(bench_distributed_tensor_dedicated_gates      distributed_tensor/bench_distributed_tensor_dedicated_gates.cpp)

# Module: document
themis_add_standard_benchmark(bench_document_diff_merge                     document/bench_document_diff_merge.cpp)
themis_add_standard_benchmark(bench_document_round_trip                     document/bench_document_round_trip.cpp)
themis_add_standard_benchmark(bench_document_serialization_gates            document/bench_document_serialization_gates.cpp)
themis_add_standard_benchmark(bench_document_store                          document/bench_document_store.cpp)

# Module: evaluation
themis_add_standard_benchmark(bench_evaluation_dedicated_gates              evaluation/bench_evaluation_dedicated_gates.cpp)

# Module: geo
themis_add_standard_benchmark(bench_geo_cuda_kernel_gating                  geo/bench_geo_cuda_kernel_gating.cpp)
themis_add_standard_benchmark(bench_geo_release_gates                       geo/bench_geo_release_gates.cpp)

# Module: governance
themis_add_standard_benchmark(bench_audit_export_high_volume                governance/bench_audit_export_high_volume.cpp)

# Module: gpu
themis_add_standard_benchmark(bench_gpu_cpu_breakeven_category_a            gpu/bench_gpu_cpu_breakeven_category_a.cpp)
themis_add_standard_benchmark(bench_gpu_cpu_breakeven_category_b            gpu/bench_gpu_cpu_breakeven_category_b.cpp)
themis_add_standard_benchmark(bench_gpu_dedicated_gates                     gpu/bench_gpu_dedicated_gates.cpp)

# Module: graph
themis_add_standard_benchmark(bench_graph_dedicated_gates                   graph/bench_graph_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_graph_release_gates                     graph/bench_graph_release_gates.cpp)

# Module: llm_streaming
themis_add_standard_benchmark(bench_llm_streaming_dedicated_gates           llm_streaming/bench_llm_streaming_dedicated_gates.cpp)

# Module: network
themis_add_standard_benchmark(bench_network_protocol_path                   network/bench_network_protocol_path.cpp)
themis_add_standard_benchmark(bench_network_release_gates                   network/bench_network_release_gates.cpp)
themis_add_standard_benchmark(bench_network_routing_gates                   network/bench_network_routing_gates.cpp)

# Module: prompt_engineering
themis_add_standard_benchmark(bench_rewrite_engine                          prompt_engineering/bench_rewrite_engine.cpp)

# Module: replication
themis_add_standard_benchmark(bench_geo_placement_wal_shipping              replication/bench_geo_placement_wal_shipping.cpp)
themis_add_standard_benchmark(bench_replication_dedicated_gates             replication/bench_replication_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_replication_multi_dc_multi_writer       replication/bench_replication_multi_dc_multi_writer.cpp)
themis_add_standard_benchmark(bench_replication_release_gates               replication/bench_replication_release_gates.cpp)

# Module: scheduler
themis_add_standard_benchmark(bench_scheduler_dedicated_gates               scheduler/bench_scheduler_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_scheduler_release_gates                 scheduler/bench_scheduler_release_gates.cpp)

# Module: security
themis_add_standard_benchmark(bench_audit_export_gates                      security/bench_audit_export_gates.cpp)
themis_add_standard_benchmark(bench_sbom_verification_gates                 security/bench_sbom_verification_gates.cpp)
themis_add_standard_benchmark(bench_security_phase2_crypto_gates            security/bench_security_phase2_crypto_gates.cpp)
themis_add_standard_benchmark(bench_security_phase3_policy_gates            security/bench_security_phase3_policy_gates.cpp)
themis_add_standard_benchmark(bench_security_release_gates                  security/bench_security_release_gates.cpp)

# Module: sharding
themis_add_standard_benchmark(bench_sharding_dedicated_gates                sharding/bench_sharding_dedicated_gates.cpp)

# Module: storage
themis_add_standard_benchmark(bench_storage_dedicated_gates                 storage/bench_storage_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_storage_release_gates                   storage/bench_storage_release_gates.cpp)

# Module: temporal
themis_add_standard_benchmark(bench_temporal_dedicated_gates                temporal/bench_temporal_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_temporal_release_gates                  temporal/bench_temporal_release_gates.cpp)

# Module: tensor
themis_add_standard_benchmark(bench_tensor_dedicated_gates                  tensor/bench_tensor_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_tensor_release_gates                    tensor/bench_tensor_release_gates.cpp)

# Module: timeseries
themis_add_standard_benchmark(bench_timeseries_dedicated_gates              timeseries/bench_timeseries_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_timeseries_release_gates                timeseries/bench_timeseries_release_gates.cpp)

# Module: training
themis_add_standard_benchmark(bench_training_dedicated_gates                training/bench_training_dedicated_gates.cpp)
themis_add_standard_benchmark(bench_training_lifecycle_gates                training/bench_training_lifecycle_gates.cpp)

# Module: updates
themis_add_standard_benchmark(bench_updates_cluster_stress_q4               updates/bench_updates_cluster_stress_q4.cpp)

# Module: utils
themis_add_standard_benchmark(bench_utils_benchmark_runner                  utils/benchmark_runner.cpp)

# Module: voice
themis_add_standard_benchmark(bench_voice_dedicated_gates                   voice/bench_voice_dedicated_gates.cpp)

# Module: wave8
themis_add_standard_benchmark(bench_w8a_incident_regression_shielding       wave8/bench_w8a_incident_regression_shielding.cpp)
themis_add_standard_benchmark(bench_w8b_threshold_hardening_drift_detection wave8/bench_w8b_threshold_hardening_drift_detection.cpp)
themis_add_standard_benchmark(bench_w8c_deterministic_ci_harness            wave8/bench_w8c_deterministic_ci_harness.cpp)
themis_add_standard_benchmark(bench_w8d_operability_runbooks_ownership      wave8/bench_w8d_operability_runbooks_ownership.cpp)

# Module: wave9
themis_add_standard_benchmark(bench_w9a_security_overhead_audit             wave9/bench_w9a_security_overhead_audit.cpp)
themis_add_standard_benchmark(bench_w9b_sla_measurement_compliance          wave9/bench_w9b_sla_measurement_compliance.cpp)
themis_add_standard_benchmark(bench_w9c_chaos_fault_recovery                wave9/bench_w9c_chaos_fault_recovery.cpp)
themis_add_standard_benchmark(bench_w9d_multi_tenant_isolation              wave9/bench_w9d_multi_tenant_isolation.cpp)

# ===== CTest REGISTRATION BATCH — Phase 2 Task 3 =====
# Generated: 2026-10-11
# Targets: 385
# Strategy: Module heuristic from target name + release_critical markers
# Integration: Add to benchmarks/CMakeLists.txt before final endfunction()

if(THEMIS_BUILD_BENCHMARKS)

  # Module: acceleration (3)
  if(TARGET bench_acceleration_cuda_gates)
    themis_register_benchmark_ctest(
      NAME   bench_acceleration_cuda_gates
      TARGET bench_acceleration_cuda_gates
      MODULE "acceleration"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_acceleration_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_acceleration_dedicated_gates
      TARGET bench_acceleration_dedicated_gates
      MODULE "acceleration"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_acceleration_dispatch)
    themis_register_benchmark_ctest(
      NAME   bench_acceleration_dispatch
      TARGET bench_acceleration_dispatch
      MODULE "acceleration"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: access_coordinator (1)
  if(TARGET bench_access_coordinator_gates)
    themis_register_benchmark_ctest(
      NAME   bench_access_coordinator_gates
      TARGET bench_access_coordinator_gates
      MODULE "access_coordinator"
      TIMEOUT 120
    )
  endif()
  # Module: active_vram (1)
  if(TARGET bench_active_vram_allocator)
    themis_register_benchmark_ctest(
      NAME   bench_active_vram_allocator
      TARGET bench_active_vram_allocator
      MODULE "active_vram"
      TIMEOUT 120
    )
  endif()
  # Module: advanced_patterns (1)
  if(TARGET bench_advanced_patterns)
    themis_register_benchmark_ctest(
      NAME   bench_advanced_patterns
      TARGET bench_advanced_patterns
      MODULE "advanced_patterns"
      TIMEOUT 120
    )
  endif()
  # Module: advanced_scenarios (1)
  if(TARGET bench_advanced_scenarios)
    themis_register_benchmark_ctest(
      NAME   bench_advanced_scenarios
      TARGET bench_advanced_scenarios
      MODULE "advanced_scenarios"
      TIMEOUT 120
    )
  endif()
  # Module: ai_plugin (1)
  if(TARGET bench_ai_plugin_generator)
    themis_register_benchmark_ctest(
      NAME   bench_ai_plugin_generator
      TARGET bench_ai_plugin_generator
      MODULE "ai_plugin"
      TIMEOUT 120
    )
  endif()
  # Module: analytics_critical (1)
  if(TARGET bench_analytics_critical_paths_focused)
    themis_register_benchmark_ctest(
      NAME   bench_analytics_critical_paths_focused
      TARGET bench_analytics_critical_paths_focused
      MODULE "analytics_critical"
      TIMEOUT 120
    )
  endif()
  # Module: analytics_dedicated (1)
  if(TARGET bench_analytics_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_analytics_dedicated_gates
      TARGET bench_analytics_dedicated_gates
      MODULE "analytics_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: analytics_gap (1)
  if(TARGET bench_analytics_gap_closure)
    themis_register_benchmark_ctest(
      NAME   bench_analytics_gap_closure
      TARGET bench_analytics_gap_closure
      MODULE "analytics_gap"
      TIMEOUT 120
    )
  endif()
  # Module: analytics_release (1)
  if(TARGET bench_analytics_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_analytics_release_gates
      TARGET bench_analytics_release_gates
      MODULE "analytics_release"
      TIMEOUT 120
    )
  endif()
  # Module: ann_distance (1)
  if(TARGET bench_ann_distance_cpu_vs_flat)
    themis_register_benchmark_ctest(
      NAME   bench_ann_distance_cpu_vs_flat
      TARGET bench_ann_distance_cpu_vs_flat
      MODULE "ann_distance"
      TIMEOUT 120
    )
  endif()
  # Module: api_endpoints (1)
  if(TARGET bench_api_endpoints)
    themis_register_benchmark_ctest(
      NAME   bench_api_endpoints
      TARGET bench_api_endpoints
      MODULE "api_endpoints"
      TIMEOUT 120
    )
  endif()
  # Module: api_release (1)
  if(TARGET bench_api_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_api_release_gates
      TARGET bench_api_release_gates
      MODULE "api_release"
      TIMEOUT 120
    )
  endif()
  # Module: api_transport (1)
  if(TARGET bench_api_transport)
    themis_register_benchmark_ctest(
      NAME   bench_api_transport
      TARGET bench_api_transport
      MODULE "api_transport"
      TIMEOUT 120
    )
  endif()
  # Module: approximate_radius (1)
  if(TARGET bench_approximate_radius_search)
    themis_register_benchmark_ctest(
      NAME   bench_approximate_radius_search
      TARGET bench_approximate_radius_search
      MODULE "approximate_radius"
      TIMEOUT 120
    )
  endif()
  # Module: aql_assistance (1)
  if(TARGET bench_aql_assistance_gates)
    themis_register_benchmark_ctest(
      NAME   bench_aql_assistance_gates
      TARGET bench_aql_assistance_gates
      MODULE "aql_assistance"
      TIMEOUT 120
    )
  endif()
  # Module: aql_consistency (1)
  if(TARGET bench_aql_consistency_performance_gates)
    themis_register_benchmark_ctest(
      NAME   bench_aql_consistency_performance_gates
      TARGET bench_aql_consistency_performance_gates
      MODULE "aql_consistency"
      TIMEOUT 120
    )
  endif()
  # Module: aql_functions (1)
  if(TARGET bench_aql_functions)
    themis_register_benchmark_ctest(
      NAME   bench_aql_functions
      TARGET bench_aql_functions
      MODULE "aql_functions"
      TIMEOUT 120
    )
  endif()
  # Module: aql_geo (1)
  if(TARGET bench_aql_geo_filter)
    themis_register_benchmark_ctest(
      NAME   bench_aql_geo_filter
      TARGET bench_aql_geo_filter
      MODULE "aql_geo"
      TIMEOUT 120
    )
  endif()
  # Module: aql_helper (1)
  if(TARGET bench_aql_helper_paths)
    themis_register_benchmark_ctest(
      NAME   bench_aql_helper_paths
      TARGET bench_aql_helper_paths
      MODULE "aql_helper"
      TIMEOUT 120
    )
  endif()
  # Module: aql_mutations (1)
  if(TARGET bench_aql_mutations)
    themis_register_benchmark_ctest(
      NAME   bench_aql_mutations
      TARGET bench_aql_mutations
      MODULE "aql_mutations"
      TIMEOUT 120
    )
  endif()
  # Module: aql_translation (1)
  if(TARGET bench_aql_translation)
    themis_register_benchmark_ctest(
      NAME   bench_aql_translation
      TARGET bench_aql_translation
      MODULE "aql_translation"
      TIMEOUT 120
    )
  endif()
  # Module: arm_memory (1)
  if(TARGET bench_arm_memory)
    themis_register_benchmark_ctest(
      NAME   bench_arm_memory
      TARGET bench_arm_memory
      MODULE "arm_memory"
      TIMEOUT 120
    )
  endif()
  # Module: arm_simd (1)
  if(TARGET bench_arm_simd)
    themis_register_benchmark_ctest(
      NAME   bench_arm_simd
      TARGET bench_arm_simd
      MODULE "arm_simd"
      TIMEOUT 120
    )
  endif()
  # Module: async_io (1)
  if(TARGET bench_async_io_multiscan)
    themis_register_benchmark_ctest(
      NAME   bench_async_io_multiscan
      TARGET bench_async_io_multiscan
      MODULE "async_io"
      TIMEOUT 120
    )
  endif()
  # Module: audit_export (2)
  if(TARGET bench_audit_export_gates)
    themis_register_benchmark_ctest(
      NAME   bench_audit_export_gates
      TARGET bench_audit_export_gates
      MODULE "audit_export"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_audit_export_high_volume)
    themis_register_benchmark_ctest(
      NAME   bench_audit_export_high_volume
      TARGET bench_audit_export_high_volume
      MODULE "audit_export"
      TIMEOUT 120
    )
  endif()
  # Module: auth_hotpaths (1)
  if(TARGET bench_auth_hotpaths)
    themis_register_benchmark_ctest(
      NAME   bench_auth_hotpaths
      TARGET bench_auth_hotpaths
      MODULE "auth_hotpaths"
      TIMEOUT 120
    )
  endif()
  # Module: auth_token (1)
  if(TARGET bench_auth_token_validation)
    themis_register_benchmark_ctest(
      NAME   bench_auth_token_validation
      TARGET bench_auth_token_validation
      MODULE "auth_token"
      TIMEOUT 120
    )
  endif()
  # Module: auto_buffers (1)
  if(TARGET bench_auto_buffers)
    themis_register_benchmark_ctest(
      NAME   bench_auto_buffers
      TARGET bench_auto_buffers
      MODULE "auto_buffers"
      TIMEOUT 120
    )
  endif()
  # Module: backend_comparison (1)
  if(TARGET bench_backend_comparison)
    themis_register_benchmark_ctest(
      NAME   bench_backend_comparison
      TARGET bench_backend_comparison
      MODULE "backend_comparison"
      TIMEOUT 120
    )
  endif()
  # Module: base (29)
  if(TARGET bench_analytics_distributed_coordinator)
    themis_register_benchmark_ctest(
      NAME   bench_analytics_distributed_coordinator
      TARGET bench_analytics_distributed_coordinator
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_analytics_operability_paths)
    themis_register_benchmark_ctest(
      NAME   bench_analytics_operability_paths
      TARGET bench_analytics_operability_paths
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_changefeed_throughput)
    themis_register_benchmark_ctest(
      NAME   bench_changefeed_throughput
      TARGET bench_changefeed_throughput
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_chaos_microbenchmarks)
    themis_register_benchmark_ctest(
      NAME   bench_chaos_microbenchmarks
      TARGET bench_chaos_microbenchmarks
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_comprehensive)
    themis_register_benchmark_ctest(
      NAME   bench_comprehensive
      TARGET bench_comprehensive
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_compression)
    themis_register_benchmark_ctest(
      NAME   bench_compression
      TARGET bench_compression
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_crud)
    themis_register_benchmark_ctest(
      NAME   bench_crud
      TARGET bench_crud
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_distributed_coordinator)
    themis_register_benchmark_ctest(
      NAME   bench_distributed_coordinator
      TARGET bench_distributed_coordinator
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_distributed_knowledge)
    themis_register_benchmark_ctest(
      NAME   bench_distributed_knowledge
      TARGET bench_distributed_knowledge
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_distributed_knowledge_or)
    themis_register_benchmark_ctest(
      NAME   bench_distributed_knowledge_or
      TARGET bench_distributed_knowledge_or
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_document_serialization_gates)
    themis_register_benchmark_ctest(
      NAME   bench_document_serialization_gates
      TARGET bench_document_serialization_gates
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_encryption)
    themis_register_benchmark_ctest(
      NAME   bench_encryption
      TARGET bench_encryption
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_exporters)
    themis_register_benchmark_ctest(
      NAME   bench_exporters
      TARGET bench_exporters
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_latency_comprehensive)
    themis_register_benchmark_ctest(
      NAME   bench_latency_comprehensive
      TARGET bench_latency_comprehensive
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_maintenance_distributed_gates)
    themis_register_benchmark_ctest(
      NAME   bench_maintenance_distributed_gates
      TARGET bench_maintenance_distributed_gates
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_mmdb)
    themis_register_benchmark_ctest(
      NAME   bench_mmdb
      TARGET bench_mmdb
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_multithreading_comprehensive)
    themis_register_benchmark_ctest(
      NAME   bench_multithreading_comprehensive
      TARGET bench_multithreading_comprehensive
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_mvcc)
    themis_register_benchmark_ctest(
      NAME   bench_mvcc
      TARGET bench_mvcc
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_observability_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_observability_release_gates
      TARGET bench_observability_release_gates
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_pagerank)
    themis_register_benchmark_ctest(
      NAME   bench_pagerank
      TARGET bench_pagerank
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_postgres_transactions)
    themis_register_benchmark_ctest(
      NAME   bench_postgres_transactions
      TARGET bench_postgres_transactions
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_residual_quantization)
    themis_register_benchmark_ctest(
      NAME   bench_residual_quantization
      TARGET bench_residual_quantization
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_sanity)
    themis_register_benchmark_ctest(
      NAME   bench_sanity
      TARGET bench_sanity
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_scalability_comprehensive)
    themis_register_benchmark_ctest(
      NAME   bench_scalability_comprehensive
      TARGET bench_scalability_comprehensive
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_tpcc)
    themis_register_benchmark_ctest(
      NAME   bench_tpcc
      TARGET bench_tpcc
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_tpch)
    themis_register_benchmark_ctest(
      NAME   bench_tpch
      TARGET bench_tpch
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_whisper_transcription)
    themis_register_benchmark_ctest(
      NAME   bench_whisper_transcription
      TARGET bench_whisper_transcription
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_ycsb)
    themis_register_benchmark_ctest(
      NAME   bench_ycsb
      TARGET bench_ycsb
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  if(TARGET benchmark_distributed_hybrid_search)
    themis_register_benchmark_ctest(
      NAME   benchmark_distributed_hybrid_search
      TARGET benchmark_distributed_hybrid_search
      MODULE "base"
      TIMEOUT 120
    )
  endif()
  # Module: base_hot (1)
  if(TARGET bench_base_hot_paths)
    themis_register_benchmark_ctest(
      NAME   bench_base_hot_paths
      TARGET bench_base_hot_paths
      MODULE "base_hot"
      TIMEOUT 120
    )
  endif()
  # Module: base_wasm (1)
  if(TARGET bench_base_wasm_sandbox)
    themis_register_benchmark_ctest(
      NAME   bench_base_wasm_sandbox
      TARGET bench_base_wasm_sandbox
      MODULE "base_wasm"
      TIMEOUT 120
    )
  endif()
  # Module: batch_insert (1)
  if(TARGET bench_batch_insert)
    themis_register_benchmark_ctest(
      NAME   bench_batch_insert
      TARGET bench_batch_insert
      MODULE "batch_insert"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_huge (1)
  if(TARGET benchmark_huge_pages)
    themis_register_benchmark_ctest(
      NAME   benchmark_huge_pages
      TARGET benchmark_huge_pages
      MODULE "benchmark_huge"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_hybrid (1)
  if(TARGET benchmark_hybrid_search)
    themis_register_benchmark_ctest(
      NAME   benchmark_hybrid_search
      TARGET benchmark_hybrid_search
      MODULE "benchmark_hybrid"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_image (1)
  if(TARGET benchmark_image_analysis)
    themis_register_benchmark_ctest(
      NAME   benchmark_image_analysis
      TARGET benchmark_image_analysis
      MODULE "benchmark_image"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_lirs (1)
  if(TARGET benchmark_lirs_cache)
    themis_register_benchmark_ctest(
      NAME   benchmark_lirs_cache
      TARGET benchmark_lirs_cache
      MODULE "benchmark_lirs"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_mimalloc (1)
  if(TARGET benchmark_mimalloc)
    themis_register_benchmark_ctest(
      NAME   benchmark_mimalloc
      TARGET benchmark_mimalloc
      MODULE "benchmark_mimalloc"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_rcu (1)
  if(TARGET benchmark_rcu_index)
    themis_register_benchmark_ctest(
      NAME   benchmark_rcu_index
      TARGET benchmark_rcu_index
      MODULE "benchmark_rcu"
      TIMEOUT 120
    )
  endif()
  # Module: benchmark_safe (1)
  if(TARGET benchmark_safe_fail)
    themis_register_benchmark_ctest(
      NAME   benchmark_safe_fail
      TARGET benchmark_safe_fail
      MODULE "benchmark_safe"
      TIMEOUT 120
    )
  endif()
  # Module: binary_quantization (1)
  if(TARGET bench_binary_quantization)
    themis_register_benchmark_ctest(
      NAME   bench_binary_quantization
      TARGET bench_binary_quantization
      MODULE "binary_quantization"
      TIMEOUT 120
    )
  endif()
  # Module: blob_zstd (1)
  if(TARGET bench_blob_zstd)
    themis_register_benchmark_ctest(
      NAME   bench_blob_zstd
      TARGET bench_blob_zstd
      MODULE "blob_zstd"
      TIMEOUT 120
    )
  endif()
  # Module: branch_manager (1)
  if(TARGET bench_branch_manager)
    themis_register_benchmark_ctest(
      NAME   bench_branch_manager
      TARGET bench_branch_manager
      MODULE "branch_manager"
      TIMEOUT 120
    )
  endif()
  # Module: cache_eviction (1)
  if(TARGET bench_cache_eviction_coordinator)
    themis_register_benchmark_ctest(
      NAME   bench_cache_eviction_coordinator
      TARGET bench_cache_eviction_coordinator
      MODULE "cache_eviction"
      TIMEOUT 120
    )
  endif()
  # Module: cdc (2)
  if(TARGET bench_cdc_delivery_gates)
    themis_register_benchmark_ctest(
      NAME   bench_cdc_delivery_gates
      TARGET bench_cdc_delivery_gates
      MODULE "cdc"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_cdc_pipeline)
    themis_register_benchmark_ctest(
      NAME   bench_cdc_pipeline
      TARGET bench_cdc_pipeline
      MODULE "cdc"
      TIMEOUT 120
    )
  endif()
  # Module: chaos_release (1)
  if(TARGET bench_chaos_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_chaos_release_gates
      TARGET bench_chaos_release_gates
      MODULE "chaos_release"
      TIMEOUT 120
    )
  endif()
  # Module: chaos_stress (1)
  if(TARGET bench_chaos_stress)
    themis_register_benchmark_ctest(
      NAME   bench_chaos_stress
      TARGET bench_chaos_stress
      MODULE "chaos_stress"
      TIMEOUT 120
    )
  endif()
  # Module: chimera_adapter (1)
  if(TARGET bench_chimera_adapter)
    themis_register_benchmark_ctest(
      NAME   bench_chimera_adapter
      TARGET bench_chimera_adapter
      MODULE "chimera_adapter"
      TIMEOUT 120
    )
  endif()
  # Module: chimera_dedicated (1)
  if(TARGET bench_chimera_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_chimera_dedicated_gates
      TARGET bench_chimera_dedicated_gates
      MODULE "chimera_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: config_migration (1)
  if(TARGET bench_config_migration_scanner)
    themis_register_benchmark_ctest(
      NAME   bench_config_migration_scanner
      TARGET bench_config_migration_scanner
      MODULE "config_migration"
      TIMEOUT 120
    )
  endif()
  # Module: config_path (1)
  if(TARGET bench_config_path_resolver)
    themis_register_benchmark_ctest(
      NAME   bench_config_path_resolver
      TARGET bench_config_path_resolver
      MODULE "config_path"
      TIMEOUT 120
    )
  endif()
  # Module: config_release (1)
  if(TARGET bench_config_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_config_release_gates
      TARGET bench_config_release_gates
      MODULE "config_release"
      TIMEOUT 120
    )
  endif()
  # Module: content_processor (1)
  if(TARGET bench_content_processor_paths)
    themis_register_benchmark_ctest(
      NAME   bench_content_processor_paths
      TARGET bench_content_processor_paths
      MODULE "content_processor"
      TIMEOUT 120
    )
  endif()
  # Module: content_release (1)
  if(TARGET bench_content_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_content_release_gates
      TARGET bench_content_release_gates
      MODULE "content_release"
      TIMEOUT 120
    )
  endif()
  # Module: content_versioning (1)
  if(TARGET bench_content_versioning)
    themis_register_benchmark_ctest(
      NAME   bench_content_versioning
      TARGET bench_content_versioning
      MODULE "content_versioning"
      TIMEOUT 120
    )
  endif()
  # Module: core_performance (1)
  if(TARGET bench_core_performance)
    themis_register_benchmark_ctest(
      NAME   bench_core_performance
      TARGET bench_core_performance
      MODULE "core_performance"
      TIMEOUT 120
    )
  endif()
  # Module: cross_functional (1)
  if(TARGET bench_cross_functional_end_to_end)
    themis_register_benchmark_ctest(
      NAME   bench_cross_functional_end_to_end
      TARGET bench_cross_functional_end_to_end
      MODULE "cross_functional"
      TIMEOUT 120
    )
  endif()
  # Module: csv_export (1)
  if(TARGET bench_csv_export)
    themis_register_benchmark_ctest(
      NAME   bench_csv_export
      TARGET bench_csv_export
      MODULE "csv_export"
      TIMEOUT 120
    )
  endif()
  # Module: cuda_vs (1)
  if(TARGET bench_cuda_vs_cpu)
    themis_register_benchmark_ctest(
      NAME   bench_cuda_vs_cpu
      TARGET bench_cuda_vs_cpu
      MODULE "cuda_vs"
      TIMEOUT 120
    )
  endif()
  # Module: cycle_metrics (1)
  if(TARGET bench_cycle_metrics)
    themis_register_benchmark_ctest(
      NAME   bench_cycle_metrics
      TARGET bench_cycle_metrics
      MODULE "cycle_metrics"
      TIMEOUT 120
    )
  endif()
  # Module: data_transfer (1)
  if(TARGET bench_data_transfer)
    themis_register_benchmark_ctest(
      NAME   bench_data_transfer
      TARGET bench_data_transfer
      MODULE "data_transfer"
      TIMEOUT 120
    )
  endif()
  # Module: delegate_evaluator (1)
  if(TARGET bench_delegate_evaluator)
    themis_register_benchmark_ctest(
      NAME   bench_delegate_evaluator
      TARGET bench_delegate_evaluator
      MODULE "delegate_evaluator"
      TIMEOUT 120
    )
  endif()
  # Module: di_logging (1)
  if(TARGET bench_di_logging)
    themis_register_benchmark_ctest(
      NAME   bench_di_logging
      TARGET bench_di_logging
      MODULE "di_logging"
      TIMEOUT 120
    )
  endif()
  # Module: diff_engine (1)
  if(TARGET bench_diff_engine)
    themis_register_benchmark_ctest(
      NAME   bench_diff_engine
      TARGET bench_diff_engine
      MODULE "diff_engine"
      TIMEOUT 120
    )
  endif()
  # Module: distributed_merge (1)
  if(TARGET bench_distributed_merge_gates)
    themis_register_benchmark_ctest(
      NAME   bench_distributed_merge_gates
      TARGET bench_distributed_merge_gates
      MODULE "distributed_merge"
      TIMEOUT 120
    )
  endif()
  # Module: dk_release (1)
  if(TARGET bench_dk_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_dk_release_gates
      TARGET bench_dk_release_gates
      MODULE "dk_release"
      TIMEOUT 120
    )
  endif()
  # Module: docker_raid (1)
  if(TARGET bench_docker_raid_comprehensive)
    themis_register_benchmark_ctest(
      NAME   bench_docker_raid_comprehensive
      TARGET bench_docker_raid_comprehensive
      MODULE "docker_raid"
      TIMEOUT 120
    )
  endif()
  # Module: document_diff (1)
  if(TARGET bench_document_diff_merge)
    themis_register_benchmark_ctest(
      NAME   bench_document_diff_merge
      TARGET bench_document_diff_merge
      MODULE "document_diff"
      TIMEOUT 120
    )
  endif()
  # Module: document_round (1)
  if(TARGET bench_document_round_trip)
    themis_register_benchmark_ctest(
      NAME   bench_document_round_trip
      TARGET bench_document_round_trip
      MODULE "document_round"
      TIMEOUT 120
    )
  endif()
  # Module: document_store (1)
  if(TARGET bench_document_store)
    themis_register_benchmark_ctest(
      NAME   bench_document_store
      TARGET bench_document_store
      MODULE "document_store"
      TIMEOUT 120
    )
  endif()
  # Module: edge_cases (1)
  if(TARGET bench_edge_cases_comprehensive)
    themis_register_benchmark_ctest(
      NAME   bench_edge_cases_comprehensive
      TARGET bench_edge_cases_comprehensive
      MODULE "edge_cases"
      TIMEOUT 120
    )
  endif()
  # Module: embedding_cache (1)
  if(TARGET bench_embedding_cache_performance)
    themis_register_benchmark_ctest(
      NAME   bench_embedding_cache_performance
      TARGET bench_embedding_cache_performance
      MODULE "embedding_cache"
      TIMEOUT 120
    )
  endif()
  # Module: ethics_ai (2)
  if(TARGET bench_ethics_ai_ldm)
    themis_register_benchmark_ctest(
      NAME   bench_ethics_ai_ldm
      TARGET bench_ethics_ai_ldm
      MODULE "ethics_ai"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_ethics_ai_plugin)
    themis_register_benchmark_ctest(
      NAME   bench_ethics_ai_plugin
      TARGET bench_ethics_ai_plugin
      MODULE "ethics_ai"
      TIMEOUT 120
    )
  endif()
  # Module: evaluation_dedicated (1)
  if(TARGET bench_evaluation_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_evaluation_dedicated_gates
      TARGET bench_evaluation_dedicated_gates
      MODULE "evaluation_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: execution_dedicated (1)
  if(TARGET bench_execution_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_execution_dedicated_gates
      TARGET bench_execution_dedicated_gates
      MODULE "execution_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: exporters_dedicated (1)
  if(TARGET bench_exporters_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_exporters_dedicated_gates
      TARGET bench_exporters_dedicated_gates
      MODULE "exporters_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: exporters_release (1)
  if(TARGET bench_exporters_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_exporters_release_gates
      TARGET bench_exporters_release_gates
      MODULE "exporters_release"
      TIMEOUT 120
    )
  endif()
  # Module: extended_context (1)
  if(TARGET bench_extended_context)
    themis_register_benchmark_ctest(
      NAME   bench_extended_context
      TARGET bench_extended_context
      MODULE "extended_context"
      TIMEOUT 120
    )
  endif()
  # Module: failover_phase2 (1)
  if(TARGET bench_failover_phase2_phase3_gates)
    themis_register_benchmark_ctest(
      NAME   bench_failover_phase2_phase3_gates
      TARGET bench_failover_phase2_phase3_gates
      MODULE "failover_phase2"
      TIMEOUT 120
    )
  endif()
  # Module: failover_release (1)
  if(TARGET bench_failover_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_failover_release_gates
      TARGET bench_failover_release_gates
      MODULE "failover_release"
      TIMEOUT 120
    )
  endif()
  # Module: failover_wave (1)
  if(TARGET bench_failover_wave_b_gates)
    themis_register_benchmark_ctest(
      NAME   bench_failover_wave_b_gates
      TARGET bench_failover_wave_b_gates
      MODULE "failover_wave"
      TIMEOUT 120
    )
  endif()
  # Module: flash_attention (1)
  if(TARGET bench_flash_attention)
    themis_register_benchmark_ctest(
      NAME   bench_flash_attention
      TARGET bench_flash_attention
      MODULE "flash_attention"
      TIMEOUT 120
    )
  endif()
  # Module: fts_phase (1)
  if(TARGET bench_fts_phase_b)
    themis_register_benchmark_ctest(
      NAME   bench_fts_phase_b
      TARGET bench_fts_phase_b
      MODULE "fts_phase"
      TIMEOUT 120
    )
  endif()
  # Module: fused_kernels (1)
  if(TARGET bench_fused_kernels)
    themis_register_benchmark_ctest(
      NAME   bench_fused_kernels
      TARGET bench_fused_kernels
      MODULE "fused_kernels"
      TIMEOUT 120
    )
  endif()
  # Module: fused_lora (1)
  if(TARGET bench_fused_lora_kernels)
    themis_register_benchmark_ctest(
      NAME   bench_fused_lora_kernels
      TARGET bench_fused_lora_kernels
      MODULE "fused_lora"
      TIMEOUT 120
    )
  endif()
  # Module: geo_cuda (1)
  if(TARGET bench_geo_cuda_kernel_gating)
    themis_register_benchmark_ctest(
      NAME   bench_geo_cuda_kernel_gating
      TARGET bench_geo_cuda_kernel_gating
      MODULE "geo_cuda"
      TIMEOUT 120
    )
  endif()
  # Module: geo_dbscan (1)
  if(TARGET bench_geo_dbscan)
    themis_register_benchmark_ctest(
      NAME   bench_geo_dbscan
      TARGET bench_geo_dbscan
      MODULE "geo_dbscan"
      TIMEOUT 120
    )
  endif()
  # Module: geo_placement (1)
  if(TARGET bench_geo_placement_wal_shipping)
    themis_register_benchmark_ctest(
      NAME   bench_geo_placement_wal_shipping
      TARGET bench_geo_placement_wal_shipping
      MODULE "geo_placement"
      TIMEOUT 120
    )
  endif()
  # Module: geo_release (1)
  if(TARGET bench_geo_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_geo_release_gates
      TARGET bench_geo_release_gates
      MODULE "geo_release"
      TIMEOUT 120
    )
  endif()
  # Module: geojson_parse (1)
  if(TARGET bench_geojson_parse)
    themis_register_benchmark_ctest(
      NAME   bench_geojson_parse
      TARGET bench_geojson_parse
      MODULE "geojson_parse"
      TIMEOUT 120
    )
  endif()
  # Module: geospatial_phase2 (1)
  if(TARGET bench_geospatial_phase2)
    themis_register_benchmark_ctest(
      NAME   bench_geospatial_phase2
      TARGET bench_geospatial_phase2
      MODULE "geospatial_phase2"
      TIMEOUT 120
    )
  endif()
  # Module: gnn_embeddings (1)
  if(TARGET bench_gnn_embeddings)
    themis_register_benchmark_ctest(
      NAME   bench_gnn_embeddings
      TARGET bench_gnn_embeddings
      MODULE "gnn_embeddings"
      TIMEOUT 120
    )
  endif()
  # Module: gorilla_codec (1)
  if(TARGET bench_gorilla_codec)
    themis_register_benchmark_ctest(
      NAME   bench_gorilla_codec
      TARGET bench_gorilla_codec
      MODULE "gorilla_codec"
      TIMEOUT 120
    )
  endif()
  # Module: gossip_config (1)
  if(TARGET bench_gossip_config)
    themis_register_benchmark_ctest(
      NAME   bench_gossip_config
      TARGET bench_gossip_config
      MODULE "gossip_config"
      TIMEOUT 120
    )
  endif()
  # Module: governance_policy (1)
  if(TARGET bench_governance_policy_latency)
    themis_register_benchmark_ctest(
      NAME   bench_governance_policy_latency
      TARGET bench_governance_policy_latency
      MODULE "governance_policy"
      TIMEOUT 120
    )
  endif()
  # Module: governance_release (1)
  if(TARGET bench_governance_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_governance_release_gates
      TARGET bench_governance_release_gates
      MODULE "governance_release"
      TIMEOUT 120
    )
  endif()
  # Module: gpu (20)
  if(TARGET bench_ann_cpu_gpu_dispatch)
    themis_register_benchmark_ctest(
      NAME   bench_ann_cpu_gpu_dispatch
      TARGET bench_ann_cpu_gpu_dispatch
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_category_b_gpu_cpu_parity)
    themis_register_benchmark_ctest(
      NAME   bench_category_b_gpu_cpu_parity
      TARGET bench_category_b_gpu_cpu_parity
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_geo_cpu_gpu)
    themis_register_benchmark_ctest(
      NAME   bench_geo_cpu_gpu
      TARGET bench_geo_cpu_gpu
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_a8_baselines)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_a8_baselines
      TARGET bench_gpu_a8_baselines
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_backends)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_backends
      TARGET bench_gpu_backends
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_cpu_breakeven_category_a)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_cpu_breakeven_category_a
      TARGET bench_gpu_cpu_breakeven_category_a
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_cpu_breakeven_category_b)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_cpu_breakeven_category_b
      TARGET bench_gpu_cpu_breakeven_category_b
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_dedicated_gates
      TARGET bench_gpu_dedicated_gates
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_erasure)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_erasure
      TARGET bench_gpu_erasure
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_hardware_capability)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_hardware_capability
      TARGET bench_gpu_hardware_capability
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_module)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_module
      TARGET bench_gpu_module
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_phase2_phase3_gates)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_phase2_phase3_gates
      TARGET bench_gpu_phase2_phase3_gates
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_phase_c_gates)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_phase_c_gates
      TARGET bench_gpu_phase_c_gates
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_training_cycle)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_training_cycle
      TARGET bench_gpu_training_cycle
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_vector_index)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_vector_index
      TARGET bench_gpu_vector_index
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_gpu_vram_allocation)
    themis_register_benchmark_ctest(
      NAME   bench_gpu_vram_allocation
      TARGET bench_gpu_vram_allocation
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_lora_gpu)
    themis_register_benchmark_ctest(
      NAME   bench_lora_gpu
      TARGET bench_lora_gpu
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_multi_gpu_lora_advanced)
    themis_register_benchmark_ctest(
      NAME   bench_multi_gpu_lora_advanced
      TARGET bench_multi_gpu_lora_advanced
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_multi_gpu_scaling)
    themis_register_benchmark_ctest(
      NAME   bench_multi_gpu_scaling
      TARGET bench_multi_gpu_scaling
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_qlora_gpu_kernels)
    themis_register_benchmark_ctest(
      NAME   bench_qlora_gpu_kernels
      TARGET bench_qlora_gpu_kernels
      MODULE "gpu"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: graph (4)
  if(TARGET bench_graph_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_graph_dedicated_gates
      TARGET bench_graph_dedicated_gates
      MODULE "graph"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_graph_query_optimizer)
    themis_register_benchmark_ctest(
      NAME   bench_graph_query_optimizer
      TARGET bench_graph_query_optimizer
      MODULE "graph"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_graph_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_graph_release_gates
      TARGET bench_graph_release_gates
      MODULE "graph"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_graph_traversal)
    themis_register_benchmark_ctest(
      NAME   bench_graph_traversal
      TARGET bench_graph_traversal
      MODULE "graph"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: hnsw_prefilter (1)
  if(TARGET bench_hnsw_prefilter_minimal)
    themis_register_benchmark_ctest(
      NAME   bench_hnsw_prefilter_minimal
      TARGET bench_hnsw_prefilter_minimal
      MODULE "hnsw_prefilter"
      TIMEOUT 120
    )
  endif()
  # Module: hot_reload (1)
  if(TARGET bench_hot_reload_manager)
    themis_register_benchmark_ctest(
      NAME   bench_hot_reload_manager
      TARGET bench_hot_reload_manager
      MODULE "hot_reload"
      TIMEOUT 120
    )
  endif()
  # Module: hotspots_micro (1)
  if(TARGET bench_hotspots_micro)
    themis_register_benchmark_ctest(
      NAME   bench_hotspots_micro
      TARGET bench_hotspots_micro
      MODULE "hotspots_micro"
      TIMEOUT 120
    )
  endif()
  # Module: hsm_provider (1)
  if(TARGET bench_hsm_provider)
    themis_register_benchmark_ctest(
      NAME   bench_hsm_provider
      TARGET bench_hsm_provider
      MODULE "hsm_provider"
      TIMEOUT 120
    )
  endif()
  # Module: hybrid_aql (1)
  if(TARGET bench_hybrid_aql_sugar)
    themis_register_benchmark_ctest(
      NAME   bench_hybrid_aql_sugar
      TARGET bench_hybrid_aql_sugar
      MODULE "hybrid_aql"
      TIMEOUT 120
    )
  endif()
  # Module: hybrid_fusion (1)
  if(TARGET bench_hybrid_fusion_gates)
    themis_register_benchmark_ctest(
      NAME   bench_hybrid_fusion_gates
      TARGET bench_hybrid_fusion_gates
      MODULE "hybrid_fusion"
      TIMEOUT 120
    )
  endif()
  # Module: hybrid_vector (1)
  if(TARGET bench_hybrid_vector_geo)
    themis_register_benchmark_ctest(
      NAME   bench_hybrid_vector_geo
      TARGET bench_hybrid_vector_geo
      MODULE "hybrid_vector"
      TIMEOUT 120
    )
  endif()
  # Module: image_analysis (2)
  if(TARGET bench_image_analysis)
    themis_register_benchmark_ctest(
      NAME   bench_image_analysis
      TARGET bench_image_analysis
      MODULE "image_analysis"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_image_analysis_latency)
    themis_register_benchmark_ctest(
      NAME   bench_image_analysis_latency
      TARGET bench_image_analysis_latency
      MODULE "image_analysis"
      TIMEOUT 120
    )
  endif()
  # Module: importer_throughput (1)
  if(TARGET bench_importer_throughput)
    themis_register_benchmark_ctest(
      NAME   bench_importer_throughput
      TARGET bench_importer_throughput
      MODULE "importer_throughput"
      TIMEOUT 120
    )
  endif()
  # Module: importers_release (1)
  if(TARGET bench_importers_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_importers_release_gates
      TARGET bench_importers_release_gates
      MODULE "importers_release"
      TIMEOUT 120
    )
  endif()
  # Module: index_rebuild (1)
  if(TARGET bench_index_rebuild)
    themis_register_benchmark_ctest(
      NAME   bench_index_rebuild
      TARGET bench_index_rebuild
      MODULE "index_rebuild"
      TIMEOUT 120
    )
  endif()
  # Module: ingestion_dedicated (1)
  if(TARGET bench_ingestion_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_ingestion_dedicated_gates
      TARGET bench_ingestion_dedicated_gates
      MODULE "ingestion_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: ingestion_extraction (1)
  if(TARGET bench_ingestion_extraction)
    themis_register_benchmark_ctest(
      NAME   bench_ingestion_extraction
      TARGET bench_ingestion_extraction
      MODULE "ingestion_extraction"
      TIMEOUT 120
    )
  endif()
  # Module: ingestion_kv (1)
  if(TARGET bench_ingestion_kv)
    themis_register_benchmark_ctest(
      NAME   bench_ingestion_kv
      TARGET bench_ingestion_kv
      MODULE "ingestion_kv"
      TIMEOUT 120
    )
  endif()
  # Module: ingestion_quality (1)
  if(TARGET bench_ingestion_quality_judge)
    themis_register_benchmark_ctest(
      NAME   bench_ingestion_quality_judge
      TARGET bench_ingestion_quality_judge
      MODULE "ingestion_quality"
      TIMEOUT 120
    )
  endif()
  # Module: ingestion_release (1)
  if(TARGET bench_ingestion_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_ingestion_release_gates
      TARGET bench_ingestion_release_gates
      MODULE "ingestion_release"
      TIMEOUT 120
    )
  endif()
  # Module: insert_profiling (1)
  if(TARGET bench_insert_profiling)
    themis_register_benchmark_ctest(
      NAME   bench_insert_profiling
      TARGET bench_insert_profiling
      MODULE "insert_profiling"
      TIMEOUT 120
    )
  endif()
  # Module: interval_tree (1)
  if(TARGET bench_interval_tree_erase)
    themis_register_benchmark_ctest(
      NAME   bench_interval_tree_erase
      TARGET bench_interval_tree_erase
      MODULE "interval_tree"
      TIMEOUT 120
    )
  endif()
  # Module: kernel_block (1)
  if(TARGET kernel_block_size_bench)
    themis_register_benchmark_ctest(
      NAME   kernel_block_size_bench
      TARGET kernel_block_size_bench
      MODULE "kernel_block"
      TIMEOUT 120
    )
  endif()
  # Module: knowledge_gap (1)
  if(TARGET bench_knowledge_gap_detector_phase2)
    themis_register_benchmark_ctest(
      NAME   bench_knowledge_gap_detector_phase2
      TARGET bench_knowledge_gap_detector_phase2
      MODULE "knowledge_gap"
      TIMEOUT 120
    )
  endif()
  # Module: layered_retrieval (1)
  if(TARGET bench_layered_retrieval_phase5)
    themis_register_benchmark_ctest(
      NAME   bench_layered_retrieval_phase5
      TARGET bench_layered_retrieval_phase5
      MODULE "layered_retrieval"
      TIMEOUT 120
    )
  endif()
  # Module: learned_quantization (1)
  if(TARGET bench_learned_quantization)
    themis_register_benchmark_ctest(
      NAME   bench_learned_quantization
      TARGET bench_learned_quantization
      MODULE "learned_quantization"
      TIMEOUT 120
    )
  endif()
  # Module: legal_lora (1)
  if(TARGET bench_legal_lora_pipeline)
    themis_register_benchmark_ctest(
      NAME   bench_legal_lora_pipeline
      TARGET bench_legal_lora_pipeline
      MODULE "legal_lora"
      TIMEOUT 120
    )
  endif()
  # Module: llama_cpp (1)
  if(TARGET bench_llama_cpp_inference)
    themis_register_benchmark_ctest(
      NAME   bench_llama_cpp_inference
      TARGET bench_llama_cpp_inference
      MODULE "llama_cpp"
      TIMEOUT 120
    )
  endif()
  # Module: llm (11)
  if(TARGET bench_embedded_llm)
    themis_register_benchmark_ctest(
      NAME   bench_embedded_llm
      TARGET bench_embedded_llm
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_hotpaths)
    themis_register_benchmark_ctest(
      NAME   bench_llm_hotpaths
      TARGET bench_llm_hotpaths
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_inference_performance)
    themis_register_benchmark_ctest(
      NAME   bench_llm_inference_performance
      TARGET bench_llm_inference_performance
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_infrastructure)
    themis_register_benchmark_ctest(
      NAME   bench_llm_infrastructure
      TARGET bench_llm_infrastructure
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_judge_integration)
    themis_register_benchmark_ctest(
      NAME   bench_llm_judge_integration
      TARGET bench_llm_judge_integration
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_raid_pipeline)
    themis_register_benchmark_ctest(
      NAME   bench_llm_raid_pipeline
      TARGET bench_llm_raid_pipeline
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_real_models)
    themis_register_benchmark_ctest(
      NAME   bench_llm_real_models
      TARGET bench_llm_real_models
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_response_cache)
    themis_register_benchmark_ctest(
      NAME   bench_llm_response_cache
      TARGET bench_llm_response_cache
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_streaming_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_llm_streaming_dedicated_gates
      TARGET bench_llm_streaming_dedicated_gates
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_llm_wiki_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_llm_wiki_dedicated_gates
      TARGET bench_llm_wiki_dedicated_gates
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET llm_bench)
    themis_register_benchmark_ctest(
      NAME   llm_bench
      TARGET llm_bench
      MODULE "llm"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: locality_aware (1)
  if(TARGET bench_locality_aware_router)
    themis_register_benchmark_ctest(
      NAME   bench_locality_aware_router
      TARGET bench_locality_aware_router
      MODULE "locality_aware"
      TIMEOUT 120
    )
  endif()
  # Module: lock_contention (1)
  if(TARGET bench_lock_contention)
    themis_register_benchmark_ctest(
      NAME   bench_lock_contention
      TARGET bench_lock_contention
      MODULE "lock_contention"
      TIMEOUT 120
    )
  endif()
  # Module: lora_auto (1)
  if(TARGET bench_lora_auto_binding)
    themis_register_benchmark_ctest(
      NAME   bench_lora_auto_binding
      TARGET bench_lora_auto_binding
      MODULE "lora_auto"
      TIMEOUT 120
    )
  endif()
  # Module: lora_framework (1)
  if(TARGET bench_lora_framework)
    themis_register_benchmark_ctest(
      NAME   bench_lora_framework
      TARGET bench_lora_framework
      MODULE "lora_framework"
      TIMEOUT 120
    )
  endif()
  # Module: lora_inline (1)
  if(TARGET bench_lora_inline)
    themis_register_benchmark_ctest(
      NAME   bench_lora_inline
      TARGET bench_lora_inline
      MODULE "lora_inline"
      TIMEOUT 120
    )
  endif()
  # Module: lora_training (1)
  if(TARGET bench_lora_training)
    themis_register_benchmark_ctest(
      NAME   bench_lora_training
      TARGET bench_lora_training
      MODULE "lora_training"
      TIMEOUT 120
    )
  endif()
  # Module: lossy_vs (1)
  if(TARGET bench_lossy_vs_lossless)
    themis_register_benchmark_ctest(
      NAME   bench_lossy_vs_lossless
      TARGET bench_lossy_vs_lossless
      MODULE "lossy_vs"
      TIMEOUT 120
    )
  endif()
  # Module: maintenance_release (1)
  if(TARGET bench_maintenance_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_maintenance_release_gates
      TARGET bench_maintenance_release_gates
      MODULE "maintenance_release"
      TIMEOUT 120
    )
  endif()
  # Module: metadata_cache (1)
  if(TARGET bench_metadata_cache)
    themis_register_benchmark_ctest(
      NAME   bench_metadata_cache
      TARGET bench_metadata_cache
      MODULE "metadata_cache"
      TIMEOUT 120
    )
  endif()
  # Module: metadata_consistency (1)
  if(TARGET bench_metadata_consistency_lineage_gates)
    themis_register_benchmark_ctest(
      NAME   bench_metadata_consistency_lineage_gates
      TARGET bench_metadata_consistency_lineage_gates
      MODULE "metadata_consistency"
      TIMEOUT 120
    )
  endif()
  # Module: metadata_release (1)
  if(TARGET bench_metadata_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_metadata_release_gates
      TARGET bench_metadata_release_gates
      MODULE "metadata_release"
      TIMEOUT 120
    )
  endif()
  # Module: metrics_collector (1)
  if(TARGET bench_metrics_collector)
    themis_register_benchmark_ctest(
      NAME   bench_metrics_collector
      TARGET bench_metrics_collector
      MODULE "metrics_collector"
      TIMEOUT 120
    )
  endif()
  # Module: mixed_precision (1)
  if(TARGET bench_mixed_precision_perf)
    themis_register_benchmark_ctest(
      NAME   bench_mixed_precision_perf
      TARGET bench_mixed_precision_perf
      MODULE "mixed_precision"
      TIMEOUT 120
    )
  endif()
  # Module: module_load (1)
  if(TARGET bench_module_load_hot_reload)
    themis_register_benchmark_ctest(
      NAME   bench_module_load_hot_reload
      TARGET bench_module_load_hot_reload
      MODULE "module_load"
      TIMEOUT 120
    )
  endif()
  # Module: multi_lora (1)
  if(TARGET bench_multi_lora_fusion)
    themis_register_benchmark_ctest(
      NAME   bench_multi_lora_fusion
      TARGET bench_multi_lora_fusion
      MODULE "multi_lora"
      TIMEOUT 120
    )
  endif()
  # Module: multishard_exact (1)
  if(TARGET bench_multishard_exact)
    themis_register_benchmark_ctest(
      NAME   bench_multishard_exact
      TARGET bench_multishard_exact
      MODULE "multishard_exact"
      TIMEOUT 120
    )
  endif()
  # Module: multitask_lora (1)
  if(TARGET bench_multitask_lora_training)
    themis_register_benchmark_ctest(
      NAME   bench_multitask_lora_training
      TARGET bench_multitask_lora_training
      MODULE "multitask_lora"
      TIMEOUT 120
    )
  endif()
  # Module: network (3)
  if(TARGET bench_network_protocol_path)
    themis_register_benchmark_ctest(
      NAME   bench_network_protocol_path
      TARGET bench_network_protocol_path
      MODULE "network"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_network_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_network_release_gates
      TARGET bench_network_release_gates
      MODULE "network"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_network_routing_gates)
    themis_register_benchmark_ctest(
      NAME   bench_network_routing_gates
      TARGET bench_network_routing_gates
      MODULE "network"
      TIMEOUT 120
    )
  endif()
  # Module: observability_block (1)
  if(TARGET bench_observability_block_b_gates)
    themis_register_benchmark_ctest(
      NAME   bench_observability_block_b_gates
      TARGET bench_observability_block_b_gates
      MODULE "observability_block"
      TIMEOUT 120
    )
  endif()
  # Module: observability_goals (1)
  if(TARGET bench_observability_goals)
    themis_register_benchmark_ctest(
      NAME   bench_observability_goals
      TARGET bench_observability_goals
      MODULE "observability_goals"
      TIMEOUT 120
    )
  endif()
  # Module: observability_phase2 (1)
  if(TARGET bench_observability_phase2_exporter_stress)
    themis_register_benchmark_ctest(
      NAME   bench_observability_phase2_exporter_stress
      TARGET bench_observability_phase2_exporter_stress
      MODULE "observability_phase2"
      TIMEOUT 120
    )
  endif()
  # Module: olap_analytics (1)
  if(TARGET bench_olap_analytics)
    themis_register_benchmark_ctest(
      NAME   bench_olap_analytics
      TARGET bench_olap_analytics
      MODULE "olap_analytics"
      TIMEOUT 120
    )
  endif()
  # Module: olap_performance (1)
  if(TARGET bench_olap_performance)
    themis_register_benchmark_ctest(
      NAME   bench_olap_performance
      TARGET bench_olap_performance
      MODULE "olap_performance"
      TIMEOUT 120
    )
  endif()
  # Module: onnx_clip (2)
  if(TARGET bench_onnx_clip_cpu)
    themis_register_benchmark_ctest(
      NAME   bench_onnx_clip_cpu
      TARGET bench_onnx_clip_cpu
      MODULE "onnx_clip"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_onnx_clip_vit_backend)
    themis_register_benchmark_ctest(
      NAME   bench_onnx_clip_vit_backend
      TARGET bench_onnx_clip_vit_backend
      MODULE "onnx_clip"
      TIMEOUT 120
    )
  endif()
  # Module: p0_crud (1)
  if(TARGET bench_p0_crud_baseline)
    themis_register_benchmark_ctest(
      NAME   bench_p0_crud_baseline
      TARGET bench_p0_crud_baseline
      MODULE "p0_crud"
      TIMEOUT 120
    )
  endif()
  # Module: parquet_export (1)
  if(TARGET bench_parquet_export)
    themis_register_benchmark_ctest(
      NAME   bench_parquet_export
      TARGET bench_parquet_export
      MODULE "parquet_export"
      TIMEOUT 120
    )
  endif()
  # Module: performance_release (1)
  if(TARGET bench_performance_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_performance_release_gates
      TARGET bench_performance_release_gates
      MODULE "performance_release"
      TIMEOUT 120
    )
  endif()
  # Module: phase1_flash (1)
  if(TARGET bench_phase1_flash_attention)
    themis_register_benchmark_ctest(
      NAME   bench_phase1_flash_attention
      TARGET bench_phase1_flash_attention
      MODULE "phase1_flash"
      TIMEOUT 120
    )
  endif()
  # Module: phase3_optimization (1)
  if(TARGET bench_phase3_optimization)
    themis_register_benchmark_ctest(
      NAME   bench_phase3_optimization
      TARGET bench_phase3_optimization
      MODULE "phase3_optimization"
      TIMEOUT 120
    )
  endif()
  # Module: phase4_performance (1)
  if(TARGET bench_phase4_performance)
    themis_register_benchmark_ctest(
      NAME   bench_phase4_performance
      TARGET bench_phase4_performance
      MODULE "phase4_performance"
      TIMEOUT 120
    )
  endif()
  # Module: pii_stream (1)
  if(TARGET bench_pii_stream_scanner)
    themis_register_benchmark_ctest(
      NAME   bench_pii_stream_scanner
      TARGET bench_pii_stream_scanner
      MODULE "pii_stream"
      TIMEOUT 120
    )
  endif()
  # Module: plugin_hot (1)
  if(TARGET bench_plugin_hot_plug)
    themis_register_benchmark_ctest(
      NAME   bench_plugin_hot_plug
      TARGET bench_plugin_hot_plug
      MODULE "plugin_hot"
      TIMEOUT 120
    )
  endif()
  # Module: plugin_system (1)
  if(TARGET bench_plugin_system)
    themis_register_benchmark_ctest(
      NAME   bench_plugin_system
      TARGET bench_plugin_system
      MODULE "plugin_system"
      TIMEOUT 120
    )
  endif()
  # Module: plugins_release (1)
  if(TARGET bench_plugins_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_plugins_release_gates
      TARGET bench_plugins_release_gates
      MODULE "plugins_release"
      TIMEOUT 120
    )
  endif()
  # Module: policy_evaluation (1)
  if(TARGET bench_policy_evaluation)
    themis_register_benchmark_ctest(
      NAME   bench_policy_evaluation
      TARGET bench_policy_evaluation
      MODULE "policy_evaluation"
      TIMEOUT 120
    )
  endif()
  # Module: postgres_e2e (1)
  if(TARGET bench_postgres_e2e)
    themis_register_benchmark_ctest(
      NAME   bench_postgres_e2e
      TARGET bench_postgres_e2e
      MODULE "postgres_e2e"
      TIMEOUT 120
    )
  endif()
  # Module: postgres_protocol (1)
  if(TARGET bench_postgres_protocol)
    themis_register_benchmark_ctest(
      NAME   bench_postgres_protocol
      TARGET bench_postgres_protocol
      MODULE "postgres_protocol"
      TIMEOUT 120
    )
  endif()
  # Module: process_advanced (1)
  if(TARGET bench_process_advanced_workflows)
    themis_register_benchmark_ctest(
      NAME   bench_process_advanced_workflows
      TARGET bench_process_advanced_workflows
      MODULE "process_advanced"
      TIMEOUT 120
    )
  endif()
  # Module: process_concurrency (1)
  if(TARGET bench_process_concurrency_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_concurrency_gates
      TARGET bench_process_concurrency_gates
      MODULE "process_concurrency"
      TIMEOUT 120
    )
  endif()
  # Module: process_dedicated (1)
  if(TARGET bench_process_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_dedicated_gates
      TARGET bench_process_dedicated_gates
      MODULE "process_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: process_determinism (1)
  if(TARGET bench_process_determinism_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_determinism_gates
      TARGET bench_process_determinism_gates
      MODULE "process_determinism"
      TIMEOUT 120
    )
  endif()
  # Module: process_diagnostics (1)
  if(TARGET bench_process_diagnostics_overhead)
    themis_register_benchmark_ctest(
      NAME   bench_process_diagnostics_overhead
      TARGET bench_process_diagnostics_overhead
      MODULE "process_diagnostics"
      TIMEOUT 120
    )
  endif()
  # Module: process_import (1)
  if(TARGET bench_process_import_retrieval)
    themis_register_benchmark_ctest(
      NAME   bench_process_import_retrieval
      TARGET bench_process_import_retrieval
      MODULE "process_import"
      TIMEOUT 120
    )
  endif()
  # Module: process_linker (1)
  if(TARGET bench_process_linker_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_linker_gates
      TARGET bench_process_linker_gates
      MODULE "process_linker"
      TIMEOUT 120
    )
  endif()
  # Module: process_mining (1)
  if(TARGET bench_process_mining)
    themis_register_benchmark_ctest(
      NAME   bench_process_mining
      TARGET bench_process_mining
      MODULE "process_mining"
      TIMEOUT 120
    )
  endif()
  # Module: process_parser (1)
  if(TARGET bench_process_parser_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_parser_gates
      TARGET bench_process_parser_gates
      MODULE "process_parser"
      TIMEOUT 120
    )
  endif()
  # Module: process_release (1)
  if(TARGET bench_process_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_release_gates
      TARGET bench_process_release_gates
      MODULE "process_release"
      TIMEOUT 120
    )
  endif()
  # Module: process_retrieval (1)
  if(TARGET bench_process_retrieval)
    themis_register_benchmark_ctest(
      NAME   bench_process_retrieval
      TARGET bench_process_retrieval
      MODULE "process_retrieval"
      TIMEOUT 120
    )
  endif()
  # Module: process_retriever (1)
  if(TARGET bench_process_retriever_gates)
    themis_register_benchmark_ctest(
      NAME   bench_process_retriever_gates
      TARGET bench_process_retriever_gates
      MODULE "process_retriever"
      TIMEOUT 120
    )
  endif()
  # Module: product_quantization (1)
  if(TARGET bench_product_quantization)
    themis_register_benchmark_ctest(
      NAME   bench_product_quantization
      TARGET bench_product_quantization
      MODULE "product_quantization"
      TIMEOUT 120
    )
  endif()
  # Module: projects_release (1)
  if(TARGET bench_projects_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_projects_release_gates
      TARGET bench_projects_release_gates
      MODULE "projects_release"
      TIMEOUT 120
    )
  endif()
  # Module: prompt_engineering (1)
  if(TARGET bench_prompt_engineering)
    themis_register_benchmark_ctest(
      NAME   bench_prompt_engineering
      TARGET bench_prompt_engineering
      MODULE "prompt_engineering"
      TIMEOUT 120
    )
  endif()
  # Module: query (6)
  if(TARGET bench_adaptive_query_cache)
    themis_register_benchmark_ctest(
      NAME   bench_adaptive_query_cache
      TARGET bench_adaptive_query_cache
      MODULE "query"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_adaptive_query_compilation)
    themis_register_benchmark_ctest(
      NAME   bench_adaptive_query_compilation
      TARGET bench_adaptive_query_compilation
      MODULE "query"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_continuous_query)
    themis_register_benchmark_ctest(
      NAME   bench_continuous_query
      TARGET bench_continuous_query
      MODULE "query"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_query)
    themis_register_benchmark_ctest(
      NAME   bench_query
      TARGET bench_query
      MODULE "query"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_query_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_query_dedicated_gates
      TARGET bench_query_dedicated_gates
      MODULE "query"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_query_lazy_eval)
    themis_register_benchmark_ctest(
      NAME   bench_query_lazy_eval
      TARGET bench_query_lazy_eval
      MODULE "query"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: rag (3)
  if(TARGET bench_rag_ethics)
    themis_register_benchmark_ctest(
      NAME   bench_rag_ethics
      TARGET bench_rag_ethics
      MODULE "rag"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_rag_evaluation)
    themis_register_benchmark_ctest(
      NAME   bench_rag_evaluation
      TARGET bench_rag_evaluation
      MODULE "rag"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_rag_hybrid_retriever)
    themis_register_benchmark_ctest(
      NAME   bench_rag_hybrid_retriever
      TARGET bench_rag_hybrid_retriever
      MODULE "rag"
      TIMEOUT 120
    )
  endif()
  # Module: raid_lora (1)
  if(TARGET bench_raid_lora)
    themis_register_benchmark_ctest(
      NAME   bench_raid_lora
      TARGET bench_raid_lora
      MODULE "raid_lora"
      TIMEOUT 120
    )
  endif()
  # Module: random_access (1)
  if(TARGET bench_random_access_prefetch)
    themis_register_benchmark_ctest(
      NAME   bench_random_access_prefetch
      TARGET bench_random_access_prefetch
      MODULE "random_access"
      TIMEOUT 120
    )
  endif()
  # Module: replication (4)
  if(TARGET bench_replication_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_replication_dedicated_gates
      TARGET bench_replication_dedicated_gates
      MODULE "replication"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_replication_multi_dc_multi_writer)
    themis_register_benchmark_ctest(
      NAME   bench_replication_multi_dc_multi_writer
      TARGET bench_replication_multi_dc_multi_writer
      MODULE "replication"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_replication_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_replication_release_gates
      TARGET bench_replication_release_gates
      MODULE "replication"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_replication_throughput)
    themis_register_benchmark_ctest(
      NAME   bench_replication_throughput
      TARGET bench_replication_throughput
      MODULE "replication"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: retrieval_dedicated (1)
  if(TARGET bench_retrieval_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_retrieval_dedicated_gates
      TARGET bench_retrieval_dedicated_gates
      MODULE "retrieval_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: rewrite_engine (1)
  if(TARGET bench_rewrite_engine)
    themis_register_benchmark_ctest(
      NAME   bench_rewrite_engine
      TARGET bench_rewrite_engine
      MODULE "rewrite_engine"
      TIMEOUT 120
    )
  endif()
  # Module: rotary_embeddings (1)
  if(TARGET bench_rotary_embeddings)
    themis_register_benchmark_ctest(
      NAME   bench_rotary_embeddings
      TARGET bench_rotary_embeddings
      MODULE "rotary_embeddings"
      TIMEOUT 120
    )
  endif()
  # Module: rpc_grpc (2)
  if(TARGET bench_rpc_grpc_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_rpc_grpc_dedicated_gates
      TARGET bench_rpc_grpc_dedicated_gates
      MODULE "rpc_grpc"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_rpc_grpc_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_rpc_grpc_release_gates
      TARGET bench_rpc_grpc_release_gates
      MODULE "rpc_grpc"
      TIMEOUT 120
    )
  endif()
  # Module: saga_compensation (1)
  if(TARGET bench_saga_compensation)
    themis_register_benchmark_ctest(
      NAME   bench_saga_compensation
      TARGET bench_saga_compensation
      MODULE "saga_compensation"
      TIMEOUT 120
    )
  endif()
  # Module: sbom_verification (1)
  if(TARGET bench_sbom_verification_gates)
    themis_register_benchmark_ctest(
      NAME   bench_sbom_verification_gates
      TARGET bench_sbom_verification_gates
      MODULE "sbom_verification"
      TIMEOUT 120
    )
  endif()
  # Module: scheduler (3)
  if(TARGET bench_scheduler_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_scheduler_dedicated_gates
      TARGET bench_scheduler_dedicated_gates
      MODULE "scheduler"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_scheduler_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_scheduler_release_gates
      TARGET bench_scheduler_release_gates
      MODULE "scheduler"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_task_scheduler)
    themis_register_benchmark_ctest(
      NAME   bench_task_scheduler
      TARGET bench_task_scheduler
      MODULE "scheduler"
      TIMEOUT 120
    )
  endif()
  # Module: scraper_pipeline (1)
  if(TARGET bench_scraper_pipeline_depth)
    themis_register_benchmark_ctest(
      NAME   bench_scraper_pipeline_depth
      TARGET bench_scraper_pipeline_depth
      MODULE "scraper_pipeline"
      TIMEOUT 120
    )
  endif()
  # Module: scraper_release (1)
  if(TARGET bench_scraper_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_scraper_release_gates
      TARGET bench_scraper_release_gates
      MODULE "scraper_release"
      TIMEOUT 120
    )
  endif()
  # Module: search_dedicated (1)
  if(TARGET bench_search_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_search_dedicated_gates
      TARGET bench_search_dedicated_gates
      MODULE "search_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: search_release (1)
  if(TARGET bench_search_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_search_release_gates
      TARGET bench_search_release_gates
      MODULE "search_release"
      TIMEOUT 120
    )
  endif()
  # Module: security (5)
  if(TARGET bench_compliance_security_governance)
    themis_register_benchmark_ctest(
      NAME   bench_compliance_security_governance
      TARGET bench_compliance_security_governance
      MODULE "security"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_security)
    themis_register_benchmark_ctest(
      NAME   bench_security
      TARGET bench_security
      MODULE "security"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_security_phase2_crypto_gates)
    themis_register_benchmark_ctest(
      NAME   bench_security_phase2_crypto_gates
      TARGET bench_security_phase2_crypto_gates
      MODULE "security"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_security_phase3_policy_gates)
    themis_register_benchmark_ctest(
      NAME   bench_security_phase3_policy_gates
      TARGET bench_security_phase3_policy_gates
      MODULE "security"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_security_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_security_release_gates
      TARGET bench_security_release_gates
      MODULE "security"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: server (3)
  if(TARGET bench_server_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_server_dedicated_gates
      TARGET bench_server_dedicated_gates
      MODULE "server"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_server_hotpaths)
    themis_register_benchmark_ctest(
      NAME   bench_server_hotpaths
      TARGET bench_server_hotpaths
      MODULE "server"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_server_http3_gates)
    themis_register_benchmark_ctest(
      NAME   bench_server_http3_gates
      TARGET bench_server_http3_gates
      MODULE "server"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: shard_resource (1)
  if(TARGET bench_shard_resource_manager)
    themis_register_benchmark_ctest(
      NAME   bench_shard_resource_manager
      TARGET bench_shard_resource_manager
      MODULE "shard_resource"
      TIMEOUT 120
    )
  endif()
  # Module: shard_routing (1)
  if(TARGET bench_shard_routing)
    themis_register_benchmark_ctest(
      NAME   bench_shard_routing
      TARGET bench_shard_routing
      MODULE "shard_routing"
      TIMEOUT 120
    )
  endif()
  # Module: sharding (3)
  if(TARGET bench_sharding_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_sharding_dedicated_gates
      TARGET bench_sharding_dedicated_gates
      MODULE "sharding"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_sharding_performance)
    themis_register_benchmark_ctest(
      NAME   bench_sharding_performance
      TARGET bench_sharding_performance
      MODULE "sharding"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_sharding_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_sharding_release_gates
      TARGET bench_sharding_release_gates
      MODULE "sharding"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: simd_distance (1)
  if(TARGET bench_simd_distance)
    themis_register_benchmark_ctest(
      NAME   bench_simd_distance
      TARGET bench_simd_distance
      MODULE "simd_distance"
      TIMEOUT 120
    )
  endif()
  # Module: simple_insert (1)
  if(TARGET bench_simple_insert_test)
    themis_register_benchmark_ctest(
      NAME   bench_simple_insert_test
      TARGET bench_simple_insert_test
      MODULE "simple_insert"
      TIMEOUT 120
    )
  endif()
  # Module: snapshot_manager (1)
  if(TARGET bench_snapshot_manager)
    themis_register_benchmark_ctest(
      NAME   bench_snapshot_manager
      TARGET bench_snapshot_manager
      MODULE "snapshot_manager"
      TIMEOUT 120
    )
  endif()
  # Module: spatial_index (1)
  if(TARGET bench_spatial_index)
    themis_register_benchmark_ctest(
      NAME   bench_spatial_index
      TARGET bench_spatial_index
      MODULE "spatial_index"
      TIMEOUT 120
    )
  endif()
  # Module: spatial_join (1)
  if(TARGET bench_spatial_join)
    themis_register_benchmark_ctest(
      NAME   bench_spatial_join
      TARGET bench_spatial_join
      MODULE "spatial_join"
      TIMEOUT 120
    )
  endif()
  # Module: ssm_phase0 (1)
  if(TARGET bench_ssm_phase0_baseline)
    themis_register_benchmark_ctest(
      NAME   bench_ssm_phase0_baseline
      TARGET bench_ssm_phase0_baseline
      MODULE "ssm_phase0"
      TIMEOUT 120
    )
  endif()
  # Module: stable_diffusion (1)
  if(TARGET bench_stable_diffusion_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_stable_diffusion_release_gates
      TARGET bench_stable_diffusion_release_gates
      MODULE "stable_diffusion"
      TIMEOUT 120
    )
  endif()
  # Module: storage (6)
  if(TARGET bench_encrypted_storage_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_encrypted_storage_dedicated_gates
      TARGET bench_encrypted_storage_dedicated_gates
      MODULE "storage"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_storage_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_storage_dedicated_gates
      TARGET bench_storage_dedicated_gates
      MODULE "storage"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_storage_performance)
    themis_register_benchmark_ctest(
      NAME   bench_storage_performance
      TARGET bench_storage_performance
      MODULE "storage"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_storage_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_storage_release_gates
      TARGET bench_storage_release_gates
      MODULE "storage"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_user_storage_encrypted_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_user_storage_encrypted_release_gates
      TARGET bench_user_storage_encrypted_release_gates
      MODULE "storage"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_user_storage_mount_latency)
    themis_register_benchmark_ctest(
      NAME   bench_user_storage_mount_latency
      TARGET bench_user_storage_mount_latency
      MODULE "storage"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: stream_protocol (1)
  if(TARGET bench_stream_protocol)
    themis_register_benchmark_ctest(
      NAME   bench_stream_protocol
      TARGET bench_stream_protocol
      MODULE "stream_protocol"
      TIMEOUT 120
    )
  endif()
  # Module: streaming_window (1)
  if(TARGET bench_streaming_window)
    themis_register_benchmark_ctest(
      NAME   bench_streaming_window
      TARGET bench_streaming_window
      MODULE "streaming_window"
      TIMEOUT 120
    )
  endif()
  # Module: temporal_dedicated (1)
  if(TARGET bench_temporal_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_temporal_dedicated_gates
      TARGET bench_temporal_dedicated_gates
      MODULE "temporal_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: temporal_queries (1)
  if(TARGET bench_temporal_queries)
    themis_register_benchmark_ctest(
      NAME   bench_temporal_queries
      TARGET bench_temporal_queries
      MODULE "temporal_queries"
      TIMEOUT 120
    )
  endif()
  # Module: temporal_release (1)
  if(TARGET bench_temporal_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_temporal_release_gates
      TARGET bench_temporal_release_gates
      MODULE "temporal_release"
      TIMEOUT 120
    )
  endif()
  # Module: tensor (10)
  if(TARGET bench_distributed_tensor_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_distributed_tensor_dedicated_gates
      TARGET bench_distributed_tensor_dedicated_gates
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_compression_routing)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_compression_routing
      TARGET bench_tensor_compression_routing
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_cpu_gpu_dispatch)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_cpu_gpu_dispatch
      TARGET bench_tensor_cpu_gpu_dispatch
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_dedicated_gates
      TARGET bench_tensor_dedicated_gates
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_deduplication_manager)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_deduplication_manager
      TARGET bench_tensor_deduplication_manager
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_dynamic_update)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_dynamic_update
      TARGET bench_tensor_dynamic_update
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_fingerprint)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_fingerprint
      TARGET bench_tensor_fingerprint
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_fingerprint_graph)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_fingerprint_graph
      TARGET bench_tensor_fingerprint_graph
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_integration_baseline)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_integration_baseline
      TARGET bench_tensor_integration_baseline
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_tensor_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_tensor_release_gates
      TARGET bench_tensor_release_gates
      MODULE "tensor"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: text_extraction (1)
  if(TARGET bench_text_extraction)
    themis_register_benchmark_ctest(
      NAME   bench_text_extraction
      TARGET bench_text_extraction
      MODULE "text_extraction"
      TIMEOUT 120
    )
  endif()
  # Module: themis_core (1)
  if(TARGET bench_themis_core)
    themis_register_benchmark_ctest(
      NAME   bench_themis_core
      TARGET bench_themis_core
      MODULE "themis_core"
      TIMEOUT 120
    )
  endif()
  # Module: themis_dedicated (1)
  if(TARGET bench_themis_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_themis_dedicated_gates
      TARGET bench_themis_dedicated_gates
      MODULE "themis_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: themis_release (1)
  if(TARGET bench_themis_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_themis_release_gates
      TARGET bench_themis_release_gates
      MODULE "themis_release"
      TIMEOUT 120
    )
  endif()
  # Module: thread_pool (1)
  if(TARGET bench_thread_pool_saturation)
    themis_register_benchmark_ctest(
      NAME   bench_thread_pool_saturation
      TARGET bench_thread_pool_saturation
      MODULE "thread_pool"
      TIMEOUT 120
    )
  endif()
  # Module: timeseries_adaptive (1)
  if(TARGET bench_timeseries_adaptive_flush)
    themis_register_benchmark_ctest(
      NAME   bench_timeseries_adaptive_flush
      TARGET bench_timeseries_adaptive_flush
      MODULE "timeseries_adaptive"
      TIMEOUT 120
    )
  endif()
  # Module: timeseries_dedicated (1)
  if(TARGET bench_timeseries_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_timeseries_dedicated_gates
      TARGET bench_timeseries_dedicated_gates
      MODULE "timeseries_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: timeseries_ingestion (1)
  if(TARGET bench_timeseries_ingestion)
    themis_register_benchmark_ctest(
      NAME   bench_timeseries_ingestion
      TARGET bench_timeseries_ingestion
      MODULE "timeseries_ingestion"
      TIMEOUT 120
    )
  endif()
  # Module: timeseries_release (1)
  if(TARGET bench_timeseries_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_timeseries_release_gates
      TARGET bench_timeseries_release_gates
      MODULE "timeseries_release"
      TIMEOUT 120
    )
  endif()
  # Module: toolbox_dedicated (1)
  if(TARGET bench_toolbox_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_toolbox_dedicated_gates
      TARGET bench_toolbox_dedicated_gates
      MODULE "toolbox_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: toolbox_native (1)
  if(TARGET bench_toolbox_native_workloads)
    themis_register_benchmark_ctest(
      NAME   bench_toolbox_native_workloads
      TARGET bench_toolbox_native_workloads
      MODULE "toolbox_native"
      TIMEOUT 120
    )
  endif()
  # Module: toolbox_release (1)
  if(TARGET bench_toolbox_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_toolbox_release_gates
      TARGET bench_toolbox_release_gates
      MODULE "toolbox_release"
      TIMEOUT 120
    )
  endif()
  # Module: training_dedicated (1)
  if(TARGET bench_training_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_training_dedicated_gates
      TARGET bench_training_dedicated_gates
      MODULE "training_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: training_lifecycle (1)
  if(TARGET bench_training_lifecycle_gates)
    themis_register_benchmark_ctest(
      NAME   bench_training_lifecycle_gates
      TARGET bench_training_lifecycle_gates
      MODULE "training_lifecycle"
      TIMEOUT 120
    )
  endif()
  # Module: transaction (2)
  if(TARGET bench_transaction_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_transaction_dedicated_gates
      TARGET bench_transaction_dedicated_gates
      MODULE "transaction"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  if(TARGET bench_transaction_throughput)
    themis_register_benchmark_ctest(
      NAME   bench_transaction_throughput
      TARGET bench_transaction_throughput
      MODULE "transaction"
      TIMEOUT 120
    EXTRA_LABELS release_critical
    )
  endif()
  # Module: update_pipeline (1)
  if(TARGET bench_update_pipeline)
    themis_register_benchmark_ctest(
      NAME   bench_update_pipeline
      TARGET bench_update_pipeline
      MODULE "update_pipeline"
      TIMEOUT 120
    )
  endif()
  # Module: updates_canary (1)
  if(TARGET bench_updates_canary_resilience)
    themis_register_benchmark_ctest(
      NAME   bench_updates_canary_resilience
      TARGET bench_updates_canary_resilience
      MODULE "updates_canary"
      TIMEOUT 120
    )
  endif()
  # Module: updates_cluster (1)
  if(TARGET bench_updates_cluster_stress_q4)
    themis_register_benchmark_ctest(
      NAME   bench_updates_cluster_stress_q4
      TARGET bench_updates_cluster_stress_q4
      MODULE "updates_cluster"
      TIMEOUT 120
    )
  endif()
  # Module: updates_coordinated (1)
  if(TARGET bench_updates_coordinated_hardening)
    themis_register_benchmark_ctest(
      NAME   bench_updates_coordinated_hardening
      TARGET bench_updates_coordinated_hardening
      MODULE "updates_coordinated"
      TIMEOUT 120
    )
  endif()
  # Module: updates_long (1)
  if(TARGET bench_updates_long_run_stability)
    themis_register_benchmark_ctest(
      NAME   bench_updates_long_run_stability
      TARGET bench_updates_long_run_stability
      MODULE "updates_long"
      TIMEOUT 120
    )
  endif()
  # Module: updates_release (1)
  if(TARGET bench_updates_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_updates_release_gates
      TARGET bench_updates_release_gates
      MODULE "updates_release"
      TIMEOUT 120
    )
  endif()
  # Module: updates_schema (1)
  if(TARGET bench_updates_schema_diversity)
    themis_register_benchmark_ctest(
      NAME   bench_updates_schema_diversity
      TARGET bench_updates_schema_diversity
      MODULE "updates_schema"
      TIMEOUT 120
    )
  endif()
  # Module: utils_benchmark (1)
  if(TARGET bench_utils_benchmark_runner)
    themis_register_benchmark_ctest(
      NAME   bench_utils_benchmark_runner
      TARGET bench_utils_benchmark_runner
      MODULE "utils_benchmark"
      TIMEOUT 120
    )
  endif()
  # Module: utils_release (1)
  if(TARGET bench_utils_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_utils_release_gates
      TARGET bench_utils_release_gates
      MODULE "utils_release"
      TIMEOUT 120
    )
  endif()
  # Module: v1_3 (2)
  if(TARGET bench_v1_3_0_features)
    themis_register_benchmark_ctest(
      NAME   bench_v1_3_0_features
      TARGET bench_v1_3_0_features
      MODULE "v1_3"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_v1_3_4_optimizations)
    themis_register_benchmark_ctest(
      NAME   bench_v1_3_4_optimizations
      TARGET bench_v1_3_4_optimizations
      MODULE "v1_3"
      TIMEOUT 120
    )
  endif()
  # Module: vector_compression (1)
  if(TARGET bench_vector_compression_lossless)
    themis_register_benchmark_ctest(
      NAME   bench_vector_compression_lossless
      TARGET bench_vector_compression_lossless
      MODULE "vector_compression"
      TIMEOUT 120
    )
  endif()
  # Module: vector_prefilter (1)
  if(TARGET bench_vector_prefilter)
    themis_register_benchmark_ctest(
      NAME   bench_vector_prefilter
      TARGET bench_vector_prefilter
      MODULE "vector_prefilter"
      TIMEOUT 120
    )
  endif()
  # Module: vector_search (3)
  if(TARGET bench_vector_search)
    themis_register_benchmark_ctest(
      NAME   bench_vector_search
      TARGET bench_vector_search
      MODULE "vector_search"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_vector_search_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_vector_search_dedicated_gates
      TARGET bench_vector_search_dedicated_gates
      MODULE "vector_search"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_vector_search_gates)
    themis_register_benchmark_ctest(
      NAME   bench_vector_search_gates
      TARGET bench_vector_search_gates
      MODULE "vector_search"
      TIMEOUT 120
    )
  endif()
  # Module: video_processor (1)
  if(TARGET bench_video_processor)
    themis_register_benchmark_ctest(
      NAME   bench_video_processor
      TARGET bench_video_processor
      MODULE "video_processor"
      TIMEOUT 120
    )
  endif()
  # Module: voice_a8 (1)
  if(TARGET bench_voice_a8_baselines)
    themis_register_benchmark_ctest(
      NAME   bench_voice_a8_baselines
      TARGET bench_voice_a8_baselines
      MODULE "voice_a8"
      TIMEOUT 120
    )
  endif()
  # Module: voice_assistant (2)
  if(TARGET bench_voice_assistant)
    themis_register_benchmark_ctest(
      NAME   bench_voice_assistant
      TARGET bench_voice_assistant
      MODULE "voice_assistant"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_voice_assistant_latency)
    themis_register_benchmark_ctest(
      NAME   bench_voice_assistant_latency
      TARGET bench_voice_assistant_latency
      MODULE "voice_assistant"
      TIMEOUT 120
    )
  endif()
  # Module: voice_audio (1)
  if(TARGET bench_voice_audio_preprocessing)
    themis_register_benchmark_ctest(
      NAME   bench_voice_audio_preprocessing
      TARGET bench_voice_audio_preprocessing
      MODULE "voice_audio"
      TIMEOUT 120
    )
  endif()
  # Module: voice_dedicated (1)
  if(TARGET bench_voice_dedicated_gates)
    themis_register_benchmark_ctest(
      NAME   bench_voice_dedicated_gates
      TARGET bench_voice_dedicated_gates
      MODULE "voice_dedicated"
      TIMEOUT 120
    )
  endif()
  # Module: voice_session (1)
  if(TARGET bench_voice_session_lifecycle)
    themis_register_benchmark_ctest(
      NAME   bench_voice_session_lifecycle
      TARGET bench_voice_session_lifecycle
      MODULE "voice_session"
      TIMEOUT 120
    )
  endif()
  # Module: voice_streaming (1)
  if(TARGET bench_voice_streaming_throughput)
    themis_register_benchmark_ctest(
      NAME   bench_voice_streaming_throughput
      TARGET bench_voice_streaming_throughput
      MODULE "voice_streaming"
      TIMEOUT 120
    )
  endif()
  # Module: voice_stt (1)
  if(TARGET bench_voice_stt_tts_latency)
    themis_register_benchmark_ctest(
      NAME   bench_voice_stt_tts_latency
      TARGET bench_voice_stt_tts_latency
      MODULE "voice_stt"
      TIMEOUT 120
    )
  endif()
  # Module: voice_wake (1)
  if(TARGET bench_voice_wake_word_batch)
    themis_register_benchmark_ctest(
      NAME   bench_voice_wake_word_batch
      TARGET bench_voice_wake_word_batch
      MODULE "voice_wake"
      TIMEOUT 120
    )
  endif()
  # Module: vulkan_lora (1)
  if(TARGET bench_vulkan_lora)
    themis_register_benchmark_ctest(
      NAME   bench_vulkan_lora
      TARGET bench_vulkan_lora
      MODULE "vulkan_lora"
      TIMEOUT 120
    )
  endif()
  # Module: wal_apply (1)
  if(TARGET bench_wal_apply_grpc)
    themis_register_benchmark_ctest(
      NAME   bench_wal_apply_grpc
      TARGET bench_wal_apply_grpc
      MODULE "wal_apply"
      TIMEOUT 120
    )
  endif()
  # Module: wal_stress (1)
  if(TARGET bench_wal_stress)
    themis_register_benchmark_ctest(
      NAME   bench_wal_stress
      TARGET bench_wal_stress
      MODULE "wal_stress"
      TIMEOUT 120
    )
  endif()
  # Module: wavew (24)
  if(TARGET bench_w2a_mixed_path)
    themis_register_benchmark_ctest(
      NAME   bench_w2a_mixed_path
      TARGET bench_w2a_mixed_path
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w2b_concurrency)
    themis_register_benchmark_ctest(
      NAME   bench_w2b_concurrency
      TARGET bench_w2b_concurrency
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w2c_recovery)
    themis_register_benchmark_ctest(
      NAME   bench_w2c_recovery
      TARGET bench_w2c_recovery
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w4a_release_gates)
    themis_register_benchmark_ctest(
      NAME   bench_w4a_release_gates
      TARGET bench_w4a_release_gates
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w4b_resilience)
    themis_register_benchmark_ctest(
      NAME   bench_w4b_resilience
      TARGET bench_w4b_resilience
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w4c_determinism)
    themis_register_benchmark_ctest(
      NAME   bench_w4c_determinism
      TARGET bench_w4c_determinism
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w4d_diagnostics)
    themis_register_benchmark_ctest(
      NAME   bench_w4d_diagnostics
      TARGET bench_w4d_diagnostics
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w5a_production_workloads)
    themis_register_benchmark_ctest(
      NAME   bench_w5a_production_workloads
      TARGET bench_w5a_production_workloads
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w5b_degradation)
    themis_register_benchmark_ctest(
      NAME   bench_w5b_degradation
      TARGET bench_w5b_degradation
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w5c_determinism)
    themis_register_benchmark_ctest(
      NAME   bench_w5c_determinism
      TARGET bench_w5c_determinism
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w5d_governance)
    themis_register_benchmark_ctest(
      NAME   bench_w5d_governance
      TARGET bench_w5d_governance
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w7_p2d05_compression_state_store)
    themis_register_benchmark_ctest(
      NAME   bench_w7_p2d05_compression_state_store
      TARGET bench_w7_p2d05_compression_state_store
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w7a_release_critical_signoff)
    themis_register_benchmark_ctest(
      NAME   bench_w7a_release_critical_signoff
      TARGET bench_w7a_release_critical_signoff
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w7b_endurance_soak)
    themis_register_benchmark_ctest(
      NAME   bench_w7b_endurance_soak
      TARGET bench_w7b_endurance_soak
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w7c_degradation_fault_recovery)
    themis_register_benchmark_ctest(
      NAME   bench_w7c_degradation_fault_recovery
      TARGET bench_w7c_degradation_fault_recovery
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w7d_guardrails_variance_operability)
    themis_register_benchmark_ctest(
      NAME   bench_w7d_guardrails_variance_operability
      TARGET bench_w7d_guardrails_variance_operability
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w8a_incident_regression_shielding)
    themis_register_benchmark_ctest(
      NAME   bench_w8a_incident_regression_shielding
      TARGET bench_w8a_incident_regression_shielding
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w8b_threshold_hardening_drift_detection)
    themis_register_benchmark_ctest(
      NAME   bench_w8b_threshold_hardening_drift_detection
      TARGET bench_w8b_threshold_hardening_drift_detection
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w8c_deterministic_ci_harness)
    themis_register_benchmark_ctest(
      NAME   bench_w8c_deterministic_ci_harness
      TARGET bench_w8c_deterministic_ci_harness
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w8d_operability_runbooks_ownership)
    themis_register_benchmark_ctest(
      NAME   bench_w8d_operability_runbooks_ownership
      TARGET bench_w8d_operability_runbooks_ownership
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w9a_security_overhead_audit)
    themis_register_benchmark_ctest(
      NAME   bench_w9a_security_overhead_audit
      TARGET bench_w9a_security_overhead_audit
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w9b_sla_measurement_compliance)
    themis_register_benchmark_ctest(
      NAME   bench_w9b_sla_measurement_compliance
      TARGET bench_w9b_sla_measurement_compliance
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w9c_chaos_fault_recovery)
    themis_register_benchmark_ctest(
      NAME   bench_w9c_chaos_fault_recovery
      TARGET bench_w9c_chaos_fault_recovery
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  if(TARGET bench_w9d_multi_tenant_isolation)
    themis_register_benchmark_ctest(
      NAME   bench_w9d_multi_tenant_isolation
      TARGET bench_w9d_multi_tenant_isolation
      MODULE "wavew"
      TIMEOUT 120
    )
  endif()
  # Module: wikipedia_throughput (1)
  if(TARGET bench_wikipedia_throughput)
    themis_register_benchmark_ctest(
      NAME   bench_wikipedia_throughput
      TARGET bench_wikipedia_throughput
      MODULE "wikipedia_throughput"
      TIMEOUT 120
    )
  endif()

endif()
