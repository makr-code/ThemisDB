# Performance Optimization Benchmarks
# Uses Google Benchmark for performance validation

cmake_minimum_required(VERSION 3.15)

# Find Google Benchmark
find_package(benchmark CONFIG)

if(NOT benchmark_FOUND)
    message(WARNING "Google Benchmark not found. Benchmarks will not be built.")
    message(WARNING "Install with: vcpkg install benchmark")
    return()
endif()

message(STATUS "Found Google Benchmark: ${benchmark_DIR}")

# Helper function to add a benchmark
function(add_perf_benchmark name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} 
        PRIVATE 
            benchmark::benchmark
            benchmark::benchmark_main
            themis_core  # Link against themis_core to get compile definitions
    )
    target_include_directories(${name} 
        PRIVATE 
            ${CMAKE_SOURCE_DIR}/include
    )
    
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
    )
endfunction()

# Mimalloc benchmark
if(THEMIS_BUILD_BENCHMARKS)
    add_perf_benchmark(benchmark_mimalloc 
        benchmark_mimalloc.cpp
    )
    
    add_perf_benchmark(benchmark_huge_pages
        benchmark_huge_pages.cpp
    )
    
    add_perf_benchmark(benchmark_rcu_index
        benchmark_rcu_index.cpp
    )
    
    add_perf_benchmark(benchmark_lirs_cache
        benchmark_lirs_cache.cpp
    )
    
    add_perf_benchmark(benchmark_safe_fail
        benchmark_safe_fail.cpp
    )
    
    message(STATUS "Performance benchmarks enabled:")
    message(STATUS "  - benchmark_mimalloc")
    message(STATUS "  - benchmark_huge_pages")
    message(STATUS "  - benchmark_rcu_index")
    message(STATUS "  - benchmark_lirs_cache")
    message(STATUS "  - benchmark_safe_fail")
    message(STATUS "")
    message(STATUS "Run benchmarks with:")
    message(STATUS "  ./build/benchmarks/benchmark_mimalloc")
    message(STATUS "  ./build/benchmarks/benchmark_huge_pages")
    message(STATUS "  ./build/benchmarks/benchmark_rcu_index")
    message(STATUS "  ./build/benchmarks/benchmark_lirs_cache")
    message(STATUS "  ./build/benchmarks/benchmark_safe_fail")
endif()
