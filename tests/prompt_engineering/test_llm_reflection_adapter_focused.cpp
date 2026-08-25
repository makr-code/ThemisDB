/**
 * @file test_llm_reflection_adapter_focused.cpp
 * @brief Group RA — ILLMProviderReflectionAdapter interface-contract and LLM-path tests.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/llm_reflection_adapter.h"
#include "prompt_engineering/meta_prompt_generator.h"  // ILLMProvider

#include <memory>
#include <string>
#include <optional>

using namespace themis::prompt_engineering;

// ── Minimal ILLMProvider stub ─────────────────────────────────────────────────

class EchoLLMProvider final : public ILLMProvider {
public:
    explicit EchoLLMProvider(std::string tag = "echo") : tag_(std::move(tag)) {}

    std::string complete(const std::string& prompt) const override {
        last_prompt_ = prompt;
        return tag_ + ":" + prompt;
    }

    std::string name() const override { return tag_; }

    mutable std::string last_prompt_;

private:
    std::string tag_;
};

// ── Minimal IReflectionScorer stub ────────────────────────────────────────────

class FixedScorer final : public IReflectionScorer {
public:
    explicit FixedScorer(double value) : value_(value) {}
    double score(const std::string& /*p*/, const std::string& /*r*/) const override {
        return value_;
    }
private:
    double value_;
};

// ── RA1: null llm provider → generate returns empty string ───────────────────
TEST(LlmReflectionAdapterFocused, RA1_NullProvider_GenerateReturnsEmpty) {
    ILLMProviderReflectionAdapter adapter(nullptr);
    EXPECT_EQ(adapter.generate("hello"), "");
}

// ── RA2: null llm provider → critique returns empty string ───────────────────
TEST(LlmReflectionAdapterFocused, RA2_NullProvider_CritiqueReturnsEmpty) {
    ILLMProviderReflectionAdapter adapter(nullptr);
    EXPECT_EQ(adapter.critique("prompt", "response"), "");
}

// ── RA3: null llm provider → revise returns original response ────────────────
TEST(LlmReflectionAdapterFocused, RA3_NullProvider_ReviseReturnsOriginalResponse) {
    ILLMProviderReflectionAdapter adapter(nullptr);
    EXPECT_EQ(adapter.revise("prompt", "my response", "critique"), "my response");
}

// ── RA4: valid provider → generate delegates to complete() ───────────────────
TEST(LlmReflectionAdapterFocused, RA4_ValidProvider_GenerateDelegatesToComplete) {
    auto llm  = std::make_shared<EchoLLMProvider>("gpt");
    ILLMProviderReflectionAdapter adapter(llm);

    const std::string result = adapter.generate("test prompt");
    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(llm->last_prompt_.empty());
}

// ── RA5: setScorer / hasScorer / clearScorer lifecycle ────────────────────────
TEST(LlmReflectionAdapterFocused, RA5_ScorerLifecycle) {
    auto llm  = std::make_shared<EchoLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);

    EXPECT_FALSE(adapter.hasScorer());

    adapter.setScorer(std::make_shared<FixedScorer>(0.8));
    EXPECT_TRUE(adapter.hasScorer());

    adapter.clearScorer();
    EXPECT_FALSE(adapter.hasScorer());
}

// ── RA6: injected scorer is used by score() ───────────────────────────────────
TEST(LlmReflectionAdapterFocused, RA6_InjectedScorer_UsedByScore) {
    auto llm = std::make_shared<EchoLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    adapter.setScorer(std::make_shared<FixedScorer>(0.99));

    double s = adapter.score("prompt", "response");
    EXPECT_DOUBLE_EQ(s, 0.99);
}

// ── RA7: without scorer, score() falls back to heuristic (non-negative) ───────
TEST(LlmReflectionAdapterFocused, RA7_NoScorer_HeuristicScoreNonNegative) {
    auto llm = std::make_shared<EchoLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);

    double s = adapter.score("prompt", "a meaningful non-empty response");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

// ── RA8: heuristic score for empty response is 0 ────────────────────────────
TEST(LlmReflectionAdapterFocused, RA8_NoScorer_EmptyResponse_ScoreZero) {
    auto llm = std::make_shared<EchoLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);

    double s = adapter.score("prompt", "");
    EXPECT_DOUBLE_EQ(s, 0.0);
}

// ── RA9: setStrategy / getStrategy roundtrip ─────────────────────────────────
TEST(LlmReflectionAdapterFocused, RA9_SetGetStrategy_Roundtrip) {
    auto llm = std::make_shared<EchoLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm, ReflectionStrategy::SELF_REFINE);
    EXPECT_EQ(adapter.getStrategy(), ReflectionStrategy::SELF_REFINE);

    adapter.setStrategy(ReflectionStrategy::CHAIN_OF_THOUGHT);
    EXPECT_EQ(adapter.getStrategy(), ReflectionStrategy::CHAIN_OF_THOUGHT);
}

// ── RA10: name() reflects wrapped provider ────────────────────────────────────
TEST(LlmReflectionAdapterFocused, RA10_Name_ContainsProviderName) {
    auto llm = std::make_shared<EchoLLMProvider>("my-provider");
    ILLMProviderReflectionAdapter adapter(llm);
    EXPECT_NE(adapter.name().find("my-provider"), std::string::npos);
}

// ── RA11: name() with null provider does not crash ────────────────────────────
TEST(LlmReflectionAdapterFocused, RA11_NullProvider_NameDoesNotCrash) {
    ILLMProviderReflectionAdapter adapter(nullptr);
    EXPECT_NE(adapter.name(), "");  // "null" variant expected
}
