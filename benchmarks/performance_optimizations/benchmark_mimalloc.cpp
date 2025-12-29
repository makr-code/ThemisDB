// Google Benchmark for Mimalloc Performance
// Tests allocation performance with and without mimalloc

#include <benchmark/benchmark.h>
#include <performance/allocator.h>
#include <vector>
#include <random>
#include <iostream>
#include <cstring>

using namespace themis::memory;

// Benchmark: Simple allocation and deallocation
static void BM_SimpleAllocation(benchmark::State& state) {
    const size_t alloc_size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = allocate(alloc_size);
        benchmark::DoNotOptimize(ptr);
        deallocate(ptr);
    }
    
    state.SetBytesProcessed(state.iterations() * alloc_size);
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Multiple allocations
static void BM_MultipleAllocations(benchmark::State& state) {
    const size_t num_allocs = state.range(0);
    const size_t alloc_size = 256;
    
    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(num_allocs);
        
        // Allocate
        for (size_t i = 0; i < num_allocs; ++i) {
            ptrs.push_back(allocate(alloc_size));
        }
        
        benchmark::DoNotOptimize(ptrs);
        
        // Deallocate
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_allocs);
}

// Benchmark: Random size allocations
static void BM_RandomSizeAllocations(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(64, 4096);
    
    for (auto _ : state) {
        size_t size = size_dist(gen);
        void* ptr = allocate(size);
        benchmark::DoNotOptimize(ptr);
        deallocate(ptr);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Aligned allocations
static void BM_AlignedAllocation(benchmark::State& state) {
    const size_t alloc_size = state.range(0);
    const size_t alignment = 64;
    
    for (auto _ : state) {
        void* ptr = allocate_aligned(alloc_size, alignment);
        benchmark::DoNotOptimize(ptr);
        deallocate_aligned(ptr, alignment);
    }
    
    state.SetBytesProcessed(state.iterations() * alloc_size);
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Allocation/Deallocation with usage
static void BM_AllocationWithUsage(benchmark::State& state) {
    const size_t alloc_size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = allocate(alloc_size);
        
        // Simulate usage by writing data
        std::memset(ptr, 0x42, alloc_size);
        benchmark::DoNotOptimize(ptr);
        
        deallocate(ptr);
    }
    
    state.SetBytesProcessed(state.iterations() * alloc_size);
}

// Benchmark: Thread-local allocation pattern
static void BM_ThreadLocalPattern(benchmark::State& state) {
    const size_t alloc_size = 1024;
    const size_t num_allocs = 100;
    
    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(num_allocs);
        
        // Allocate many small objects (mimalloc shines here)
        for (size_t i = 0; i < num_allocs; ++i) {
            ptrs.push_back(allocate(alloc_size));
        }
        
        // Free them all
        for (void* ptr : ptrs) {
            deallocate(ptr);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_allocs);
}

// Register benchmarks with different sizes
BENCHMARK(BM_SimpleAllocation)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_MultipleAllocations)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_RandomSizeAllocations)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_AlignedAllocation)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_AllocationWithUsage)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(65536)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ThreadLocalPattern)
    ->ThreadRange(1, 8)
    ->Unit(benchmark::kMicrosecond);

// Custom main to show allocator info
int main(int argc, char** argv) {
    std::cout << "\n=================================================================\n";
    std::cout << "Mimalloc Allocator Benchmark\n";
    std::cout << "=================================================================\n";
    std::cout << "Allocator: " << allocator_name() << "\n";
    std::cout << "Mimalloc enabled: " << (is_mimalloc_enabled() ? "YES" : "NO") << "\n";
    
    if (is_mimalloc_enabled()) {
        std::cout << "\nExpected improvement: +10-20% vs system allocator\n";
        std::cout << "Best performance: Multi-threaded, many small allocations\n";
    } else {
        std::cout << "\n⚠️  Mimalloc NOT enabled - using system allocator\n";
        std::cout << "Build with: cmake -DTHEMIS_ENABLE_MIMALLOC=ON\n";
    }
    
    std::cout << "=================================================================\n\n";
    
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    
    return 0;
}
