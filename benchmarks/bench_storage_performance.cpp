/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_storage_performance.cpp                      ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     561                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_storage_performance.cpp
 * @brief Real Google Benchmark performance tests for storage components
 * 
 * Tests storage performance with:
 * - mimalloc vs system allocator comparison
 * - Huge pages performance (2MB/1GB)
 * - RCU index read/write performance
 * - Memory allocation patterns
 * - Baseline vs optimized variants
 * 
 * Output: JSON format for CI regression tracking
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "performance/allocator.h"
#include "performance/huge_pages.h"
#include "performance/rcu.h"
#include "performance/rcu_hash_table.h"
#include <vector>
#include <string>
#include <random>
#include <cstring>
#include <thread>

using namespace themis;
using namespace themis::memory;
using namespace themis::rcu;

namespace {

// ═══════════════════════════════════════════════════════════
// Test Data Generators
// ═══════════════════════════════════════════════════════════

std::vector<size_t> generateAllocSizes(size_t count) {
    std::vector<size_t> sizes;
    sizes.reserve(count);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(64, 4096);
    for (size_t i = 0; i < count; ++i) {
        sizes.push_back(dist(rng));
    }
    return sizes;
}

std::vector<std::string> generateKeys(size_t count) {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        keys.push_back("key_" + std::to_string(i));
    }
    return keys;
}

// ═══════════════════════════════════════════════════════════
// Memory Allocator Benchmarks (mimalloc vs system)
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: System allocator for small allocations
 * Target: <100ns per allocation
 */
