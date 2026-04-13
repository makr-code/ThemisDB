/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_multi_lora_fusion.cpp                        ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:11:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     446                                            ║
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
 * ThemisDB Multi-LoRA Fusion Benchmark
 * 
 * Performance benchmarks comparing:
 * - Base model inference
 * - Single LoRA inference
 * - Multi-LoRA fusion (2-3 adapters)
 * - Dynamic fusion with weight updates
 * - Cached vs uncached fusion
 */

#include <benchmark/benchmark.h>
#include "llm/multi_lora_manager.h"
#include "llm/llm_plugin_interface.h"
#include <vector>
#include <random>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Benchmark Fixtures
// ═══════════════════════════════════════════════════════════

class MultiLoRAFusionFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        config_.max_lora_vram_mb = 2048;
        config_.max_lora_slots = 32;
        config_.lora_ttl = std::chrono::seconds(3600);
        config_.enable_multi_lora_batch = true;
        config_.enable_adapter_fusion = true;
        
        manager_ = std::make_unique<MultiLoRAManager>(config_);
        
        // Load base LoRAs for benchmarking
        manager_->loadLoRA("bench-lora-1", "/path/to/bench1.bin", "base-model", 1.0f);
        manager_->loadLoRA("bench-lora-2", "/path/to/bench2.bin", "base-model", 1.0f);
        manager_->loadLoRA("bench-lora-3", "/path/to/bench3.bin", "base-model", 1.0f);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        manager_.reset();
    }
    
protected:
    MultiLoRAManager::Config config_;
    std::unique_ptr<MultiLoRAManager> manager_;
};

