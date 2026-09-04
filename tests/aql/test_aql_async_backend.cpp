/**
 * @file test_aql_async_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "aql/iasync_llm_backend.h"
#include <chrono>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::aql;
using namespace themis::llm;

// ============================================================================
// Stub ILLMPlugin implementation
// ============================================================================

/**
 * Minimal stub plugin that returns fixed responses without any LLM.
 */
class StubLLMPlugin : public ILLMPlugin {
public:
    std::string infer_response_text = "stub_response";
    std::vector<float> embed_response = {0.1f, 0.2f, 0.3f};
    bool multimodal_supported = false;

    bool should_throw_infer = false;
    bool should_throw_embed = false;
    std::string throw_message = "stub_error";

    // ── Model management ────────────────────────────────────────
    bool loadModel(const std::string& /*path*/, const nlohmann::json& /*cfg*/ = {}) override {
        return true;
    }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override { return std::nullopt; }
    bool isModelLoaded() const override { return true; }

    // ── LoRA management ─────────────────────────────────────────
    bool loadLoRA(const std::string& /*id*/, const std::string& /*path*/,
                  float /*scale*/ = 1.0f) override { return true; }
    bool unloadLoRA(const std::string& /*id*/) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }

    // ── Inference ───────────────────────────────────────────────
    InferenceResponse generate(const InferenceRequest& /*req*/) override {
        if (should_throw_infer) {
          throw std::runtime_error(throw_message);
        }
        InferenceResponse resp;
        resp.text = infer_response_text;
        return resp;
    }

    InferenceResponse generateRAG(const RAGContext& /*ctx*/,
                                   const InferenceRequest& req) override {
        return generate(req);
    }

    std::vector<float> embed(const std::string& /*text*/) override {
        if (should_throw_embed) {
          throw std::runtime_error(throw_message);
        }
        return embed_response;
    }

    // ── Capabilities ─────────────────────────────────────────────
    LLMCapabilities getCapabilities() const override {
        LLMCapabilities caps;
        caps.supports_multimodal = multimodal_supported;
        return caps;
    }

    nlohmann::json getMemoryStats() const override { return {}; }
    nlohmann::json getPerformanceStats() const override { return {}; }

    // ── Distributed ──────────────────────────────────────────────
    std::vector<uint8_t> exportLoRA(const std::string& /*id*/) override { return {}; }
    bool importLoRA(const std::string& /*id*/,
                    const std::vector<uint8_t>& /*data*/) override { return true; }
};

// ============================================================================
// Minimal concrete IAsyncLLMBackend for interface test
// ============================================================================

/**
 * Simplest possible concrete implementation of IAsyncLLMBackend.
 * Returns pre-set values synchronously via a ready-future.
 */
class MinimalAsyncBackend : public IAsyncLLMBackend {
public:
    std::string response_text = "hello";
    std::vector<float> embed_vec = {1.0f, 2.0f};

    std::future<themis::Result<std::string>>
    inferAsync(const InferenceRequest& /*req*/) override {
        std::promise<themis::Result<std::string>> p;
        p.set_value(response_text);
        return p.get_future();
    }

    std::future<themis::Result<std::vector<float>>>
    embedAsync(const std::string& /*text*/) override {
        std::promise<themis::Result<std::vector<float>>> p;
        p.set_value(embed_vec);
        return p.get_future();
    }
};

// ============================================================================
// IAsyncLLMBackend interface tests
// ============================================================================

TEST(IAsyncLLMBackendTest, CanImplementInterface) {
    MinimalAsyncBackend backend;
    EXPECT_FALSE(backend.supportsMultiModal());  // default
}

TEST(IAsyncLLMBackendTest, InferAsync_ReturnsCorrectText) {
    MinimalAsyncBackend backend;
    backend.response_text = "test_output";

    InferenceRequest req;
    req.prompt = "Hello";

    auto fut = backend.inferAsync(req);
    auto result = fut.get();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "test_output");
}

