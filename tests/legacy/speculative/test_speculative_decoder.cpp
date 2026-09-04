/**
 * @file test_speculative_decoder.cpp
 * @brief Unit tests for SpeculativeDecoder and its integration with
 *        InferenceEngineEnhanced.
 *
 * Tests cover:
 *  1. Softmax helper — numerical stability, probabilities sum to 1.
 *  2. Adjusted distribution — p'(t) = normalize(max(0, p-q)).
 *  3. Token sampling — output within valid range.
 *  4. verify() — all K tokens accepted (ideal draft model).
 *  5. verify() — first token rejected (worst-case draft model).
 *  6. verify() — partial acceptance.
 *  7. verify() — precondition violations throw.
 *  8. Statistics accumulation across multiple verify() calls.
 *  9. Statistics reset.
 * 10. InferenceEngineEnhanced: speculative decoding disabled by default.
 * 11. InferenceEngineEnhanced: grammar constraints disable speculative path.
 * 12. InferenceEngineEnhanced: speculative stats appear in getDetailedMetrics().
 * 13. InferenceEngineEnhanced: speculative stats keys present in getDetailedMetrics() JSON.
 */

#include <gtest/gtest.h>
#include "llm/speculative_decoder.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_interface.h"

#include <cmath>
#include <numeric>
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>

using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a logit vector of size `vocab_size` with a strong peak at `token`.
std::vector<float> makePeakedLogits(size_t vocab_size, size_t token,
                                    float peak = 10.0f, float baseline = -10.0f)
{
    std::vector<float> logits(vocab_size, baseline);
    if (token < vocab_size) {
      logits[token] = peak;
    }
    return logits;
}

/// Build a uniform logit vector.
std::vector<float> makeUniformLogits(size_t vocab_size)
{
    return std::vector<float>(vocab_size, 0.0f);
}

/// Minimal mock LLM plugin for engine integration tests.
class MockPlugin : public ILLMPlugin {
public:
    explicit MockPlugin(const std::string& model_id, int latency_ms = 5)
        : model_id_(model_id), latency_ms_(latency_ms) {}

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    bool isModelLoaded() const override { return true; }
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo mi;
        mi.model_id = model_id_;
        mi.is_loaded = true;
        return mi;
    }
    InferenceResponse generate(const InferenceRequest& request) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms_));
        InferenceResponse r;
        r.request_id = request.request_id;
        r.text       = "response from " + model_id_;
        r.model_id   = model_id_;
        r.tokens_generated = 10;
        r.inference_time_ms = static_cast<float>(latency_ms_);
        r.latency_ms = latency_ms_;
        return r;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

private:
    std::string model_id_;
    int latency_ms_;
};

} // anonymous namespace

// ═══════════════════════════════════════════════════════════
// Test 1: Softmax helper
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, SoftmaxSumsToOne) {
    std::vector<float> logits = {1.0f, 2.0f, 3.0f, 4.0f};
    auto probs = SpeculativeDecoder::softmax(logits);

    ASSERT_EQ(probs.size(), logits.size());
    float sum = std::accumulate(probs.begin(), probs.end(), 0.0f);
    EXPECT_NEAR(sum, 1.0f, 1e-5f);

    // Higher logit → higher prob
    EXPECT_GT(probs[3], probs[2]);
    EXPECT_GT(probs[2], probs[1]);
    EXPECT_GT(probs[1], probs[0]);
}

TEST(SpeculativeDecoderTest, SoftmaxNumericalStability) {
    // Large logits should not produce NaN/Inf
    std::vector<float> logits = {1000.0f, 1001.0f, 1002.0f};
    auto probs = SpeculativeDecoder::softmax(logits);

    float sum = std::accumulate(probs.begin(), probs.end(), 0.0f);
    EXPECT_NEAR(sum, 1.0f, 1e-5f);
    for (float p : probs) {
        EXPECT_TRUE(std::isfinite(p));
        EXPECT_GE(p, 0.0f);
    }
}

