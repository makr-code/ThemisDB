// GPU Module Benchmark Suite
// Measures latency of core GPU sub-systems:
//   - GPUMemoryManager::TryAllocateGPU / DeallocateGPU
//   - GPUMemoryPool acquire / release
//   - GPUMetrics record / snapshot
//   - GPUPolicy check
//   - GPUConfig validate / simulate
//   - GPUAdminAPI getStatsJson / simulateJson

#include <benchmark/benchmark.h>
#include <cstdint>

#include "themis/gpu/memory_manager.h"
#include "themis/gpu/memory_pool.h"
#include "themis/gpu/metrics.h"
#include "themis/gpu/policy.h"
#include "themis/gpu/config.h"
#include "themis/gpu/admin_api.h"

#ifndef THEMIS_ENABLE_GPU

static void BM_GPUModule_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("GPU module benchmarks are disabled in this build");
        break;
    }
}
// Disabled: GPU module subsystem requires dedicated GPU hardware | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_GPUModule_GPUDisabled);

BENCHMARK_MAIN();

#else

using namespace themis::gpu;

// ============================================================================
// GPUMemoryManager benchmarks
// ============================================================================

// Allocation size used in all benchmarks below: 1 MB.
static constexpr uint64_t kAllocBytes = 1ULL * 1024 * 1024;

static void BM_MemoryManager_TryAllocate(benchmark::State& state) {
    auto& mgr = GPUMemoryManager::GetInstance();
    for (auto _ : state) {
        bool ok = mgr.TryAllocateGPU(kAllocBytes, "bench");
        benchmark::DoNotOptimize(ok);
        if (ok) mgr.DeallocateGPU(kAllocBytes);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                             static_cast<int64_t>(kAllocBytes));
}
BENCHMARK(BM_MemoryManager_TryAllocate)->Unit(benchmark::kNanosecond);

