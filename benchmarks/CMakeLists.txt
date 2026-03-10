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
    ${CMAKE_BINARY_DIR}/proto_generated
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
    
    # Optimization flags for benchmarks
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(MSVC)
            target_compile_options(bench_lora_framework PRIVATE
                /O2
                /DNDEBUG
            )
        else()
            target_compile_options(bench_lora_framework PRIVATE
                -O3
                -march=native
                -DNDEBUG
            )
        endif()
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
if(THEMIS_ENABLE_LLM AND THEMIS_ENABLE_GPU AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_backend_comparison.cpp")
    message(STATUS "Adding Backend Comparison benchmarks")
    
    add_executable(bench_backend_comparison
        bench_backend_comparison.cpp
    )
    
    target_link_libraries(bench_backend_comparison PRIVATE
        themis_core

        ${BENCHMARK_LIBS}
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
        ${CMAKE_SOURCE_DIR}/src/rag/rag_judge.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/relevance_evaluator.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/faithfulness_evaluator.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/coherence_evaluator.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/completeness_evaluator.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/prompt_templates.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/response_parser.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/llm_judge_integration.cpp
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
# RAG Hybrid Retriever Benchmarks (Phase 3: BM25 + vector, configurable RRF)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_rag_hybrid_retriever.cpp")
    message(STATUS "Adding RAG Hybrid Retriever benchmarks")

    add_executable(bench_rag_hybrid_retriever
        bench_rag_hybrid_retriever.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_rag_hybrid_retriever PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

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
        bench_rag_evaluation.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/distributed_rag_evaluator.cpp
        ${CMAKE_SOURCE_DIR}/src/rag/prompt_injection_detector.cpp
    )

    target_link_libraries(bench_rag_evaluation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(bench_rag_evaluation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_rag_evaluation PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

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
# Graph Traversal Benchmarks (parallel multi-source BFS/DFS, graph analytics)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_graph_traversal.cpp")
    message(STATUS "Adding Graph Traversal benchmarks")

    add_executable(bench_graph_traversal
        bench_graph_traversal.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_graph_traversal PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_graph_traversal
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Graph Traversal: BFS/DFS, parallel multi-source traversal, fan-out threshold, graph analytics")
endif()

# Graph Query Optimizer Benchmarks (cost-based algorithm selection, plan caching)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_graph_query_optimizer.cpp")
    message(STATUS "Adding Graph Query Optimizer benchmarks")

    add_executable(bench_graph_query_optimizer
        bench_graph_query_optimizer.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_graph_query_optimizer PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_graph_query_optimizer
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Graph Query Optimizer: BFS/DFS/Dijkstra/A*, plan caching, parallel execution, adaptive cost model")
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

# Adaptive Query Cache Performance Benchmarks (Issue #1597)
# Covers the Production Readiness Checklist item "Performance benchmarks" for
# the multi-level L1/L2/L3 adaptive query cache (src/cache/).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_adaptive_query_cache.cpp")
    message(STATUS "Adding Adaptive Query Cache performance benchmarks")

    add_executable(bench_adaptive_query_cache
        bench_adaptive_query_cache.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_adaptive_query_cache PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_adaptive_query_cache
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Adaptive Query Cache: L1/L2 put/get, miss, tenant isolation, circuit breaker, concurrent access, invalidation")
endif()

# LLM Speculative Decoding & Inference Engine Benchmarks
# Runs automatically on every PR touching async_inference_engine.cpp or
# inference_engine_enhanced.cpp.  Acceptance criterion: tokens/sec regression < 5 %.
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm_bench.cpp")
    message(STATUS "Adding LLM speculative-decoding benchmarks")

    add_executable(llm_bench
        llm_bench.cpp
    )

    target_link_libraries(llm_bench PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(llm_bench PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(llm_bench PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS llm_bench
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LLM: Speculative decoding verify(), InferenceEngineEnhanced throughput")
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
    if(TARGET themis_llm)
        target_link_libraries(bench_gpu_vram_allocation PRIVATE themis_llm)
    endif()
    
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

# ============================================================================
# HotReloadManager Benchmarks
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_hot_reload_manager.cpp")
    message(STATUS "Adding HotReloadManager benchmarks")

    add_executable(bench_hot_reload_manager
        bench_hot_reload_manager.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_hot_reload_manager PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_cuda_vs_cpu.cpp")
    message(STATUS "Adding CUDA vs CPU acceleration benchmarks")

    add_executable(bench_cuda_vs_cpu
        bench_cuda_vs_cpu.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_cuda_vs_cpu PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_di_logging.cpp")
    message(STATUS "Adding DI overhead and logging throughput benchmarks")

    add_executable(bench_di_logging
        bench_di_logging.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_di_logging PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_auth_token_validation.cpp")
    message(STATUS "Adding auth token validation latency benchmarks")

    add_executable(bench_auth_token_validation
        bench_auth_token_validation.cpp
    )

    target_link_libraries(bench_auth_token_validation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_auth_token_validation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_auth_token_validation PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_auth_token_validation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Auth benchmarks: JWT RS256 (cached JWKS), JTI blacklist, TOTP, API token lookup")
endif()

# ============================================================================
# API Endpoint Performance Benchmarks (Issue #1511)
# Measures per-operation latency for GraphQL parsing (cached/uncached),
# GraphQL execution with mock resolvers, correlation ID UUID v4 generation,
# JSON response serialisation, HTTP message construction, and end-to-end
# REST endpoint roundtrips via an in-process HttpServer.
#
# Targets (src/api/FUTURE_ENHANCEMENTS.md):
#   - GraphQL parse+execute (10-field query): < 2 ms p99
#   - Correlation ID generation overhead:     < 10 µs/req
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_api_endpoints.cpp")
    message(STATUS "Adding API endpoint performance benchmarks")

    add_executable(bench_api_endpoints
        bench_api_endpoints.cpp
    )

    set(_bench_api_endpoint_libs
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    # Boost.Asio/Beast are often header-only; link imported targets only when available.
    if(TARGET Boost::asio)
        list(APPEND _bench_api_endpoint_libs Boost::asio)
    endif()
    if(TARGET Boost::beast)
        list(APPEND _bench_api_endpoint_libs Boost::beast)
    endif()
    if(TARGET Boost::boost)
        list(APPEND _bench_api_endpoint_libs Boost::boost)
    elseif(TARGET Boost::headers)
        list(APPEND _bench_api_endpoint_libs Boost::headers)
    elseif(DEFINED Boost_INCLUDE_DIRS)
        target_include_directories(bench_api_endpoints PRIVATE ${Boost_INCLUDE_DIRS})
    endif()

    target_link_libraries(bench_api_endpoints PRIVATE ${_bench_api_endpoint_libs})

    target_compile_definitions(bench_api_endpoints PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_api_endpoints PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_api_endpoints
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  API benchmarks: GraphQL parse/execute, correlation ID UUID, JSON serialisation, HTTP endpoint roundtrips")
endif()

# ============================================================================
# Governance Policy Evaluation Latency Benchmarks (Issue #1779)
# Measures per-operation latency of PolicyEngine::evaluate() and
# PolicyEngine::checkQueryPermission() at query time.
# Test matrix: heuristic fallback vs YAML-loaded profiles, all four VS
# classification levels, CCPA opt-out overhead, field-masking overhead,
# high-volume throughput (1/10/100/1 000 requests per batch).
# Targets: evaluate() p99 < 0.5 ms, checkQueryPermission() p99 < 0.5 ms.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_governance_policy_latency.cpp")
    message(STATUS "Adding governance policy evaluation latency benchmarks")

    add_executable(bench_governance_policy_latency
        bench_governance_policy_latency.cpp
    )

    target_link_libraries(bench_governance_policy_latency PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        yaml-cpp
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_governance_policy_latency PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    message(STATUS "  Governance Policy Latency: policy evaluation, hot-swap latency, cache hit rates")
else()
    message(STATUS "Governance Policy Latency benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_governance_policy_latency.cpp")
endif()

# Compliance / Security / Governance Performance Benchmarks
# Measures evaluation latency for GDPR, HIPAA, SOC 2, PCI-DSS, and CCPA/CPRA
# compliance rule paths, data masking throughput, and cross-regulation conflict
# detection overhead.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_compliance_security_governance.cpp")
    message(STATUS "Adding compliance/security/governance benchmarks")

    add_executable(bench_compliance_security_governance
        bench_compliance_security_governance.cpp
    )

    target_link_libraries(bench_compliance_security_governance PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(bench_compliance_security_governance PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    message(STATUS "  Compliance/Security/Governance: GDPR/HIPAA/SOC2/PCI-DSS/CCPA rule evaluation, data masking throughput")
else()
    message(STATUS "Compliance/Security/Governance benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_compliance_security_governance.cpp")
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_config_path_resolver.cpp")
    message(STATUS "Adding ConfigPathResolver cache hit rate and resolution latency benchmarks")

    add_executable(bench_config_path_resolver
        bench_config_path_resolver.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_config_path_resolver PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_config_path_resolver
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Config benchmarks: cache hit/miss latency, mapLegacyToNew, deprecation aggregator, metrics scrape, concurrent hit rate")
else()
    message(STATUS "ConfigPathResolver benchmark not found at ${CMAKE_CURRENT_SOURCE_DIR}/bench_config_path_resolver.cpp")
endif()

# ============================================================================
# Acceleration Dispatch Benchmarks
# CPUVectorBackend::computeDistances, batchKnnSearch;
# CPUGeoBackend::haversineDistance, batchHaversineDistances, pointInPolygon
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_acceleration_dispatch.cpp")
    message(STATUS "Adding Acceleration Dispatch benchmarks")

    add_executable(bench_acceleration_dispatch
        bench_acceleration_dispatch.cpp
    )

    target_link_libraries(bench_acceleration_dispatch PRIVATE
        themis_core
        ${BENCHMARK_LIBS}
        Threads::Threads
    )

    target_compile_definitions(bench_acceleration_dispatch PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_acceleration_dispatch PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_acceleration_dispatch
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Acceleration: computeDistances (L2/cosine), batchKnnSearch, haversineDistance, batchHaversineDistances, pointInPolygon")
endif()

# ============================================================================
# CDC Pipeline Benchmarks
# ConsumerGroupManager: createGroup/deleteGroup, fetchEventsAtLeastOnce,
# acknowledgeEvents, concurrent fan-out
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_cdc_pipeline.cpp")
    message(STATUS "Adding CDC Pipeline benchmarks")

    add_executable(bench_cdc_pipeline
        bench_cdc_pipeline.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_cdc_pipeline PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_cdc_pipeline
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  CDC Pipeline: createGroup, deleteGroup, fetchEventsAtLeastOnce, acknowledgeEvents, concurrent fetch")
endif()

# ============================================================================
# Temporal Queries Benchmarks
# BiTemporalTable: insertWithValidTime, queryBiTemporal,
# queryCurrentByValidTime, updateForValidTime, deleteForValidTime, getHistory
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_temporal_queries.cpp")
    message(STATUS "Adding Temporal Queries benchmarks")

    add_executable(bench_temporal_queries
        bench_temporal_queries.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_temporal_queries PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_temporal_queries
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Temporal: insertWithValidTime, queryBiTemporal, queryCurrentByValidTime, updateForValidTime, deleteForValidTime, getHistory")
endif()

# ============================================================================
# Task Scheduler Benchmarks
# TaskScheduler: registerTask, unregisterTask, executeTaskNow, listTasks,
# getStats, concurrent registerTask
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_task_scheduler.cpp")
    message(STATUS "Adding Task Scheduler benchmarks")

    add_executable(bench_task_scheduler
        bench_task_scheduler.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_task_scheduler PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

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
        bench_replication_throughput.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_replication_throughput PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_update_pipeline.cpp")
    message(STATUS "Adding Update Pipeline benchmarks")

    add_executable(bench_update_pipeline
        bench_update_pipeline.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench_update_pipeline PRIVATE
            -O3
            -march=native
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_update_pipeline
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Updates: state machine transitions, rollback path, manifest JSON round-trip, generatePatch, applyPatch")
endif()

# ============================================================================
# Index Module Benchmarks (Issue: #1884 — Performance benchmarks: QPS, recall, memory)
# These benchmarks cover vector search, quantization, spatial indexing, and
# approximate radius search.  All CPU-only benchmarks run without GPU hardware.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_vector_search.cpp")
    message(STATUS "Adding Vector Search benchmarks (Issue #1884)")

    add_executable(bench_vector_search
        bench_vector_search.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_vector_search PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

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
        bench_index_rebuild.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_index_rebuild PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_index_rebuild
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  IndexRebuild: secondary index build time, online rebuild read-QPS impact, WriteBatch throughput")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_spatial_index.cpp")
    message(STATUS "Adding Spatial Index benchmarks (Issue #1884)")

    add_executable(bench_spatial_index
        bench_spatial_index.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_spatial_index PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_spatial_index
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  SpatialIndex: R-tree insert/search QPS, Z-order range query, KNN spatial queries")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_approximate_radius_search.cpp")
    message(STATUS "Adding Approximate Radius Search benchmarks (Issue #1884)")

    add_executable(bench_approximate_radius_search
        bench_approximate_radius_search.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_approximate_radius_search PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_approximate_radius_search
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ApproximateRadiusSearch: radius search QPS, recall vs radius, batch search throughput")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_binary_quantization.cpp")
    message(STATUS "Adding Binary Quantization benchmarks (Issue #1884)")

    add_executable(bench_binary_quantization
        bench_binary_quantization.cpp
    )

    target_link_libraries(bench_binary_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_binary_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_binary_quantization PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_binary_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  BinaryQuantization: encode throughput, Hamming distance QPS, memory reduction factor")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_product_quantization.cpp")
    message(STATUS "Adding Product Quantization benchmarks (Issue #1884)")

    add_executable(bench_product_quantization
        bench_product_quantization.cpp
    )

    target_link_libraries(bench_product_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_product_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_product_quantization PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_product_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ProductQuantization: train throughput, encode QPS, ADC distance table, memory footprint")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_residual_quantization.cpp")
    message(STATUS "Adding Residual Quantization benchmarks (Issue #1884)")

    add_executable(bench_residual_quantization
        bench_residual_quantization.cpp
    )

    target_link_libraries(bench_residual_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_residual_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_residual_quantization PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_residual_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  ResidualQuantization: multi-stage encode throughput, reconstruction error, cascade QPS")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_learned_quantization.cpp")
    message(STATUS "Adding Learned Quantization benchmarks (Issue #1884)")

    add_executable(bench_learned_quantization
        bench_learned_quantization.cpp
    )

    target_link_libraries(bench_learned_quantization PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_learned_quantization PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_learned_quantization PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_learned_quantization
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  LearnedQuantization: neural codebook training, encode throughput, distortion vs compression rate")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_hnsw_prefilter_minimal.cpp")
    message(STATUS "Adding HNSW Pre-filter benchmarks (Issue #1884)")

    add_executable(bench_hnsw_prefilter_minimal
        bench_hnsw_prefilter_minimal.cpp
    )

    target_link_libraries(bench_hnsw_prefilter_minimal PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    if(TARGET hnswlib::hnswlib)
        target_link_libraries(bench_hnsw_prefilter_minimal PRIVATE hnswlib::hnswlib)
    endif()

    target_compile_definitions(bench_hnsw_prefilter_minimal PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_hnsw_prefilter_minimal PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_hnsw_prefilter_minimal
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  HnswPrefilterMinimal: HNSW pre-filter vs post-filter recall/QPS, selective-filter throughput")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_hybrid_vector_geo.cpp")
    message(STATUS "Adding Hybrid Vector+Geo benchmarks (Issue #1884)")

    add_executable(bench_hybrid_vector_geo
        bench_hybrid_vector_geo.cpp
    )

    target_link_libraries(bench_hybrid_vector_geo PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_hybrid_vector_geo PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_hybrid_vector_geo PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_hybrid_vector_geo
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  HybridVectorGeo: combined vector+geo search QPS, geo-filter overhead, fused query latency")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_vector_prefilter.cpp")
    message(STATUS "Adding Vector Prefilter benchmarks (Issue #1884)")

    add_executable(bench_vector_prefilter
        bench_vector_prefilter.cpp
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

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_vector_prefilter PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_vector_prefilter
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  VectorPrefilter: pre-filter selectivity vs recall, attribute filter overhead, ANN+filter QPS")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_vector_compression_lossless.cpp")
    message(STATUS "Adding Vector Compression (Lossless) benchmarks (Issue #1884)")

    add_executable(bench_vector_compression_lossless
        bench_vector_compression_lossless.cpp
    )

    target_link_libraries(bench_vector_compression_lossless PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(bench_vector_compression_lossless PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
            set(BENCHMARK_ARCH_FLAGS "-march=native")
        endif()
        target_compile_options(bench_vector_compression_lossless PRIVATE
            -O3
            ${BENCHMARK_ARCH_FLAGS}
            -DNDEBUG
        )
    endif()

    install(TARGETS bench_vector_compression_lossless
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  VectorCompressionLossless: SIMD delta/varint encode throughput, compression ratio, decompress latency")
endif()

# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_gpu_vector_index.cpp")
    if(THEMIS_ENABLE_GPU)
        message(STATUS "Adding GPU Vector Index benchmarks (THEMIS_ENABLE_GPU=ON, Issue #1884)")

        add_executable(bench_gpu_vector_index
            bench_gpu_vector_index.cpp
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

        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            if(NOT DEFINED BENCHMARK_ARCH_FLAGS)
                set(BENCHMARK_ARCH_FLAGS "-march=native")
            endif()
            target_compile_options(bench_gpu_vector_index PRIVATE
                -O3
                ${BENCHMARK_ARCH_FLAGS}
                -DNDEBUG
            )
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