TEST(IAsyncLLMBackendTest, EmbedAsync_ReturnsCorrectVector) {
    MinimalAsyncBackend backend;
    backend.embed_vec = {0.5f, 1.5f, 2.5f};

    auto fut = backend.embedAsync("some text");
    auto result = fut.get();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), std::size_t(3));
    EXPECT_FLOAT_EQ(result.value()[0], 0.5f);
    EXPECT_FLOAT_EQ(result.value()[1], 1.5f);
    EXPECT_FLOAT_EQ(result.value()[2], 2.5f);
}

// ============================================================================
// ThreadPoolAsyncLLMBackend tests
// ============================================================================

TEST(ThreadPoolAsyncLLMBackendTest, NullPlugin_ThrowsOnConstruction) {
    EXPECT_THROW(
        ThreadPoolAsyncLLMBackend(nullptr),
        std::invalid_argument
    );
}

TEST(ThreadPoolAsyncLLMBackendTest, InferAsync_ReturnsStubText) {
    auto plugin = std::make_shared<StubLLMPlugin>();
    plugin->infer_response_text = "async_response";

    ThreadPoolAsyncLLMBackend backend(plugin);

    InferenceRequest req;
    req.prompt = "Test prompt";

    auto fut = backend.inferAsync(req);
    auto result = fut.get();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "async_response");
}

TEST(ThreadPoolAsyncLLMBackendTest, EmbedAsync_ReturnsStubVector) {
    auto plugin = std::make_shared<StubLLMPlugin>();
    plugin->embed_response = {0.1f, 0.2f, 0.3f, 0.4f};

    ThreadPoolAsyncLLMBackend backend(plugin);

    auto fut = backend.embedAsync("embed this");
    auto result = fut.get();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), std::size_t(4));
    EXPECT_FLOAT_EQ(result.value()[0], 0.1f);
    EXPECT_FLOAT_EQ(result.value()[3], 0.4f);
}

TEST(ThreadPoolAsyncLLMBackendTest, InferAsync_PluginThrows_ReturnsError) {
    auto plugin = std::make_shared<StubLLMPlugin>();
    plugin->should_throw_infer = true;
    plugin->throw_message      = "inference_failure";

    ThreadPoolAsyncLLMBackend backend(plugin);

    InferenceRequest req;
    req.prompt = "Test";

    auto fut = backend.inferAsync(req);
    auto result = fut.get();

    EXPECT_FALSE(result.has_value());
    // The error message should contain the original exception message.
    EXPECT_FALSE(result.error().context().empty());
}

TEST(ThreadPoolAsyncLLMBackendTest, EmbedAsync_PluginThrows_ReturnsError) {
    auto plugin = std::make_shared<StubLLMPlugin>();
    plugin->should_throw_embed = true;
    plugin->throw_message      = "embed_failure";

    ThreadPoolAsyncLLMBackend backend(plugin);

    auto fut = backend.embedAsync("text");
    auto result = fut.get();

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().context().empty());
}

TEST(ThreadPoolAsyncLLMBackendTest, SupportsMultiModal_WhenPluginSays_ReturnsTrue) {
    auto plugin = std::make_shared<StubLLMPlugin>();
    plugin->multimodal_supported = true;

    ThreadPoolAsyncLLMBackend backend(plugin);

    EXPECT_TRUE(backend.supportsMultiModal());
}

TEST(ThreadPoolAsyncLLMBackendTest, SupportsMultiModal_WhenPluginSaysNot_ReturnsFalse) {
    auto plugin = std::make_shared<StubLLMPlugin>();
    plugin->multimodal_supported = false;

    ThreadPoolAsyncLLMBackend backend(plugin);

    EXPECT_FALSE(backend.supportsMultiModal());
}

TEST(ThreadPoolAsyncLLMBackendTest, InferAsync_IsActuallyAsync) {
    // Verify the call returns a future before the work completes
    // (i.e. it runs on a separate thread).
    auto plugin = std::make_shared<StubLLMPlugin>();

    ThreadPoolAsyncLLMBackend backend(plugin);

    InferenceRequest req;
    req.prompt = "async check";

    auto start = std::chrono::steady_clock::now();
    auto fut   = backend.inferAsync(req);

    // We should be able to get the result without deadlock.
    auto result = fut.get();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_TRUE(result.has_value());
    // Sanity check: completed within a reasonable time
    EXPECT_LT(elapsed, std::chrono::seconds(5));
}
