/**
 * @file bench_lora_auto_binding.cpp
 * @brief Performance benchmarks for LoRA auto-binding and lifecycle management
 * 
 * Measures performance of the newly implemented features:
 * - Auto-binding overhead during inference
 * - Context switch detection and rebinding
 * - Adapter reuse optimization
 * - TTL-based eviction
 * - Memory pressure handling
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "benchmark_artifact_preflight.h"
#include "llm/multi_lora_manager.h"
#include <chrono>
#include <string>
#include <vector>
#include <thread>

using namespace themis::llm;

namespace {

// ═══════════════════════════════════════════════════════════
// Configuration Helpers
// ═══════════════════════════════════════════════════════════

MultiLoRAManager::Config createBenchConfig(size_t slots = 8, size_t vram_mb = 512) {
    MultiLoRAManager::Config cfg;
    cfg.max_lora_vram_mb = vram_mb;
    cfg.max_lora_slots = slots;
    cfg.enable_multi_lora_batch = true;
    cfg.lora_ttl = std::chrono::seconds(60);  // 60s TTL for benchmarks
    cfg.enable_lazy_load = true;
    return cfg;
}

// Mock context pointers for benchmarking
void* getMockContext(int id) {
    return reinterpret_cast<void*>(static_cast<uintptr_t>(0x1000 + id * 0x100));
}

// ═══════════════════════════════════════════════════════════
// Auto-Binding Overhead Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Benchmark: Measure overhead of applying adapter for the first time
 * Target: <10ms as per requirements
 */
static void BM_AutoBinding_FirstApplication(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    void* ctx = getMockContext(1);
    
    // Pre-load adapter
    mgr.loadLoRA("test-adapter", lora_path, base_model, 1.0f);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        mgr.applyLoRA("test-adapter", reinterpret_cast<llama_context*>(ctx));
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        // Remove for next iteration
        mgr.removeLoRA("test-adapter", reinterpret_cast<llama_context*>(ctx));
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("First application overhead");
}
BENCHMARK(BM_AutoBinding_FirstApplication)->UseManualTime();

/**
 * Benchmark: Measure overhead when adapter is already applied (reuse path)
 * Target: <1ms for intelligent reuse
 */
static void BM_AutoBinding_ReuseOptimization(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    void* ctx = getMockContext(1);
    
    // Pre-load and apply adapter
    mgr.loadLoRA("test-adapter", lora_path, base_model, 1.0f);
    mgr.applyLoRA("test-adapter", reinterpret_cast<llama_context*>(ctx));
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        // Check if already applied (simulates intelligent reuse)
        auto* lora = mgr.getLoRA("test-adapter");
        if (lora && lora->is_active) {
            benchmark::DoNotOptimize(lora);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e9);
        
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("Reuse path overhead");
}
BENCHMARK(BM_AutoBinding_ReuseOptimization)->UseManualTime();

/**
 * Benchmark: Measure adapter switching overhead
 * Target: <10ms for context switch
 */
static void BM_AutoBinding_AdapterSwitching(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    void* ctx = getMockContext(1);
    
    // Pre-load two adapters (same stub file, different logical names)
    mgr.loadLoRA("adapter-a", lora_path, base_model, 1.0f);
    mgr.loadLoRA("adapter-b", lora_path, base_model, 1.0f);
    
    bool use_a = true;
    for (auto _ : state) {
        const std::string& adapter_id = use_a ? "adapter-a" : "adapter-b";
        
        auto start = std::chrono::high_resolution_clock::now();
        mgr.applyLoRA(adapter_id, reinterpret_cast<llama_context*>(ctx));
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        use_a = !use_a;
        benchmark::ClobberMemory();
    }
    
    auto stats = mgr.getStatistics();
    state.counters["switches"] = static_cast<double>(stats.switches);
    state.SetLabel("Adapter switching");
}
BENCHMARK(BM_AutoBinding_AdapterSwitching)->UseManualTime();

// ═══════════════════════════════════════════════════════════
// Context Switch Detection Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Benchmark: Context pointer comparison overhead
 * Target: O(1), negligible overhead
 */
static void BM_ContextSwitch_Detection(benchmark::State& state) {
    void* last_context = getMockContext(1);
    void* current_context = getMockContext(2);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        bool context_changed = (last_context != current_context);
        benchmark::DoNotOptimize(context_changed);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e9);
        
        // Alternate contexts
        if (context_changed) {
            last_context = current_context;
        }
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("Pointer comparison");
}
BENCHMARK(BM_ContextSwitch_Detection)->UseManualTime();

/**
 * Benchmark: Full context switch handling with rebinding
 * Target: <10ms for detection + rebinding
 */
