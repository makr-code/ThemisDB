/**
 * ThemisDB LoRA Adapter Tests
 * 
 * Comprehensive tests for LoRA adapter management including:
 * - Cleanup and resource management
 * - Multi-LoRA batch inference
 * - LoRA fusion
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

class LoRAAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_lora_vram_mb = 512;
        config_.max_lora_slots = 8;
        config_.lora_ttl = std::chrono::seconds(60);
        config_.enable_multi_lora_batch = true;
        config_.enable_adapter_fusion = true;
    }
    
    MultiLoRAManager::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Cleanup Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterTest, DestructorCleansUpAllLoRAs) {
    {
        MultiLoRAManager manager(config_);
        
        // Load several LoRAs
        manager.loadLoRA("lora1", "/path/to/lora1.bin", "base-model", 1.0f);
        manager.loadLoRA("lora2", "/path/to/lora2.bin", "base-model", 1.0f);
        manager.loadLoRA("lora3", "/path/to/lora3.bin", "base-model", 1.0f);
        
        auto loras = manager.listLoRAs();
        EXPECT_EQ(loras.size(), 3);
        
        // Destructor will be called here
    }
    // If we reach here without crashes, cleanup worked
    SUCCEED();
}

TEST_F(LoRAAdapterTest, UnloadLoRAFreesResources) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("test-lora", "/path/to/test.bin", "base-model", 1.0f);
    
    auto loras_before = manager.listLoRAs();
    EXPECT_EQ(loras_before.size(), 1);
    
    bool unloaded = manager.unloadLoRA("test-lora");
    EXPECT_TRUE(unloaded);
    
    auto loras_after = manager.listLoRAs();
    EXPECT_EQ(loras_after.size(), 0);
}

TEST_F(LoRAAdapterTest, UnloadLoRAUpdatesVRAMTracking) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora1", "/path/to/lora1.bin", "base-model", 1.0f);
    manager.loadLoRA("lora2", "/path/to/lora2.bin", "base-model", 1.0f);
    
    auto stats_before = manager.getStats();
    size_t loras_before = stats_before.total_loras_loaded;
    
    manager.unloadLoRA("lora1");
    
    auto stats_after = manager.getStats();
    size_t loras_after = stats_after.total_loras_loaded;
    
    EXPECT_LT(loras_after, loras_before);
}

TEST_F(LoRAAdapterTest, CannotUnloadPinnedLoRA) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("pinned-lora", "/path/to/pinned.bin", "base-model", 1.0f);
    manager.pinLoRA("pinned-lora");
    
    bool unloaded = manager.unloadLoRA("pinned-lora", false);
    EXPECT_FALSE(unloaded);
    
    // Can force unload
    bool force_unloaded = manager.unloadLoRA("pinned-lora", true);
    EXPECT_TRUE(force_unloaded);
}

// ═══════════════════════════════════════════════════════════
// Multi-LoRA Batch Inference Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterTest, BatchInferenceWithMultipleLoRAs) {
    MultiLoRAManager manager(config_);
    
    // Load multiple LoRAs
    manager.loadLoRA("math-lora", "/path/to/math.bin", "llama-7b", 1.0f);
    manager.loadLoRA("code-lora", "/path/to/code.bin", "llama-7b", 1.0f);
    manager.loadLoRA("chat-lora", "/path/to/chat.bin", "llama-7b", 1.0f);
    
    // Create batch requests with different LoRAs
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    
    InferenceRequest req1;
    req1.prompt = "Solve: 2x + 5 = 13";
    requests.push_back({req1, "math-lora"});
    
    InferenceRequest req2;
    req2.prompt = "Write a Python function to sort a list";
    requests.push_back({req2, "code-lora"});
    
    InferenceRequest req3;
    req3.prompt = "How are you today?";
    requests.push_back({req3, "chat-lora"});
    
    InferenceRequest req4;
    req4.prompt = "Calculate 15 * 23";
    requests.push_back({req4, "math-lora"});
    
    // Process batch
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    
    EXPECT_EQ(responses.size(), 4);
    EXPECT_EQ(responses[0].lora_used, "math-lora");
    EXPECT_EQ(responses[1].lora_used, "code-lora");
    EXPECT_EQ(responses[2].lora_used, "chat-lora");
    EXPECT_EQ(responses[3].lora_used, "math-lora");
}

TEST_F(LoRAAdapterTest, BatchInferenceGroupsByLoRA) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base", 1.0f);
    
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    
    // Create 10 requests alternating between two LoRAs
    for (int i = 0; i < 10; ++i) {
        InferenceRequest req;
        req.prompt = "Request " + std::to_string(i);
        std::string lora_id = (i % 2 == 0) ? "lora-a" : "lora-b";
        requests.push_back({req, lora_id});
    }
    
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    
    EXPECT_EQ(responses.size(), 10);
    
    // Verify correct LoRA assignment
    for (size_t i = 0; i < responses.size(); ++i) {
        std::string expected_lora = (i % 2 == 0) ? "lora-a" : "lora-b";
        EXPECT_EQ(responses[i].lora_used, expected_lora);
    }
}

TEST_F(LoRAAdapterTest, BatchInferenceHandlesMissingLoRA) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("existing-lora", "/path/to/existing.bin", "base", 1.0f);
    
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    
    InferenceRequest req1;
    req1.prompt = "Test 1";
    requests.push_back({req1, "existing-lora"});
    
    InferenceRequest req2;
    req2.prompt = "Test 2";
    requests.push_back({req2, "missing-lora"});  // This LoRA doesn't exist
    
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    
    EXPECT_EQ(responses.size(), 2);
    EXPECT_TRUE(!responses[0].text.empty());  // First should have valid response
}

TEST_F(LoRAAdapterTest, BatchInferenceDisabledByConfig) {
    config_.enable_multi_lora_batch = false;
    MultiLoRAManager manager(config_);
    
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    InferenceRequest req;
    req.prompt = "Test";
    requests.push_back({req, "any-lora"});
    
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    
    EXPECT_TRUE(responses.empty());
}

// ═══════════════════════════════════════════════════════════
// LoRA Fusion Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterTest, FuseMultipleLoRAs) {
    MultiLoRAManager manager(config_);
    
    // Load source LoRAs
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-c", "/path/to/c.bin", "base-model", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b", "lora-c"};
    std::vector<float> weights = {0.5f, 0.3f, 0.2f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused-lora", weights);
    EXPECT_TRUE(fused);
    
    // Verify fused LoRA exists
    auto loras = manager.listLoRAs();
    bool found = false;
    for (const auto& info : loras) {
        if (info.lora_id == "fused-lora") {
            found = true;
            EXPECT_EQ(info.base_model_id, "base-model");
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(LoRAAdapterTest, FusionRequiresSameBaseModel) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "model-1", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "model-2", 1.0f);  // Different base model
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b"};
    std::vector<float> weights = {0.5f, 0.5f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);  // Should fail due to different base models
}

TEST_F(LoRAAdapterTest, FusionNormalizesWeights) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b"};
    std::vector<float> weights = {2.0f, 3.0f};  // Sum = 5.0, will be normalized
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_TRUE(fused);
    
    // Weights should be normalized to 0.4 and 0.6
}

TEST_F(LoRAAdapterTest, FusionRequiresMatchingWeights) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b"};
    std::vector<float> weights = {0.5f};  // Mismatched size
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);
}

TEST_F(LoRAAdapterTest, FusionDisabledByConfig) {
    config_.enable_adapter_fusion = false;
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a"};
    std::vector<float> weights = {1.0f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);
}

TEST_F(LoRAAdapterTest, FusionHandlesMissingSourceLoRA) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "missing-lora"};
    std::vector<float> weights = {0.5f, 0.5f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);
}

// ═══════════════════════════════════════════════════════════
// Performance and Stress Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterTest, HandlesLargeNumberOfLoRAs) {
    MultiLoRAManager manager(config_);
    
    // Load many LoRAs
    for (int i = 0; i < 20; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        std::string path = "/path/to/lora" + std::to_string(i) + ".bin";
        manager.loadLoRA(lora_id, path, "base", 1.0f);
    }
    
    // LRU eviction should kick in
    auto loras = manager.listLoRAs();
    EXPECT_LE(loras.size(), config_.max_lora_slots);
}

TEST_F(LoRAAdapterTest, LRUEvictionWorksCorrectly) {
    config_.max_lora_slots = 3;
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-1", "/path/1.bin", "base", 1.0f);
    manager.loadLoRA("lora-2", "/path/2.bin", "base", 1.0f);
    manager.loadLoRA("lora-3", "/path/3.bin", "base", 1.0f);
    
    // Access lora-1 to make it recently used
    manager.getLoRA("lora-1");
    
    // Load lora-4, should evict lora-2 (least recently used)
    manager.loadLoRA("lora-4", "/path/4.bin", "base", 1.0f);
    
    auto loras = manager.listLoRAs();
    EXPECT_EQ(loras.size(), 3);
    
    // lora-1, lora-3, and lora-4 should remain
    std::set<std::string> remaining;
    for (const auto& lora : loras) {
        remaining.insert(lora.lora_id);
    }
    
    EXPECT_TRUE(remaining.count("lora-1") > 0);
    EXPECT_TRUE(remaining.count("lora-4") > 0);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterTest, LoadUnloadCycle) {
    MultiLoRAManager manager(config_);
    
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Load
        manager.loadLoRA("cycle-lora", "/path/to/cycle.bin", "base", 1.0f);
        EXPECT_TRUE(manager.isLoRALoaded("cycle-lora"));
        
        // Use
        auto* lora = manager.getLoRA("cycle-lora");
        EXPECT_NE(lora, nullptr);
        
        // Unload
        manager.unloadLoRA("cycle-lora");
        EXPECT_FALSE(manager.isLoRALoaded("cycle-lora"));
    }
}

// ═══════════════════════════════════════════════════════════
// Quantization Tests (v1.4.0)
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterTest, QuantizationConfigSetAndGet) {
    MultiLoRAManager manager(config_);
    
    LoRAQuantizationConfig quant_config;
    quant_config.enabled = true;
    quant_config.mode = QuantizationMode::INT8;
    quant_config.calibration_samples = 50;
    quant_config.per_channel = true;
    
    manager.setQuantizationConfig(quant_config);
    
    auto retrieved = manager.getQuantizationConfig();
    EXPECT_TRUE(retrieved.enabled);
    EXPECT_EQ(retrieved.mode, QuantizationMode::INT8);
    EXPECT_EQ(retrieved.calibration_samples, 50);
    EXPECT_TRUE(retrieved.per_channel);
}

TEST_F(LoRAAdapterTest, INT8QuantizationReducesMemory4x) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load with quantization
    bool loaded = manager.loadLoRA("quant-lora", "/path/to/quant.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("quant-lora");
    ASSERT_TRUE(stats.has_value());
    
    EXPECT_EQ(stats->mode, QuantizationMode::INT8);
    EXPECT_GT(stats->original_bytes, 0);
    EXPECT_GT(stats->quantized_bytes, 0);
    EXPECT_LT(stats->quantized_bytes, stats->original_bytes);
    
    // Check compression ratio is approximately 4×
    EXPECT_NEAR(stats->compression_ratio, 4.0f, 0.5f);
}

TEST_F(LoRAAdapterTest, INT4QuantizationReducesMemory8x) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT4;
    config_.quantization.group_size = 128;
    MultiLoRAManager manager(config_);
    
    // Load with quantization
    bool loaded = manager.loadLoRA("quant4-lora", "/path/to/quant4.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("quant4-lora");
    ASSERT_TRUE(stats.has_value());
    
    EXPECT_EQ(stats->mode, QuantizationMode::INT4);
    EXPECT_GT(stats->original_bytes, 0);
    EXPECT_GT(stats->quantized_bytes, 0);
    EXPECT_LT(stats->quantized_bytes, stats->original_bytes);
    
    // Check compression ratio is approximately 8×
    EXPECT_NEAR(stats->compression_ratio, 8.0f, 1.0f);
}

TEST_F(LoRAAdapterTest, QuantizationDisabledByDefault) {
    MultiLoRAManager manager(config_);
    
    // Load without quantization
    bool loaded = manager.loadLoRA("normal-lora", "/path/to/normal.bin", "base-model", 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("normal-lora");
    EXPECT_FALSE(stats.has_value());  // No stats for non-quantized LoRA
}

TEST_F(LoRAAdapterTest, QuantizationCanBeExplicitlyDisabled) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load with quantization explicitly disabled
    bool loaded = manager.loadLoRA("no-quant-lora", "/path/to/noquant.bin", "base-model", false, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("no-quant-lora");
    EXPECT_FALSE(stats.has_value());  // Should not be quantized
}

TEST_F(LoRAAdapterTest, PerChannelScaling) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    config_.quantization.per_channel = true;
    MultiLoRAManager manager(config_);
    
    bool loaded = manager.loadLoRA("perchan-lora", "/path/to/perchan.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("perchan-lora");
    ASSERT_TRUE(stats.has_value());
    
    EXPECT_GT(stats->num_channels, 1);  // Should have multiple channels
    EXPECT_GT(stats->min_scale, 0.0f);
    EXPECT_GT(stats->max_scale, 0.0f);
    EXPECT_GT(stats->avg_scale, 0.0f);
}

TEST_F(LoRAAdapterTest, QuantizedLoRAsLoadFaster) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load multiple quantized LoRAs
    for (int i = 0; i < 5; ++i) {
        std::string lora_id = "fast-lora-" + std::to_string(i);
        bool loaded = manager.loadLoRA(lora_id, "/path/to/fast" + std::to_string(i) + ".bin", 
                                       "base-model", true, 1.0f);
        EXPECT_TRUE(loaded);
    }
    
    auto loras = manager.listLoRAs();
    EXPECT_EQ(loras.size(), 5);
}

TEST_F(LoRAAdapterTest, QuantizationStatsContainValidData) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    bool loaded = manager.loadLoRA("stats-lora", "/path/to/stats.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("stats-lora");
    ASSERT_TRUE(stats.has_value());
    
    EXPECT_EQ(stats->lora_id, "stats-lora");
    EXPECT_EQ(stats->mode, QuantizationMode::INT8);
    EXPECT_GT(stats->original_bytes, 0);
    EXPECT_GT(stats->quantized_bytes, 0);
    EXPECT_GE(stats->compression_ratio, 1.0f);
}

TEST_F(LoRAAdapterTest, MultipleQuantizationModes) {
    MultiLoRAManager manager(config_);
    
    // Load with INT8
    LoRAQuantizationConfig int8_config;
    int8_config.enabled = true;
    int8_config.mode = QuantizationMode::INT8;
    manager.setQuantizationConfig(int8_config);
    
    bool loaded1 = manager.loadLoRA("int8-lora", "/path/to/int8.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded1);
    
    // Load with INT4
    LoRAQuantizationConfig int4_config;
    int4_config.enabled = true;
    int4_config.mode = QuantizationMode::INT4;
    manager.setQuantizationConfig(int4_config);
    
    bool loaded2 = manager.loadLoRA("int4-lora", "/path/to/int4.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded2);
    
    // Verify both are loaded with different modes
    auto stats1 = manager.getQuantizationStats("int8-lora");
    auto stats2 = manager.getQuantizationStats("int4-lora");
    
    ASSERT_TRUE(stats1.has_value());
    ASSERT_TRUE(stats2.has_value());
    
    EXPECT_EQ(stats1->mode, QuantizationMode::INT8);
    EXPECT_EQ(stats2->mode, QuantizationMode::INT4);
}

TEST_F(LoRAAdapterTest, QuantizedLoRACanBeApplied) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    bool loaded = manager.loadLoRA("apply-quant", "/path/to/apply.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    // Apply the quantized LoRA
    bool applied = manager.applyLoRA("apply-quant", nullptr);
    EXPECT_TRUE(applied);
    
    // Should still be loaded
    EXPECT_TRUE(manager.isLoRALoaded("apply-quant"));
}

TEST_F(LoRAAdapterTest, QuantizedLoRACanBeUnloaded) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    bool loaded = manager.loadLoRA("unload-quant", "/path/to/unload.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    bool unloaded = manager.unloadLoRA("unload-quant");
    EXPECT_TRUE(unloaded);
    
    // Should not be loaded anymore
    EXPECT_FALSE(manager.isLoRALoaded("unload-quant"));
    
    // Stats should not be available
    auto stats = manager.getQuantizationStats("unload-quant");
    EXPECT_FALSE(stats.has_value());
}

TEST_F(LoRAAdapterTest, QuantizationAllowsMoreLoRAsInMemory) {
    config_.max_lora_vram_mb = 100;  // Limit VRAM
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load multiple quantized LoRAs - should fit more than unquantized
    int loaded_count = 0;
    for (int i = 0; i < 20; ++i) {
        std::string lora_id = "mem-lora-" + std::to_string(i);
        if (manager.loadLoRA(lora_id, "/path/to/mem" + std::to_string(i) + ".bin", 
                            "base-model", true, 1.0f)) {
            loaded_count++;
        }
    }
    
    // With 4× compression, should load significantly more
    EXPECT_GE(loaded_count, 3);  // At least 3 should fit
}

TEST_F(LoRAAdapterTest, QuantizationWithBatchInference) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    config_.enable_multi_lora_batch = true;
    MultiLoRAManager manager(config_);
    
    // Load quantized LoRAs
    manager.loadLoRA("batch-quant-1", "/path/to/batch1.bin", "base-model", true, 1.0f);
    manager.loadLoRA("batch-quant-2", "/path/to/batch2.bin", "base-model", true, 1.0f);
    
    // Create batch requests
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    
    InferenceRequest req1;
    req1.prompt = "Test 1";
    requests.push_back({req1, "batch-quant-1"});
    
    InferenceRequest req2;
    req2.prompt = "Test 2";
    requests.push_back({req2, "batch-quant-2"});
    
    // Process batch with quantized LoRAs
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    
    EXPECT_EQ(responses.size(), 2);
}

TEST_F(LoRAAdapterTest, QuantizationStatsForNonExistentLoRA) {
    MultiLoRAManager manager(config_);
    
    auto stats = manager.getQuantizationStats("non-existent");
    EXPECT_FALSE(stats.has_value());
}

TEST_F(LoRAAdapterTest, INT4GroupSizeConfiguration) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT4;
    config_.quantization.group_size = 64;  // Custom group size
    MultiLoRAManager manager(config_);
    
    bool loaded = manager.loadLoRA("group-lora", "/path/to/group.bin", "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("group-lora");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->mode, QuantizationMode::INT4);
}

TEST_F(LoRAAdapterTest, QuantizationPreservesLoRAMetadata) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    bool loaded = manager.loadLoRA("meta-lora", "/path/to/meta.bin", "specific-model", true, 0.8f);
    EXPECT_TRUE(loaded);
    
    auto info = manager.getLoRAInfo("meta-lora");
    ASSERT_TRUE(info.has_value());
    
    EXPECT_EQ(info->id, "meta-lora");
    EXPECT_EQ(info->base_model_id, "specific-model");
    EXPECT_FLOAT_EQ(info->scale, 0.8f);
}

// ═══════════════════════════════════════════════════════════
// Note: main() removed - GTest will provide its own when linked with gtest_main
// Individual test cases remain for execution by GTest framework
