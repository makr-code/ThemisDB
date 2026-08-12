/**
 * @file test_speculative_decoder_real_logits.cpp
 * @brief Unit tests for ILLMPlugin::generateDraftTokens() and its integration
 *        with SpeculativeDecoder::verify().
 *
 * Test IDs: SD-REAL-01 .. SD-REAL-08
 *
 * Covers:
 *  - Default implementation of generateDraftTokens() (STUB #261):
 *      SD-REAL-01  returns exactly k tokens
 *      SD-REAL-02  maps UTF-8 char codes to token IDs (char % vocab_size)
 *      SD-REAL-03  logit rows are peaked (+5 / −5) at the mapped token ID
 *      SD-REAL-04  short text pads remaining positions with token 0
 *      SD-REAL-05  vocab_size_hint=0 falls back to 32 000
 *  - Integration with SpeculativeDecoder::verify():
 *      SD-REAL-06  non-constant draft IDs exercise distinct acceptance paths
 *      SD-REAL-07  output from generateDraftTokens() is directly verify()-compatible
 *  - Overridability:
 *      SD-REAL-08  a plugin that overrides generateDraftTokens() with real logits
 *                  is used in preference to the default
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_interface.h"
#include "llm/speculative_decoder.h"

#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Minimal concrete ILLMPlugin that satisfies the pure-virtual contract.
// generate() returns a fixed text string; all other methods are no-ops.
// generateDraftTokens() is NOT overridden — uses the base-class default.
// ─────────────────────────────────────────────────────────────────────────────
class StubPlugin : public ILLMPlugin {
public:
    explicit StubPlugin(std::string draft_text = "hello",
                        size_t      vocab      = 256)
        : draft_text_(std::move(draft_text)), vocab_(vocab) {}

    // ── ILLMPlugin minimal stubs ─────────────────────────────────────────
    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo m;
        m.model_id  = "stub";
        m.is_loaded = true;
        m.vocab_size = vocab_;
        return m;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& req) override {
        last_max_tokens = req.max_tokens;
        InferenceResponse r;
        r.text    = draft_text_;
        r.success = true;
        return r;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                  const InferenceRequest& req) override {
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
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

    // Inspection helpers for tests.
    mutable int last_max_tokens = -1;

private:
    std::string draft_text_;
    size_t      vocab_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Plugin that overrides generateDraftTokens() with "real" logits
// (peaked at known token IDs supplied by the test).
// ─────────────────────────────────────────────────────────────────────────────
class OverridePlugin : public StubPlugin {
public:
    explicit OverridePlugin(std::vector<int> ids, size_t vocab = 256)
        : StubPlugin("", vocab), ids_(std::move(ids)), vocab_(vocab) {}

    DraftTokensResult generateDraftTokens(const InferenceRequest&,
                                          size_t k,
                                          size_t /*hint*/) override {
        called = true;
        DraftTokensResult result;
        result.vocab_size = vocab_;
        for (size_t i = 0; i < k; ++i) {
            const int id = (i < ids_.size()) ? ids_[i] : 0;
            result.tokens.push_back(id);
            std::vector<float> row(vocab_, -5.0f);
            row[static_cast<size_t>(id)] = 5.0f;
            result.logits.push_back(std::move(row));
        }
        return result;
    }

    bool called = false;

private:
    std::vector<int> ids_;
    size_t           vocab_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: verify that a logit row is peaked at `peak_id` and all others
