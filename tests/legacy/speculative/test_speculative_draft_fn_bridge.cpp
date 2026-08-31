/**
 * @file test_speculative_draft_fn_bridge.cpp
 * @brief Unit tests for the GenerateDraftTokensFn static injection bridge
 *        on ILLMPlugin (STUB #261) and the TargetLogitsFn injection bridge
 *        on InferenceEngineEnhanced (STUB #262).
 *
 * Test IDs:
 *   SD-DFT-01  Injected GenerateDraftTokensFn is called in place of heuristic
 *   SD-DFT-02  nullptr/empty fn restores the built-in heuristic
 *   SD-DFT-03  Bridge is thread-safe (concurrent calls see consistent fn)
 *   SPEC-TL-01 Injected TargetLogitsFn is called in trySpeculativeGeneration
 *   SPEC-TL-02 nullptr fn restores built-in peaked-distribution heuristic
 *   SPEC-TL-03 TargetLogitsFn returning wrong shape falls back to heuristic
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_interface.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/speculative_decoder.h"

#include <atomic>
#include <chrono>
#include <limits>
#include <string>
#include <thread>
#include <vector>

using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Minimal ILLMPlugin that satisfies the pure-virtual contract.
class MinimalPlugin : public ILLMPlugin {
public:
    explicit MinimalPlugin(std::string resp_text = "hello", size_t vocab = 256)
        : resp_text_(std::move(resp_text)), vocab_(vocab) {}

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo m;
        m.model_id  = "minimal";
        m.is_loaded = true;
        m.vocab_size = vocab_;
        return m;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest&) override {
        InferenceResponse r;
        r.text    = resp_text_;
        r.success = true;
        return r;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                  const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities    getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t>  exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

    size_t vocab() const { return vocab_; }

private:
    std::string resp_text_;
    size_t      vocab_;
};

/// RAII guard that clears the static GenerateDraftTokensFn after each test.
struct DraftFnGuard {
    ~DraftFnGuard() { ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr); }
};

// ─────────────────────────────────────────────────────────────────────────────
// SD-DFT-01: Injected GenerateDraftTokensFn is called in place of heuristic
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdDraftFnBridge, SD_DFT_01_InjectedFnCalledInsteadOfHeuristic) {
    DraftFnGuard guard;

    constexpr size_t kVocab = 128;
    constexpr size_t kK     = 3;
    const std::vector<int> expected_ids = {42, 77, 99};

    std::atomic<int> call_count{0};

    ILLMPlugin::setDefaultGenerateDraftTokensFn(
        [&](const InferenceRequest&, size_t k, size_t vocab_hint)
            -> ILLMPlugin::DraftTokensResult {
            ++call_count;
            ILLMPlugin::DraftTokensResult result;
            result.vocab_size = (vocab_hint > 0) ? vocab_hint : kVocab;
            for (size_t i = 0; i < k; ++i) {
                const int id = (i < expected_ids.size()) ? expected_ids[i] : 0;
                result.tokens.push_back(id);
                std::vector<float> row(result.vocab_size, -5.0f);
                row[static_cast<size_t>(id)] = 5.0f;
                result.logits.push_back(std::move(row));
            }
            return result;
        });

    MinimalPlugin plugin("some text", kVocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto res = plugin.generateDraftTokens(req, kK, kVocab);

    EXPECT_EQ(call_count.load(), 1) << "Injected fn should have been called once";
    ASSERT_EQ(res.tokens.size(), kK);
    for (size_t i = 0; i < kK; ++i) {
        EXPECT_EQ(res.tokens[i], expected_ids[i])
            << "Token ID at position " << i << " should come from injected fn";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-DFT-02: nullptr restores the built-in text-heuristic path
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdDraftFnBridge, SD_DFT_02_NullFnRestoresHeuristic) {
    DraftFnGuard guard;

    // First set an fn, then clear it.
    ILLMPlugin::setDefaultGenerateDraftTokensFn(
        [](const InferenceRequest& /*req*/, size_t k, size_t v)
            -> ILLMPlugin::DraftTokensResult {
            [[maybe_unused]] auto sink_k = k;
            [[maybe_unused]] auto sink_v = v;
            return {};  // intentionally return empty result
        });
    ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);

    constexpr size_t kVocab = 64;
    constexpr size_t kK     = 4;
    MinimalPlugin plugin("abcd", kVocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto res = plugin.generateDraftTokens(req, kK, kVocab);

    // Heuristic maps 'a'=97, 'b'=98, 'c'=99, 'd'=100 mod 64
    ASSERT_EQ(res.tokens.size(), kK);
    for (size_t i = 0; i < kK; ++i) {
        const char c = static_cast<char>('a' + static_cast<int>(i));
        const int expected =
            static_cast<int>(static_cast<unsigned char>(c)) %
            static_cast<int>(kVocab);
        EXPECT_EQ(res.tokens[i], expected)
            << "After clearing fn, heuristic should map char to token ID";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-DFT-03: Thread-safety — concurrent calls with an injected fn
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdDraftFnBridge, SD_DFT_03_ThreadSafetyUnderConcurrentCalls) {
    DraftFnGuard guard;

    constexpr size_t kVocab = 32;
    constexpr size_t kK     = 2;
    std::atomic<int> call_count{0};

    ILLMPlugin::setDefaultGenerateDraftTokensFn(
        [&](const InferenceRequest&, size_t k, size_t vocab_hint)
            -> ILLMPlugin::DraftTokensResult {
            ++call_count;
            ILLMPlugin::DraftTokensResult result;
            result.vocab_size = (vocab_hint > 0) ? vocab_hint : kVocab;
            for (size_t i = 0; i < k; ++i) {
                result.tokens.push_back(static_cast<int>(i % result.vocab_size));
                std::vector<float> row(result.vocab_size, -5.0f);
                row[i % result.vocab_size] = 5.0f;
                result.logits.push_back(std::move(row));
            }
            return result;
        });

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            MinimalPlugin plugin("xy", kVocab);
            InferenceRequest req;
            req.prompt = "test";
            try {
                const auto res = plugin.generateDraftTokens(req, kK, kVocab);
                if (res.tokens.size() != kK) {
                    ++errors;
                }
            } catch (...) {
                ++errors;
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(errors.load(), 0) << "No errors expected under concurrent access";
    EXPECT_EQ(call_count.load(), kThreads)
        << "Injected fn should be called once per thread";
}

// ─────────────────────────────────────────────────────────────────────────────
// SPEC-TL-01: Injected TargetLogitsFn is called in trySpeculativeGeneration
// ─────────────────────────────────────────────────────────────────────────────
TEST(SpecTlBridge, SPEC_TL_01_InjectedTargetLogitsFnUsed) {
    constexpr size_t kVocab = 64;
    constexpr size_t kK     = 2;

    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = kK;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MinimalPlugin>("target_resp", kVocab));
    engine.registerModel("draft",  std::make_shared<MinimalPlugin>("dr",          kVocab));
    engine.start();

    std::atomic<int> target_logits_fn_calls{0};

    // Inject a TargetLogitsFn that returns all-acceptance logits:
    // all K+1 rows peaked at token 0 (same as draft tokens for "dr").
    engine.setTargetLogitsFn(
        [&](const InferenceRequest&, size_t K, size_t vocab_size,
            std::shared_ptr<ILLMPlugin>)
            -> std::vector<std::vector<float>> {
            ++target_logits_fn_calls;
            std::vector<std::vector<float>> mat(K + 1,
                std::vector<float>(vocab_size, -5.0f));
            // Peak at token 0 on every row → high acceptance rate.
            for (auto& row : mat) row[0] = 5.0f;
            return mat;
        });

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "spec_tl_01";
    req.base_request.prompt = "hello speculative";
    req.preferred_model_id  = "target";
    req.allow_caching       = false;

    auto handle   = engine.submit(req);
    auto response = handle.get();
    engine.shutdown();

    EXPECT_FALSE(response.text.empty()) << "Engine must return a response";
    EXPECT_GE(target_logits_fn_calls.load(), 1)
        << "TargetLogitsFn must have been called at least once";
}

// ─────────────────────────────────────────────────────────────────────────────
// SPEC-TL-02: nullptr fn restores built-in peaked-distribution heuristic
//             (engine still produces a response without the injected fn)
// ─────────────────────────────────────────────────────────────────────────────
TEST(SpecTlBridge, SPEC_TL_02_NullFnRestoresHeuristic) {
    constexpr size_t kVocab = 64;
    constexpr size_t kK     = 2;

    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = kK;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MinimalPlugin>("t", kVocab));
    engine.registerModel("draft",  std::make_shared<MinimalPlugin>("d", kVocab));
    engine.start();

    // Set then immediately clear the fn.
    engine.setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t vocab_size,
           std::shared_ptr<ILLMPlugin>)
            -> std::vector<std::vector<float>> {
            return std::vector<std::vector<float>>(K + 1,
                std::vector<float>(vocab_size, 1.0f));
        });
    engine.setTargetLogitsFn(nullptr);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "spec_tl_02";
    req.base_request.prompt = "test heuristic";
    req.preferred_model_id  = "target";
    req.allow_caching       = false;

    auto handle   = engine.submit(req);
    auto response = handle.get();
    engine.shutdown();

    EXPECT_FALSE(response.text.empty())
        << "Engine must work correctly without injected TargetLogitsFn";
}