static void BM_MemoryManager_Deallocate(benchmark::State& state) {
    auto& mgr = GPUMemoryManager::GetInstance();
    for (auto _ : state) {
        // Pre-alloc outside the timed loop.
        state.PauseTiming();
        mgr.TryAllocateGPU(kAllocBytes, "bench");
        state.ResumeTiming();

        mgr.DeallocateGPU(kAllocBytes);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryManager_Deallocate)->Unit(benchmark::kNanosecond);

static void BM_MemoryManager_GetStats(benchmark::State& state) {
    auto& mgr = GPUMemoryManager::GetInstance();
    for (auto _ : state) {
        auto s = mgr.GetStats();
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryManager_GetStats)->Unit(benchmark::kNanosecond);

// ============================================================================
// GPUMemoryPool benchmarks
// ============================================================================

static void BM_MemoryPool_Acquire_Release(benchmark::State& state) {
    const size_t slab_size = 256ULL * 1024 * 1024;  // 256 MB slabs
    const size_t num_slabs = 4;
    GPUMemoryPool pool(slab_size * num_slabs, slab_size, num_slabs);

    const uint64_t alloc_size = static_cast<uint64_t>(state.range(0)) * 1024;
    for (auto _ : state) {
        uint64_t offset = 0;
        bool acquired = pool.tryAcquire(alloc_size, "bench", offset);
        benchmark::DoNotOptimize(acquired);
        benchmark::DoNotOptimize(offset);
        if (acquired) {
            pool.release(offset);
        }
    }
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                             static_cast<int64_t>(alloc_size));
}
BENCHMARK(BM_MemoryPool_Acquire_Release)
    ->Arg(64)    // 64 KB
    ->Arg(256)   // 256 KB
    ->Arg(1024)  // 1 MB
    ->Arg(4096)  // 4 MB
    ->Unit(benchmark::kNanosecond);

static void BM_MemoryPool_Stats(benchmark::State& state) {
    GPUMemoryPool pool(512ULL * 1024 * 1024, 256ULL * 1024 * 1024, 2);
    for (auto _ : state) {
        auto s = pool.getStats();
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryPool_Stats)->Unit(benchmark::kNanosecond);

// ============================================================================
// GPUMetrics benchmarks
// ============================================================================

static void BM_Metrics_RecordAllocSuccess(benchmark::State& state) {
    auto& met = GPUMetrics::GetInstance();
    for (auto _ : state) {
        met.recordAllocSuccess(kAllocBytes, "tenant_bench");
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Metrics_RecordAllocSuccess)->Unit(benchmark::kNanosecond);

static void BM_Metrics_RecordFallback(benchmark::State& state) {
    auto& met = GPUMetrics::GetInstance();
    for (auto _ : state) {
        met.recordFallback("oom");
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Metrics_RecordFallback)->Unit(benchmark::kNanosecond);

static void BM_Metrics_Snapshot(benchmark::State& state) {
    auto& met = GPUMetrics::GetInstance();
    // Warm up some counters.
    for (int i = 0; i < 100; ++i) met.recordAllocSuccess(kAllocBytes);
    for (auto _ : state) {
        auto snap = met.snapshot();
        benchmark::DoNotOptimize(snap);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Metrics_Snapshot)->Unit(benchmark::kMicrosecond);

// ============================================================================
// GPUPolicy benchmarks
// ============================================================================

static void BM_Policy_CheckAllowed(benchmark::State& state) {
    GPUPolicy policy({"bench_caller"});  // pre-granted
    for (auto _ : state) {
        auto d = policy.check("bench_caller", GPUPolicy::Capability::GPU_ALLOCATE);
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Policy_CheckAllowed)->Unit(benchmark::kNanosecond);

static void BM_Policy_CheckDenied(benchmark::State& state) {
    GPUPolicy policy;  // default deny
    for (auto _ : state) {
        auto d = policy.check("unknown_caller", GPUPolicy::Capability::GPU_ALLOCATE);
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Policy_CheckDenied)->Unit(benchmark::kNanosecond);

// ============================================================================
// GPUConfig benchmarks
// ============================================================================

static void BM_Config_Validate(benchmark::State& state) {
    GPUConfig cfg;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    for (auto _ : state) {
        auto r = cfg.validate();
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Config_Validate)->Unit(benchmark::kNanosecond);

static void BM_Config_SimulateAllocation(benchmark::State& state) {
    GPUConfig cfg;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    const uint64_t current = 512ULL * 1024 * 1024;
    const uint64_t request = static_cast<uint64_t>(state.range(0)) * 1024 * 1024;
    for (auto _ : state) {
        auto [ok, reason] = cfg.simulateAllocation(request, current);
        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(reason);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Config_SimulateAllocation)
    ->Arg(64)    // 64 MB — accepted
    ->Arg(4096)  // 4 GB — likely rejected
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// GPUAdminAPI benchmarks
// ============================================================================

static void BM_AdminAPI_GetStatsJson(benchmark::State& state) {
    GPUConfig cfg;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    GPUAdminAPI api(cfg);
    for (auto _ : state) {
        auto json = api.getStatsJson();
        benchmark::DoNotOptimize(json);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AdminAPI_GetStatsJson)->Unit(benchmark::kMicrosecond);

static void BM_AdminAPI_SimulateJson(benchmark::State& state) {
    GPUConfig cfg;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    GPUAdminAPI api(cfg);
    const uint64_t request = 256ULL * 1024 * 1024;
    for (auto _ : state) {
        auto json = api.simulateJson(request);
        benchmark::DoNotOptimize(json);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AdminAPI_SimulateJson)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Concurrent contention benchmark: N threads hammering TryAllocateGPU
// ============================================================================

static void BM_MemoryManager_ConcurrentAlloc(benchmark::State& state) {
    auto& mgr = GPUMemoryManager::GetInstance();
    for (auto _ : state) {
        bool ok = mgr.TryAllocateGPU(4096, "concurrent_bench");
        benchmark::DoNotOptimize(ok);
        if (ok) mgr.DeallocateGPU(4096);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryManager_ConcurrentAlloc)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
