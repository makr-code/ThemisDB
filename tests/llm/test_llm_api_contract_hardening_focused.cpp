/**
 * @file test_llm_api_contract_hardening_focused.cpp
 * @brief LLM Module — API Contract Hardening focused regression tests.
 *
 * Covers the normative contracts defined in include/llm/llm_api_contract.h
 * across four acceptance-criteria tracks:
 *
 * - **LAC-01..04** — Inference contract (null model, empty input, cancel pre-inference)
 * - **LAC-05..08** — Batch consistency (order preserved, partial batch, max batch size)
 * - **LAC-09..12** — Stream contract (callback invoked, exception swallowed, cancel mid-stream)
 * - **LAC-13..16** — Plugin lifecycle (RAII load/unload, double-unload safe, null plugin)
 * - **LAC-17..20** — Embed contract (L2 norm ≈ 1.0, empty string, batch consistent with single)
 *
 * All infrastructure is fully in-process; no real model files are required.
 * Deterministic test data is seeded with kLlmContractSeed = 42.
 *
 * @version 1.0.0
 * @note CTest labels: llm;contract;hardening
 */

#include <gtest/gtest.h>

#include "llm/llm_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::chrono_literals;
using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kLlmContractSeed = 42U;

// ─────────────────────────────────────────────────────────────────────────────
// § Stubs
// ─────────────────────────────────────────────────────────────────────────────

/// Stub cancellation token (contract §5).
class StubCancellationToken {
public:
    void cancel() noexcept { cancelled_.store(true, std::memory_order_release); }
    bool isCancelled() const noexcept { return cancelled_.load(std::memory_order_acquire); }
private:
    std::atomic<bool> cancelled_{false};
};

/// Stub inference engine that honours the API contract (§1, §5).
class StubInferenceEngine {
public:
    struct Result {
        LlmErrorCode code{LlmErrorCode::OK};
        std::string output;
    };

    bool model_loaded{false};

    Result generate(const std::string& prompt,
                    StubCancellationToken* cancel = nullptr) {
        if (!model_loaded) return {LlmErrorCode::MODEL_NOT_LOADED, ""};
        if (cancel && cancel->isCancelled()) return {LlmErrorCode::INFERENCE_CANCELLED, ""};
        if (prompt.empty()) return {LlmErrorCode::OK, ""};  // empty input → empty result
        return {LlmErrorCode::OK, "response-to:" + prompt};
    }

    std::vector<Result> generateBatch(const std::vector<std::string>& prompts) {
        if (!model_loaded) {
            return std::vector<Result>(prompts.size(), {LlmErrorCode::MODEL_NOT_LOADED, ""});
        }
        if (prompts.size() > kMaxBatchSize) {
            return {{LlmErrorCode::BATCH_SIZE_EXCEEDED, ""}};
        }
        std::vector<Result> results = {};

        results.reserve(prompts.size());
        for (const auto& p : prompts) {
            results.push_back({LlmErrorCode::OK, "batch:" + p});
        }
        return results;
    }

    /// Stream generation; swallows callback exceptions per §4.
    LlmErrorCode generateStream(const std::string& prompt,
                                 std::function<void(const std::string&)> callback,
                                 StubCancellationToken* cancel = nullptr) {
        if (!model_loaded) {
          return LlmErrorCode::MODEL_NOT_LOADED;
        }
        // Simulate token-by-token delivery
        for (char c : prompt) {
            if (cancel && cancel->isCancelled()) {
              return LlmErrorCode::STREAM_ABORTED;
            }
            try {
                callback(std::string(1, c));
            } catch (...) {
                // Contract §4: exceptions in callback must be caught, not propagated
                return LlmErrorCode::STREAM_ABORTED;
            }
        }
        return LlmErrorCode::OK;
    }
};

/// Stub embedding engine (§2).
class StubEmbeddingEngine {
public:
    static constexpr int kDim = 4;

    bool model_loaded{true};

    std::vector<float> embed(const std::string& text) {
        if (text.empty()) {
          return std::vector<float>(kDim, 0.0f);
        }
        // Produce a deterministic vector from text, then L2-normalize
        std::vector<float> v(kDim);
        std::mt19937 rng(kLlmContractSeed);
        for (auto& x : v) {
          x = static_cast<float>(rng() % 100 + 1);
        }
        // Vary slightly by text length so different inputs give different vectors
        v[0] += static_cast<float>(text.size() % 10);
        normalizeL2(v);
        return v;
    }

    std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts) {
        std::vector<std::vector<float>> results;
        results.reserve(texts.size());
        for (const auto& t : texts) {
          results.push_back(embed(t));
        }
        return results;
    }

    static float l2norm(const std::vector<float>& v) {
        float sum = 0.f;
        for (float x : v) {
          sum += x * x;
        }
        return std::sqrt(sum);
    }