static void BM_ContextSwitch_Rebinding(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    
    // Pre-load adapter
    mgr.loadLoRA("test-adapter", lora_path, base_model, 1.0f);
    
    void* ctx1 = getMockContext(1);
    void* ctx2 = getMockContext(2);
    bool use_ctx1 = true;
    
    for (auto _ : state) {
        void* current_ctx = use_ctx1 ? ctx1 : ctx2;
        
        auto start = std::chrono::high_resolution_clock::now();
        // Simulate context switch detection and rebinding
        mgr.removeLoRA("test-adapter", reinterpret_cast<llama_context*>(current_ctx));
        mgr.applyLoRA("test-adapter", reinterpret_cast<llama_context*>(current_ctx));
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        use_ctx1 = !use_ctx1;
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("Full rebinding");
}
BENCHMARK(BM_ContextSwitch_Rebinding)->UseManualTime();

// ═══════════════════════════════════════════════════════════
// Cache Performance Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Benchmark: Cache hit rate with intelligent reuse
 * Measures: Average hit rate over multiple requests
 */
static void BM_Cache_HitRate(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    
    // Pre-load adapters (same stub file, different logical names)
    std::vector<std::string> adapter_ids;
    for (int i = 0; i < 5; ++i) {
        std::string id = "adapter-" + std::to_string(i);
        mgr.loadLoRA(id, lora_path, base_model, 1.0f);
        adapter_ids.push_back(id);
    }
    
    size_t idx = 0;
    for (auto _ : state) {
        // Access pattern: 80% reuse, 20% different
        const std::string& adapter_id = (idx % 5 < 4) 
            ? adapter_ids[0]  // 80% on first adapter
            : adapter_ids[idx % adapter_ids.size()];  // 20% distributed
        
        auto* lora = mgr.getLoRA(adapter_id);
        benchmark::DoNotOptimize(lora);
        ++idx;
    }
    
    auto stats = mgr.getStatistics();
    double hit_rate = (stats.cache_hits + stats.cache_misses > 0)
        ? static_cast<double>(stats.cache_hits) / (stats.cache_hits + stats.cache_misses)
        : 0.0;
    
    state.counters["hit_rate"] = hit_rate;
    state.counters["hits"] = static_cast<double>(stats.cache_hits);
    state.counters["misses"] = static_cast<double>(stats.cache_misses);
}
BENCHMARK(BM_Cache_HitRate);

/**
 * Benchmark: LRU eviction performance
 * Measures: Time to evict LRU adapter
 */
static void BM_Cache_LRUEviction(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager::Config cfg = createBenchConfig(3, 256);  // Small cache
    MultiLoRAManager mgr(cfg);
    const std::string base_model = "bench-model";
    
    for (auto _ : state) {
        // Fill cache (same stub file, different logical names)
        for (int i = 0; i < 3; ++i) {
            mgr.loadLoRA("adapter-" + std::to_string(i), lora_path, base_model, 1.0f);
        }
        
        // Trigger eviction by loading 4th adapter
        auto start = std::chrono::high_resolution_clock::now();
        mgr.loadLoRA("adapter-new", lora_path, base_model, 1.0f);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        // Cleanup for next iteration
        for (int i = 0; i < 4; ++i) {
            mgr.unloadLoRA("adapter-" + std::to_string(i), true);
        }
        mgr.unloadLoRA("adapter-new", true);
        
        benchmark::ClobberMemory();
    }
    
    auto stats = mgr.getStatistics();
    state.counters["evictions"] = static_cast<double>(stats.evictions);
    state.SetLabel("LRU eviction time");
}
BENCHMARK(BM_Cache_LRUEviction)->UseManualTime();

// ═══════════════════════════════════════════════════════════
// Memory Management Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Benchmark: Memory statistics retrieval
 * Measures: Overhead of checking memory usage
 */
static void BM_Memory_StatsRetrieval(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    
    // Pre-load some adapters
    for (int i = 0; i < 3; ++i) {
        mgr.loadLoRA("adapter-" + std::to_string(i), lora_path, base_model, 1.0f);
    }
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto stats = mgr.getMemoryStats();
        benchmark::DoNotOptimize(stats);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e9);
    }
    
    state.SetLabel("Memory stats overhead");
}
BENCHMARK(BM_Memory_StatsRetrieval)->UseManualTime();

/**
 * Benchmark: Proactive eviction on memory pressure
 * Simulates: >80% VRAM usage triggering eviction
 */
static void BM_Memory_PressureEviction(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager::Config cfg = createBenchConfig(10, 100);  // Small VRAM budget
    MultiLoRAManager mgr(cfg);
    const std::string base_model = "bench-model";
    
    for (auto _ : state) {
        // Load adapters until memory pressure
        for (int i = 0; i < 8; ++i) {
            mgr.loadLoRA("adapter-" + std::to_string(i), lora_path, base_model, 1.0f);
        }
        
        // Trigger memory pressure eviction
        auto start = std::chrono::high_resolution_clock::now();
        auto mem_stats = mgr.getMemoryStats();
        size_t vram_usage_pct = mem_stats["vram_usage_pct"].get<double>();
        if (vram_usage_pct > 80) {
            mgr.evictLRU();
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        // Cleanup
        for (int i = 0; i < 8; ++i) {
            mgr.unloadLoRA("adapter-" + std::to_string(i), true);
        }
        
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("Pressure eviction");
}
BENCHMARK(BM_Memory_PressureEviction)->UseManualTime();

// ═══════════════════════════════════════════════════════════
// Adapter Pinning Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Benchmark: Pinning overhead
 * Measures: Cost of marking adapter as pinned
 */
static void BM_Pinning_PinUnpin(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig());
    const std::string base_model = "bench-model";
    
    mgr.loadLoRA("test-adapter", lora_path, base_model, 1.0f);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        mgr.pinLoRA("test-adapter");
        mgr.unpinLoRA("test-adapter");
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e9);
    }
    
    state.SetLabel("Pin/unpin overhead");
}
BENCHMARK(BM_Pinning_PinUnpin)->UseManualTime();

