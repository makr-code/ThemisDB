/**
 * @file test_llama_cpp_real_inference.cpp
 * @brief End-to-end real-inference integration tests for LlamaCppPlugin and LlamaWrapper.
 *
 * Test groups
 * ───────────
 *   A – Plugin load lifecycle
 *   B – Real generate()
 *   C – Real streaming via stream_callback / generateStream()
 *   D – Real embeddings via LlamaCppPlugin::embed()
 *   E – Error & edge cases
 *
 * Model discovery
 * ───────────────
 *   1. THEMIS_TEST_MODEL_PATH environment variable
 *   2. Filesystem scan: ./models/, ../models/, ../../models/ for well-known
 *      TinyLlama GGUF filenames.
 *
 * Skip policy
 * ───────────
 *   • THEMIS_ENABLE_LLM not defined → every test GTEST_SKIP()s immediately.
 *   • No GGUF model found        → every test GTEST_SKIP()s with a hint.
 *   • Model found + flag set     → real llama.cpp inference executes.
 *
 * @note Do NOT add stub / simulation paths to this file.  Its sole purpose is
 *       to validate real llama.cpp inference.  Stub-mode tests live in
 *       src/llama_cpp/tests/test_llama_cpp_plugin.cpp.
 *
 * @author ThemisDB Team
 * @date 2026
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include <filesystem>
#include <cstdlib>
#include <atomic>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

#ifdef THEMIS_ENABLE_LLM
#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/llm_plugin_interface.h"
#endif

using namespace themis;

#ifdef THEMIS_ENABLE_LLM
using namespace themis::llamacpp;
using namespace themis::llm;
#endif

// ═══════════════════════════════════════════════════════════
// Test fixture
// ═══════════════════════════════════════════════════════════

class LlamaCppRealInferenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* env_path = std::getenv("THEMIS_TEST_MODEL_PATH");
        if (env_path && std::filesystem::exists(env_path)) {
            model_path_      = env_path;
            model_available_ = true;
            spdlog::info("[LlamaCppRealInferenceTest] model from env: {}", model_path_);
        } else {
            for (const auto& root : {".", "./models", "../models", "../../models"}) {
                for (const auto& name : {
                        "TinyLlama-1.1B-Chat-v1.0.gguf",
                        "tinyllama-1.1b-chat-v1.0.gguf",
                        "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf",
                        "tinyllama_1.1b.gguf",
                        "test_model.gguf"}) {
                    auto p = std::filesystem::path(root) / name;
                    if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
                        model_path_      = p.string();
                        model_available_ = true;
                        spdlog::info("[LlamaCppRealInferenceTest] found model: {}", model_path_);
                        break;
                    }
                }
                if (model_available_) break;
            }
        }
        if (!model_available_) {
            spdlog::warn("[LlamaCppRealInferenceTest] no GGUF model found — all real-inference tests will be skipped.");
            spdlog::info("  Set THEMIS_TEST_MODEL_PATH or place a GGUF file in ./models/");
        }
    }

    void TearDown() override {}

#ifdef THEMIS_ENABLE_LLM
    /**
     * Create a loaded LlamaCppPlugin.
     * @param[out] plugin  Plugin to initialise.
     * @return true on successful loadModel().
     */
    bool makePlugin(LlamaCppPlugin& plugin, int n_ctx = 512, int n_batch = 128) {
        nlohmann::json cfg;
        cfg["n_ctx"]   = n_ctx;
        cfg["n_batch"] = n_batch;
        return plugin.loadModel(model_path_, cfg);
    }

    /** Build a simple InferenceRequest. */
    static llm::InferenceRequest makeReq(
            const std::string& prompt,
            int max_tokens    = 16,
            float temperature = 0.0f) {
        llm::InferenceRequest req;
        req.prompt      = prompt;
        req.max_tokens  = max_tokens;
        req.temperature = temperature;
        return req;
    }

    /** Compute L2 distance between two float vectors of equal size. */
    static double l2Distance(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size() || a.empty()) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            double diff = static_cast<double>(a[i] - b[i]);
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
#endif

    std::string model_path_;
    bool model_available_ = false;
};

// ═══════════════════════════════════════════════════════════
// Group A – Plugin load lifecycle
// ═══════════════════════════════════════════════════════════

/** A1: loadModel() returns true for a valid GGUF file. */
TEST_F(LlamaCppRealInferenceTest, A1_LoadModel_ReturnsTrue) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    EXPECT_TRUE(makePlugin(plugin)) << "loadModel() should return true for a valid GGUF";
#endif
}

/** A2: isModelLoaded() is true after a successful load. */
TEST_F(LlamaCppRealInferenceTest, A2_IsModelLoaded_TrueAfterLoad) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    EXPECT_TRUE(plugin.isModelLoaded()) << "isModelLoaded() should be true after loadModel()";
#endif
}