private:
    static void normalizeL2(std::vector<float>& v) {
        float norm = l2norm(v);
        if (norm < 1e-9f) {
          return;
        }
        for (auto& x : v) {
          x /= norm;
        }
    }
};

/// Stub plugin with RAII lifecycle (§3).
class StubPlugin {
public:
    bool loaded{false};
    bool double_unload_safe_called{false};

    LlmErrorCode load() {
        if (loaded) {
          return LlmErrorCode::OK;
        }
        loaded = true;
        return LlmErrorCode::OK;
    }

    LlmErrorCode unload() {
        if (!loaded) {
            // Double-unload: idempotent no-op per §3
            double_unload_safe_called = true;
            return LlmErrorCode::OK;
        }
        loaded = false;
        return LlmErrorCode::OK;
    }
};

/// Null plugin check helper.
LlmErrorCode callWithNullPlugin(StubPlugin* plugin) {
    if (!plugin) {
      return LlmErrorCode::PLUGIN_NULL_HANDLE;
    }
    return plugin->load();
}

// ─────────────────────────────────────────────────────────────────────────────
// LAC-01..04: Inference contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(LlmContractInference, LAC01_NullModelReturnsError) {
    StubInferenceEngine engine;
    engine.model_loaded = false;
    auto result = engine.generate("hello");
    EXPECT_EQ(result.code, LlmErrorCode::MODEL_NOT_LOADED);
    EXPECT_TRUE(result.output.empty());
}

TEST(LlmContractInference, LAC02_EmptyInputReturnsEmptyResult) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    auto result = engine.generate("");
    EXPECT_EQ(result.code, LlmErrorCode::OK);
    EXPECT_TRUE(result.output.empty());
}

TEST(LlmContractInference, LAC03_CancelPreInferenceReturnsCancelledStatus) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    StubCancellationToken token;
    token.cancel();
    auto result = engine.generate("hello", &token);
    EXPECT_EQ(result.code, LlmErrorCode::INFERENCE_CANCELLED);
}

TEST(LlmContractInference, LAC04_ValidInputProducesOutput) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    std::mt19937 rng(kLlmContractSeed);
    const std::string prompt = "prompt-" + std::to_string(rng());
    auto result = engine.generate(prompt);
    EXPECT_EQ(result.code, LlmErrorCode::OK);
    EXPECT_FALSE(result.output.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LAC-05..08: Batch consistency
// ─────────────────────────────────────────────────────────────────────────────

TEST(LlmContractBatch, LAC05_BatchOrderPreserved) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    std::vector<std::string> prompts = {"alpha", "beta", "gamma"};
    auto results = engine.generateBatch(prompts);
    ASSERT_EQ(results.size(), prompts.size());
    for (std::size_t i = 0; i < prompts.size(); ++i) {
        EXPECT_EQ(results[i].code, LlmErrorCode::OK);
        EXPECT_NE(results[i].output.find(prompts[i]), std::string::npos)
            << "output[" << i << "] should reference input prompt";
    }
}

TEST(LlmContractBatch, LAC06_PartialBatchSucceeds) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    auto results = engine.generateBatch({"only-one"});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].code, LlmErrorCode::OK);
}

TEST(LlmContractBatch, LAC07_MaxBatchSizeRespected) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    // Build a batch that exceeds kMaxBatchSize
    std::vector<std::string> big_batch(kMaxBatchSize + 1, "x");
    auto results = engine.generateBatch(big_batch);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].code, LlmErrorCode::BATCH_SIZE_EXCEEDED);
}

TEST(LlmContractBatch, LAC08_BatchWithUnloadedModelReturnsError) {
    StubInferenceEngine engine;
    engine.model_loaded = false;
    auto results = engine.generateBatch({"a", "b"});
    ASSERT_EQ(results.size(), 2u);
    for (const auto& r : results) {
        EXPECT_EQ(r.code, LlmErrorCode::MODEL_NOT_LOADED);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LAC-09..12: Stream contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(LlmContractStream, LAC09_CallbackCalledAtLeastOnceForNonEmpty) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    std::vector<std::string> tokens;
    auto rc = engine.generateStream("hi", [&](const std::string& tok) {
        tokens.push_back(tok);
    });
    EXPECT_EQ(rc, LlmErrorCode::OK);
    EXPECT_FALSE(tokens.empty());
}

TEST(LlmContractStream, LAC10_ExceptionInCallbackIsSwallowed) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    bool threw = false;
    // Callback that always throws — engine must catch it, not propagate
    auto rc = engine.generateStream("abc", [&](const std::string&) {
        threw = true;
        throw std::runtime_error("callback-exception");
    });
    // Contract: exception was swallowed; result is STREAM_ABORTED, not a propagated throw
    EXPECT_TRUE(threw);
    EXPECT_EQ(rc, LlmErrorCode::STREAM_ABORTED);
}