/**
 * Benchmark: Eviction with pinned adapters
 * Measures: Performance when some adapters are protected
 */
static void BM_Pinning_EvictionProtection(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager::Config cfg = createBenchConfig(4, 256);
    MultiLoRAManager mgr(cfg);
    const std::string base_model = "bench-model";
    
    for (auto _ : state) {
        // Load and pin 2 adapters (same stub file, different logical names)
        mgr.loadLoRA("pinned-1", lora_path, base_model, 1.0f);
        mgr.pinLoRA("pinned-1");
        mgr.loadLoRA("pinned-2", lora_path, base_model, 1.0f);
        mgr.pinLoRA("pinned-2");
        
        // Load 2 unpinned adapters
        mgr.loadLoRA("temp-1", lora_path, base_model, 1.0f);
        mgr.loadLoRA("temp-2", lora_path, base_model, 1.0f);
        
        // Try to load 5th adapter - should evict unpinned
        auto start = std::chrono::high_resolution_clock::now();
        mgr.loadLoRA("new-adapter", lora_path, base_model, 1.0f);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        // Cleanup
        mgr.unloadLoRA("pinned-1", true);
        mgr.unloadLoRA("pinned-2", true);
        mgr.unloadLoRA("new-adapter", true);
        
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("Eviction with pinning");
}
BENCHMARK(BM_Pinning_EvictionProtection)->UseManualTime();

// ═══════════════════════════════════════════════════════════
// Comprehensive End-to-End Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Benchmark: Complete lifecycle (load → apply → remove → unload)
 * Measures: Total overhead for full lifecycle
 */
static void BM_Lifecycle_Complete(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    void* ctx = getMockContext(1);
    
    for (auto _ : state) {
        MultiLoRAManager mgr(createBenchConfig());
        const std::string base_model = "bench-model";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Load
        mgr.loadLoRA("test-adapter", lora_path, base_model, 1.0f);
        
        // Apply
        mgr.applyLoRA("test-adapter", reinterpret_cast<llama_context*>(ctx));
        
        // Remove
        mgr.removeLoRA("test-adapter", reinterpret_cast<llama_context*>(ctx));
        
        // Unload
        mgr.unloadLoRA("test-adapter", true);
        
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1e6);
        
        benchmark::ClobberMemory();
    }
    
    state.SetLabel("Full lifecycle");
}
BENCHMARK(BM_Lifecycle_Complete)->UseManualTime();

/**
 * Benchmark: Throughput with multiple adapters
 * Measures: Operations per second with realistic workload
 */
static void BM_Throughput_MultiAdapter(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveLoraPath(), "LoRA adapter");
    const std::string lora_path = themis::bench::resolveLoraPath();
    MultiLoRAManager mgr(createBenchConfig(8, 512));
    const std::string base_model = "bench-model";
    void* ctx = getMockContext(1);
    
    // Pre-load adapters (same stub file, different logical names)
    std::vector<std::string> adapters;
    for (int i = 0; i < 5; ++i) {
        std::string id = "adapter-" + std::to_string(i);
        mgr.loadLoRA(id, lora_path, base_model, 1.0f);
        adapters.push_back(id);
    }
    
    size_t idx = 0;
    for (auto _ : state) {
        const std::string& adapter_id = adapters[idx++ % adapters.size()];
        
        // Simulate inference request with adapter
        mgr.applyLoRA(adapter_id, reinterpret_cast<llama_context*>(ctx));
        benchmark::DoNotOptimize(mgr.getLoRA(adapter_id));
        mgr.removeLoRA(adapter_id, reinterpret_cast<llama_context*>(ctx));
        
        benchmark::ClobberMemory();
    }
    
    auto stats = mgr.getStatistics();
    state.counters["ops_per_sec"] = benchmark::Counter(
        state.iterations(), 
        benchmark::Counter::kIsRate
    );
    state.counters["hit_rate"] = (stats.cache_hits + stats.cache_misses > 0)
        ? static_cast<double>(stats.cache_hits) / (stats.cache_hits + stats.cache_misses)
        : 0.0;
}
BENCHMARK(BM_Throughput_MultiAdapter);

} // anonymous namespace

BENCHMARK_MAIN();
