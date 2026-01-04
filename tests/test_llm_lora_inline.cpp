#include <gtest/gtest.h>
#include "llm/llm_plugin_interface.h"
#include "llm/llm_plugin_manager.h"
#include "llm/multi_lora_manager.h"
#include <optional>
#include <string>
#include <vector>
#include <algorithm>

using themis::llm::InferenceRequest;
using themis::llm::InferenceResponse;
using themis::llm::LLMCapabilities;
using themis::llm::LLMPluginManager;
using themis::llm::LoRAInfo;
using themis::llm::ModelInfo;
using themis::llm::MultiLoRAManager;
using themis::llm::ILLMPlugin;
using themis::llm::json;

namespace {

class FakeLlamaCppPlugin : public ILLMPlugin {
public:
    explicit FakeLlamaCppPlugin(const MultiLoRAManager::Config& cfg = {})
        : config_(cfg), lora_manager_(cfg) {}

    bool loadModel(const std::string& model_path, const json& config = {}) override {
        (void)config;
        model_path_ = model_path;
        model_loaded_ = true;
        return true;
    }

    void unloadModel() override {
        model_loaded_ = false;
        model_path_.clear();
        lora_manager_.evictLRU(config_.max_lora_slots);
        lora_manager_.evictExpired();
    }

    std::optional<ModelInfo> getModelInfo() const override {
        if (!model_loaded_) {
            return std::nullopt;
        }
        ModelInfo info;
        info.name = "fake-llama";
        info.path = model_path_;
        info.model_id = model_path_;
        info.is_loaded = true;
        info.format = "gguf";
        return info;
    }

    bool isModelLoaded() const override { return model_loaded_; }

    bool loadLoRA(const std::string& lora_id, const std::string& lora_path, float scale = 1.0f) override {
        if (!model_loaded_) {
            return false;
        }
        return lora_manager_.loadLoRA(lora_id, lora_path, model_path_, scale);
    }

    bool unloadLoRA(const std::string& lora_id) override {
        return lora_manager_.unloadLoRA(lora_id, true);
    }

    std::vector<LoRAInfo> listLoRAs() const override { return lora_manager_.listLoRAs(); }

    InferenceResponse generate(const InferenceRequest& request) override {
        InferenceResponse resp;
        resp.request_id = request.request_id;
        resp.text = "stubbed";
        resp.model_id = model_path_;
        resp.model_used = model_path_;
        resp.tokens_prompt = static_cast<int>(request.prompt.size());
        resp.tokens_generated = std::min(request.max_tokens, 32);

        if (request.lora_adapter_id) {
            lora_manager_.applyLoRA(*request.lora_adapter_id, nullptr);
            resp.lora_used = request.lora_adapter_id;
        }

        return resp;
    }

    InferenceResponse generateRAG(const themis::llm::RAGContext& rag_context, const InferenceRequest& request) override {
        InferenceRequest rag_request = request;
        rag_request.prompt = rag_context.query + "\n" + request.prompt;
        return generate(rag_request);
    }

    std::vector<float> embed(const std::string& text) override {
        return std::vector<float>(text.size() % 8 + 8, 1.0f);
    }

    LLMCapabilities getCapabilities() const override {
        LLMCapabilities caps;
        caps.supports_instruct = true;
        caps.supports_chat = true;
        caps.supports_completion = true;
        caps.supports_lora = true;
        caps.supports_batching = true;
        caps.supports_streaming = false;
        return caps;
    }

    json getMemoryStats() const override { return lora_manager_.getMemoryStats(); }
    json getPerformanceStats() const override { return lora_manager_.getCacheStats(); }

    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override {
        return lora_manager_.exportLoRA(lora_id);
    }

    bool importLoRA(const std::string& lora_id, const std::vector<uint8_t>& data) override {
        const std::string base_model = model_path_.empty() ? "fake-base" : model_path_;
        return lora_manager_.importLoRA(lora_id, data, base_model);
    }

    json loraCacheStats() const { return lora_manager_.getCacheStats(); }

private:
    MultiLoRAManager::Config config_;
    MultiLoRAManager lora_manager_;
    bool model_loaded_{false};
    std::string model_path_;
};

LLMPluginManager makeManagerWithPlugin(const MultiLoRAManager::Config& cfg = {}) {
    LLMPluginManager mgr;
    mgr.registerPlugin("fake-llama", std::make_unique<FakeLlamaCppPlugin>(cfg));
    mgr.setDefaultPlugin("fake-llama");
    return mgr;
}

} // namespace

