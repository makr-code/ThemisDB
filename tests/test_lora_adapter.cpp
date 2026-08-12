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
#include "llm/lora_security_validator.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class LoRAAdapterUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_lora_vram_mb = 512;
        config_.max_lora_slots = 8;
        config_.lora_ttl = std::chrono::seconds(60);
        config_.enable_multi_lora_batch = true;
        config_.enable_adapter_fusion = true;

        test_dir_ = std::filesystem::temp_directory_path() / "themis_lora_adapter_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string createMockAdapter(const std::string& name, size_t size_bytes = 1024) {
        auto path = test_dir_ / (name + ".bin");
        std::ofstream file(path, std::ios::binary);

        for (size_t i = 0; i < size_bytes; ++i) {
            file.put(static_cast<char>(i % 256));
        }

        file.close();
        return path.string();
    }
    
    std::filesystem::path test_dir_;
    MultiLoRAManager::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Cleanup Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterUnitTest, DestructorCleansUpAllLoRAs) {
    {
        MultiLoRAManager manager(config_);
        
        // Load several LoRAs
        auto lora1 = createMockAdapter("lora1");
        auto lora2 = createMockAdapter("lora2");
        auto lora3 = createMockAdapter("lora3");
        manager.loadLoRA("lora1", lora1, "base-model", 1.0f);
        manager.loadLoRA("lora2", lora2, "base-model", 1.0f);
        manager.loadLoRA("lora3", lora3, "base-model", 1.0f);
        
        auto loras = manager.listLoRAs();
        EXPECT_EQ(loras.size(), 3);
        
        // Destructor will be called here
    }
    // If we reach here without crashes, cleanup worked
    SUCCEED();
}

TEST_F(LoRAAdapterUnitTest, UnloadLoRAFreesResources) {
    MultiLoRAManager manager(config_);
    
    auto lora_path = createMockAdapter("test-lora");
    manager.loadLoRA("test-lora", lora_path, "base-model", 1.0f);
    
    auto loras_before = manager.listLoRAs();
    EXPECT_EQ(loras_before.size(), 1);
    
    bool unloaded = manager.unloadLoRA("test-lora");
    EXPECT_TRUE(unloaded);
    
    auto loras_after = manager.listLoRAs();
    EXPECT_EQ(loras_after.size(), 0);
}

TEST_F(LoRAAdapterUnitTest, UnloadLoRAUpdatesVRAMTracking) {
    MultiLoRAManager manager(config_);
    
    auto lora1 = createMockAdapter("lora1");
    auto lora2 = createMockAdapter("lora2");
    manager.loadLoRA("lora1", lora1, "base-model", 1.0f);
    manager.loadLoRA("lora2", lora2, "base-model", 1.0f);
    
    auto stats_before = manager.getStats();
    size_t loras_before = stats_before.total_loras_loaded;
    
    manager.unloadLoRA("lora1");
    
    auto stats_after = manager.getStats();
    size_t loras_after = stats_after.total_loras_loaded;
    
    EXPECT_LT(loras_after, loras_before);
}