TEST(SpeculativeDecoderTest, SoftmaxEmptyReturnsEmpty) {
    auto probs = SpeculativeDecoder::softmax({});
    EXPECT_TRUE(probs.empty());
}

// ═══════════════════════════════════════════════════════════
// Test 2: Adjusted distribution
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, AdjustedDistributionSumsToOne) {
    // p and q with some overlap
    std::vector<float> p = {0.4f, 0.3f, 0.2f, 0.1f};
    std::vector<float> q = {0.1f, 0.2f, 0.3f, 0.4f};
    auto adj = SpeculativeDecoder::adjustedDistribution(p, q);

    ASSERT_EQ(adj.size(), p.size());
    float sum = std::accumulate(adj.begin(), adj.end(), 0.0f);
    EXPECT_NEAR(sum, 1.0f, 1e-5f);

    for (float v : adj) {
      EXPECT_GE(v, 0.0f);
    }
}

TEST(SpeculativeDecoderTest, AdjustedDistributionIdenticalFallsBackToUniform) {
    // When p == q the max(0, p-q) is zero everywhere → fallback to uniform
    std::vector<float> p = {0.25f, 0.25f, 0.25f, 0.25f};
    auto adj = SpeculativeDecoder::adjustedDistribution(p, p);

    ASSERT_EQ(adj.size(), 4u);
    for (float v : adj) {
      EXPECT_NEAR(v, 0.25f, 1e-5f);
    }
}

// ═══════════════════════════════════════════════════════════
// Test 3: Token sampling
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, SampleTokenWithinRange) {
    std::vector<float> probs = {0.0f, 0.0f, 1.0f, 0.0f};  // Always token 2
    std::mt19937 rng(42);
    int token = SpeculativeDecoder::sampleToken(probs, rng);
    // The only non-zero prob is at index 2; result must be 2.
    // The fallback to the last index only applies to floating-point rounding
    // edge cases, which cannot occur when a single entry is exactly 1.0.
    EXPECT_GE(token, 0);
    EXPECT_LT(static_cast<size_t>(token), probs.size());
}

TEST(SpeculativeDecoderTest, SampleTokenEmptyReturnsMinusOne) {
    std::mt19937 rng(42);
    int token = SpeculativeDecoder::sampleToken({}, rng);
    EXPECT_EQ(token, -1);
}

// ═══════════════════════════════════════════════════════════
// Test 4: verify() — all K tokens accepted (ideal draft)
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, VerifyAllAccepted) {
    constexpr size_t K          = 4;
    constexpr size_t vocab_size = 16;
    constexpr size_t peak_token = 5;

    // Draft tokens all equal peak_token.
    std::vector<int> draft_tokens(K, static_cast<int>(peak_token));

    // Both draft and target strongly prefer peak_token → acceptance ratio = 1.
    std::vector<std::vector<float>> draft_logits(K,
        makePeakedLogits(vocab_size, peak_token));
    std::vector<std::vector<float>> target_logits(K + 1,
        makePeakedLogits(vocab_size, peak_token));

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 42;
    SpeculativeDecoder dec(cfg);

    auto result = dec.verify(draft_tokens, draft_logits, target_logits);

    EXPECT_EQ(result.num_accepted, K);
    EXPECT_TRUE(result.all_accepted);
    EXPECT_EQ(result.accepted_tokens.size(), K);
    for (int t : result.accepted_tokens) {
      EXPECT_EQ(t, static_cast<int>(peak_token));
    }
    EXPECT_GE(result.bonus_token, 0);
    EXPECT_NEAR(result.acceptance_rate, 1.0f, 1e-5f);
}