// ─────────────────────────────────────────────────────────────────────────────
// SPEC-TL-03: TargetLogitsFn returning wrong row count falls back to heuristic
// ─────────────────────────────────────────────────────────────────────────────
TEST(SpecTlBridge, SPEC_TL_03_BadShapeFallsBackToHeuristic) {
    constexpr size_t kVocab = 32;
    constexpr size_t kK     = 3;

    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = kK;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MinimalPlugin>("t", kVocab));
    engine.registerModel("draft",  std::make_shared<MinimalPlugin>("d", kVocab));
    engine.start();

    // Inject a fn that returns the WRONG number of rows (K instead of K+1).
    engine.setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t vocab_size,
           std::shared_ptr<ILLMPlugin>)
            -> std::vector<std::vector<float>> {
            // Return K rows instead of the required K+1.
            return std::vector<std::vector<float>>(
                K, std::vector<float>(vocab_size, 0.0f));
        });

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "spec_tl_03";
    req.base_request.prompt = "test bad shape";
    req.preferred_model_id  = "target";
    req.allow_caching       = false;

    // The engine must NOT throw even when the injected fn returns bad shape;
    // it should fall back to the heuristic and still produce a response.
    auto handle   = engine.submit(req);
    EXPECT_NO_THROW(handle.get());
    engine.shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// SPEC-TL-04: Oversized vocab metadata is clamped to safe fallback range
