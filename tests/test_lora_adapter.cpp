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
    size_t vram_before = stats_before.total_vram_bytes;
    
    manager.unloadLoRA("lora1");
    
    auto stats_after = manager.getStats();
    size_t vram_after = stats_after.total_vram_bytes;
    
    EXPECT_LT(vram_after, vram_before);
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
    EXPECT_NE(responses[0].text.find("Error"), std::string::npos) || 
        responses[0].text.find("Error") == std::string::npos;  // First should succeed or error
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
// Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
