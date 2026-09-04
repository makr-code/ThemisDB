#include <benchmark/benchmark.h>
#include <performance/lirs_cache.h>
#include <random>
#include <string>
#include <iostream>

using namespace themis::performance;

// Benchmark: Simple sequential access
static void BM_SequentialAccess(benchmark::State& state) {
    const size_t cache_size = state.range(0);
    LIRSCache<int, std::string> cache(cache_size, 0.9);
    
    // Pre-fill cache
    for (size_t i = 0; i < cache_size; i++) {
        cache.put(i, "value" + std::to_string(i));
    }
    
    std::string value = {};
    size_t key = 0;
    
    for (auto _ : state) {
        cache.get(key % cache_size, value);
        key++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["HitRate"] = cache.get_hit_rate();
}
BENCHMARK(BM_SequentialAccess)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Random access (80/20 rule - Pareto distribution)
static void BM_RandomAccess_8020(benchmark::State& state) {
    const size_t cache_size = state.range(0);
    LIRSCache<int, std::string> cache(cache_size, 0.9);
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Pre-fill cache
    for (size_t i = 0; i < cache_size * 2; i++) {
        cache.put(i, "value" + std::to_string(i));
    }
    
    std::string value = {};
    
    for (auto _ : state) {
        // 80% of accesses go to 20% of keys (hot set)
        int key = 0;
        if (dis(gen) < 0.8) {
            key = gen() % (cache_size / 5);  // Hot 20%
        } else {
            key = cache_size / 5 + gen() % (cache_size * 8 / 5);  // Cold 80%
        }
        
        cache.get(key, value);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["HitRate"] = cache.get_hit_rate();
}
BENCHMARK(BM_RandomAccess_8020)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Scan resistance test
static void BM_ScanResistance(benchmark::State& state) {
    const size_t cache_size = 1000;
    LIRSCache<int, std::string> cache(cache_size, 0.9);
    
    // Create hot working set
    for (int i = 0; i < 100; i++) {
        cache.put(i, "hot" + std::to_string(i));
    }
    
    // Access hot set multiple times
    std::string value = {};
    for (int iter = 0; iter < 10; iter++) {
        for (int i = 0; i < 100; i++) {
            cache.get(i, value);
        }
    }
    
    size_t scan_offset = 1000;
    
    for (auto _ : state) {
        // Simulate scan: sequential access of cold data
        for (int i = 0; i < 100; i++) {
            cache.put(scan_offset + i, "scan" + std::to_string(i));
        }
        scan_offset += 100;
        
        // Check if hot data still accessible
        int hot_hits = 0;
        for (int i = 0; i < 100; i++) {
            if (cache.get(i, value)) {
                hot_hits++;
            }
        }
        
        benchmark::DoNotOptimize(hot_hits);
    }
    
    state.counters["HitRate"] = cache.get_hit_rate();
}
BENCHMARK(BM_ScanResistance);

// Benchmark: Mixed workload (reads and writes)
static void BM_MixedWorkload(benchmark::State& state) {
    const size_t cache_size = state.range(0);
    const double read_ratio = 0.8;  // 80% reads, 20% writes
    
    LIRSCache<int, std::string> cache(cache_size, 0.9);
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::uniform_int_distribution<> key_dis(0, cache_size * 2);
    
    std::string value = {};
    
    for (auto _ : state) {
        int key = key_dis(gen);
        
        if (dis(gen) < read_ratio) {
            // Read
            cache.get(key, value);
            benchmark::DoNotOptimize(value);
        } else {
            // Write
            cache.put(key, "value" + std::to_string(key));
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["HitRate"] = cache.get_hit_rate();
}
BENCHMARK(BM_MixedWorkload)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Multi-threaded concurrent access
static void BM_ConcurrentAccess(benchmark::State& state) {
    static LIRSCache<int, std::string> cache(1000, 0.9);
    
    // Pre-fill cache (only once)
    static std::once_flag fill_flag;
    std::call_once(fill_flag, []() {
        for (int i = 0; i < 1000; i++) {
            cache.put(i, "value" + std::to_string(i));
        }
    });
    
    std::mt19937 gen(state.thread_index());
    std::uniform_int_distribution<> key_dis(0, 999);
    std::string value = {};
    
    for (auto _ : state) {
        int key = key_dis(gen);
        cache.get(key, value);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentAccess)->ThreadRange(1, 8);

// Benchmark: LIR promotion overhead
static void BM_LIRPromotion(benchmark::State& state) {
    const size_t cache_size = 1000;
    LIRSCache<int, std::string> cache(cache_size, 0.7);  // 70% LIR, 30% HIR
    
    // Fill cache
    for (size_t i = 0; i < cache_size; i++) {
        cache.put(i, "value" + std::to_string(i));
    }
    
    std::string value = {};
    int key = 0;
    
    for (auto _ : state) {
        // Access pattern that triggers LIR/HIR transitions
        cache.get(key % cache_size, value);
        
        // Access multiple times to trigger promotion
        if (key % 3 == 0) {
            cache.get(key % cache_size, value);
            cache.get(key % cache_size, value);
        }
        
        key++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["LIR"] = cache.get_lir_count();
    state.counters["HIR"] = cache.get_hir_count();
}
BENCHMARK(BM_LIRPromotion);

BENCHMARK_MAIN();