static void BM_Allocator_System_Small(benchmark::State& state) {
    const size_t alloc_size = 128;
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 1000; ++i) {
            void* ptr = ::operator new(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            ::operator delete(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel("system_alloc");
}
BENCHMARK(BM_Allocator_System_Small);

/**
 * Optimized: ThemisDB allocator (may use mimalloc) for small allocations
 * Target: <80ns per allocation (20% improvement)
 */
static void BM_Allocator_Themis_Small(benchmark::State& state) {
    const size_t alloc_size = 128;
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 1000; ++i) {
            void* ptr = allocate(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel(allocator_name());
}
BENCHMARK(BM_Allocator_Themis_Small);

/**
 * Baseline: System allocator for large allocations
 * Target: <1us per allocation
 */
static void BM_Allocator_System_Large(benchmark::State& state) {
    const size_t alloc_size = 1024 * 1024; // 1MB
    std::vector<void*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 100; ++i) {
            void* ptr = ::operator new(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            ::operator delete(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetBytesProcessed(state.iterations() * 100 * alloc_size);
    state.SetLabel("system_alloc");
}
BENCHMARK(BM_Allocator_System_Large);

/**
 * Optimized: ThemisDB allocator for large allocations
 * Target: <800ns per allocation (20% improvement)
 */
static void BM_Allocator_Themis_Large(benchmark::State& state) {
    const size_t alloc_size = 1024 * 1024; // 1MB
    std::vector<void*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (int i = 0; i < 100; ++i) {
            void* ptr = allocate(alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetBytesProcessed(state.iterations() * 100 * alloc_size);
    state.SetLabel(allocator_name());
}
BENCHMARK(BM_Allocator_Themis_Large);

/**
 * Mixed allocation pattern (realistic workload)
 */
static void BM_Allocator_Mixed(benchmark::State& state) {
    auto sizes = generateAllocSizes(1000);
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (size_t size : sizes) {
            void* ptr = allocate(size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel(allocator_name());
}
BENCHMARK(BM_Allocator_Mixed);

// ═══════════════════════════════════════════════════════════
// Huge Pages Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Regular pages memory access
 * Target: Baseline for comparison
 */
static void BM_Memory_RegularPages_Sequential(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    void* mem = allocate(size);
    if (!mem) {
        state.SkipWithError("Memory allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    for (auto _ : state) {
        // Sequential access pattern
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        size_t count = size / sizeof(uint64_t);
        
        for (size_t i = 0; i < count; ++i) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    state.SetLabel("regular_pages");
    deallocate(mem);
}
BENCHMARK(BM_Memory_RegularPages_Sequential);

/**
 * Optimized: Huge pages memory access (if available)
 * Target: 5-10% improvement due to better TLB utilization
 */
static void BM_Memory_HugePages_Sequential(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    
    if (!huge_pages_available()) {
        state.SkipWithError("Huge pages not available");
        return;
    }
    
    void* mem = allocate_huge_pages(size);
    if (!mem) {
        state.SkipWithError("Huge pages allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    for (auto _ : state) {
        // Sequential access pattern
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        size_t count = size / sizeof(uint64_t);
        
        for (size_t i = 0; i < count; ++i) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    state.SetLabel("huge_pages");
    deallocate(mem);
}
BENCHMARK(BM_Memory_HugePages_Sequential);

/**
 * Random access pattern - shows TLB benefits more clearly
 */
static void BM_Memory_RegularPages_Random(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    void* mem = allocate(size);
    if (!mem) {
        state.SkipWithError("Memory allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    // Generate random indices
    std::vector<size_t> indices;
    indices.reserve(10000);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, (size / sizeof(uint64_t)) - 1);
    for (int i = 0; i < 10000; ++i) {
        indices.push_back(dist(rng));
    }
    
    for (auto _ : state) {
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        
        for (size_t idx : indices) {
            sum += data[idx];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
    state.SetLabel("regular_pages_random");
    deallocate(mem);
}
BENCHMARK(BM_Memory_RegularPages_Random);

/**
 * Random access with huge pages
 */
static void BM_Memory_HugePages_Random(benchmark::State& state) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    
    if (!huge_pages_available()) {
        state.SkipWithError("Huge pages not available");
        return;
    }
    
    void* mem = allocate_huge_pages(size);
    if (!mem) {
        state.SkipWithError("Huge pages allocation failed");
        return;
    }
    
    std::memset(mem, 0, size);
    
    // Generate random indices
    std::vector<size_t> indices;
    indices.reserve(10000);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, (size / sizeof(uint64_t)) - 1);
    for (int i = 0; i < 10000; ++i) {
        indices.push_back(dist(rng));
    }
    
    for (auto _ : state) {
        volatile uint64_t sum = 0;
        uint64_t* data = static_cast<uint64_t*>(mem);
        
        for (size_t idx : indices) {
            sum += data[idx];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
    state.SetLabel("huge_pages_random");
    deallocate(mem);
}
BENCHMARK(BM_Memory_HugePages_Random);

// ═══════════════════════════════════════════════════════════
// RCU Index Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * RCU Read Performance - Single Thread
 * Target: <50ns per read
 */
static void BM_RCU_Read_SingleThread(benchmark::State& state) {
    if (!GracePeriodManager::is_enabled()) {
        state.SkipWithError("RCU not enabled at compile time");
        return;
    }
    
    // Simulate shared data structure
    std::shared_ptr<std::vector<int>> data = 
        std::make_shared<std::vector<int>>(10000, 42);
    
    for (auto _ : state) {
        ReadLock lock;
        
        // Read operations
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) {
            sum += (*data)[i];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetLabel("rcu_read");
}
BENCHMARK(BM_RCU_Read_SingleThread);

/**
 * RCU Write Performance - Update and synchronize
 * Target: <1ms per write (including synchronization)
 */
static void BM_RCU_Write_WithSync(benchmark::State& state) {
    if (!GracePeriodManager::is_enabled()) {
        state.SkipWithError("RCU not enabled at compile time");
        return;
    }
    
    GracePeriodManager& mgr = GracePeriodManager::instance();
    std::shared_ptr<std::vector<int>> data = 
        std::make_shared<std::vector<int>>(10000, 42);
    
    for (auto _ : state) {
        // Copy-modify-update pattern
        auto new_data = std::make_shared<std::vector<int>>(*data);
        (*new_data)[0] = 100;
        
        // Atomic update (in real code)
        data = new_data;
        
        // Wait for readers to finish
        mgr.synchronize_rcu();
        
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("rcu_write");
}
BENCHMARK(BM_RCU_Write_WithSync);

/**
 * RCU Multi-threaded Read Performance
 * Target: Linear scalability up to 8 threads
 */
static void BM_RCU_Read_MultiThread(benchmark::State& state) {
    if (!GracePeriodManager::is_enabled()) {
        state.SkipWithError("RCU not enabled at compile time");
        return;
    }
    
    std::shared_ptr<std::vector<int>> data = 
        std::make_shared<std::vector<int>>(10000, 42);
    
    for (auto _ : state) {
        ReadLock lock;
        
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) {
            sum += (*data)[i % data->size()];
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetLabel("rcu_read_mt");
}
BENCHMARK(BM_RCU_Read_MultiThread)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8);

// ═══════════════════════════════════════════════════════════
// Memory Usage Tracking
// ═══════════════════════════════════════════════════════════

/**
 * Measure memory overhead of allocator
 */
static void BM_Memory_Overhead(benchmark::State& state) {
    const size_t num_allocations = state.range(0);
    const size_t alloc_size = 128;
    
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocations);
    
    for (auto _ : state) {
        state.PauseTiming();
        ptrs.clear();
        state.ResumeTiming();
        
        // Allocate
        for (size_t i = 0; i < num_allocations; ++i) {
            void* ptr = allocate(alloc_size);
            ptrs.push_back(ptr);
        }
        
        benchmark::ClobberMemory();
        
        // Cleanup
        state.PauseTiming();
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
        state.ResumeTiming();
    }
    
    state.SetBytesProcessed(state.iterations() * num_allocations * alloc_size);
    state.SetLabel(std::string(allocator_name()) + "_overhead");
}
BENCHMARK(BM_Memory_Overhead)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

} // namespace

// ═══════════════════════════════════════════════════════════
// Main - Configure JSON output for CI
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
