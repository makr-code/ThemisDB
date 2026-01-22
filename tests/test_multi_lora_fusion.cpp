/**
 * ThemisDB Multi-LoRA Adapter Fusion Tests
 * 
 * Comprehensive tests for advanced adapter fusion including:
 * - Dynamic fusion with weight updates
 * - Alpha scheduling for A/B testing
 * - Fusion cache with invalidation
 * - Compatibility validation
 * - Performance benchmarks
 */

#include <gtest/gtest.h>
#include "llm/multi_lora_manager.h"
#include "llm/llm_plugin_interface.h"
#include <thread>
#include <chrono>
#include <algorithm>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class MultiLoRAFusionTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_lora_vram_mb = 512;
        config_.max_lora_slots = 16;
        config_.lora_ttl = std::chrono::seconds(300);
        config_.enable_multi_lora_batch = true;
        config_.enable_adapter_fusion = true;
    }
    
    MultiLoRAManager::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Static Fusion Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, StaticFusionBasic) {
    MultiLoRAManager manager(config_);
    
    // Load source LoRAs
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base-model", 1.0f);
    
    // Create static fusion config
    FusionConfig fusion_config;
    fusion_config.strategy = FusionStrategy::STATIC;
    fusion_config.source_lora_ids = {"lora-a", "lora-b"};
    fusion_config.weights = {0.6f, 0.4f};
    fusion_config.enable_cache = true;
    
    bool fused = manager.fuseLoRAsAdvanced("static-fusion", fusion_config);
    EXPECT_TRUE(fused);
    
    // Verify fusion is loaded
    EXPECT_TRUE(manager.isLoRALoaded("static-fusion"));
}

TEST_F(MultiLoRAFusionTest, StaticFusionCacheHit) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-1", "/path/to/1.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-2", "/path/to/2.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"lora-1", "lora-2"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    config.cache_ttl = std::chrono::seconds(60);
    
    // First fusion - cache miss
    bool first = manager.fuseLoRAsAdvanced("cached-fusion", config);
    EXPECT_TRUE(first);
    
    auto metrics_before = manager.getFusionMetrics();
    size_t cache_misses_before = metrics_before.cache_misses;
    
    // Second fusion with same ID - should hit cache
    bool second = manager.fuseLoRAsAdvanced("cached-fusion", config);
    EXPECT_TRUE(second);
    
    auto metrics_after = manager.getFusionMetrics();
    EXPECT_GT(metrics_after.cache_hits, metrics_before.cache_hits);
}

TEST_F(MultiLoRAFusionTest, StaticFusionCacheExpiry) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-x", "/path/to/x.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-y", "/path/to/y.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"lora-x", "lora-y"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    config.cache_ttl = std::chrono::seconds(1);  // Very short TTL
    
    // First fusion
    manager.fuseLoRAsAdvanced("expiry-test", config);
    
    // Wait for cache to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto metrics_before = manager.getFusionMetrics();
    
    // Second fusion - cache should be expired
    manager.fuseLoRAsAdvanced("expiry-test", config);
    
    auto metrics_after = manager.getFusionMetrics();
    EXPECT_GT(metrics_after.cache_misses, metrics_before.cache_misses);
}

// ═══════════════════════════════════════════════════════════
// Dynamic Fusion Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, DynamicFusionWeightUpdate) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("dynamic-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("dynamic-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::DYNAMIC;
    config.source_lora_ids = {"dynamic-a", "dynamic-b"};
    config.weights = {0.7f, 0.3f};
    config.enable_cache = true;
    
    bool fused = manager.fuseLoRAsAdvanced("dynamic-fusion", config);
    EXPECT_TRUE(fused);
    
    // Get initial weights
    auto initial_weights = manager.getCurrentFusionWeights("dynamic-fusion");
    EXPECT_EQ(initial_weights.size(), 2);
    
    // Update weights
    std::vector<float> new_weights = {0.3f, 0.7f};
    bool updated = manager.updateFusionWeights("dynamic-fusion", new_weights);
    EXPECT_TRUE(updated);
    
    // Verify weights were updated
    auto current_weights = manager.getCurrentFusionWeights("dynamic-fusion");
    EXPECT_EQ(current_weights.size(), 2);
    EXPECT_NEAR(current_weights[0], 0.3f, 0.01f);
    EXPECT_NEAR(current_weights[1], 0.7f, 0.01f);
}

TEST_F(MultiLoRAFusionTest, DynamicFusionInvalidatesCache) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("cache-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("cache-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::DYNAMIC;
    config.source_lora_ids = {"cache-a", "cache-b"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    
    manager.fuseLoRAsAdvanced("cache-test", config);
    
    auto cache_before = manager.listFusionCache();
    size_t count_before = cache_before.size();
    
    // Update weights should invalidate cache
    manager.updateFusionWeights("cache-test", {0.4f, 0.6f});
    
    // Cache entry should be invalidated
    auto metrics = manager.getFusionMetrics();
    EXPECT_GT(metrics.invalidations, 0);
}

TEST_F(MultiLoRAFusionTest, CannotUpdateStaticFusion) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("static-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("static-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"static-a", "static-b"};
    config.weights = {0.5f, 0.5f};
    
    manager.fuseLoRAsAdvanced("static-test", config);
    
    // Should fail to update STATIC fusion
    bool updated = manager.updateFusionWeights("static-test", {0.3f, 0.7f});
    EXPECT_FALSE(updated);
}

