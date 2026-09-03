/*
 * tests/llm/test_w10d_local_draft_bridge.cpp
 *
 * Wave 10-D — ILLMPlugin::setDefaultGenerateDraftTokensFn() local draft bridge
 *
 * Covers: SD-LOCAL-01, SD-LOCAL-02
 *
 *   SD-LOCAL-01 — When a GenerateDraftTokensFn is injected via
 *                 ILLMPlugin::setDefaultGenerateDraftTokensFn(), a call to
 *                 generateDraftTokens() on any ILLMPlugin instance routes
 *                 through the injected fn (not the byte-modulo heuristic).
 *                 This mirrors what InferenceEngineEnhanced::trySpeculativeGeneration()
 *                 does in the local-draft path when a TokenizerFn is registered.
 *
 *   SD-LOCAL-02 — When no GenerateDraftTokensFn is set (or it is cleared to
 *                 nullptr), generateDraftTokens() falls back to the built-in
 *                 byte-modulo heuristic: token IDs are derived from UTF-8 byte
 *                 values of the generated text modulo vocab_size.
 *
 * Design notes
 * ────────────
 * These tests exercise the ILLMPlugin bridge API directly, following the
 * same pattern as tests/llm/test_wave9_speculative_decode_bridges.cpp and
 * reusing the minimal mock-plugin shape from test_llm_phase5_hardening.cpp
 * (P5MockPlugin).
 *
 * A LocalDraftBridgeGuard RAII wrapper ensures the global static
 * ILLMPlugin::s_default_draft_fn_ is always cleared after each test,
 * mirroring the engine's own clear-after-call contract and preventing
 * cross-test state pollution.
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_interface.h"

using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// W10D mock draft plugin — deterministic text from generate()
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Minimal ILLMPlugin implementation for W10-D bridge tests.
 *
 * generate() returns a configurable text string, allowing tests to control
 * the byte-modulo token IDs produced by the fallback path.
 * All other pure-virtual methods are no-ops returning safe defaults.
 */
class W10DMockDraftPlugin final : public ILLMPlugin {
public:
    /// Draft text returned by generate(); may be set before each test.
    std::string draft_text = "ABCDE";

    // ── Core inference ──────────────────────────────────────────────────────
    InferenceResponse generate(const InferenceRequest& req) override {
        InferenceResponse resp;
        resp.request_id = req.request_id;
        resp.model_id   = "w10d-mock-draft";
        resp.text       = draft_text;
        resp.success    = true;
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                  const InferenceRequest& req) override {
        return generate(req);
    }

    // ── Model lifecycle ─────────────────────────────────────────────────────
    bool loadModel(const std::string&, const json&) override {
        loaded_ = true; return true;
    }
    void unloadModel()              override { loaded_ = false; }
    bool isModelLoaded() const      override { return loaded_; }
    std::optional<ModelInfo> getModelInfo() const override {
        if (!loaded_) return std::nullopt;
        ModelInfo info{};
        info.model_id  = "w10d-mock-draft";
        info.is_loaded = true;
        return info;
    }

    // ── LoRA ────────────────────────────────────────────────────────────────
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t>  exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