/** A3: getModelInfo() returns valid info with a non-empty model_id. */
TEST_F(LlamaCppRealInferenceTest, A3_GetModelInfo_ValidInfo) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    auto info = plugin.getModelInfo();
    ASSERT_TRUE(info.has_value()) << "getModelInfo() should return a value when model is loaded";
    EXPECT_FALSE(info->model_id.empty()) << "model_id should not be empty";
    spdlog::info("[A3] model_id={} context_length={}", info->model_id, info->context_length);
#endif
}

/** A4: unloadModel() causes isModelLoaded() to return false. */
TEST_F(LlamaCppRealInferenceTest, A4_UnloadModel_IsLoadedFalse) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    plugin.unloadModel();
    EXPECT_FALSE(plugin.isModelLoaded()) << "isModelLoaded() should be false after unloadModel()";
#endif
}

// ═══════════════════════════════════════════════════════════
// Group B – Real generate()
// ═══════════════════════════════════════════════════════════

/** B1: generate() with a simple prompt returns success=true. */
TEST_F(LlamaCppRealInferenceTest, B1_Generate_SuccessTrue) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    auto resp = plugin.generate(makeReq("Hello, world!", 16));
    EXPECT_TRUE(resp.success) << "generate() should succeed. error=" << resp.error_message;
#endif
}

/** B2: Generated text field is non-empty. */
TEST_F(LlamaCppRealInferenceTest, B2_Generate_TextNonEmpty) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    auto resp = plugin.generate(makeReq("The capital of France is", 8));
    ASSERT_TRUE(resp.success) << resp.error_message;
    EXPECT_FALSE(resp.text.empty()) << "Generated text should not be empty";
    spdlog::info("[B2] text='{}'", resp.text);
#endif
}

/** B3: tokens_generated > 0. */
TEST_F(LlamaCppRealInferenceTest, B3_Generate_TokensGeneratedPositive) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    auto resp = plugin.generate(makeReq("One plus one equals", 8));
    ASSERT_TRUE(resp.success) << resp.error_message;
    EXPECT_GT(resp.tokens_generated, 0) << "tokens_generated should be > 0";
    spdlog::info("[B3] tokens_generated={}", resp.tokens_generated);
#endif
}

/** B4: generate() with max_tokens=1 produces at most a few tokens. */
TEST_F(LlamaCppRealInferenceTest, B4_Generate_MaxTokens1_LimitRespected) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    auto resp = plugin.generate(makeReq("The sky is", /*max_tokens=*/1));
    ASSERT_TRUE(resp.success) << resp.error_message;
    // Allow a tiny overrun (EOS can cause off-by-one in some backends)
    EXPECT_LE(resp.tokens_generated, 4) << "max_tokens=1 should produce very few tokens";
    spdlog::info("[B4] tokens_generated={} text='{}'", resp.tokens_generated, resp.text);
#endif
}

/** B5: Two calls with same prompt + temperature=0.0 produce identical output. */
TEST_F(LlamaCppRealInferenceTest, B5_Generate_Deterministic_Temperature0) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));
    const auto req = makeReq("Count: one two three", 12, 0.0f);
    auto resp1 = plugin.generate(req);
    auto resp2 = plugin.generate(req);
    ASSERT_TRUE(resp1.success) << "First call failed: " << resp1.error_message;
    ASSERT_TRUE(resp2.success) << "Second call failed: " << resp2.error_message;
    EXPECT_EQ(resp1.text, resp2.text)
        << "Deterministic generation (temp=0) must produce identical output";
    spdlog::info("[B5] deterministic output='{}'", resp1.text);
#endif
}

// ═══════════════════════════════════════════════════════════
// Group C – Real streaming
// ═══════════════════════════════════════════════════════════

/** C1: generate() with stream_callback delivers at least 1 token via callback. */
TEST_F(LlamaCppRealInferenceTest, C1_Streaming_CallbackReceivesTokens) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    std::atomic<int> token_count{0};
    auto req = makeReq("Hello streaming world", 16, 0.0f);
    req.stream_callback = [&token_count](const std::string& /*token*/) {
        token_count.fetch_add(1, std::memory_order_relaxed);
    };

    auto resp = plugin.generate(req);
    ASSERT_TRUE(resp.success) << resp.error_message;
    EXPECT_GT(token_count.load(), 0) << "stream_callback should have been called at least once";
    spdlog::info("[C1] stream callback calls={}", token_count.load());
#endif
}

/** C2: Concatenated callback tokens equal the final response.text. */
TEST_F(LlamaCppRealInferenceTest, C2_Streaming_ConcatenatedTokensMatchText) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    std::string streamed;
    auto req = makeReq("The answer is", 12, 0.0f);
    req.stream_callback = [&streamed](const std::string& token) {
        streamed += token;
    };

    auto resp = plugin.generate(req);
    ASSERT_TRUE(resp.success) << resp.error_message;
    EXPECT_EQ(streamed, resp.text)
        << "Concatenated stream tokens should equal response.text";
    spdlog::info("[C2] streamed='{}' resp.text='{}'", streamed, resp.text);
#endif
}

