/**
 * @file bench_active_vram_allocator.cpp
 * @brief Benchmarks for ActiveVRAMAllocator (LLM-MISSING-001)
 *
 * Measures:
 *  - Single-allocation throughput at various sizes
 *  - Allocation + immediate free round-trip
 *  - Batch allocation then batch free
 *  - evictLRU() cost
 *  - defragment() overhead
 *  - spillLRUToCPU() latency
 *  - getStats() query cost
 *  - allocateWithFragmentation() bridge API
 *
 * Run with:
 *   ./bench_active_vram_allocator --benchmark_filter=.*VRAM.*
 */

#include <benchmark/benchmark.h>
#include "llm/active_vram_allocator.h"

using namespace themis::llm;

// ---------------------------------------------------------------------------
// Shared allocator setup
// ---------------------------------------------------------------------------

static ActiveVRAMAllocator::Config benchConfig() {
    ActiveVRAMAllocator::Config cfg;
    cfg.max_vram_bytes         = 512ULL * 1024 * 1024;  // 512 MB
    cfg.max_cpu_spill_bytes    = 512ULL * 1024 * 1024;
    cfg.enable_defragmentation = true;
    cfg.enable_cpu_spilling    = true;
    cfg.oom_threshold_fraction = 0.95f;
    cfg.min_free_vram_reserve  = 0;
    cfg.block_alignment        = 4096;
    return cfg;
}

// ---------------------------------------------------------------------------
// Allocation throughput at varying sizes
// ---------------------------------------------------------------------------

static void BM_VRAM_Allocate_4KB(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());
    for (auto _ : state) {
        auto h = alloc.allocate(4096, "bench_model");
        benchmark::DoNotOptimize(h);
        if (h) alloc.free(*h);
    }
    state.SetBytesProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_VRAM_Allocate_4KB);

static void BM_VRAM_Allocate_1MB(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());
    for (auto _ : state) {
        auto h = alloc.allocate(1ULL * 1024 * 1024, "bench_model");
        benchmark::DoNotOptimize(h);
        if (h) alloc.free(*h);
    }
    state.SetBytesProcessed(state.iterations() * 1024 * 1024);
}
BENCHMARK(BM_VRAM_Allocate_1MB);

static void BM_VRAM_Allocate_64MB(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());
    for (auto _ : state) {
        auto h = alloc.allocate(64ULL * 1024 * 1024, "bench_model");
        benchmark::DoNotOptimize(h);
        if (h) alloc.free(*h);
    }
    state.SetBytesProcessed(state.iterations() * 64 * 1024 * 1024);
}
BENCHMARK(BM_VRAM_Allocate_64MB);

// Parametric size benchmark
static void BM_VRAM_Allocate_Parametric(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());
    const size_t bytes = static_cast<size_t>(state.range(0));
    for (auto _ : state) {
        auto h = alloc.allocate(bytes, "bench_param");
        benchmark::DoNotOptimize(h);
        if (h) alloc.free(*h);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(bytes));
}
BENCHMARK(BM_VRAM_Allocate_Parametric)
    ->Arg(4096)
    ->Arg(65536)
    ->Arg(1 << 20)
    ->Arg(16 << 20);

// ---------------------------------------------------------------------------
// Batch allocation / free
// ---------------------------------------------------------------------------