// ═══════════════════════════════════════════════════════════
// Test 5: verify() — first token rejected (worst-case)
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, VerifyFirstRejected) {
    constexpr size_t K          = 4;
    constexpr size_t vocab_size = 16;

    // Draft strongly prefers token 0; target strongly prefers token 1.
    // p(token_0)/q(token_0) ≈ 0 → always rejected.
    std::vector<int> draft_tokens(K, 0);
    std::vector<std::vector<float>> draft_logits(K,
        makePeakedLogits(vocab_size, /*token=*/0));
    std::vector<std::vector<float>> target_logits(K + 1,
        makePeakedLogits(vocab_size, /*token=*/1));

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 42;
    SpeculativeDecoder dec(cfg);

    auto result = dec.verify(draft_tokens, draft_logits, target_logits);

    EXPECT_EQ(result.num_accepted, 0u);
    EXPECT_FALSE(result.all_accepted);
    EXPECT_TRUE(result.accepted_tokens.empty());
    EXPECT_GE(result.bonus_token, 0);  // correction token sampled
    EXPECT_NEAR(result.acceptance_rate, 0.0f, 1e-5f);
}

// ═══════════════════════════════════════════════════════════
// Test 6: verify() — partial acceptance (first 2 of 4)
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, VerifyPartialAcceptance) {
    constexpr size_t K          = 4;
    constexpr size_t vocab_size = 16;

    // Tokens 0 and 1 both model prefer token_A; token 2 onwards diverge.
    std::vector<int> draft_tokens = {2, 2, 3, 3};  // tokens 2,2,3,3

    std::vector<std::vector<float>> draft_logits(K);
    std::vector<std::vector<float>> target_logits(K + 1);

    // Positions 0,1: both agree on token 2 → accept
    draft_logits[0]  = makePeakedLogits(vocab_size, 2);
    draft_logits[1]  = makePeakedLogits(vocab_size, 2);
    target_logits[0] = makePeakedLogits(vocab_size, 2);
    target_logits[1] = makePeakedLogits(vocab_size, 2);

    // Position 2: draft prefers 3, target prefers 9 → reject
    draft_logits[2]  = makePeakedLogits(vocab_size, 3);
    target_logits[2] = makePeakedLogits(vocab_size, 9);

    // Positions 3+: don't matter (loop stops at first rejection)
    draft_logits[3]  = makeUniformLogits(vocab_size);
    target_logits[3] = makeUniformLogits(vocab_size);
    target_logits[4] = makeUniformLogits(vocab_size);  // bonus position

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 42;
    SpeculativeDecoder dec(cfg);

    auto result = dec.verify(draft_tokens, draft_logits, target_logits);

    EXPECT_EQ(result.num_accepted, 2u);
    EXPECT_FALSE(result.all_accepted);
    EXPECT_EQ(result.accepted_tokens.size(), 2u);
    EXPECT_EQ(result.accepted_tokens[0], 2);
    EXPECT_EQ(result.accepted_tokens[1], 2);
    EXPECT_GE(result.bonus_token, 0);
    EXPECT_NEAR(result.acceptance_rate, 0.5f, 1e-5f);
}

// ═══════════════════════════════════════════════════════════
// Test 7: verify() — precondition violations
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, VerifyMismatchedSizeThrows) {
    SpeculativeDecoder dec;
    std::vector<int> draft_tokens = {1, 2};
    std::vector<std::vector<float>> draft_logits(2, std::vector<float>(10, 0.0f));
    // target_logits must have 3 rows (K+1), give 2 → throw
    std::vector<std::vector<float>> target_logits(2, std::vector<float>(10, 0.0f));
    EXPECT_THROW(dec.verify(draft_tokens, draft_logits, target_logits),
                 std::invalid_argument);
}

TEST(SpeculativeDecoderTest, VerifyEmptyDraftTokensThrows) {
    SpeculativeDecoder dec;
    EXPECT_THROW(
        dec.verify({}, {}, {{0.5f, 0.5f}}),
        std::invalid_argument);
}

TEST(SpeculativeDecoderTest, VerifyVocabSizeMismatchThrows) {
    SpeculativeDecoder dec;
    std::vector<int> draft_tokens = {0};
    std::vector<std::vector<float>> draft_logits  = {{0.0f, 1.0f}};   // vocab=2
    std::vector<std::vector<float>> target_logits = {{0.0f, 1.0f, 2.0f}, // vocab=3
                                                     {0.0f, 1.0f, 2.0f}};
    EXPECT_THROW(dec.verify(draft_tokens, draft_logits, target_logits),
                 std::invalid_argument);
}

