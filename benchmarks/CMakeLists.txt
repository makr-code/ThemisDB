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
# Existing Benchmarks
# ============================================================================

# Add existing benchmark subdirectories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/performance_optimizations")
    add_subdirectory(performance_optimizations)
endif()

# ============================================================================
# Benchmark Utilities
# ============================================================================

# Custom target to run all benchmarks
add_custom_target(run_benchmarks
    COMMAND bench_lora_framework --benchmark_out=benchmark_results.json --benchmark_out_format=json
    DEPENDS bench_lora_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all ThemisDB benchmarks..."
)

# Custom target to run benchmarks with various filters
add_custom_target(run_benchmarks_quick
    COMMAND bench_lora_framework --benchmark_filter="Storage|Manager" --benchmark_min_time=0.1
    DEPENDS bench_lora_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running quick benchmarks..."
)

add_custom_target(run_benchmarks_detailed
    COMMAND bench_lora_framework --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
    DEPENDS bench_lora_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running detailed benchmarks with statistics..."
)

message(STATUS "Benchmarks configured successfully")
message(STATUS "  - Build benchmarks with: cmake --build . --target bench_lora_framework")
message(STATUS "  - Run benchmarks with: ./benchmarks/bench_lora_framework")
message(STATUS "  - Or use: make run_benchmarks")