// ═══════════════════════════════════════════════════════════
// Scheduled Fusion Tests (A/B Testing)
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, ScheduledFusionAlphaSchedule) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("schedule-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("schedule-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"schedule-a", "schedule-b"};
    config.weights = {0.5f, 0.5f};
    
    AlphaSchedule schedule;
    schedule.schedule_id = "ab-test";
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.static_weights = {0.9f, 0.1f};  // Start with 90/10
    schedule.a_weight = 0.9f;
    schedule.b_weight = 0.1f;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.transition_duration = std::chrono::seconds(10);
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("scheduled-fusion", config);
    EXPECT_TRUE(fused);
    
    // Set the schedule
    bool set = manager.setAlphaSchedule("scheduled-fusion", schedule);
    EXPECT_TRUE(set);
    
    // Get current weights (should follow schedule)
    auto weights = manager.getCurrentFusionWeights("scheduled-fusion");
    EXPECT_EQ(weights.size(), 2);
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionCustomFunction) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("func-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("func-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"func-a", "func-b"};
    config.weights = {0.5f, 0.5f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    
    // Custom scheduling function: sine wave oscillation
    schedule.schedule_func = [](double time_offset) -> std::vector<float> {
        float phase = std::sin(time_offset / 10.0);  // Oscillate over 20 seconds
        float weight_a = 0.5f + 0.5f * phase;
        float weight_b = 1.0f - weight_a;
        return {weight_a, weight_b};
    };
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("func-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("func-fusion", schedule);
    
    auto weights = manager.getCurrentFusionWeights("func-fusion");
    EXPECT_EQ(weights.size(), 2);
    EXPECT_NEAR(weights[0] + weights[1], 1.0f, 0.01f);
}

// ═══════════════════════════════════════════════════════════
// Cache Management Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, ManualCacheInvalidation) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("inv-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("inv-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"inv-a", "inv-b"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    
    manager.fuseLoRAsAdvanced("invalidate-test", config);
    
    auto cache_before = manager.listFusionCache();
    EXPECT_FALSE(cache_before.empty());
    
    // Manually invalidate
    bool invalidated = manager.invalidateFusionCache("invalidate-test");
    EXPECT_TRUE(invalidated);
    
    // Verify metrics updated
    auto metrics = manager.getFusionMetrics();
    EXPECT_GT(metrics.invalidations, 0);
}

TEST_F(MultiLoRAFusionTest, ClearAllFusionCache) {
    MultiLoRAManager manager(config_);
    
    // Create multiple fusions
    for (int i = 0; i < 5; ++i) {
        std::string lora_a = "clear-a-" + std::to_string(i);
        std::string lora_b = "clear-b-" + std::to_string(i);
        std::string fusion_id = "clear-fusion-" + std::to_string(i);
        
        manager.loadLoRA(lora_a, "/path/to/a" + std::to_string(i) + ".bin", "base", 1.0f);
        manager.loadLoRA(lora_b, "/path/to/b" + std::to_string(i) + ".bin", "base", 1.0f);
        
        FusionConfig config;
        config.strategy = FusionStrategy::STATIC;
        config.source_lora_ids = {lora_a, lora_b};
        config.weights = {0.5f, 0.5f};
        config.enable_cache = true;
        
        manager.fuseLoRAsAdvanced(fusion_id, config);
    }
    
    auto cache_before = manager.listFusionCache();
    EXPECT_EQ(cache_before.size(), 5);
    
    // Clear all
    size_t cleared = manager.clearFusionCache();
    EXPECT_EQ(cleared, 5);
    
    auto cache_after = manager.listFusionCache();
    EXPECT_TRUE(cache_after.empty());
}

TEST_F(MultiLoRAFusionTest, ListFusionCacheEntries) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("list-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("list-b", "/path/to/b.bin", "base-model", 1.0f);
    manager.loadLoRA("list-c", "/path/to/c.bin", "base-model", 1.0f);
    
    // Create multiple fusion entries
    FusionConfig config1;
    config1.strategy = FusionStrategy::STATIC;
    config1.source_lora_ids = {"list-a", "list-b"};
    config1.weights = {0.5f, 0.5f};
    config1.enable_cache = true;
    
    FusionConfig config2;
    config2.strategy = FusionStrategy::DYNAMIC;
    config2.source_lora_ids = {"list-b", "list-c"};
    config2.weights = {0.6f, 0.4f};
    config2.enable_cache = true;
    
    manager.fuseLoRAsAdvanced("list-fusion-1", config1);
    manager.fuseLoRAsAdvanced("list-fusion-2", config2);
    
    auto entries = manager.listFusionCache();
    EXPECT_EQ(entries.size(), 2);
    
    // Verify entry metadata
    bool found_static = false;
    bool found_dynamic = false;
    
    for (const auto& entry : entries) {
        if (entry.strategy == FusionStrategy::STATIC) {
            found_static = true;
            EXPECT_EQ(entry.source_lora_ids.size(), 2);
        }
        if (entry.strategy == FusionStrategy::DYNAMIC) {
            found_dynamic = true;
        }
    }
    
    EXPECT_TRUE(found_static);
    EXPECT_TRUE(found_dynamic);
}

