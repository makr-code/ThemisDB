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