static void BM_VRAM_BatchAllocate(benchmark::State& state) {
    const int kBatch = static_cast<int>(state.range(0));
    ActiveVRAMAllocator alloc(benchConfig());

    for (auto _ : state) {
        std::vector<ActiveVRAMAllocator::AllocationHandle> handles;
        handles.reserve(kBatch);

        for (int i = 0; i < kBatch; ++i) {
            auto h = alloc.allocate(4096, "batch_" + std::to_string(i));
            if (h) handles.push_back(*h);
        }
        benchmark::DoNotOptimize(handles);

        for (auto& h : handles) alloc.free(h);
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_VRAM_BatchAllocate)->Arg(1)->Arg(8)->Arg(32)->Arg(128);

// ---------------------------------------------------------------------------
// evictLRU cost
// ---------------------------------------------------------------------------

static void BM_VRAM_EvictLRU(benchmark::State& state) {
    const int kPrealloc = 16;
    ActiveVRAMAllocator alloc(benchConfig());

    // Keep a steady pool of live allocations
    std::vector<ActiveVRAMAllocator::AllocationHandle> live;
    live.reserve(kPrealloc);
    for (int i = 0; i < kPrealloc; ++i) {
        auto h = alloc.allocate(4096, "evict_pool_" + std::to_string(i));
        if (h) live.push_back(*h);
    }

    for (auto _ : state) {
        // Evict one, then refill
        size_t freed = alloc.evictLRU();
        benchmark::DoNotOptimize(freed);

        auto h = alloc.allocate(4096, "refill");
        if (h) live.push_back(*h);

        // Keep pool size stable
        if (live.size() > static_cast<size_t>(kPrealloc) + 1) {
            live.erase(live.begin());
        }
    }

    for (auto& h : live) alloc.free(h);
}
BENCHMARK(BM_VRAM_EvictLRU);

// ---------------------------------------------------------------------------
// defragment overhead
// ---------------------------------------------------------------------------

static void BM_VRAM_Defragment(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());

    // Create some fragmentation first
    std::vector<ActiveVRAMAllocator::AllocationHandle> handles;
    for (int i = 0; i < 32; ++i) {
        auto h = alloc.allocate(4096, "frag_" + std::to_string(i));
        if (h) handles.push_back(*h);
    }
    // Free every other one to create holes
    for (size_t i = 0; i < handles.size(); i += 2) {
        alloc.free(handles[i]);
    }

    for (auto _ : state) {
        bool ok = alloc.defragment();
        benchmark::DoNotOptimize(ok);
    }

    // Clean up remaining
    for (size_t i = 1; i < handles.size(); i += 2) {
        alloc.free(handles[i]);
    }
}
BENCHMARK(BM_VRAM_Defragment);

// ---------------------------------------------------------------------------
// spillLRUToCPU latency
// ---------------------------------------------------------------------------

static void BM_VRAM_SpillToCPU(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());

    for (auto _ : state) {
        // Allocate fresh block to spill
        auto h = alloc.allocate(64 * 1024, "spill_target");
        if (!h) {
            state.SkipWithError("Allocation failed");
            break;
        }

        size_t spilled = alloc.spillLRUToCPU();
        benchmark::DoNotOptimize(spilled);

        // The handle is now spilled; no GPU ptr to free directly — evict it
        alloc.evictLRU();
    }
}
BENCHMARK(BM_VRAM_SpillToCPU);

// ---------------------------------------------------------------------------
// getStats query cost
// ---------------------------------------------------------------------------

static void BM_VRAM_GetStats(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());

    // Some live allocations to make stats non-trivial
    std::vector<ActiveVRAMAllocator::AllocationHandle> handles;
    for (int i = 0; i < 8; ++i) {
        auto h = alloc.allocate(4096, "stats_" + std::to_string(i));
        if (h) handles.push_back(*h);
    }

    for (auto _ : state) {
        auto stats = alloc.getStats();
        benchmark::DoNotOptimize(stats);
    }

    for (auto& h : handles) alloc.free(h);
}
BENCHMARK(BM_VRAM_GetStats);

// ---------------------------------------------------------------------------
// allocateWithFragmentation bridge API
// ---------------------------------------------------------------------------

static void BM_VRAM_AllocateWithFragmentation(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());

    for (auto _ : state) {
        void* ptr = nullptr;
        bool ok = alloc.allocateWithFragmentation(4096, &ptr);
        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(ptr);
    }
}
BENCHMARK(BM_VRAM_AllocateWithFragmentation);

// ---------------------------------------------------------------------------
// allocateOrRecover (with empty pool — no recovery needed)
// ---------------------------------------------------------------------------

static void BM_VRAM_AllocateOrRecover_NoOOM(benchmark::State& state) {
    ActiveVRAMAllocator alloc(benchConfig());

    for (auto _ : state) {
        auto h = alloc.allocateOrRecover(4096, "recover_bench");
        benchmark::DoNotOptimize(h);
        if (h) alloc.free(*h);
    }
}
BENCHMARK(BM_VRAM_AllocateOrRecover_NoOOM);

BENCHMARK_MAIN();