// ═══════════════════════════════════════════════════════════
// Compatibility Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, CompatibilityCheckBaseModel) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("compat-a", "/path/to/a.bin", "model-1", 1.0f);
    manager.loadLoRA("compat-b", "/path/to/b.bin", "model-2", 1.0f);
    
    FusionConfig config;
    config.source_lora_ids = {"compat-a", "compat-b"};
    
    // Should fail due to different base models
    bool compatible = manager.checkFusionCompatibility(
        config.source_lora_ids, config
    );
    EXPECT_FALSE(compatible);
}

TEST_F(MultiLoRAFusionTest, CompatibilityCheckQuantization) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load one quantized, one not
    manager.loadLoRA("quant-yes", "/path/to/yes.bin", "base", true, 1.0f);
    manager.loadLoRA("quant-no", "/path/to/no.bin", "base", false, 1.0f);
    
    FusionConfig config;
    config.source_lora_ids = {"quant-yes", "quant-no"};
    config.enforce_quantization_match = true;
    
    // Should fail due to quantization mismatch
    bool compatible = manager.checkFusionCompatibility(
        config.source_lora_ids, config
    );
    EXPECT_FALSE(compatible);
}

TEST_F(MultiLoRAFusionTest, CompatibilityCheckPasses) {
    MultiLoRAManager manager(config_);
    
    // Load compatible LoRAs
    manager.loadLoRA("pass-a", "/path/to/a.bin", "same-model", 1.0f);
    manager.loadLoRA("pass-b", "/path/to/b.bin", "same-model", 1.0f);
    manager.loadLoRA("pass-c", "/path/to/c.bin", "same-model", 1.0f);
    
    FusionConfig config;
    config.source_lora_ids = {"pass-a", "pass-b", "pass-c"};
    config.enforce_quantization_match = true;
    config.enforce_rank_match = false;
    
    // Should pass
    bool compatible = manager.checkFusionCompatibility(
        config.source_lora_ids, config
    );
    EXPECT_TRUE(compatible);
}

TEST_F(MultiLoRAFusionTest, FusionFailsOnIncompatibleLoRAs) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("fail-a", "/path/to/a.bin", "model-x", 1.0f);
    manager.loadLoRA("fail-b", "/path/to/b.bin", "model-y", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"fail-a", "fail-b"};
    config.weights = {0.5f, 0.5f};
    
    // Fusion should fail
    bool fused = manager.fuseLoRAsAdvanced("fail-fusion", config);
    EXPECT_FALSE(fused);
}

// ═══════════════════════════════════════════════════════════
// Metrics and Performance Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, FusionMetricsTracking) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("metric-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("metric-b", "/path/to/b.bin", "base", 1.0f);
    
    auto metrics_before = manager.getFusionMetrics();
    size_t total_before = metrics_before.total_fusions;
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"metric-a", "metric-b"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    
    manager.fuseLoRAsAdvanced("metrics-test", config);
    
    auto metrics_after = manager.getFusionMetrics();
    EXPECT_GT(metrics_after.total_fusions, total_before);
    EXPECT_GT(metrics_after.cache_misses, metrics_before.cache_misses);
}

TEST_F(MultiLoRAFusionTest, CacheHitRateCalculation) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("hit-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("hit-b", "/path/to/b.bin", "base", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"hit-a", "hit-b"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;
    config.cache_ttl = std::chrono::seconds(60);
    
    // First call - miss
    manager.fuseLoRAsAdvanced("hitrate-test", config);
    
    // Multiple cache hits
    for (int i = 0; i < 10; ++i) {
        manager.fuseLoRAsAdvanced("hitrate-test", config);
    }
    
    auto metrics = manager.getFusionMetrics();
    EXPECT_GT(metrics.cache_hits, 0);
    EXPECT_GT(metrics.cache_misses, 0);
    
    // Cache hit rate should be high
    float hit_rate = static_cast<float>(metrics.cache_hits) / 
                    (metrics.cache_hits + metrics.cache_misses);
    EXPECT_GT(hit_rate, 0.8f);  // At least 80% hit rate
}

