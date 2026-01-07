// Google Benchmark for Huge Pages Performance
// Tests memory access performance with and without huge pages

#include <benchmark/benchmark.h>
#include <performance/huge_pages.h>
#include <vector>
#include <random>
#include <cstring>
#include <iostream>

using namespace themis::memory;

// Benchmark: Sequential memory access
static void BM_SequentialAccess(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    
    #ifdef THEMIS_USE_HUGE_PAGES
    void* buffer = allocate_huge_pages(buffer_size);
    #else
    void* buffer = malloc(buffer_size);
    #endif
    
    if (!buffer) {
        state.SkipWithError("Failed to allocate buffer");
        return;
    }
    
    char* data = static_cast<char*>(buffer);
    
    for (auto _ : state) {
        // Sequential write
        for (size_t i = 0; i < buffer_size; ++i) {
            data[i] = static_cast<char>(i & 0xFF);
        }
        benchmark::DoNotOptimize(data);
        benchmark::ClobberMemory();
        
        // Sequential read
        volatile char sum = 0;
        for (size_t i = 0; i < buffer_size; ++i) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetBytesProcessed(state.iterations() * buffer_size * 2); // read + write
    
    #ifdef THEMIS_USE_HUGE_PAGES
    deallocate_huge_pages(buffer, buffer_size);
    #else
    free(buffer);
    #endif
}

// Benchmark: Random memory access (shows TLB benefit)
static void BM_RandomAccess(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    const size_t num_accesses = 10000;
    
    #ifdef THEMIS_USE_HUGE_PAGES
    void* buffer = allocate_huge_pages(buffer_size);
    #else
    void* buffer = malloc(buffer_size);
    #endif
    
    if (!buffer) {
        state.SkipWithError("Failed to allocate buffer");
        return;
    }
    
    char* data = static_cast<char*>(buffer);
    
    // Generate random indices
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, buffer_size - 1);
    
    std::vector<size_t> indices;
    indices.reserve(num_accesses);
    for (size_t i = 0; i < num_accesses; ++i) {
        indices.push_back(dist(gen));
    }
    
    for (auto _ : state) {
        volatile char sum = 0;
        for (size_t idx : indices) {
            sum += data[idx];
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetItemsProcessed(state.iterations() * num_accesses);
    
    #ifdef THEMIS_USE_HUGE_PAGES
    deallocate_huge_pages(buffer, buffer_size);
    #else
    free(buffer);
    #endif
}

// Benchmark: Strided access (highlights TLB misses)
static void BM_StridedAccess(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    const size_t stride = 4096; // Page size - maximizes TLB pressure
    
    #ifdef THEMIS_USE_HUGE_PAGES
    void* buffer = allocate_huge_pages(buffer_size);
    #else
    void* buffer = malloc(buffer_size);
    #endif
    
    if (!buffer) {
        state.SkipWithError("Failed to allocate buffer");
        return;
    }
    
    char* data = static_cast<char*>(buffer);
    
    for (auto _ : state) {
        volatile char sum = 0;
        for (size_t i = 0; i < buffer_size; i += stride) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetItemsProcessed(state.iterations() * (buffer_size / stride));
    
    #ifdef THEMIS_USE_HUGE_PAGES
    deallocate_huge_pages(buffer, buffer_size);
    #else
    free(buffer);
    #endif
}

// Benchmark: Memory initialization
static void BM_MemoryInitialization(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    
    for (auto _ : state) {
        #ifdef THEMIS_USE_HUGE_PAGES
        void* buffer = allocate_huge_pages(buffer_size);
        #else
        void* buffer = malloc(buffer_size);
        #endif
        
        if (!buffer) {
            state.SkipWithError("Failed to allocate buffer");
            return;
        }
        
        std::memset(buffer, 0, buffer_size);
        benchmark::DoNotOptimize(buffer);
        
        #ifdef THEMIS_USE_HUGE_PAGES
        deallocate_huge_pages(buffer, buffer_size);
        #else
        free(buffer);
        #endif
    }
    
    state.SetBytesProcessed(state.iterations() * buffer_size);
}

// Benchmark: Large allocation pattern (database buffer pool simulation)
static void BM_BufferPoolSimulation(benchmark::State& state) {
    const size_t pool_size = 64 * 1024 * 1024; // 64MB
    
    #ifdef THEMIS_USE_HUGE_PAGES
    void* pool = allocate_huge_pages(pool_size);
    #else
    void* pool = malloc(pool_size);
    #endif
    
    if (!pool) {
        state.SkipWithError("Failed to allocate buffer pool");
        return;
    }
    
    char* data = static_cast<char*>(pool);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, pool_size - 4096);
    
    for (auto _ : state) {
        // Simulate buffer pool access pattern
        size_t offset = dist(gen);
        volatile int sum = 0;
        for (size_t i = 0; i < 4096; i += 64) {
            sum += data[offset + i];
        }
        benchmark::DoNotOptimize(sum);
    }
    
    #ifdef THEMIS_USE_HUGE_PAGES
    deallocate_huge_pages(pool, pool_size);
    #else
    free(pool);
    #endif
}

// Register benchmarks
BENCHMARK(BM_SequentialAccess)
    ->Arg(4 * 1024 * 1024)      // 4MB
    ->Arg(16 * 1024 * 1024)     // 16MB
    ->Arg(64 * 1024 * 1024)     // 64MB
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_RandomAccess)
    ->Arg(4 * 1024 * 1024)
    ->Arg(16 * 1024 * 1024)
    ->Arg(64 * 1024 * 1024)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_StridedAccess)
    ->Arg(16 * 1024 * 1024)
    ->Arg(64 * 1024 * 1024)
    ->Arg(256 * 1024 * 1024)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_MemoryInitialization)
    ->Arg(4 * 1024 * 1024)
    ->Arg(16 * 1024 * 1024)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_BufferPoolSimulation)
    ->Threads(1)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kNanosecond);

// Custom main
int main(int argc, char** argv) {
    std::cout << "\n=================================================================\n";
    std::cout << "Huge Pages Memory Benchmark\n";
    std::cout << "=================================================================\n";
    std::cout << "Status: " << huge_pages_status() << "\n";
    std::cout << "Page size: " << get_huge_page_size() << " bytes\n";
    
    if (is_huge_pages_enabled()) {
        std::cout << "\nExpected improvement: +15-30% for memory-intensive workloads\n";
        std::cout << "TLB miss reduction: Up to 512x (4KB -> 2MB pages)\n";
        std::cout << "Best performance: Large memory access, random patterns\n";
    } else {
        std::cout << "\n⚠️  Huge pages NOT enabled\n";
        std::cout << "Build with: cmake -DTHEMIS_ENABLE_HUGE_PAGES=ON\n";
        std::cout << "Linux setup: echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages\n";
    }
    
    std::cout << "=================================================================\n\n";
    
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    
    return 0;
}