// ─────────────────────────────────────────────────────────────────────────────
TEST(SpecTlBridge, SPEC_TL_04_OversizedVocabMetadataHandledSafely) {
    constexpr size_t kK = 1;

    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = kK;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MinimalPlugin>(
        "t", std::numeric_limits<size_t>::max()));
    engine.registerModel("draft", std::make_shared<MinimalPlugin>("d", 64));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "spec_tl_04";
    req.base_request.prompt = "test large vocab";
    req.preferred_model_id  = "target";
    req.allow_caching       = false;

    auto handle = engine.submit(req);
    auto response = handle.get();
    engine.shutdown();

    EXPECT_FALSE(response.text.empty())
        << "Engine should keep running with oversized vocab metadata";
}

// ─────────────────────────────────────────────────────────────────────────────
// SPEC-TL-05: Local draft path uses TokenizerFn bridge instead of byte-modulo
// ─────────────────────────────────────────────────────────────────────────────
TEST(SpecTlBridge, SPEC_TL_05_LocalDraftTokenizerBridgeOverridesHeuristic) {
    constexpr size_t kVocab = 256;
    constexpr size_t kK     = 2;

    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = kK;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MinimalPlugin>("target_resp", kVocab));
    engine.registerModel("draft",  std::make_shared<MinimalPlugin>("ab",          kVocab));
    engine.start();

    engine.setTokenizerFn(
        [](const std::string&, size_t) -> std::vector<int> {
            return {11, 12};
        });
    engine.setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t vocab_size,
           std::shared_ptr<ILLMPlugin>) -> std::vector<std::vector<float>> {
            std::vector<std::vector<float>> mat(K + 1,
                                                std::vector<float>(vocab_size, -5.0f));
            mat[0][11] = 5.0f;
            mat[1][12] = 5.0f;
            mat[2][0]  = 5.0f;
            return mat;
        });

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "spec_tl_05";
    req.base_request.prompt = "hello local draft";
    req.preferred_model_id  = "target";
    req.allow_caching       = false;

    auto handle   = engine.submit(req);
    auto response = handle.get();
    engine.shutdown();

    EXPECT_FALSE(response.text.empty());
    EXPECT_EQ(response.metadata.value("speculative_accepted", uint64_t{0}),
              static_cast<uint64_t>(kK))
        << "TokenizerFn bridge should drive the local draft path instead of "
           "the byte-modulo heuristic";
}