TEST_F(MultiLoRAFusionTest, FusionTimeTracking) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("time-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("time-b", "/path/to/b.bin", "base", 1.0f);
    manager.loadLoRA("time-c", "/path/to/c.bin", "base", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"time-a", "time-b", "time-c"};
    config.weights = {0.33f, 0.33f, 0.34f};
    config.enable_cache = true;
    
    auto start = std::chrono::high_resolution_clock::now();
    bool fused = manager.fuseLoRAsAdvanced("time-test", config);
    auto end = std::chrono::high_resolution_clock::now();
    
    EXPECT_TRUE(fused);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Fusion should complete in reasonable time (< 1 second for mock implementation)
    EXPECT_LT(duration.count(), 1000);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, FusionWithBatchInference) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("batch-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("batch-b", "/path/to/b.bin", "base", 1.0f);
    
    // Create fusion
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"batch-a", "batch-b"};
    config.weights = {0.5f, 0.5f};
    
    manager.fuseLoRAsAdvanced("batch-fusion", config);
    
    // Use fused adapter in batch inference
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    
    InferenceRequest req1;
    req1.prompt = "Test with fused adapter";
    requests.push_back({req1, "batch-fusion"});
    
    InferenceRequest req2;
    req2.prompt = "Another test";
    requests.push_back({req2, "batch-fusion"});
    
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    EXPECT_EQ(responses.size(), 2);
}

TEST_F(MultiLoRAFusionTest, MultipleFusionStrategiesCoexist) {
    MultiLoRAManager manager(config_);
    
    // Load base LoRAs
    manager.loadLoRA("multi-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("multi-b", "/path/to/b.bin", "base", 1.0f);
    manager.loadLoRA("multi-c", "/path/to/c.bin", "base", 1.0f);
    
    // Create STATIC fusion
    FusionConfig static_config;
    static_config.strategy = FusionStrategy::STATIC;
    static_config.source_lora_ids = {"multi-a", "multi-b"};
    static_config.weights = {0.5f, 0.5f};
    
    manager.fuseLoRAsAdvanced("multi-static", static_config);
    
    // Create DYNAMIC fusion
    FusionConfig dynamic_config;
    dynamic_config.strategy = FusionStrategy::DYNAMIC;
    dynamic_config.source_lora_ids = {"multi-b", "multi-c"};
    dynamic_config.weights = {0.6f, 0.4f};
    
    manager.fuseLoRAsAdvanced("multi-dynamic", dynamic_config);
    
    // Both should exist
    EXPECT_TRUE(manager.isLoRALoaded("multi-static"));
    EXPECT_TRUE(manager.isLoRALoaded("multi-dynamic"));
    
    // Dynamic can be updated
    bool updated = manager.updateFusionWeights("multi-dynamic", {0.3f, 0.7f});
    EXPECT_TRUE(updated);
    
    // Static cannot be updated
    bool not_updated = manager.updateFusionWeights("multi-static", {0.3f, 0.7f});
    EXPECT_FALSE(not_updated);
}

TEST_F(MultiLoRAFusionTest, FusionMemoryAccounting) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("mem-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("mem-b", "/path/to/b.bin", "base", 1.0f);
    
    auto mem_before = manager.getMemoryStats();
    size_t loras_before = mem_before["loras_loaded"];
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"mem-a", "mem-b"};
    config.weights = {0.5f, 0.5f};
    
    manager.fuseLoRAsAdvanced("mem-fusion", config);
    
    auto mem_after = manager.getMemoryStats();
    size_t loras_after = mem_after["loras_loaded"];
    
    // Fused adapter should count as a LoRA
    EXPECT_GT(loras_after, loras_before);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases and Error Handling
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, FusionWithEmptySourceList) {
    MultiLoRAManager manager(config_);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {};
    config.weights = {};
    
    bool fused = manager.fuseLoRAsAdvanced("empty-fusion", config);
    EXPECT_FALSE(fused);
}

TEST_F(MultiLoRAFusionTest, FusionWithWeightMismatch) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("mismatch-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("mismatch-b", "/path/to/b.bin", "base", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"mismatch-a", "mismatch-b"};
    config.weights = {0.5f};  // Wrong size
    
    bool fused = manager.fuseLoRAsAdvanced("mismatch-fusion", config);
    EXPECT_FALSE(fused);
}

TEST_F(MultiLoRAFusionTest, UpdateNonExistentFusion) {
    MultiLoRAManager manager(config_);
    
    bool updated = manager.updateFusionWeights("nonexistent", {0.5f, 0.5f});
    EXPECT_FALSE(updated);
}

TEST_F(MultiLoRAFusionTest, InvalidateNonExistentFusion) {
    MultiLoRAManager manager(config_);
    
    bool invalidated = manager.invalidateFusionCache("nonexistent");
    EXPECT_FALSE(invalidated);
}

TEST_F(MultiLoRAFusionTest, GetWeightsForNonExistentFusion) {
    MultiLoRAManager manager(config_);
    
    auto weights = manager.getCurrentFusionWeights("nonexistent");
    EXPECT_TRUE(weights.empty());
}

TEST_F(MultiLoRAFusionTest, SetScheduleForNonExistentFusion) {
    MultiLoRAManager manager(config_);
    
    AlphaSchedule schedule;
    bool set = manager.setAlphaSchedule("nonexistent", schedule);
    EXPECT_FALSE(set);
}