// are below the peak.
// ─────────────────────────────────────────────────────────────────────────────
static void assertPeakedAt(const std::vector<float>& row, int peak_id,
                            size_t vocab_size) {
    ASSERT_EQ(row.size(), vocab_size);
    ASSERT_GE(peak_id, 0);
    ASSERT_LT(static_cast<size_t>(peak_id), vocab_size);
    const float peak_val = row[static_cast<size_t>(peak_id)];
    for (size_t j = 0; j < vocab_size; ++j) {
        if (static_cast<int>(j) == peak_id) continue;
        EXPECT_LT(row[j], peak_val)
            << "row[" << j << "] should be < peak row[" << peak_id << "]";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-01: generateDraftTokens() returns exactly k token entries
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_01_ReturnsSizeK) {
    StubPlugin plugin("abcde", 256);
    InferenceRequest req;
    req.prompt = "test";

    for (size_t k : {1u, 4u, 8u, 16u}) {
        const auto res = plugin.generateDraftTokens(req, k, 256);
        EXPECT_EQ(res.tokens.size(), k)
            << "Expected " << k << " draft tokens";
        EXPECT_EQ(res.logits.size(), k)
            << "Expected " << k << " logit rows";
        EXPECT_EQ(res.vocab_size, 256u);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-02: token IDs match UTF-8 byte code modulo vocab_size
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_02_TokenIdsMappedFromText) {
    const std::string text = "Hello";
    constexpr size_t  vocab = 128;
    StubPlugin plugin(text, vocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto res = plugin.generateDraftTokens(req, text.size(), vocab);
    ASSERT_EQ(res.tokens.size(), text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        const int expected =
            static_cast<int>(static_cast<unsigned char>(text[i])) %
            static_cast<int>(vocab);
        EXPECT_EQ(res.tokens[i], expected)
            << "Mismatch at position " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-03: each logit row is peaked at the corresponding token ID
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_03_LogitRowsPeakedAtTokenId) {
    const std::string text = "ABCDE";
    constexpr size_t  vocab = 200;
    StubPlugin plugin(text, vocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto res = plugin.generateDraftTokens(req, text.size(), vocab);
    ASSERT_EQ(res.logits.size(), text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        assertPeakedAt(res.logits[i], res.tokens[i], vocab);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-04: text shorter than k → remaining positions padded with token 0
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_04_ShortTextPadsWith0) {
    const std::string text = "hi";   // 2 chars
    constexpr size_t  k    = 5u;
    constexpr size_t  vocab = 256;
    StubPlugin plugin(text, vocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto res = plugin.generateDraftTokens(req, k, vocab);
    ASSERT_EQ(res.tokens.size(), k);

    // Positions 0..1: from text; positions 2..4: must be 0
    for (size_t i = text.size(); i < k; ++i) {
        EXPECT_EQ(res.tokens[i], 0)
            << "Position " << i << " beyond text length should be token 0";
        assertPeakedAt(res.logits[i], 0, vocab);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-05: vocab_size_hint=0 falls back to 32 000
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_05_VocabHintZeroFallsBackTo32000) {
    StubPlugin plugin("x", 0 /* getModelInfo reports 0 */);
    InferenceRequest req;
    req.prompt = "test";

    const auto res = plugin.generateDraftTokens(req, 1, 0 /* hint=0 */);
    ASSERT_EQ(res.vocab_size, 32000u);
    ASSERT_EQ(res.logits[0].size(), 32000u);
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-06: different draft texts produce different token IDs
//             (ensures non-constant behaviour unlike the old peaked-at-0 stub)
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_06_DifferentTextsProduceDifferentIds) {
    constexpr size_t vocab = 256;
    constexpr size_t k     = 4;
    InferenceRequest req;
    req.prompt = "test";

    StubPlugin plugin_a("AAAA", vocab);
    StubPlugin plugin_b("ZZZZ", vocab);

    const auto res_a = plugin_a.generateDraftTokens(req, k, vocab);
    const auto res_b = plugin_b.generateDraftTokens(req, k, vocab);

    // 'A' (65) != 'Z' (90) mod 256 → different token IDs
    ASSERT_EQ(res_a.tokens.size(), k);
    ASSERT_EQ(res_b.tokens.size(), k);
    EXPECT_NE(res_a.tokens[0], res_b.tokens[0])
        << "Draft token IDs should differ for different draft text";
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-07: generateDraftTokens() output is directly valid for verify()
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_07_OutputIsVerifyCompatible) {
    constexpr size_t vocab = 64;
    constexpr size_t k     = 4;
    StubPlugin plugin("test", vocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto draft = plugin.generateDraftTokens(req, k, vocab);

    // Build K+1 target logit rows peaked at token 1 (different from draft).
    std::vector<std::vector<float>> target_logits(k + 1,
        std::vector<float>(vocab, -5.0f));
    for (auto& row : target_logits) {
        row[1] = 5.0f;
    }

    SpeculativeDecoder decoder;
    EXPECT_NO_THROW({
        const auto result =
            decoder.verify(draft.tokens, draft.logits, target_logits);
        // num_accepted is in [0, k]
        EXPECT_GE(result.num_accepted, 0u);
        EXPECT_LE(result.num_accepted, k);
    }) << "verify() must accept generateDraftTokens() output without throwing";
}

// ─────────────────────────────────────────────────────────────────────────────
// SD-REAL-08: a plugin overriding generateDraftTokens() with known token IDs
//             is called and its values reach SpeculativeDecoder::verify()
// ─────────────────────────────────────────────────────────────────────────────
TEST(SdRealLogits, SD_REAL_08_OverrideIsUsedInsteadOfDefault) {
    constexpr size_t vocab = 64;
    constexpr size_t k     = 3;
    const std::vector<int> known_ids = {7, 15, 42};

    OverridePlugin plugin(known_ids, vocab);
    InferenceRequest req;
    req.prompt = "test";

    const auto draft = plugin.generateDraftTokens(req, k, vocab);

    EXPECT_TRUE(plugin.called)
        << "Overriding generateDraftTokens() should have been called";
    ASSERT_EQ(draft.tokens.size(), k);
    for (size_t i = 0; i < k; ++i) {
        EXPECT_EQ(draft.tokens[i], known_ids[i])
            << "Token ID at position " << i << " should match injected value";
        assertPeakedAt(draft.logits[i], known_ids[i], vocab);
    }

    // Ensure the result is still verify()-compatible.
    std::vector<std::vector<float>> target(k + 1,
        std::vector<float>(vocab, -5.0f));
    for (auto& row : target) row[0] = 5.0f;

    SpeculativeDecoder decoder;
    EXPECT_NO_THROW(decoder.verify(draft.tokens, draft.logits, target));
}
