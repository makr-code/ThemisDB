# ThemisDB Benchmarks CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building benchmarks
if(NOT THEMIS_BUILD_BENCHMARKS)
    message(STATUS "Benchmarks disabled (THEMIS_BUILD_BENCHMARKS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB benchmark suite")

# Find Google Benchmark
find_package(benchmark QUIET CONFIG)
if(NOT benchmark_FOUND)
    message(WARNING "Google Benchmark not found - benchmarks will not be built")
    message(WARNING "Install benchmark or use vcpkg: vcpkg install benchmark")
    return()
endif()

message(STATUS "Google Benchmark found - building benchmarks")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Common benchmark libraries
set(BENCHMARK_LIBS
    benchmark::benchmark
    benchmark::benchmark_main
)

# ============================================================================
# LoRA Framework Benchmarks (NEW)
# ============================================================================

# Check for LoRA framework benchmark file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_lora_framework.cpp")
    message(STATUS "Adding LoRA framework benchmarks")
    
    add_executable(bench_lora_framework
        bench_lora_framework.cpp
    )
    
    target_link_libraries(bench_lora_framework PRIVATE
        ${BENCHMARK_LIBS}
        # Add ThemisDB core libraries when available
        # themis_core
        # themis_llm
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_lora_framework PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    # Install benchmark
    install(TARGETS bench_lora_framework
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
else()
    message(STATUS "LoRA framework benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_lora_framework.cpp")
endif()

# Multi-LoRA Fusion Benchmarks (v1.5.0)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_multi_lora_fusion.cpp")
    message(STATUS "Adding Multi-LoRA fusion benchmarks")
    
    add_executable(bench_multi_lora_fusion
        bench_multi_lora_fusion.cpp
    )
    
    target_link_libraries(bench_multi_lora_fusion PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_multi_lora_fusion PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_lora_auto_binding.cpp")
    message(STATUS "Adding LoRA auto-binding benchmarks")
    
    add_executable(bench_lora_auto_binding
        bench_lora_auto_binding.cpp
    )
    
    target_link_libraries(bench_lora_auto_binding PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_lora_auto_binding PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_fused_kernels.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Fused Kernels benchmarks (CUDA/HIP)")
    
    add_executable(bench_fused_kernels
        bench_fused_kernels.cpp
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
    )
    
    # Optimization flags for accurate benchmarking
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_fused_kernels PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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
        message(STATUS "Fused kernels benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_fused_kernels.cpp")
    endif()
endif()

# Fused LoRA Kernels Benchmarks (Issue #36)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_fused_lora_kernels.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Fused LoRA Kernels benchmarks (CUDA/HIP)")
    
    add_executable(bench_fused_lora_kernels
        bench_fused_lora_kernels.cpp
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
    )
    
    # Optimization flags for accurate benchmarking
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_fused_lora_kernels PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_snapshot_manager.cpp")
    message(STATUS "Adding Snapshot Manager benchmarks")
    
    add_executable(bench_snapshot_manager
        bench_snapshot_manager.cpp
    )
    
    target_link_libraries(bench_snapshot_manager PRIVATE
        ${BENCHMARK_LIBS}
        # Add ThemisDB core libraries when available
        # themis_core
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_snapshot_manager PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_gpu_training_cycle.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding GPU Training Cycle benchmarks")
    
    add_executable(bench_gpu_training_cycle
        bench_gpu_training_cycle.cpp
    )
    
    target_link_libraries(bench_gpu_training_cycle PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_gpu_training_cycle PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_gpu_training_cycle PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_gpu_training_cycle
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  GPU Training Cycle: forward + backward + optimizer steps")
endif()

# 2. Multi-GPU Scaling Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_multi_gpu_scaling.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Multi-GPU Scaling benchmarks")
    
    add_executable(bench_multi_gpu_scaling
        bench_multi_gpu_scaling.cpp
    )
    
    target_link_libraries(bench_multi_gpu_scaling PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_multi_gpu_scaling PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_multi_gpu_scaling PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_multi_gpu_scaling
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Multi-GPU: data parallelism scaling efficiency")
endif()

# 2.1 Multi-GPU LoRA Advanced Benchmarks (v1.5.0)
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_multi_gpu_lora_advanced.cpp")
    message(STATUS "Adding Multi-GPU LoRA Advanced benchmarks")
    
    add_executable(bench_multi_gpu_lora_advanced
        bench_multi_gpu_lora_advanced.cpp
    )
    
    target_link_libraries(bench_multi_gpu_lora_advanced PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_multi_gpu_lora_advanced PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_multi_gpu_lora_advanced PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_multi_gpu_lora_advanced
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Multi-GPU LoRA: resource-aware eviction, dynamic scheduling, migration")
else()
    message(STATUS "Multi-GPU LoRA Advanced benchmarks skipped (LLM disabled or file missing)")
endif()

# 3. Mixed Precision Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_mixed_precision_perf.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Mixed Precision Performance benchmarks")
    
    add_executable(bench_mixed_precision_perf
        bench_mixed_precision_perf.cpp
    )
    
    target_link_libraries(bench_mixed_precision_perf PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_mixed_precision_perf PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_mixed_precision_perf PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_mixed_precision_perf
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Mixed Precision: FP32 vs FP16 throughput comparison")
endif()

# 4. Data Transfer Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_data_transfer.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
    message(STATUS "Adding Data Transfer benchmarks")
    
    add_executable(bench_data_transfer
        bench_data_transfer.cpp
    )
    
    target_link_libraries(bench_data_transfer PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_data_transfer PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_data_transfer PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_data_transfer
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Data Transfer: CPU↔GPU bandwidth and async prefetching")
endif()

# 5. Backend Comparison Benchmarks
if(THEMIS_ENABLE_GPU AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_backend_comparison.cpp")
    message(STATUS "Adding Backend Comparison benchmarks")
    
    add_executable(bench_backend_comparison
        bench_backend_comparison.cpp
    )
    
    target_link_libraries(bench_backend_comparison PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(bench_backend_comparison PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_backend_comparison PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_backend_comparison
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Backend Comparison: CUDA vs HIP vs Vulkan vs CPU")
else()
    message(STATUS "Backend Comparison benchmarks skipped (GPU disabled or file missing)")
endif()

# ============================================================================
# Existing Benchmarks
# ============================================================================

# Add existing benchmark subdirectories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/performance_optimizations")
    add_subdirectory(performance_optimizations)
endif()

# ============================================================================
# RAG Ethics Benchmarks
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_ethics.cpp")
    message(STATUS "Adding RAG Ethics benchmarks")
    
    add_executable(bench_rag_ethics
        bench_rag_ethics.cpp
    )
    
    target_link_libraries(bench_rag_ethics PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
    )
    
    target_compile_definitions(bench_rag_ethics PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_rag_ethics PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_knowledge_gap_detector_phase2.cpp")
    message(STATUS "Adding Knowledge Gap Detector Phase 2 benchmarks")
    
    add_executable(bench_knowledge_gap_detector_phase2
        bench_knowledge_gap_detector_phase2.cpp
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_knowledge_gap_detector_phase2 PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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
# Advanced Patterns Benchmarks (Sharding Strategy)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_advanced_patterns.cpp")
    message(STATUS "Adding Advanced Patterns benchmarks")
    
    add_executable(bench_advanced_patterns
        bench_advanced_patterns.cpp
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_advanced_patterns PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_distributed_coordinator.cpp")
    message(STATUS "Adding Distributed Coordinator benchmarks")
    
    add_executable(bench_distributed_coordinator
        bench_distributed_coordinator.cpp
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_distributed_coordinator PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()
    
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
# Real Performance Benchmarks (Issue: Implement Real Google Benchmark Tests)
# ============================================================================

# Storage Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_storage_performance.cpp")
    message(STATUS "Adding Storage Performance benchmarks")
    
    add_executable(bench_storage_performance
        bench_storage_performance.cpp
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
    
    # Optimization flags for benchmarks (configurable via BENCHMARK_ARCH_FLAGS)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            # Default: Use -march=native for maximum performance on build machine
            # Override with -DBENCHMARK_ARCH_FLAGS="-march=x86-64" for portability
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_storage_performance PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_storage_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Storage: mimalloc, huge pages, RCU index")
endif()

# Random Access with Prefetch Benchmarks (v1.4.1)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_random_access_prefetch.cpp")
    message(STATUS "Adding Random Access Prefetch benchmarks")
    
    add_executable(bench_random_access_prefetch
        bench_random_access_prefetch.cpp
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_random_access_prefetch PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_random_access_prefetch
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Random Access: CPU prefetch hints, data locality")
endif()

# OLAP Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_olap_performance.cpp")
    message(STATUS "Adding OLAP Performance benchmarks")
    
    add_executable(bench_olap_performance
        bench_olap_performance.cpp
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
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_olap_performance PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_olap_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  OLAP: Aggregations, GROUP BY, filters at scale")
endif()

# Embedding Cache Performance Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_embedding_cache_performance.cpp")
    message(STATUS "Adding Embedding Cache Performance benchmarks")
    
    add_executable(bench_embedding_cache_performance
        bench_embedding_cache_performance.cpp
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
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_embedding_cache_performance PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_embedding_cache_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Embedding Cache: HNSW, hit/miss, batch operations")
endif()

# LLM Inference Performance Benchmarks
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_llm_inference_performance.cpp")
    message(STATUS "Adding LLM Inference Performance benchmarks")
    
    add_executable(bench_llm_inference_performance
        bench_llm_inference_performance.cpp
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
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_llm_inference_performance PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_llm_inference_performance
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  LLM: Inference, LoRA adapters, throughput")
endif()

# ============================================================================
# Rotary Embeddings Benchmarks (NEW)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_rotary_embeddings.cpp")
    message(STATUS "Adding Rotary Embeddings benchmarks")
    
    add_executable(bench_rotary_embeddings
        bench_rotary_embeddings.cpp
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
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_rotary_embeddings PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
    install(TARGETS bench_rotary_embeddings
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    
    message(STATUS "  Rotary Embeddings: Single rotation, batch, relational, VectorIndex integration")
endif()

# ============================================================================
# GPU VRAM Allocation Benchmarks (NEW - vLLM-inspired)
# ============================================================================

if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_gpu_vram_allocation.cpp")
    message(STATUS "Adding GPU VRAM Allocation benchmarks")
    
    add_executable(bench_gpu_vram_allocation
        bench_gpu_vram_allocation.cpp
    )
    
    target_link_libraries(bench_gpu_vram_allocation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(bench_gpu_vram_allocation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_gpu_vram_allocation PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()
    
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
message(STATUS "  - Run LoRA benchmarks with: ./benchmarks/bench_lora_auto_binding")
message(STATUS "  - Or use: make run_benchmark_lora_auto_binding")
message(STATUS "  - Build all benchmarks with: cmake --build . --target bench_lora_framework")
message(STATUS "  - Run all benchmarks with: ./benchmarks/bench_lora_framework")
message(STATUS "  - Or use: make run_benchmarks")
message(STATUS "  - Performance benchmarks: bench_storage_performance, bench_olap_performance,")
message(STATUS "    bench_embedding_cache_performance, bench_llm_inference_performance, bench_rotary_embeddings, bench_gpu_vram_allocation")