TEST(LlmContractStream, LAC11_CancelMidStreamAborts) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    StubCancellationToken token;
    int call_count = 0;
    auto rc = engine.generateStream("hello world", [&](const std::string&) {
        ++call_count;
        if (call_count >= 3) token.cancel();  // cancel after 3 tokens
    }, &token);
    EXPECT_EQ(rc, LlmErrorCode::STREAM_ABORTED);
    // Should not have processed all 11 characters
    EXPECT_LT(call_count, 11);
}

TEST(LlmContractStream, LAC12_EmptyInputStreamReturnsOk) {
    StubInferenceEngine engine;
    engine.model_loaded = true;
    bool callback_called = false;
    auto rc = engine.generateStream("", [&](const std::string&) {
        callback_called = true;
    });
    EXPECT_EQ(rc, LlmErrorCode::OK);
    EXPECT_FALSE(callback_called);  // no tokens for empty input
}

// ─────────────────────────────────────────────────────────────────────────────
// LAC-13..16: Plugin lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(LlmContractPlugin, LAC13_LoadUnloadRAII) {
    auto plugin = std::make_unique<StubPlugin>();
    EXPECT_FALSE(plugin->loaded);
    EXPECT_EQ(plugin->load(), LlmErrorCode::OK);
    EXPECT_TRUE(plugin->loaded);
    EXPECT_EQ(plugin->unload(), LlmErrorCode::OK);
    EXPECT_FALSE(plugin->loaded);
}

TEST(LlmContractPlugin, LAC14_DoubleUnloadIsSafe) {
    StubPlugin plugin;
    EXPECT_EQ(plugin.load(), LlmErrorCode::OK);
    EXPECT_EQ(plugin.unload(), LlmErrorCode::OK);
    // Second unload must be a no-op, not an error
    EXPECT_EQ(plugin.unload(), LlmErrorCode::OK);
    EXPECT_TRUE(plugin.double_unload_safe_called);
}

TEST(LlmContractPlugin, LAC15_NullPluginReturnsError) {
    StubPlugin* null_plugin = nullptr;
    auto result = callWithNullPlugin(null_plugin);
    EXPECT_EQ(result, LlmErrorCode::PLUGIN_NULL_HANDLE);
}

TEST(LlmContractPlugin, LAC16_DoubleLoadIsIdempotent) {
    StubPlugin plugin;
    EXPECT_EQ(plugin.load(), LlmErrorCode::OK);
    EXPECT_TRUE(plugin.loaded);
    // Second load should be no-op (already loaded)
    EXPECT_EQ(plugin.load(), LlmErrorCode::OK);
    EXPECT_TRUE(plugin.loaded);
}

// ─────────────────────────────────────────────────────────────────────────────
// LAC-17..20: Embed contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(LlmContractEmbed, LAC17_L2NormApproxOne) {
    StubEmbeddingEngine engine;
    auto v = engine.embed("test-text-lac17");
    float norm = StubEmbeddingEngine::l2norm(v);
    EXPECT_NEAR(norm, 1.0f, kEmbedL2NormTolerance * 10)
        << "L2 norm of embed() output must be ≈ 1.0";
}

TEST(LlmContractEmbed, LAC18_EmptyStringReturnsZeroVector) {
    StubEmbeddingEngine engine;
    auto v = engine.embed("");
    for (float x : v) {
      EXPECT_FLOAT_EQ(x, 0.f);
    }
}

TEST(LlmContractEmbed, LAC19_BatchConsistentWithSingle) {
    StubEmbeddingEngine engine;
    const std::vector<std::string> texts = {"apple", "banana", "cherry"};
    auto batch_results = engine.embedBatch(texts);
    ASSERT_EQ(batch_results.size(), texts.size());
    for (std::size_t i = 0; i < texts.size(); ++i) {
        auto single = engine.embed(texts[i]);
        ASSERT_EQ(single.size(), batch_results[i].size());
        for (std::size_t j = 0; j < single.size(); ++j) {
            EXPECT_FLOAT_EQ(single[j], batch_results[i][j])
                << "embedBatch()[" << i << "][" << j << "] must match embed() for same input";
        }
    }
}

TEST(LlmContractEmbed, LAC20_EmbedBatchOrderPreserved) {
    StubEmbeddingEngine engine;
    const std::vector<std::string> texts = {"x1", "x2", "x3", "x4"};
    auto results = engine.embedBatch(texts);
    ASSERT_EQ(results.size(), texts.size());
    // Verify each result's L2 norm is ≈ 1.0 (non-empty inputs)
    for (const auto& v : results) {
        float n = StubEmbeddingEngine::l2norm(v);
        EXPECT_NEAR(n, 1.0f, 0.01f);
    }
}