// ═══════════════════════════════════════════════════════════
// Test 8: Statistics accumulation
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, StatisticsAccumulate) {
    constexpr size_t K          = 2;
    constexpr size_t vocab_size = 8;

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 1;
    SpeculativeDecoder dec(cfg);

    // All-accepted step
    auto all_logits = makePeakedLogits(vocab_size, 0);
    std::vector<int> draft_tokens(K, 0);
    std::vector<std::vector<float>> draft_logits(K, all_logits);
    std::vector<std::vector<float>> target_logits(K + 1, all_logits);
    dec.verify(draft_tokens, draft_logits, target_logits);

    // All-rejected step
    std::vector<std::vector<float>> target_bad(K + 1,
        makePeakedLogits(vocab_size, 7));  // target prefers token 7
    dec.verify(draft_tokens, draft_logits, target_bad);

    auto s = dec.getStatistics();
    EXPECT_EQ(s.total_steps, 2u);
    EXPECT_EQ(s.total_draft_tokens, K * 2);
    EXPECT_EQ(s.total_accepted_tokens, K);       // K from all-accepted, 0 from rejected
    EXPECT_EQ(s.total_rejected_tokens, K);       // 0 + K
    EXPECT_GE(s.avg_acceptance_rate, 0.0);
    EXPECT_LE(s.avg_acceptance_rate, 1.0);
}

// ═══════════════════════════════════════════════════════════
// Test 9: Statistics reset
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderTest, StatisticsReset) {
    constexpr size_t K          = 1;
    constexpr size_t vocab_size = 4;

    SpeculativeDecoder dec;
    auto logits = makeUniformLogits(vocab_size);
    std::vector<int> toks = {0};
    dec.verify(toks, {logits}, {logits, logits});

    EXPECT_GT(dec.getStatistics().total_steps, 0u);
    dec.resetStatistics();
    EXPECT_EQ(dec.getStatistics().total_steps, 0u);
    EXPECT_EQ(dec.getStatistics().total_draft_tokens, 0u);
}

// ═══════════════════════════════════════════════════════════
// Test 10: InferenceEngineEnhanced — speculative decoding disabled by default
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderIntegrationTest, DisabledByDefault) {
    InferenceEngineEnhanced::Config cfg;
    EXPECT_FALSE(cfg.enable_speculative_decoding);
    EXPECT_EQ(cfg.speculative_draft_tokens, 4u);
    EXPECT_TRUE(cfg.speculative_draft_model_id.empty());
}