/** C3: generateStream() convenience method works and produces non-empty text. */
TEST_F(LlamaCppRealInferenceTest, C3_GenerateStream_ConvenienceMethod) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    std::string streamed;
    std::function<void(const std::string&)> cb = [&streamed](const std::string& token) {
        streamed += token;
    };

    auto resp = plugin.generateStream(makeReq("The sky is", 12, 0.0f), cb);
    ASSERT_TRUE(resp.success) << resp.error_message;
    EXPECT_FALSE(resp.text.empty()) << "generateStream() should return non-empty text";
    EXPECT_EQ(streamed, resp.text) << "Stream tokens should match response text";
    spdlog::info("[C3] generateStream text='{}'", resp.text);
#endif
}

// ═══════════════════════════════════════════════════════════
// Group D – Real embeddings
// ═══════════════════════════════════════════════════════════

/** D1: embed() returns a vector with dimension > 0. */
TEST_F(LlamaCppRealInferenceTest, D1_Embed_DimensionPositive) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    auto emb = plugin.embed("Hello world");
    if (emb.empty()) {
        GTEST_SKIP() << "This model / build does not expose embedding output — skipping D1";
    }
    EXPECT_GT(emb.size(), 0u) << "embed() should return a non-empty vector";
    spdlog::info("[D1] embedding dim={}", emb.size());
#endif
}

/** D2: Embedding dimension is consistent across two calls. */
TEST_F(LlamaCppRealInferenceTest, D2_Embed_DimensionConsistent) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    auto emb1 = plugin.embed("First sentence.");
    auto emb2 = plugin.embed("Second sentence.");
    if (emb1.empty() || emb2.empty()) {
        GTEST_SKIP() << "This model / build does not expose embedding output — skipping D2";
    }
    EXPECT_EQ(emb1.size(), emb2.size())
        << "Embedding dimension should be constant across calls";
    spdlog::info("[D2] dim={}", emb1.size());
#endif
}

/** D3: Different prompts produce different embeddings (L2 distance > 0). */
TEST_F(LlamaCppRealInferenceTest, D3_Embed_DifferentPromptsAreDifferent) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    auto emb_a = plugin.embed("The cat sat on the mat.");
    auto emb_b = plugin.embed("Quantum computing leverages superposition.");
    if (emb_a.empty() || emb_b.empty()) {
        GTEST_SKIP() << "This model / build does not expose embedding output — skipping D3";
    }
    ASSERT_EQ(emb_a.size(), emb_b.size());
    double dist = l2Distance(emb_a, emb_b);
    EXPECT_GT(dist, 0.0)
        << "Different prompts should produce different embeddings (L2 dist > 0)";
    spdlog::info("[D3] L2 distance between embeddings={}", dist);
#endif
}

// ═══════════════════════════════════════════════════════════
// Group E – Error & edge cases
// ═══════════════════════════════════════════════════════════

/** E1: generate() with an empty prompt does not crash; returns gracefully. */
TEST_F(LlamaCppRealInferenceTest, E1_EmptyPrompt_GracefulReturn) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    // An empty prompt may succeed or fail, but must not hang / crash.
    auto resp = plugin.generate(makeReq("", 8));
    // No assertion on success — behaviour is implementation-defined for empty input.
    // We only assert the call returned at all.
    spdlog::info("[E1] empty prompt: success={} error='{}' text='{}'",
                 resp.success, resp.error_message, resp.text);
    SUCCEED() << "generate() with empty prompt completed without crashing";
#endif
}

/** E2: generate() with max_tokens=0 does not hang. */
TEST_F(LlamaCppRealInferenceTest, E2_MaxTokensZero_DoesNotHang) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "Real inference test requires GGUF model (set THEMIS_TEST_MODEL_PATH)";
    }
    LlamaCppPlugin plugin;
    ASSERT_TRUE(makePlugin(plugin));

    auto resp = plugin.generate(makeReq("Hello", /*max_tokens=*/0));
    // Must complete in finite time (timeout handled by CTest TIMEOUT).
    spdlog::info("[E2] max_tokens=0: success={} tokens_generated={} text='{}'",
                 resp.success, resp.tokens_generated, resp.text);
    SUCCEED() << "generate() with max_tokens=0 completed without hanging";
#endif
}

/** E3: Unloaded plugin returns success=false with a non-empty error_message. */
TEST_F(LlamaCppRealInferenceTest, E3_UnloadedPlugin_ErrorResponse) {
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    // This test intentionally does NOT load a model — it only verifies the
    // error path, so no model_available_ check is needed.
    LlamaCppPlugin plugin;  // freshly constructed, model NOT loaded

    auto resp = plugin.generate(makeReq("Hello", 8));
    EXPECT_FALSE(resp.success)
        << "generate() on unloaded plugin should return success=false";
    EXPECT_FALSE(resp.error_message.empty())
        << "generate() on unloaded plugin should return a non-empty error_message";
    spdlog::info("[E3] unloaded error_message='{}'", resp.error_message);
#endif
}