TEST_F(MultiLoRAFusionTest, SetScheduleForStaticFusion) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("noschedule-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("noschedule-b", "/path/to/b.bin", "base", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::STATIC;
    config.source_lora_ids = {"noschedule-a", "noschedule-b"};
    config.weights = {0.5f, 0.5f};
    
    manager.fuseLoRAsAdvanced("noschedule-fusion", config);
    
    AlphaSchedule schedule;
    bool set = manager.setAlphaSchedule("noschedule-fusion", schedule);
    EXPECT_FALSE(set);  // Cannot set schedule for STATIC fusion
}

// ═══════════════════════════════════════════════════════════
// Comprehensive SCHEDULED Fusion Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, ScheduledFusionWeightTransitionOverTime) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("transition-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("transition-b", "/path/to/b.bin", "base-model", 1.0f);
    
    // Create SCHEDULED fusion with linear transition from 90/10 to 10/90 over 2 seconds
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"transition-a", "transition-b"};
    config.weights = {0.9f, 0.1f};  // Initial weights
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.static_weights = {0.9f, 0.1f};  // Start: 90% A, 10% B
    schedule.a_weight = 0.1f;  // End: 10% A
    schedule.b_weight = 0.9f;  // End: 90% B
    schedule.start_time = std::chrono::system_clock::now();
    schedule.transition_duration = std::chrono::seconds(2);
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("transition-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("transition-fusion", schedule);
    
    // Test weights at different time points
    // At start: should be close to 90/10
    auto weights_start = manager.getCurrentFusionWeights("transition-fusion");
    EXPECT_EQ(weights_start.size(), 2);
    EXPECT_GT(weights_start[0], 0.8f);  // Should be close to 90%
    
    // Wait 1 second (halfway through transition)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto weights_mid = manager.getCurrentFusionWeights("transition-fusion");
    EXPECT_EQ(weights_mid.size(), 2);
    EXPECT_NEAR(weights_mid[0], 0.5f, 0.15f);  // Should be around 50% (with some tolerance)
    EXPECT_NEAR(weights_mid[1], 0.5f, 0.15f);
    
    // Wait another second (end of transition)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto weights_end = manager.getCurrentFusionWeights("transition-fusion");
    EXPECT_EQ(weights_end.size(), 2);
    EXPECT_LT(weights_end[0], 0.2f);  // Should be close to 10%
    EXPECT_GT(weights_end[1], 0.8f);  // Should be close to 90%
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionWithCustomScheduleFunction) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("custom-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("custom-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"custom-a", "custom-b"};
    config.weights = {0.5f, 0.5f};
    
    // Custom schedule: Step function that switches at 1 second
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.schedule_func = [](double time_offset) -> std::vector<float> {
        if (time_offset < 1.0) {
            return {0.9f, 0.1f};  // Before 1 second: 90/10
        } else {
            return {0.1f, 0.9f};  // After 1 second: 10/90
        }
    };
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("custom-schedule-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("custom-schedule-fusion", schedule);
    
    // Test before step
    auto weights_before = manager.getCurrentFusionWeights("custom-schedule-fusion");
    EXPECT_EQ(weights_before.size(), 2);
    EXPECT_NEAR(weights_before[0], 0.9f, 0.05f);
    EXPECT_NEAR(weights_before[1], 0.1f, 0.05f);
    
    // Wait for step to occur
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // Test after step
    auto weights_after = manager.getCurrentFusionWeights("custom-schedule-fusion");
    EXPECT_EQ(weights_after.size(), 2);
    EXPECT_NEAR(weights_after[0], 0.1f, 0.05f);
    EXPECT_NEAR(weights_after[1], 0.9f, 0.05f);
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionFallbackToStaticWeights) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("fallback-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("fallback-b", "/path/to/b.bin", "base-model", 1.0f);
    
    // Create SCHEDULED fusion but don't set a schedule
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"fallback-a", "fallback-b"};
    config.weights = {0.7f, 0.3f};  // Static fallback weights
    
    bool fused = manager.fuseLoRAsAdvanced("fallback-fusion", config);
    EXPECT_TRUE(fused);
    
    // Should fall back to static weights when no schedule is set
    auto weights = manager.getCurrentFusionWeights("fallback-fusion");
    EXPECT_EQ(weights.size(), 2);
    EXPECT_NEAR(weights[0], 0.7f, 0.01f);
    EXPECT_NEAR(weights[1], 0.3f, 0.01f);
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionDoesNotUseStaticCache) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("nocache-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("nocache-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"nocache-a", "nocache-b"};
    config.weights = {0.5f, 0.5f};
    config.enable_cache = true;  // Enable cache
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.static_weights = {0.5f, 0.5f};
    
    config.alpha_schedule = schedule;
    
    auto metrics_before = manager.getFusionMetrics();
    
    // First fusion
    manager.fuseLoRAsAdvanced("nocache-fusion", config);
    
    auto metrics_after_first = manager.getFusionMetrics();
    size_t cache_misses_after_first = metrics_after_first.cache_misses;
    
    // Second fusion with same ID - SCHEDULED should not use static cache
    // so this should still result in a cache miss
    manager.fuseLoRAsAdvanced("nocache-fusion", config);
    
    auto metrics_after_second = manager.getFusionMetrics();
    
    // For SCHEDULED, each call should process the fusion (not use static cache)
    EXPECT_GT(metrics_after_second.cache_misses, cache_misses_after_first);
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionMultipleAdapters) {
    MultiLoRAManager manager(config_);
    
    // Load three adapters for blending
    manager.loadLoRA("multi-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("multi-b", "/path/to/b.bin", "base-model", 1.0f);
    manager.loadLoRA("multi-c", "/path/to/c.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"multi-a", "multi-b", "multi-c"};
    config.weights = {0.33f, 0.33f, 0.34f};
    
    // Schedule that rotates focus among three adapters
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.schedule_func = [](double time_offset) -> std::vector<float> {
        // Rotate focus every 1 second: A -> B -> C
        int phase = static_cast<int>(time_offset) % 3;
        if (phase == 0) return {0.8f, 0.1f, 0.1f};  // Focus on A
        if (phase == 1) return {0.1f, 0.8f, 0.1f};  // Focus on B
        return {0.1f, 0.1f, 0.8f};  // Focus on C
    };
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("multi-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("multi-fusion", schedule);
    
    // Verify three weights are returned
    auto weights = manager.getCurrentFusionWeights("multi-fusion");
    EXPECT_EQ(weights.size(), 3);
    
    // Verify weights sum to approximately 1.0
    float weight_sum = weights[0] + weights[1] + weights[2];
    EXPECT_NEAR(weight_sum, 1.0f, 0.01f);
    
    // One weight should be dominant (close to 0.8)
    float max_weight = *std::max_element(weights.begin(), weights.end());
    EXPECT_GT(max_weight, 0.7f);
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionCanUpdateSchedule) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("update-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("update-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"update-a", "update-b"};
    config.weights = {0.5f, 0.5f};
    
    // Initial schedule: favor A
    AlphaSchedule schedule1;
    schedule1.strategy = FusionStrategy::SCHEDULED;
    schedule1.start_time = std::chrono::system_clock::now();
    schedule1.static_weights = {0.9f, 0.1f};
    schedule1.schedule_func = [](double) { return std::vector<float>{0.9f, 0.1f}; };
    
    config.alpha_schedule = schedule1;
    
    bool fused = manager.fuseLoRAsAdvanced("update-schedule-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("update-schedule-fusion", schedule1);
    
    auto weights1 = manager.getCurrentFusionWeights("update-schedule-fusion");
    EXPECT_NEAR(weights1[0], 0.9f, 0.05f);
    
    // Update schedule: favor B
    AlphaSchedule schedule2;
    schedule2.strategy = FusionStrategy::SCHEDULED;
    schedule2.start_time = std::chrono::system_clock::now();
    schedule2.static_weights = {0.1f, 0.9f};
    schedule2.schedule_func = [](double) { return std::vector<float>{0.1f, 0.9f}; };
    
    bool updated = manager.setAlphaSchedule("update-schedule-fusion", schedule2);
    EXPECT_TRUE(updated);
    
    auto weights2 = manager.getCurrentFusionWeights("update-schedule-fusion");
    EXPECT_NEAR(weights2[0], 0.1f, 0.05f);
    EXPECT_NEAR(weights2[1], 0.9f, 0.05f);
}

TEST_F(MultiLoRAFusionTest, ScheduledFusionMetricsTracking) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("metrics-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("metrics-b", "/path/to/b.bin", "base-model", 1.0f);
    
    auto metrics_before = manager.getFusionMetrics();
    size_t fusions_before = metrics_before.total_fusions;
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"metrics-a", "metrics-b"};
    config.weights = {0.5f, 0.5f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.static_weights = {0.5f, 0.5f};
    
    config.alpha_schedule = schedule;
    
    manager.fuseLoRAsAdvanced("metrics-fusion", config);
    
    auto metrics_after = manager.getFusionMetrics();
    
    // Verify total fusions increased
    EXPECT_GT(metrics_after.total_fusions, fusions_before);
    
    // Verify fusion is tracked in cache
    auto cache_entries = manager.listFusionCache();
    bool found = false;
    for (const auto& entry : cache_entries) {
        if (entry.fusion_id == "metrics-fusion") {
            found = true;
            EXPECT_EQ(entry.strategy, FusionStrategy::SCHEDULED);
            EXPECT_EQ(entry.source_lora_ids.size(), 2);
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ═══════════════════════════════════════════════════════════
// Built-in Scheduling Strategy Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiLoRAFusionTest, LinearSchedulingStrategy) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("linear-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("linear-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"linear-a", "linear-b"};
    config.weights = {0.8f, 0.2f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::LINEAR;
    schedule.static_weights = {0.8f, 0.2f};  // Start: 80% A, 20% B
    schedule.target_weights = {0.2f, 0.8f};  // End: 20% A, 80% B
    schedule.start_time = std::chrono::system_clock::now();
    schedule.transition_duration = std::chrono::seconds(2);
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("linear-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("linear-fusion", schedule);
    
    // Test at start
    auto weights_start = manager.getCurrentFusionWeights("linear-fusion");
    EXPECT_EQ(weights_start.size(), 2);
    EXPECT_GT(weights_start[0], 0.7f);  // Should be close to 80%
    
    // Wait halfway
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto weights_mid = manager.getCurrentFusionWeights("linear-fusion");
    EXPECT_NEAR(weights_mid[0], 0.5f, 0.15f);  // Should be around 50%
    
    // Wait for end
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto weights_end = manager.getCurrentFusionWeights("linear-fusion");
    EXPECT_LT(weights_end[0], 0.3f);  // Should be close to 20%
    EXPECT_GT(weights_end[1], 0.7f);  // Should be close to 80%
}

TEST_F(MultiLoRAFusionTest, ExponentialDecaySchedulingStrategy) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("exp-decay-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("exp-decay-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"exp-decay-a", "exp-decay-b"};
    config.weights = {0.9f, 0.1f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::EXPONENTIAL;
    schedule.static_weights = {0.9f, 0.1f};   // Start: 90% A, 10% B
    schedule.target_weights = {0.1f, 0.9f};   // End: 10% A, 90% B
    schedule.exponential_base = 2.0f;         // Base for exponential function
    schedule.exponential_decay = true;        // Decay (fast at start, slow at end)
    schedule.start_time = std::chrono::system_clock::now();
    schedule.transition_duration = std::chrono::seconds(3);
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("exp-decay-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("exp-decay-fusion", schedule);
    
    // Test at start - exponential decay should transition faster initially
    auto weights_start = manager.getCurrentFusionWeights("exp-decay-fusion");
    EXPECT_EQ(weights_start.size(), 2);
    EXPECT_GT(weights_start[0], 0.8f);
    
    // Test at 1/3 through - should show significant transition (faster than linear)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto weights_early = manager.getCurrentFusionWeights("exp-decay-fusion");
    // With exponential decay, we expect more transition early
    EXPECT_LT(weights_early[0], 0.7f);  // Should have transitioned more than linear
    
    // Wait for end
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto weights_end = manager.getCurrentFusionWeights("exp-decay-fusion");
    EXPECT_LT(weights_end[0], 0.2f);
    EXPECT_GT(weights_end[1], 0.8f);
}

TEST_F(MultiLoRAFusionTest, ExponentialGrowthSchedulingStrategy) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("exp-growth-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("exp-growth-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"exp-growth-a", "exp-growth-b"};
    config.weights = {0.9f, 0.1f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::EXPONENTIAL;
    schedule.static_weights = {0.9f, 0.1f};   // Start: 90% A, 10% B
    schedule.target_weights = {0.1f, 0.9f};   // End: 10% A, 90% B
    schedule.exponential_base = 2.0f;
    schedule.exponential_decay = false;       // Growth (slow at start, fast at end)
    schedule.start_time = std::chrono::system_clock::now();
    schedule.transition_duration = std::chrono::seconds(3);
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("exp-growth-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("exp-growth-fusion", schedule);
    
    // Test at start
    auto weights_start = manager.getCurrentFusionWeights("exp-growth-fusion");
    EXPECT_EQ(weights_start.size(), 2);
    EXPECT_GT(weights_start[0], 0.8f);
    
    // Test at 1/3 through - should show less transition (slower than linear)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto weights_early = manager.getCurrentFusionWeights("exp-growth-fusion");
    // With exponential growth, we expect less transition early
    EXPECT_GT(weights_early[0], 0.6f);  // Should have transitioned less than linear
    
    // Wait for end
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto weights_end = manager.getCurrentFusionWeights("exp-growth-fusion");
    EXPECT_LT(weights_end[0], 0.2f);
    EXPECT_GT(weights_end[1], 0.8f);
}

TEST_F(MultiLoRAFusionTest, StepWiseSchedulingStrategy) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("step-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("step-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"step-a", "step-b"};
    config.weights = {0.9f, 0.1f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::STEP_WISE;
    schedule.static_weights = {0.9f, 0.1f};   // Initial: 90% A, 10% B
    
    // Define step transitions: at 1s, 2s, 3s
    schedule.step_times = {1.0, 2.0, 3.0};
    schedule.step_weights = {
        {0.9f, 0.1f},  // Step 0 (before 1s): 90/10
        {0.6f, 0.4f},  // Step 1 (1-2s): 60/40
        {0.3f, 0.7f},  // Step 2 (2-3s): 30/70
        {0.1f, 0.9f}   // Step 3 (after 3s): 10/90
    };
    
    schedule.start_time = std::chrono::system_clock::now();
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("step-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("step-fusion", schedule);
    
    // Test initial step (before 1s)
    auto weights_step0 = manager.getCurrentFusionWeights("step-fusion");
    EXPECT_EQ(weights_step0.size(), 2);
    EXPECT_NEAR(weights_step0[0], 0.9f, 0.05f);
    EXPECT_NEAR(weights_step0[1], 0.1f, 0.05f);
    
    // Wait for step 1 (after 1s)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    auto weights_step1 = manager.getCurrentFusionWeights("step-fusion");
    EXPECT_NEAR(weights_step1[0], 0.6f, 0.05f);
    EXPECT_NEAR(weights_step1[1], 0.4f, 0.05f);
    
    // Wait for step 2 (after 2s total)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto weights_step2 = manager.getCurrentFusionWeights("step-fusion");
    EXPECT_NEAR(weights_step2[0], 0.3f, 0.05f);
    EXPECT_NEAR(weights_step2[1], 0.7f, 0.05f);
    
    // Wait for step 3 (after 3s total)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto weights_step3 = manager.getCurrentFusionWeights("step-fusion");
    EXPECT_NEAR(weights_step3[0], 0.1f, 0.05f);
    EXPECT_NEAR(weights_step3[1], 0.9f, 0.05f);
}

TEST_F(MultiLoRAFusionTest, StepWiseWithThreeAdapters) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("step3-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("step3-b", "/path/to/b.bin", "base-model", 1.0f);
    manager.loadLoRA("step3-c", "/path/to/c.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"step3-a", "step3-b", "step3-c"};
    config.weights = {0.7f, 0.2f, 0.1f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::STEP_WISE;
    schedule.static_weights = {0.7f, 0.2f, 0.1f};
    
    // Rotate focus among three adapters
    schedule.step_times = {1.0, 2.0};
    schedule.step_weights = {
        {0.7f, 0.2f, 0.1f},  // Step 0: Focus on A
        {0.2f, 0.7f, 0.1f},  // Step 1: Focus on B
        {0.1f, 0.2f, 0.7f}   // Step 2: Focus on C
    };
    
    schedule.start_time = std::chrono::system_clock::now();
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("step3-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("step3-fusion", schedule);
    
    // Test step 0
    auto weights_0 = manager.getCurrentFusionWeights("step3-fusion");
    EXPECT_EQ(weights_0.size(), 3);
    EXPECT_GT(weights_0[0], 0.6f);  // A should dominate
    
    // Wait for step 1
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    auto weights_1 = manager.getCurrentFusionWeights("step3-fusion");
    EXPECT_GT(weights_1[1], 0.6f);  // B should dominate
    
    // Wait for step 2
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto weights_2 = manager.getCurrentFusionWeights("step3-fusion");
    EXPECT_GT(weights_2[2], 0.6f);  // C should dominate
}

TEST_F(MultiLoRAFusionTest, CustomStrategyBackwardCompatibility) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("custom-compat-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("custom-compat-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"custom-compat-a", "custom-compat-b"};
    config.weights = {0.5f, 0.5f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::CUSTOM;
    schedule.start_time = std::chrono::system_clock::now();
    
    // Custom function: sine wave
    schedule.schedule_func = [](double time_offset) -> std::vector<float> {
        float phase = std::sin(time_offset / 5.0);
        float weight_a = 0.5f + 0.3f * phase;
        float weight_b = 1.0f - weight_a;
        return {weight_a, weight_b};
    };
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("custom-compat-fusion", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("custom-compat-fusion", schedule);
    
    auto weights = manager.getCurrentFusionWeights("custom-compat-fusion");
    EXPECT_EQ(weights.size(), 2);
    EXPECT_NEAR(weights[0] + weights[1], 1.0f, 0.01f);
}

TEST_F(MultiLoRAFusionTest, SchedulingStrategyFallbackToStatic) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("fallback-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("fallback-b", "/path/to/b.bin", "base-model", 1.0f);
    
    FusionConfig config;
    config.strategy = FusionStrategy::SCHEDULED;
    config.source_lora_ids = {"fallback-a", "fallback-b"};
    config.weights = {0.65f, 0.35f};
    
    AlphaSchedule schedule;
    schedule.strategy = FusionStrategy::SCHEDULED;
    schedule.scheduling_strategy = SchedulingStrategy::LINEAR;
    schedule.static_weights = {0.65f, 0.35f};
    // No target_weights or transition_duration - should fall back to static
    schedule.start_time = std::chrono::system_clock::now();
    
    config.alpha_schedule = schedule;
    
    bool fused = manager.fuseLoRAsAdvanced("fallback-test", config);
    EXPECT_TRUE(fused);
    
    manager.setAlphaSchedule("fallback-test", schedule);
    
    auto weights = manager.getCurrentFusionWeights("fallback-test");
    EXPECT_EQ(weights.size(), 2);
    EXPECT_NEAR(weights[0], 0.65f, 0.01f);
    EXPECT_NEAR(weights[1], 0.35f, 0.01f);
}
