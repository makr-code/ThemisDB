/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_meta_prompt_llm_provider.cpp                  ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 13:49:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 7                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6632e4e56  2026-02-21  Add License Portal and Renewal Reminder classes for ThemisDB ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bb8fd581f  2026-02-21  Prompt Engineering Module: Production-Readiness (Validati... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_meta_prompt_llm_provider.cpp
 * @brief Tests for the pluggable LLM interface on MetaPromptGenerator (issue 2.3)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/meta_prompt_generator.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Stub LLM provider
// ============================================================================

class StubLLMProvider : public ILLMProvider {
public:
    std::string complete(const std::string& /*prompt*/) const override {
        return "LLM_IMPROVED_PROMPT";
    }
    std::string name() const override { return "StubLLM"; }
};

class FailingLLMProvider : public ILLMProvider {
public:
    std::string complete(const std::string& /*prompt*/) const override {
        throw std::runtime_error("LLM backend unavailable");
    }
    std::string name() const override { return "FailingLLM"; }
};

class EmptyLLMProvider : public ILLMProvider {
public:
    std::string complete(const std::string& /*prompt*/) const override {
        return "";
    }
    std::string name() const override { return "EmptyLLM"; }
};

// ============================================================================
// Tests
// ============================================================================

TEST(MetaPromptLLMProviderTest, NoProvider_UsesTemplateFallback) {
    MetaPromptGenerator gen;
    EXPECT_FALSE(gen.hasLLMProvider());

    auto result = gen.generateImprovementPrompt("Original prompt", "Bad feedback", 0.3);
    EXPECT_FALSE(result.improvement_suggestion.empty());
    // LLM flag should not be present
    EXPECT_FALSE(result.metadata.value("llm_generated", false));
}

TEST(MetaPromptLLMProviderTest, WithProvider_UsesLLMResponse) {
    MetaPromptGenerator gen;
    gen.setLLMProvider(std::make_shared<StubLLMProvider>());
    EXPECT_TRUE(gen.hasLLMProvider());

    auto result = gen.generateImprovementPrompt("Original prompt", "Some feedback", 0.6);
    EXPECT_EQ(result.improvement_suggestion, "LLM_IMPROVED_PROMPT");
    EXPECT_TRUE(result.metadata.value("llm_generated", false));
    EXPECT_EQ(result.metadata.value("llm_provider", std::string()), "StubLLM");
}

TEST(MetaPromptLLMProviderTest, FailingProvider_FallsBackToTemplate) {
    MetaPromptGenerator gen;
    gen.setLLMProvider(std::make_shared<FailingLLMProvider>());

    auto result = gen.generateImprovementPrompt("Original prompt", "Some feedback", 0.6);
    // Should not be marked as LLM generated
    EXPECT_FALSE(result.metadata.value("llm_generated", false));
    // Should still have some improvement suggestion from template path
    EXPECT_FALSE(result.improvement_suggestion.empty());
}

TEST(MetaPromptLLMProviderTest, EmptyResponseProvider_FallsBackToTemplate) {
    MetaPromptGenerator gen;
    gen.setLLMProvider(std::make_shared<EmptyLLMProvider>());

    auto result = gen.generateImprovementPrompt("Original prompt", "Some feedback", 0.8);
    EXPECT_FALSE(result.metadata.value("llm_generated", false));
    EXPECT_FALSE(result.improvement_suggestion.empty());
}

TEST(MetaPromptLLMProviderTest, ClearProvider) {
    MetaPromptGenerator gen;
    gen.setLLMProvider(std::make_shared<StubLLMProvider>());
    EXPECT_TRUE(gen.hasLLMProvider());

    gen.clearLLMProvider();
    EXPECT_FALSE(gen.hasLLMProvider());
}