// ═══════════════════════════════════════════════════════════
// Fusion Operation Benchmarks
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(MultiLoRAFusionFixture, FusionTwoAdapters)(benchmark::State& state) {
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = false;  // Disable cache to measure fusion time
    
    size_t iteration = 0;
    for (auto _ : state) {
        std::string fusion_id = "fusion-2-" + std::to_string(iteration++);
        bool fused = manager_->fuseLoRAsAdvanced(fusion_id, config);
        benchmark::DoNotOptimize(fused);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("2 adapters");
}

BENCHMARK_F(MultiLoRAFusionFixture, FusionThreeAdapters)(benchmark::State& state) {
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2", "bench-lora-3"};
    config.weights = {0.33f, 0.33f, 0.34f};
    config.enable_cache = false;
    
    size_t iteration = 0;
    for (auto _ : state) {
        std::string fusion_id = "fusion-3-" + std::to_string(iteration++);
        bool fused = manager_->fuseLoRAsAdvanced(fusion_id, config);
        benchmark::DoNotOptimize(fused);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("3 adapters");
}

BENCHMARK_F(MultiLoRAFusionFixture, FusionWithCache)(benchmark::State& state) {
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    config.cache_ttl = std::chrono::seconds(3600);
    
    for (auto _ : state) {
        // Same fusion ID - should hit cache after first iteration
        bool fused = manager_->fuseLoRAsAdvanced("cached-fusion", config);
        benchmark::DoNotOptimize(fused);
    }
    
    auto metrics = manager_->getFusionMetrics();
    state.counters["cache_hits"] = metrics.cache_hits;
    state.counters["cache_misses"] = metrics.cache_misses;
    
    if (metrics.cache_hits + metrics.cache_misses > 0) {
        state.counters["hit_rate"] = 
            static_cast<double>(metrics.cache_hits) / 
            (metrics.cache_hits + metrics.cache_misses);
    }
    
    state.SetLabel("with cache");
}

BENCHMARK_F(MultiLoRAFusionFixture, DynamicFusionWeightUpdate)(benchmark::State& state) {
    // Setup initial fusion
    FusionConfig config;
    config.strategy = FusionStrategy::DYNAMIC;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    
    manager_->fuseLoRAsAdvanced("dynamic-bench", config);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (auto _ : state) {
        // Generate random weights
        float w1 = dist(gen);
        float w2 = 1.0f - w1;
        
        bool updated = manager_->updateFusionWeights("dynamic-bench", {w1, w2});
        benchmark::DoNotOptimize(updated);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("weight updates");
}

// ═══════════════════════════════════════════════════════════
// Cache Operations Benchmarks
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(MultiLoRAFusionFixture, CacheInvalidation)(benchmark::State& state) {
    // Create multiple fusion entries
    for (int i = 0; i < 10; ++i) {
        FusionConfig config;
        config.strategy = FusionStrategy::STATIC;
        config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
        config.weights = {0.5f, 0.5f};
        config.enable_cache = true;
        
        manager_->fuseLoRAsAdvanced("cache-inv-" + std::to_string(i), config);
    }
    
    size_t idx = 0;
    for (auto _ : state) {
        std::string fusion_id = "cache-inv-" + std::to_string(idx % 10);
        bool invalidated = manager_->invalidateFusionCache(fusion_id);
        benchmark::DoNotOptimize(invalidated);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(MultiLoRAFusionFixture, ListFusionCache)(benchmark::State& state) {
    // Create fusion entries
    for (int i = 0; i < 20; ++i) {
        FusionConfig config;
        config.strategy = FusionStrategy::STATIC;
        config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
        config.weights = {0.5f, 0.5f};
        config.enable_cache = true;
        
        manager_->fuseLoRAsAdvanced("list-bench-" + std::to_string(i), config);
    }
    
    for (auto _ : state) {
        auto entries = manager_->listFusionCache();
        benchmark::DoNotOptimize(entries);
    }
    
    state.SetItemsProcessed(state.iterations());
    auto entries = manager_->listFusionCache();
    state.counters["cache_size"] = entries.size();
}

// ═══════════════════════════════════════════════════════════
// Compatibility Check Benchmarks
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(MultiLoRAFusionFixture, CompatibilityCheck)(benchmark::State& state) {
    FusionConfig config;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2", "bench-lora-3"};
    config.enforce_quantization_match = true;
    config.enforce_gpu_placement_match = false;
    config.enforce_rank_match = false;
    
    for (auto _ : state) {
        bool compatible = manager_->checkFusionCompatibility(
            config.source_lora_ids, config
        );
        benchmark::DoNotOptimize(compatible);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("3 adapters");
}

// ═══════════════════════════════════════════════════════════
// Scheduled Fusion Benchmarks
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(MultiLoRAFusionFixture, ScheduledWeightsComputation)(benchmark::State& state) {
    // Setup scheduled fusion
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
    config.weights = {0.5f, 0.5f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.transition_duration = std::chrono::seconds(60);
    schedule.static_weights = {0.8f, 0.2f};
    schedule.a_weight = 0.8f;
    schedule.b_weight = 0.2f;
    
    config.alpha_schedule = schedule;
    
    manager_->fuseLoRAsAdvanced("scheduled-bench", config);
    manager_->setAlphaSchedule("scheduled-bench", schedule);
    
    for (auto _ : state) {
        auto weights = manager_->getCurrentFusionWeights("scheduled-bench");
        benchmark::DoNotOptimize(weights);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(MultiLoRAFusionFixture, ScheduledCustomFunction)(benchmark::State& state) {
    // Setup scheduled fusion with custom function
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
    config.weights = {0.5f, 0.5f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    
    // Sine wave scheduling
    schedule.schedule_func = [](double time_offset) -> std::vector<float> {
        float phase = std::sin(time_offset / 10.0);
        float w1 = 0.5f + 0.5f * phase;
        float w2 = 1.0f - w1;
        return {w1, w2};
    };
    
    config.alpha_schedule = schedule;
    
    manager_->fuseLoRAsAdvanced("custom-func-bench", config);
    manager_->setAlphaSchedule("custom-func-bench", schedule);
    
    for (auto _ : state) {
        auto weights = manager_->getCurrentFusionWeights("custom-func-bench");
        benchmark::DoNotOptimize(weights);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ═══════════════════════════════════════════════════════════
// Metrics Collection Benchmarks
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(MultiLoRAFusionFixture, GetFusionMetrics)(benchmark::State& state) {
    // Create some fusion activity
    for (int i = 0; i < 5; ++i) {
        FusionConfig config;
        config.strategy = FusionStrategy::STATIC;
        config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
        config.weights = {0.5f, 0.5f};
        config.enable_cache = true;
        
        manager_->fuseLoRAsAdvanced("metrics-" + std::to_string(i), config);
    }
    
    for (auto _ : state) {
        auto metrics = manager_->getFusionMetrics();
        benchmark::DoNotOptimize(metrics);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ═══════════════════════════════════════════════════════════
// Comparative Benchmarks: Fusion Strategies
// ═══════════════════════════════════════════════════════════

static void BM_FusionStrategy(benchmark::State& state, FusionStrategy strategy) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 2048;
    config.max_lora_slots = 32;
    config.enable_adapter_fusion = true;
    
    MultiLoRAManager manager(config);
    
    // Load LoRAs
    manager.loadLoRA("strat-1", "/path/to/1.bin", "base", 1.0f);
    manager.loadLoRA("strat-2", "/path/to/2.bin", "base", 1.0f);
    
    FusionConfig fusion_config;
    fusion_config.strategy = strategy;
    fusion_config.source_lora_ids = {"strat-1", "strat-2"};
    fusion_config.weights = {0.5f, 0.5f};
    fusion_config.enable_cache = (strategy == FusionStrategy::STATIC);
    
    size_t iteration = 0;
    for (auto _ : state) {
        std::string id = "strat-fusion-" + std::to_string(iteration++);
        bool fused = manager.fuseLoRAsAdvanced(id, fusion_config);
        benchmark::DoNotOptimize(fused);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    std::string label;
    switch (strategy) {
        case FusionStrategy::STATIC: label = "STATIC"; break;
        case FusionStrategy::DYNAMIC: label = "DYNAMIC"; break;
        case FusionStrategy::SCHEDULED: label = "SCHEDULED"; break;
    }
    state.SetLabel(label);
}

BENCHMARK_CAPTURE(BM_FusionStrategy, Static, FusionStrategy::STATIC);
BENCHMARK_CAPTURE(BM_FusionStrategy, Dynamic, FusionStrategy::DYNAMIC);
BENCHMARK_CAPTURE(BM_FusionStrategy, Scheduled, FusionStrategy::SCHEDULED);

// ═══════════════════════════════════════════════════════════
// Scalability Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_FusionScalability(benchmark::State& state) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 4096;
    config.max_lora_slots = 64;
    config.enable_adapter_fusion = true;
    
    MultiLoRAManager manager(config);
    
    int num_adapters = state.range(0);
    
    // Load required number of LoRAs
    for (int i = 0; i < num_adapters; ++i) {
        std::string id = "scale-lora-" + std::to_string(i);
        manager.loadLoRA(id, "/path/to/" + std::to_string(i) + ".bin", "base", 1.0f);
    }
    
    // Prepare fusion config
    FusionConfig fusion_config;
    fusion_config.strategy = FusionStrategy::STATIC;
    fusion_config.enable_cache = false;
    
    for (int i = 0; i < num_adapters; ++i) {
        fusion_config.source_lora_ids.push_back("scale-lora-" + std::to_string(i));
        fusion_config.weights.push_back(1.0f / num_adapters);
    }
    
    size_t iteration = 0;
    for (auto _ : state) {
        std::string fusion_id = "scale-fusion-" + std::to_string(iteration++);
        bool fused = manager.fuseLoRAsAdvanced(fusion_id, fusion_config);
        benchmark::DoNotOptimize(fused);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(num_adapters);
}

BENCHMARK(BM_FusionScalability)
    ->RangeMultiplier(2)
    ->Range(2, 16)
    ->Complexity();

// ═══════════════════════════════════════════════════════════
// Memory Efficiency Benchmark
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(MultiLoRAFusionFixture, CacheClearPerformance)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Create many fusion entries
        for (int i = 0; i < 100; ++i) {
            FusionConfig config;
            config.strategy = FusionStrategy::STATIC;
            config.source_lora_ids = {"bench-lora-1", "bench-lora-2"};
            config.weights = {0.5f, 0.5f};
            config.enable_cache = true;
            
            manager_->fuseLoRAsAdvanced("clear-" + std::to_string(i), config);
        }
        
        state.ResumeTiming();
        
        // Measure clear time
        size_t cleared = manager_->clearFusionCache();
        benchmark::DoNotOptimize(cleared);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}

// Run all benchmarks
BENCHMARK_MAIN();