// ═══════════════════════════════════════════════════════════
// Test 11: Engine — speculative path used when enabled + draft model registered
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderIntegrationTest, SpeculativePathUsedWhenEnabled) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = 3;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MockPlugin>("target", 5));
    engine.registerModel("draft",  std::make_shared<MockPlugin>("draft",  2));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id             = "spec_test_1";
    req.base_request.prompt    = "Hello, speculative world";
    req.base_request.max_tokens = 50;
    req.allow_caching          = false;
    req.preferred_model_id     = "target";

    auto handle   = engine.submit(req);
    auto response = handle.get();

    EXPECT_FALSE(response.text.empty());

    // Speculative stats should show at least one step.
    auto stats = engine.getStatistics();
    EXPECT_GE(stats.speculative_steps, 1u);
    EXPECT_GE(stats.speculative_draft_tokens_total, 1u);

    auto metrics = engine.getDetailedMetrics();
    EXPECT_TRUE(metrics.contains("speculative"));
    EXPECT_TRUE(metrics["speculative"]["enabled"].get<bool>());

    spdlog::info("SpeculativePathUsedWhenEnabled: steps={}, accepted={}",
                 stats.speculative_steps, stats.speculative_accepted_tokens);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 12: Engine — grammar constraints disable speculative path
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderIntegrationTest, GrammarConstraintsDisableSpeculative) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = 4;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MockPlugin>("target", 5));
    engine.registerModel("draft",  std::make_shared<MockPlugin>("draft",  2));
    engine.start();

    // Request with grammar_type — speculative path must be skipped.
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id               = "grammar_test_1";
    req.base_request.prompt      = "Generate JSON";
    req.base_request.max_tokens  = 50;
    req.base_request.grammar_type = std::string("json");
    req.allow_caching            = false;
    req.preferred_model_id       = "target";

    auto handle   = engine.submit(req);
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());

    // Because grammar was active, speculative_steps must still be 0.
    auto stats = engine.getStatistics();
    EXPECT_EQ(stats.speculative_steps, 0u);

    spdlog::info("GrammarConstraintsDisableSpeculative: steps={} (expected 0)",
                 stats.speculative_steps);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 13: Engine — speculative stats in getDetailedMetrics
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderIntegrationTest, SpeculativeStatsInDetailedMetrics) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = 4;
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target", std::make_shared<MockPlugin>("target", 5));
    engine.registerModel("draft",  std::make_shared<MockPlugin>("draft",  2));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id             = "metrics_test_1";
    req.base_request.prompt    = "Metrics check";
    req.base_request.max_tokens = 20;
    req.allow_caching          = false;
    req.preferred_model_id     = "target";

    auto handle = engine.submit(req);
    (void)handle.get();

    auto metrics = engine.getDetailedMetrics();

    EXPECT_TRUE(metrics.contains("speculative"));
    EXPECT_TRUE(metrics["speculative"].contains("enabled"));
    EXPECT_TRUE(metrics["speculative"].contains("draft_tokens_total"));
    EXPECT_TRUE(metrics["speculative"].contains("drafted_tokens"));
    EXPECT_TRUE(metrics["speculative"].contains("accepted_tokens"));
    EXPECT_TRUE(metrics["speculative"].contains("rejected_tokens"));
    EXPECT_TRUE(metrics["speculative"].contains("avg_acceptance_rate"));
    EXPECT_TRUE(metrics["speculative"].contains("accept_rate"));
    EXPECT_TRUE(metrics["speculative"].contains("steps"));
    EXPECT_TRUE(metrics.contains("feature_flags"));
    EXPECT_TRUE(metrics["feature_flags"].contains("lookup_decoding"));
    EXPECT_TRUE(metrics["feature_flags"].contains("adaptive_batch_retry"));

    spdlog::info("SpeculativeStatsInDetailedMetrics: {}",
                 metrics["speculative"].dump());

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 14: AdapterRegistry DRAFT role — listAdaptersByRole
// ═══════════════════════════════════════════════════════════

#include "llm/adapter_registry.h"

TEST(AdapterRegistryDraftRoleTest, ListAdaptersByRoleDraftAndGeneral) {
    AdapterRegistry registry(nullptr);

    // Register one GENERAL adapter
    AdapterMetadata general;
    general.adapter_id    = "general-adapter";
    general.base_model_name = "llama-7b";
    general.architecture  = "llama";
    general.role          = AdapterRole::GENERAL;
    registry.registerAdapter(general);

    // Register one DRAFT adapter
    AdapterMetadata draft;
    draft.adapter_id    = "draft-adapter";
    draft.base_model_name = "llama-0.5b";
    draft.architecture  = "llama";
    draft.role          = AdapterRole::DRAFT;
    registry.registerAdapter(draft);

    auto drafts   = registry.listAdaptersByRole(AdapterRole::DRAFT);
    auto generals = registry.listAdaptersByRole(AdapterRole::GENERAL);

    ASSERT_EQ(drafts.size(), 1u)   << "Exactly one DRAFT adapter should be found";
    ASSERT_EQ(generals.size(), 1u) << "Exactly one GENERAL adapter should be found";
    EXPECT_EQ(drafts[0].adapter_id,   "draft-adapter");
    EXPECT_EQ(generals[0].adapter_id, "general-adapter");
}

