/**
 * @file test_llm_gossip_adapter_publisher.cpp
 * @brief Focused tests: GossipAdapterPublisher wired into LLMPluginManager.
 *
 * Test IDs: GAP-01 .. GAP-05
 *
 * GAP-01  setAdapterPublisher(nullptr) is safe (no-op)
 * GAP-02  setAdapterPublisher() stores publisher; lastAnnouncement is empty before any load
 * GAP-03  loadLoRA() triggers announce() → lastAnnouncement() reflects the LoRA id
 * GAP-04  unloadLoRA() triggers withdrawal announcement
 * GAP-05  No publisher set → loadLoRA/unloadLoRA work without any gossip call
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_manager.h"
#include "distributed_knowledge/adapter_capability_announcement.h"

#include <atomic>
#include <string>
#include <vector>

using namespace themis::llm;
using namespace themis::distributed_knowledge;

// ─────────────────────────────────────────────────────────────────────────────
// Stub ILLMPlugin that tracks loadLoRA/unloadLoRA calls
// ─────────────────────────────────────────────────────────────────────────────

class StubLLMPlugin final : public themis::llm::ILLMPlugin {
public:
    std::vector<std::string> loaded_loras;
    std::vector<std::string> unloaded_loras;

    // Model Management
    bool loadModel(const std::string& model_path,
                   [[maybe_unused]] const json& config = {}) override { 
        return true; 
    }
    void unloadModel() override {}
    std::optional<themis::llm::ModelInfo> getModelInfo() const override { 
        return std::nullopt; 
    }
    bool isModelLoaded() const override { return true; }
    
    // Inference
    themis::llm::InferenceResponse generate(
        [[maybe_unused]] const themis::llm::InferenceRequest& request) override { 
        return {}; 
    }
    themis::llm::InferenceResponse generateRAG(
        [[maybe_unused]] const themis::llm::RAGContext& rag_context,
        [[maybe_unused]] const themis::llm::InferenceRequest& request) override { 
        return {}; 
    }
    std::vector<float> embed([[maybe_unused]] const std::string& text) override { 
        return {}; 
    }
    
    // Capabilities
    themis::llm::LLMCapabilities getCapabilities() const override { 
        return {}; 
    }
    json getMemoryStats() const override { return {}; }
    json getPerformanceStats() const override { return {}; }
    
    // LoRA Management
    bool loadLoRA(const std::string& lora_id,
                  [[maybe_unused]] const std::string& lora_path,
                  [[maybe_unused]] float scale = 1.0f) override {
        loaded_loras.push_back(lora_id);
        return true;
    }
    bool unloadLoRA(const std::string& lora_id) override {
        unloaded_loras.push_back(lora_id);
        return true;
    }
    std::vector<themis::llm::LoRAInfo> listLoRAs() const override { return {}; }
    
    // Distributed Features
    std::vector<uint8_t> exportLoRA([[maybe_unused]] const std::string& lora_id) override { 
        return {}; 
    }
    bool importLoRA([[maybe_unused]] const std::string& lora_id,
                    [[maybe_unused]] const std::vector<uint8_t>& data) override { 
        return true; 
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class GossipAdapterPublisherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Fresh LLMPluginManager per test (not the singleton)
        manager_ = std::make_unique<LLMPluginManager>();

        // Register a stub plugin
        auto stub = std::make_unique<StubLLMPlugin>();
        stub_raw_ = stub.get();
        manager_->registerPlugin("stub", std::move(stub));
        manager_->setDefaultPlugin("stub");

        // Build a GossipAdapterPublisher that records JSON payloads
        publisher_ = std::make_unique<GossipAdapterPublisher>(
            "shard-test",
            [this](nlohmann::json payload) {
                ++announce_call_count_;
                last_payload_ = std::move(payload);
            });
    }

    std::unique_ptr<LLMPluginManager> manager_;
    StubLLMPlugin* stub_raw_ = nullptr;
    std::unique_ptr<GossipAdapterPublisher> publisher_;
    std::atomic<int> announce_call_count_{0};
    nlohmann::json last_payload_;
};

// ─────────────────────────────────────────────────────────────────────────────
// GAP-01: setAdapterPublisher(nullptr) is a safe no-op
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(GossipAdapterPublisherTest, GAP01_SetNullPublisherIsSafe) {
    EXPECT_NO_THROW(manager_->setAdapterPublisher(nullptr, "shard-1"));
    // loadLoRA should still work without gossip
    EXPECT_TRUE(manager_->loadLoRA("lora-safe", "path.gguf", "base"));
    EXPECT_EQ(announce_call_count_.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// GAP-02: Publisher set; lastAnnouncement is empty before any load
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(GossipAdapterPublisherTest, GAP02_PublisherSetNoAnnounceBeforeLoad) {
    manager_->setAdapterPublisher(publisher_.get(), "shard-test");
    EXPECT_FALSE(publisher_->lastAnnouncement().has_value());
    EXPECT_EQ(announce_call_count_.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// GAP-03: loadLoRA() triggers announce() → publisher carries the LoRA id
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(GossipAdapterPublisherTest, GAP03_LoadLoRATriggersAnnouncement) {
    manager_->setAdapterPublisher(publisher_.get(), "shard-test");

    const bool ok = manager_->loadLoRA("my-lora-v2", "model.gguf", "base");
    EXPECT_TRUE(ok);

    // Plugin received the call
    ASSERT_EQ(stub_raw_->loaded_loras.size(), 1u);
    EXPECT_EQ(stub_raw_->loaded_loras[0], "my-lora-v2");

    // Gossip was triggered
    EXPECT_EQ(announce_call_count_.load(), 1);

    // lastAnnouncement reflects the adapter id
    auto ann = publisher_->lastAnnouncement();
    ASSERT_TRUE(ann.has_value());
    EXPECT_EQ(ann->adapter_id, "my-lora-v2");
    EXPECT_EQ(ann->shard_id,   "shard-test");
}

// ─────────────────────────────────────────────────────────────────────────────
// GAP-04: unloadLoRA() triggers a withdrawal announcement
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(GossipAdapterPublisherTest, GAP04_UnloadLoRASendsWithdrawal) {
    manager_->setAdapterPublisher(publisher_.get(), "shard-test");

    manager_->loadLoRA("my-lora-v2", "model.gguf", "base");
    const int count_after_load = announce_call_count_.load();

    const bool ok = manager_->unloadLoRA("my-lora-v2");
    EXPECT_TRUE(ok);

    // Plugin received unload
    ASSERT_EQ(stub_raw_->unloaded_loras.size(), 1u);
    EXPECT_EQ(stub_raw_->unloaded_loras[0], "my-lora-v2");

    // A second gossip call was made for the withdrawal
    EXPECT_GT(announce_call_count_.load(), count_after_load);

    // The withdrawal announcement carries is_withdrawal=true and the correct adapter_id
    auto ann = publisher_->lastAnnouncement();
    ASSERT_TRUE(ann.has_value());
    EXPECT_TRUE(ann->is_withdrawal);
    EXPECT_EQ(ann->adapter_id, "my-lora-v2");
}

// ─────────────────────────────────────────────────────────────────────────────
// GAP-05: No publisher → loadLoRA/unloadLoRA work, no gossip
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(GossipAdapterPublisherTest, GAP05_NoPublisherNoGossip) {
    // publisher_ not set on manager_

    EXPECT_TRUE(manager_->loadLoRA("lora-no-gossip", "model.gguf", "base"));
    EXPECT_TRUE(manager_->unloadLoRA("lora-no-gossip"));

    // Plugin saw both operations
    EXPECT_EQ(stub_raw_->loaded_loras.size(),   1u);
    EXPECT_EQ(stub_raw_->unloaded_loras.size(), 1u);

    // No gossip calls
    EXPECT_EQ(announce_call_count_.load(), 0);
}