    // ── Misc ────────────────────────────────────────────────────────────────
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities    getCapabilities() const override { return {}; }
    json               getMemoryStats()  const override { return {}; }
    json               getPerformanceStats() const override { return {}; }

private:
    bool loaded_{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RAII guard: clear the global draft fn on scope exit
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RAII wrapper that clears ILLMPlugin::s_default_draft_fn_ on destruction.
 *
 * Prevents global state from leaking between tests, mirroring the clear-after-
 * call contract enforced by InferenceEngineEnhanced::trySpeculativeGeneration().
 */
struct LocalDraftBridgeGuard {
    ~LocalDraftBridgeGuard() {
        ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SD-LOCAL-01 — Injected GenerateDraftTokensFn is invoked, not byte-modulo
// ─────────────────────────────────────────────────────────────────────────────

TEST(W10DLocalDraftBridgeTest, SD_LOCAL_01_InjectedFnUsedNotByteModulo) {
    LocalDraftBridgeGuard guard;

    bool fn_called             = false;
    constexpr int kSentinelTok = 9999;  // value impossible from byte-modulo with VOCAB=32000

    // Inject a real fn that returns a distinctive sentinel token ID.
    ILLMPlugin::setDefaultGenerateDraftTokensFn(
        [&](const InferenceRequest& /*req*/,
            size_t k, size_t vocab) -> ILLMPlugin::DraftTokensResult
        {
            fn_called = true;
            ILLMPlugin::DraftTokensResult r;
            r.vocab_size = vocab;
            r.tokens.reserve(k);
            r.logits.reserve(k);
            for (size_t i = 0; i < k; ++i) {
                r.tokens.push_back(kSentinelTok);
                std::vector<float> row(vocab, -5.0f);
                if (kSentinelTok < static_cast<int>(vocab)) {
                    row[static_cast<size_t>(kSentinelTok)] = 5.0f;
                }
                r.logits.push_back(std::move(row));
            }
            return r;
        });

    W10DMockDraftPlugin plugin;
    plugin.draft_text = "hello";  // byte values: 104, 101, 108, 108, 111 — all < 32000

    constexpr size_t K     = 3;
    constexpr size_t VOCAB = 32000;

    InferenceRequest req;
    req.prompt     = "speculative test";
    req.max_tokens = static_cast<int>(K);

    const auto result = plugin.generateDraftTokens(req, K, VOCAB);

    // The injected fn must have been called.
    EXPECT_TRUE(fn_called)
        << "GenerateDraftTokensFn was NOT invoked — byte-modulo heuristic fired "
           "instead of the injected bridge fn (STUB #261 bridge regression)";

    ASSERT_EQ(result.tokens.size(), K);
    for (size_t i = 0; i < K; ++i) {
        EXPECT_EQ(result.tokens[i], kSentinelTok)
            << "token[" << i << "] = " << result.tokens[i]
            << "; expected sentinel " << kSentinelTok
            << " — byte-modulo must not be used when a fn is injected";
    }

    EXPECT_EQ(result.vocab_size, VOCAB);
    ASSERT_EQ(result.logits.size(), K);
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-LOCAL-02 — No fn set → byte-modulo fallback is used
// ─────────────────────────────────────────────────────────────────────────────

TEST(W10DLocalDraftBridgeTest, SD_LOCAL_02_ByteModuloFallbackWhenNoFnSet) {
    LocalDraftBridgeGuard guard;

    // Explicitly clear any previously injected fn.
    ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);

    W10DMockDraftPlugin plugin;
    // "ABC" = ASCII 65, 66, 67 — byte-modulo values with VOCAB=100 are 65, 66, 67.
    plugin.draft_text = "ABC";

    constexpr size_t K     = 3;
    constexpr size_t VOCAB = 100;

    InferenceRequest req;
    req.prompt     = "speculative fallback test";
    req.max_tokens = static_cast<int>(K);

    const auto result = plugin.generateDraftTokens(req, K, VOCAB);

    ASSERT_EQ(result.tokens.size(), K);
    ASSERT_EQ(result.logits.size(), K);
    EXPECT_EQ(result.vocab_size, VOCAB);

    // Byte-modulo contract: token[i] = static_cast<unsigned char>(text[i]) % vocab
    const std::string& text = plugin.draft_text;
    for (size_t i = 0; i < K; ++i) {
        const int expected = static_cast<int>(
            static_cast<unsigned char>(text[i])) % static_cast<int>(VOCAB);
        EXPECT_EQ(result.tokens[i], expected)
            << "token[" << i << "]: byte-modulo expected " << expected
            << " but got " << result.tokens[i]
            << " — fallback heuristic did not produce expected ID";

        // The peaked logit for this token must be the maximum in its row.
        const auto& row = result.logits[i];
        ASSERT_LT(static_cast<size_t>(result.tokens[i]), row.size());
        const float peak = row[static_cast<size_t>(result.tokens[i])];
        for (size_t j = 0; j < row.size(); ++j) {
            if (static_cast<int>(j) != result.tokens[i]) {
                EXPECT_LE(row[j], peak)
                    << "logit[" << i << "][" << j << "] exceeds the peak at ["
                    << result.tokens[i] << "] — peaked distribution invariant violated";
            }
        }
    }
}