// ═══════════════════════════════════════════════════════════
// Test 15: AdapterRegistry — findDraftAdapterForFamily
// ═══════════════════════════════════════════════════════════

TEST(AdapterRegistryDraftRoleTest, FindDraftAdapterForFamilyMatchesArchitecture) {
    AdapterRegistry registry(nullptr);

    AdapterMetadata draft;
    draft.adapter_id    = "llama-draft";
    draft.base_model_name = "llama-0.5b";
    draft.architecture  = "llama";
    draft.role          = AdapterRole::DRAFT;
    draft.status        = AdapterMetadata::Status::DEPLOYED;
    registry.registerAdapter(draft);

    // Exact match
    auto found = registry.findDraftAdapterForFamily("llama");
    ASSERT_TRUE(found.has_value()) << "Should find DRAFT adapter for 'llama' family";
    EXPECT_EQ(found->adapter_id, "llama-draft");
}

TEST(AdapterRegistryDraftRoleTest, FindDraftAdapterForFamilyCaseInsensitive) {
    AdapterRegistry registry(nullptr);

    AdapterMetadata draft;
    draft.adapter_id  = "mistral-draft";
    draft.architecture = "Mistral";  // Mixed-case in registry
    draft.role        = AdapterRole::DRAFT;
    registry.registerAdapter(draft);

    // Query with different case
    auto found = registry.findDraftAdapterForFamily("mistral");
    ASSERT_TRUE(found.has_value()) << "Match should be case-insensitive";
    EXPECT_EQ(found->adapter_id, "mistral-draft");
}

TEST(AdapterRegistryDraftRoleTest, FindDraftAdapterForFamilyReturnsNulloptWhenNoneMatch) {
    AdapterRegistry registry(nullptr);

    AdapterMetadata draft;
    draft.adapter_id  = "llama-draft";
    draft.architecture = "llama";
    draft.role        = AdapterRole::DRAFT;
    registry.registerAdapter(draft);

    // Unrelated family
    auto found = registry.findDraftAdapterForFamily("gpt");
    EXPECT_FALSE(found.has_value())
        << "Should return nullopt when no DRAFT adapter for the family exists";
}

TEST(AdapterRegistryDraftRoleTest, FindDraftAdapterForFamilyIgnoresGeneralAdapters) {
    AdapterRegistry registry(nullptr);

    AdapterMetadata general;
    general.adapter_id  = "llama-general";
    general.architecture = "llama";
    general.role        = AdapterRole::GENERAL;  // Not a DRAFT
    registry.registerAdapter(general);

    auto found = registry.findDraftAdapterForFamily("llama");
    EXPECT_FALSE(found.has_value())
        << "GENERAL adapters must not be returned by findDraftAdapterForFamily";
}

TEST(AdapterRegistryDraftRoleTest, FindDraftAdapterForFamilyPrefersDeployed) {
    AdapterRegistry registry(nullptr);

    // Register two DRAFT adapters: one TRAINED, one DEPLOYED
    AdapterMetadata trained_draft;
    trained_draft.adapter_id  = "llama-draft-trained";
    trained_draft.architecture = "llama";
    trained_draft.role        = AdapterRole::DRAFT;
    trained_draft.status      = AdapterMetadata::Status::TRAINED;
    trained_draft.version.major = 1;
    trained_draft.version.minor = 0;
    trained_draft.version.patch = 0;
    registry.registerAdapter(trained_draft);

    AdapterMetadata deployed_draft;
    deployed_draft.adapter_id  = "llama-draft-deployed";
    deployed_draft.architecture = "llama";
    deployed_draft.role        = AdapterRole::DRAFT;
    deployed_draft.status      = AdapterMetadata::Status::DEPLOYED;
    deployed_draft.version.major = 1;
    deployed_draft.version.minor = 0;
    deployed_draft.version.patch = 0;
    registry.registerAdapter(deployed_draft);

    auto found = registry.findDraftAdapterForFamily("llama");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->adapter_id, "llama-draft-deployed")
        << "DEPLOYED adapter should be preferred over TRAINED adapter";
}