TEST(InlineLoRATest, LoadListUnload) {
    auto mgr = makeManagerWithPlugin();
    ASSERT_TRUE(mgr.loadModel("/models/base.gguf"));

    ASSERT_TRUE(mgr.loadLoRA("lora-alpha", "/loras/lora-alpha.bin", "base"));
    auto loras = mgr.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_EQ(loras.front().lora_id, "lora-alpha");

    EXPECT_TRUE(mgr.unloadLoRA("lora-alpha"));
    EXPECT_TRUE(mgr.listLoRAs().empty());
}

TEST(InlineLoRATest, InlineGenerationUsesAdapter) {
    auto mgr = makeManagerWithPlugin();
    ASSERT_TRUE(mgr.loadModel("/models/base.gguf"));
    ASSERT_TRUE(mgr.loadLoRA("lora-beta", "/loras/lora-beta.bin", "base"));

    InferenceRequest req;
    req.prompt = "hello raid";
    req.lora_adapter_id = std::string("lora-beta");
    req.max_tokens = 48;

    auto resp = mgr.generate(req);
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(*resp.lora_used, "lora-beta");
    EXPECT_EQ(resp.tokens_generated, 32);
}

TEST(InlineLoRATest, LruEvictionWhenSlotsExceeded) {
    MultiLoRAManager::Config cfg;
    cfg.max_lora_slots = 1;
    auto mgr = makeManagerWithPlugin(cfg);
    ASSERT_TRUE(mgr.loadModel("/models/base.gguf"));

    ASSERT_TRUE(mgr.loadLoRA("lora-one", "/loras/lora-one.bin", "base"));
    ASSERT_TRUE(mgr.loadLoRA("lora-two", "/loras/lora-two.bin", "base"));

    auto loras = mgr.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_EQ(loras.front().lora_id, "lora-two");

    auto* plugin = dynamic_cast<FakeLlamaCppPlugin*>(mgr.getDefaultPlugin());
    ASSERT_NE(plugin, nullptr);
    auto stats = plugin->loraCacheStats();
    ASSERT_TRUE(stats.contains("evictions"));
    EXPECT_GE(stats["evictions"].get<size_t>(), 1U);
}

TEST(InlineLoRATest, UnloadAndReload) {
    auto mgr = makeManagerWithPlugin();
    ASSERT_TRUE(mgr.loadModel("/models/base.gguf"));
    ASSERT_TRUE(mgr.loadLoRA("lora-reload", "/loras/lora-reload.bin", "base"));

    // Verify loaded
    auto loras = mgr.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_TRUE(loras.front().is_loaded);

    // Unload
    EXPECT_TRUE(mgr.unloadLoRA("lora-reload"));
    EXPECT_TRUE(mgr.listLoRAs().empty());

    // Reload
    EXPECT_TRUE(mgr.loadLoRA("lora-reload", "/loras/lora-reload.bin", "base"));
    loras = mgr.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_EQ(loras.front().lora_id, "lora-reload");
    EXPECT_TRUE(loras.front().is_loaded);
}

TEST(InlineLoRATest, ResponseFieldsPopulatedCorrectly) {
    auto mgr = makeManagerWithPlugin();
    ASSERT_TRUE(mgr.loadModel("/models/test.gguf"));
    ASSERT_TRUE(mgr.loadLoRA("inference-test", "/loras/test.bin", "test"));

    InferenceRequest req;
    req.request_id = "req-123";
    req.prompt = "testing";
    req.max_tokens = 42;
    req.lora_adapter_id = std::string("inference-test");

    auto resp = mgr.generate(req);

    // Verify response fields
    EXPECT_EQ(resp.request_id, "req-123");
    EXPECT_FALSE(resp.text.empty());
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(resp.lora_used.value(), "inference-test");
    EXPECT_GT(resp.tokens_generated, 0);
    EXPECT_GE(resp.inference_time_ms, 0.0f);
    EXPECT_FALSE(resp.model_used.empty());
}