TEST_F(LoRAAdapterUnitTest, CannotUnloadPinnedLoRA) {
    MultiLoRAManager manager(config_);
    
    auto pinned_path = createMockAdapter("pinned-lora");
    manager.loadLoRA("pinned-lora", pinned_path, "base-model", 1.0f);
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

TEST_F(LoRAAdapterUnitTest, BatchInferenceWithMultipleLoRAs) {
    MultiLoRAManager manager(config_);
    
    // Load multiple LoRAs
    auto math_path = createMockAdapter("math-lora");
    auto code_path = createMockAdapter("code-lora");
    auto chat_path = createMockAdapter("chat-lora");
    manager.loadLoRA("math-lora", math_path, "llama-7b", 1.0f);
    manager.loadLoRA("code-lora", code_path, "llama-7b", 1.0f);
    manager.loadLoRA("chat-lora", chat_path, "llama-7b", 1.0f);
    
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

TEST_F(LoRAAdapterUnitTest, BatchInferenceGroupsByLoRA) {
    MultiLoRAManager manager(config_);
    
    auto lora_a = createMockAdapter("lora-a");
    auto lora_b = createMockAdapter("lora-b");
    manager.loadLoRA("lora-a", lora_a, "base", 1.0f);
    manager.loadLoRA("lora-b", lora_b, "base", 1.0f);
    
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

TEST_F(LoRAAdapterUnitTest, BatchInferenceHandlesMissingLoRA) {
    MultiLoRAManager manager(config_);
    
    auto existing_path = createMockAdapter("existing-lora");
    manager.loadLoRA("existing-lora", existing_path, "base", 1.0f);
    
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

TEST_F(LoRAAdapterUnitTest, BatchInferenceDisabledByConfig) {
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

TEST_F(LoRAAdapterUnitTest, FuseMultipleLoRAs) {
    MultiLoRAManager manager(config_);
    
    // Load source LoRAs
    auto lora_a = createMockAdapter("lora-a");
    auto lora_b = createMockAdapter("lora-b");
    auto lora_c = createMockAdapter("lora-c");
    manager.loadLoRA("lora-a", lora_a, "base-model", 1.0f);
    manager.loadLoRA("lora-b", lora_b, "base-model", 1.0f);
    manager.loadLoRA("lora-c", lora_c, "base-model", 1.0f);
    
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

TEST_F(LoRAAdapterUnitTest, FusionRequiresSameBaseModel) {
    MultiLoRAManager manager(config_);
    
    auto lora_a = createMockAdapter("lora-a");
    auto lora_b = createMockAdapter("lora-b");
    manager.loadLoRA("lora-a", lora_a, "model-1", 1.0f);
    manager.loadLoRA("lora-b", lora_b, "model-2", 1.0f);  // Different base model
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b"};
    std::vector<float> weights = {0.5f, 0.5f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);  // Should fail due to different base models
}

TEST_F(LoRAAdapterUnitTest, FusionNormalizesWeights) {
    MultiLoRAManager manager(config_);
    
    auto lora_a = createMockAdapter("lora-a");
    auto lora_b = createMockAdapter("lora-b");
    manager.loadLoRA("lora-a", lora_a, "base", 1.0f);
    manager.loadLoRA("lora-b", lora_b, "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b"};
    std::vector<float> weights = {2.0f, 3.0f};  // Sum = 5.0, will be normalized
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_TRUE(fused);
    
    // Weights should be normalized to 0.4 and 0.6
}

TEST_F(LoRAAdapterUnitTest, FusionRequiresMatchingWeights) {
    MultiLoRAManager manager(config_);
    
    auto lora_a = createMockAdapter("lora-a");
    auto lora_b = createMockAdapter("lora-b");
    manager.loadLoRA("lora-a", lora_a, "base", 1.0f);
    manager.loadLoRA("lora-b", lora_b, "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "lora-b"};
    std::vector<float> weights = {0.5f};  // Mismatched size
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);
}

TEST_F(LoRAAdapterUnitTest, FusionDisabledByConfig) {
    config_.enable_adapter_fusion = false;
    MultiLoRAManager manager(config_);
    
    auto lora_a = createMockAdapter("lora-a");
    manager.loadLoRA("lora-a", lora_a, "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a"};
    std::vector<float> weights = {1.0f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);
}

TEST_F(LoRAAdapterUnitTest, FusionHandlesMissingSourceLoRA) {
    MultiLoRAManager manager(config_);
    
    auto lora_a = createMockAdapter("lora-a");
    manager.loadLoRA("lora-a", lora_a, "base", 1.0f);
    
    std::vector<std::string> source_loras = {"lora-a", "missing-lora"};
    std::vector<float> weights = {0.5f, 0.5f};
    
    bool fused = manager.fuseLoRAs(source_loras, "fused", weights);
    EXPECT_FALSE(fused);
}

// ═══════════════════════════════════════════════════════════
// Performance and Stress Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterUnitTest, HandlesLargeNumberOfLoRAs) {
    MultiLoRAManager manager(config_);
    
    // Load many LoRAs
    for (int i = 0; i < 20; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        std::string path = createMockAdapter("lora-" + std::to_string(i));
        manager.loadLoRA(lora_id, path, "base", 1.0f);
    }
    
    // LRU eviction should kick in
    auto loras = manager.listLoRAs();
    EXPECT_LE(loras.size(), config_.max_lora_slots);
}

TEST_F(LoRAAdapterUnitTest, LRUEvictionWorksCorrectly) {
    config_.max_lora_slots = 3;
    MultiLoRAManager manager(config_);
    
    auto lora_1 = createMockAdapter("lora-1");
    auto lora_2 = createMockAdapter("lora-2");
    auto lora_3 = createMockAdapter("lora-3");
    manager.loadLoRA("lora-1", lora_1, "base", 1.0f);
    manager.loadLoRA("lora-2", lora_2, "base", 1.0f);
    manager.loadLoRA("lora-3", lora_3, "base", 1.0f);
    
    // Access lora-1 to make it recently used
    manager.getLoRA("lora-1");
    
    // Load lora-4, should evict lora-2 (least recently used)
    auto lora_4 = createMockAdapter("lora-4");
    manager.loadLoRA("lora-4", lora_4, "base", 1.0f);
    
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

TEST_F(LoRAAdapterUnitTest, LoadUnloadCycle) {
    MultiLoRAManager manager(config_);
    auto cycle_path = createMockAdapter("cycle-lora");
    
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Load
        manager.loadLoRA("cycle-lora", cycle_path, "base", 1.0f);
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

TEST_F(LoRAAdapterUnitTest, QuantizationConfigSetAndGet) {
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

TEST_F(LoRAAdapterUnitTest, INT8QuantizationReducesMemory4x) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load with quantization
    auto quant_path = createMockAdapter("quant-lora");
    bool loaded = manager.loadLoRA("quant-lora", quant_path, "base-model", true, 1.0f);
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

TEST_F(LoRAAdapterUnitTest, INT4QuantizationReducesMemory8x) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT4;
    config_.quantization.group_size = 128;
    MultiLoRAManager manager(config_);
    
    // Load with quantization
    auto quant4_path = createMockAdapter("quant4-lora");
    bool loaded = manager.loadLoRA("quant4-lora", quant4_path, "base-model", true, 1.0f);
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

TEST_F(LoRAAdapterUnitTest, QuantizationDisabledByDefault) {
    MultiLoRAManager manager(config_);
    
    // Load without quantization
    auto normal_path = createMockAdapter("normal-lora");
    bool loaded = manager.loadLoRA("normal-lora", normal_path, "base-model", 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("normal-lora");
    EXPECT_FALSE(stats.has_value());  // No stats for non-quantized LoRA
}

TEST_F(LoRAAdapterUnitTest, QuantizationCanBeExplicitlyDisabled) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load with quantization explicitly disabled
    auto noquant_path = createMockAdapter("no-quant-lora");
    bool loaded = manager.loadLoRA("no-quant-lora", noquant_path, "base-model", false, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("no-quant-lora");
    EXPECT_FALSE(stats.has_value());  // Should not be quantized
}

TEST_F(LoRAAdapterUnitTest, PerChannelScaling) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    config_.quantization.per_channel = true;
    MultiLoRAManager manager(config_);
    
    auto perchan_path = createMockAdapter("perchan-lora");
    bool loaded = manager.loadLoRA("perchan-lora", perchan_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("perchan-lora");
    ASSERT_TRUE(stats.has_value());
    
    EXPECT_GT(stats->num_channels, 1);  // Should have multiple channels
    EXPECT_GT(stats->min_scale, 0.0f);
    EXPECT_GT(stats->max_scale, 0.0f);
    EXPECT_GT(stats->avg_scale, 0.0f);
}

TEST_F(LoRAAdapterUnitTest, QuantizedLoRAsLoadFaster) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load multiple quantized LoRAs
    for (int i = 0; i < 5; ++i) {
        std::string lora_id = "fast-lora-" + std::to_string(i);
        std::string path = createMockAdapter("fast-lora-" + std::to_string(i));
        bool loaded = manager.loadLoRA(lora_id, path, 
                                       "base-model", true, 1.0f);
        EXPECT_TRUE(loaded);
    }
    
    auto loras = manager.listLoRAs();
    EXPECT_EQ(loras.size(), 5);
}

TEST_F(LoRAAdapterUnitTest, QuantizationStatsContainValidData) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    auto stats_path = createMockAdapter("stats-lora");
    bool loaded = manager.loadLoRA("stats-lora", stats_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("stats-lora");
    ASSERT_TRUE(stats.has_value());
    
    EXPECT_EQ(stats->lora_id, "stats-lora");
    EXPECT_EQ(stats->mode, QuantizationMode::INT8);
    EXPECT_GT(stats->original_bytes, 0);
    EXPECT_GT(stats->quantized_bytes, 0);
    EXPECT_GE(stats->compression_ratio, 1.0f);
}

TEST_F(LoRAAdapterUnitTest, MultipleQuantizationModes) {
    MultiLoRAManager manager(config_);
    
    // Load with INT8
    LoRAQuantizationConfig int8_config;
    int8_config.enabled = true;
    int8_config.mode = QuantizationMode::INT8;
    manager.setQuantizationConfig(int8_config);
    
    auto int8_path = createMockAdapter("int8-lora");
    bool loaded1 = manager.loadLoRA("int8-lora", int8_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded1);
    
    // Load with INT4
    LoRAQuantizationConfig int4_config;
    int4_config.enabled = true;
    int4_config.mode = QuantizationMode::INT4;
    manager.setQuantizationConfig(int4_config);
    
    auto int4_path = createMockAdapter("int4-lora");
    bool loaded2 = manager.loadLoRA("int4-lora", int4_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded2);
    
    // Verify both are loaded with different modes
    auto stats1 = manager.getQuantizationStats("int8-lora");
    auto stats2 = manager.getQuantizationStats("int4-lora");
    
    ASSERT_TRUE(stats1.has_value());
    ASSERT_TRUE(stats2.has_value());
    
    EXPECT_EQ(stats1->mode, QuantizationMode::INT8);
    EXPECT_EQ(stats2->mode, QuantizationMode::INT4);
}

TEST_F(LoRAAdapterUnitTest, QuantizedLoRACanBeApplied) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    auto apply_path = createMockAdapter("apply-quant");
    bool loaded = manager.loadLoRA("apply-quant", apply_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    manager.setApplyAdapterFn([](const LoRASlot&) { return true; });
    bool applied = manager.applyLoRA("apply-quant", nullptr);
    EXPECT_TRUE(applied);
    
    // Should still be loaded
    EXPECT_TRUE(manager.isLoRALoaded("apply-quant"));
}

TEST_F(LoRAAdapterUnitTest, QuantizedLoRACanBeUnloaded) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    auto unload_path = createMockAdapter("unload-quant");
    bool loaded = manager.loadLoRA("unload-quant", unload_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    bool unloaded = manager.unloadLoRA("unload-quant");
    EXPECT_TRUE(unloaded);
    
    // Should not be loaded anymore
    EXPECT_FALSE(manager.isLoRALoaded("unload-quant"));
    
    // Stats should not be available
    auto stats = manager.getQuantizationStats("unload-quant");
    EXPECT_FALSE(stats.has_value());
}

TEST_F(LoRAAdapterUnitTest, QuantizationAllowsMoreLoRAsInMemory) {
    config_.max_lora_vram_mb = 100;  // Limit VRAM
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load multiple quantized LoRAs - should fit more than unquantized
    int loaded_count = 0;
    for (int i = 0; i < 20; ++i) {
        std::string lora_id = "mem-lora-" + std::to_string(i);
        std::string path = createMockAdapter("mem-lora-" + std::to_string(i));
        if (manager.loadLoRA(lora_id, path, 
                            "base-model", true, 1.0f)) {
            loaded_count++;
        }
    }
    
    // With 4× compression, should load significantly more
    EXPECT_GE(loaded_count, 3);  // At least 3 should fit
}

TEST_F(LoRAAdapterUnitTest, QuantizationWithBatchInference) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    config_.enable_multi_lora_batch = true;
    MultiLoRAManager manager(config_);
    
    // Load quantized LoRAs
    auto batch1_path = createMockAdapter("batch-quant-1");
    auto batch2_path = createMockAdapter("batch-quant-2");
    manager.loadLoRA("batch-quant-1", batch1_path, "base-model", true, 1.0f);
    manager.loadLoRA("batch-quant-2", batch2_path, "base-model", true, 1.0f);
    
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

TEST_F(LoRAAdapterUnitTest, QuantizationStatsForNonExistentLoRA) {
    MultiLoRAManager manager(config_);
    
    auto stats = manager.getQuantizationStats("non-existent");
    EXPECT_FALSE(stats.has_value());
}

TEST_F(LoRAAdapterUnitTest, INT4GroupSizeConfiguration) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT4;
    config_.quantization.group_size = 64;  // Custom group size
    MultiLoRAManager manager(config_);
    
    auto group_path = createMockAdapter("group-lora");
    bool loaded = manager.loadLoRA("group-lora", group_path, "base-model", true, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto stats = manager.getQuantizationStats("group-lora");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->mode, QuantizationMode::INT4);
}

TEST_F(LoRAAdapterUnitTest, QuantizationPreservesLoRAMetadata) {
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    auto meta_path = createMockAdapter("meta-lora");
    bool loaded = manager.loadLoRA("meta-lora", meta_path, "specific-model", true, 0.8f);
    EXPECT_TRUE(loaded);
    
    auto info = manager.getLoRAInfo("meta-lora");
    ASSERT_TRUE(info.has_value());
    
    EXPECT_EQ(info->id, "meta-lora");
    EXPECT_EQ(info->base_model_id, "specific-model");
    EXPECT_FLOAT_EQ(info->scale, 0.8f);
}

// ═══════════════════════════════════════════════════════════
// Security Validator Integration Tests (v1.20.0)
// ═══════════════════════════════════════════════════════════

/**
 * @brief Stub LoRASecurityValidator that unconditionally passes metadata checks.
 *
 * Used to verify that a passing validator does not block LoRA loading.
 */
class AlwaysPassValidator : public themis::llm::LoRASecurityValidator {
public:
    AlwaysPassValidator()
        : themis::llm::LoRASecurityValidator(themis::llm::LoRASecurityConfig{}) {}

    bool validateMetadata(const std::string& /*lora_path*/) {
        return true;
    }
};

/**
 * @brief Stub LoRASecurityValidator that unconditionally fails metadata checks.
 *
 * Used to verify that a failing validator blocks LoRA loading when
 * enforce_security_validation is true.
 */
class AlwaysFailValidator : public themis::llm::LoRASecurityValidator {
public:
    AlwaysFailValidator()
        : themis::llm::LoRASecurityValidator(themis::llm::LoRASecurityConfig{}) {}

    bool validateMetadata(const std::string& /*lora_path*/) {
        return false;
    }
};

class LoRASecurityValidatorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "themis_sec_validator_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string createMockAdapter(const std::string& name, size_t size_bytes = 512) {
        auto path = test_dir_ / (name + ".bin");
        std::ofstream file(path, std::ios::binary);
        for (size_t i = 0; i < size_bytes; ++i) {
            file.put(static_cast<char>(i % 256));
        }
        file.close();
        return path.string();
    }

    std::string createMockGGUFWithRank(const std::string& name, int rank) {
        const auto path = test_dir_ / (name + ".gguf");
        std::vector<uint8_t> buf;
        auto append_raw = [&](const void* data, size_t size) {
            const auto* p = static_cast<const uint8_t*>(data);
            buf.insert(buf.end(), p, p + size);
        };

        buf.insert(buf.end(), {'G', 'G', 'U', 'F'});
        uint32_t version = 3;
        append_raw(&version, sizeof(version));
        uint64_t tensor_count = 1;
        append_raw(&tensor_count, sizeof(tensor_count));
        uint64_t kv_count = 1;
        append_raw(&kv_count, sizeof(kv_count));

        const std::string rank_key = "lora.rank";
        uint64_t rank_key_len = rank_key.size();
        append_raw(&rank_key_len, sizeof(rank_key_len));
        append_raw(rank_key.data(), rank_key.size());
        uint32_t value_type_string = 8;  // GGUFValueType::STRING
        append_raw(&value_type_string, sizeof(value_type_string));
        const std::string rank_value = std::to_string(rank);
        uint64_t rank_value_len = rank_value.size();
        append_raw(&rank_value_len, sizeof(rank_value_len));
        append_raw(rank_value.data(), rank_value.size());

        const std::string tensor_name = "w.one";
        uint64_t tensor_name_len = tensor_name.size();
        append_raw(&tensor_name_len, sizeof(tensor_name_len));
        append_raw(tensor_name.data(), tensor_name.size());
        uint32_t n_dims = 1;
        append_raw(&n_dims, sizeof(n_dims));
        uint64_t dim = 1;
        append_raw(&dim, sizeof(dim));
        uint32_t tensor_type_f32 = 0;  // GGMLType::F32
        append_raw(&tensor_type_f32, sizeof(tensor_type_f32));
        uint64_t tensor_offset = 0;
        append_raw(&tensor_offset, sizeof(tensor_offset));

        const size_t aligned = ((buf.size() + 31) / 32) * 32;
        buf.resize(aligned + sizeof(float), 0);

        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
        out.close();
        return path.string();
    }

    std::filesystem::path test_dir_;
};

/// LSV-01: When no security_validator is configured, loadLoRA succeeds normally.
TEST_F(LoRASecurityValidatorIntegrationTest, NoValidatorAllowsLoRALoad) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 512;
    config.lora_base_dir = test_dir_.string();
    // security_validator left as nullptr (default)

    MultiLoRAManager manager(config);
    auto adapter_path = createMockAdapter("no-validator-lora");
    bool loaded = manager.loadLoRA("no-validator-lora", adapter_path, "base-model", 1.0f);
    EXPECT_TRUE(loaded) << "loadLoRA must succeed when no security validator is configured";
}

/// LSV-02: When a passing security_validator is configured, loadLoRA succeeds.
TEST_F(LoRASecurityValidatorIntegrationTest, PassingValidatorAllowsLoRALoad) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 512;
    config.lora_base_dir = test_dir_.string();
    config.security_validator = std::make_shared<AlwaysPassValidator>();
    config.enforce_security_validation = true;

    MultiLoRAManager manager(config);
    auto adapter_path = createMockAdapter("pass-validator-lora");
    bool loaded = manager.loadLoRA("pass-validator-lora", adapter_path, "base-model", 1.0f);
    EXPECT_TRUE(loaded) << "loadLoRA must succeed when security validator approves the adapter";
}

/// LSV-03: When a failing security_validator is configured and enforcement is enabled,
///         loadLoRA must be rejected.
TEST_F(LoRASecurityValidatorIntegrationTest, FailingValidatorEnforcedRejectsLoRALoad) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 512;
    config.lora_base_dir = test_dir_.string();
    config.security_validator = std::make_shared<AlwaysFailValidator>();
    config.enforce_security_validation = true;

    MultiLoRAManager manager(config);
    auto adapter_path = createMockAdapter("fail-validator-lora");
    bool loaded = manager.loadLoRA("fail-validator-lora", adapter_path, "base-model", 1.0f);
    EXPECT_FALSE(loaded) << "loadLoRA must be rejected when security validator fails and enforcement is enabled";
}

/// LSV-04: When a failing security_validator is configured but enforcement is disabled,
///         loadLoRA logs a warning and continues (non-blocking).
TEST_F(LoRASecurityValidatorIntegrationTest, FailingValidatorNotEnforcedAllowsLoRALoad) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 512;
    config.lora_base_dir = test_dir_.string();
    config.security_validator = std::make_shared<AlwaysFailValidator>();
    config.enforce_security_validation = false;  // warn-only mode

    MultiLoRAManager manager(config);
    auto adapter_path = createMockAdapter("fail-nonenforced-lora");
    bool loaded = manager.loadLoRA("fail-nonenforced-lora", adapter_path, "base-model", 1.0f);
    EXPECT_TRUE(loaded) << "loadLoRA must succeed when security validation failure is non-enforced (warn-only)";
}

/// IVB-01: GGUF rank extracted from metadata must respect bounds in loadLoRAInternal().
TEST_F(LoRASecurityValidatorIntegrationTest, RejectsOutOfBoundsRankFromGGUFMetadata) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 512;
    config.lora_base_dir = test_dir_.string();

    MultiLoRAManager manager(config);
    auto adapter_path = createMockGGUFWithRank("rank-oob-lora", 9999);
    bool loaded = manager.loadLoRA("rank-oob-lora", adapter_path, "base-model", 1.0f);
    EXPECT_FALSE(loaded) << "loadLoRA must reject out-of-bounds LoRA rank extracted from GGUF metadata";
}

// ═══════════════════════════════════════════════════════════
// Note: main() removed - GTest will provide its own when linked with gtest_main
// Individual test cases remain for execution by GTest framework