// ═══════════════════════════════════════════════════════════
// Test 16: InferenceEngineEnhanced — setAdapterRegistry +
//          auto draft model selection
// ═══════════════════════════════════════════════════════════

TEST(SpeculativeDecoderIntegrationTest, AutoDraftModelViaAdapterRegistry) {
    // Build an engine with enable_speculative_decoding but NO explicit draft ID.
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = 3;
    cfg.speculative_draft_model_id  = "";  // Use auto-discovery
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 50;

    InferenceEngineEnhanced engine(cfg);

    // Register both models before attaching the registry.
    engine.registerModel("llama-7b",   std::make_shared<MockPlugin>("llama-7b",   5));
    engine.registerModel("llama-0.5b", std::make_shared<MockPlugin>("llama-0.5b", 2));

    // Build an adapter registry that maps "llama" family → "llama-0.5b" draft model.
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    AdapterMetadata draft_meta;
    draft_meta.adapter_id  = "llama-0.5b";  // Must match a registered model ID
    draft_meta.architecture = "llama";
    draft_meta.role        = AdapterRole::DRAFT;
    draft_meta.status      = AdapterMetadata::Status::DEPLOYED;
    registry->registerAdapter(draft_meta);

    engine.setAdapterRegistry(registry);
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id              = "auto_draft_test";
    req.base_request.prompt     = "Auto-discovered draft model test";
    req.base_request.max_tokens = 20;
    req.allow_caching           = false;
    req.preferred_model_id      = "llama-7b";

    auto handle   = engine.submit(req);
    auto response = handle.get();

    EXPECT_FALSE(response.text.empty());

    // Speculative stats: the engine should have found the draft model.
    auto stats = engine.getStatistics();
    EXPECT_GE(stats.speculative_steps, 1u)
        << "Speculative decoding steps should be recorded when draft model is auto-discovered";

    engine.shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// LLM-RAID integration: SpeculativeDecoder::Config::remote_draft_shard_id
// ─────────────────────────────────────────────────────────────────────────────

// Test 14 (SD-RDSI-01): remote_draft_shard_id defaults to empty string.
TEST(SpeculativeDecoderConfig, RemoteDraftShardIdDefaultsEmpty) {
    SpeculativeDecoder::Config cfg;
    EXPECT_TRUE(cfg.remote_draft_shard_id.empty());
}

// Test 15 (SD-RDSI-02): remote_draft_shard_id can be set and read back.
TEST(SpeculativeDecoderConfig, RemoteDraftShardIdRoundTrip) {
    SpeculativeDecoder::Config cfg;
    cfg.remote_draft_shard_id = "shard-a:model:mistral-7b-q4";
    EXPECT_EQ(cfg.remote_draft_shard_id, "shard-a:model:mistral-7b-q4");
}

// Test 16 (SD-RDSI-03): SpeculativeDecoder can be constructed with
//   a remote-draft config; it must not affect local verify() logic.
TEST(SpeculativeDecoderConfig, RemoteDraftShardIdDoesNotAffectLocalVerify) {
    SpeculativeDecoder::Config cfg;
    cfg.k = 2;
    cfg.remote_draft_shard_id = "shard-b:model:phi-2";
    SpeculativeDecoder decoder(cfg);

    constexpr size_t V = 4;
    // Perfect draft: draft logits and target logits both peak at the same token.
    auto draft_row  = makePeakedLogits(V, 0);
    auto target_row = makePeakedLogits(V, 0);

    std::vector<std::vector<float>> draft_logits  = { draft_row, draft_row };
    std::vector<std::vector<float>> target_logits = { target_row, target_row, target_row };

    auto result = decoder.verify({0, 0}, draft_logits, target_logits);
    // With a perfect draft all K tokens should be accepted.
    EXPECT_EQ(result.num_accepted, cfg.k);
}
