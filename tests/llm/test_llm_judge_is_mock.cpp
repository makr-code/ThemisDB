/*
 * ThemisDB | File: test_llm_judge_is_mock.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_llm_judge_is_mock.cpp
 * @brief Unit tests for ParsedResponse::is_mock flag in LLMJudgeIntegration (Gap 7).
 *
 * Tests
 * -----
 * JGI_MOCK_01  is_mock == false when a real engine is injected (production path)
 * JGI_MOCK_02  is_mock == true when use_mock_mode == true
 * JGI_MOCK_03  is_mock == true when allow_mock == true and engine == nullptr
 * JGI_MOCK_04  isMockMode() is consistent with the is_mock flag value
 * JGI_MOCK_05  ParsedResponse::is_mock field exists and defaults to false
 * JGI_MOCK_06  allow_mock=true with a real engine must still be treated as non-mock
 *
 * Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 7 (Severity: Medium/S2)
 * Tracked: src/rag/FUTURE_ENHANCEMENTS.md §Gap 7
 */

#include <gtest/gtest.h>
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "rag/prompt_templates.h"
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

namespace {

// A minimal ILLMInferenceEngine that returns a valid score JSON.
struct FakeEngine : ILLMInferenceEngine {
    std::string generate(const std::string& /*prompt*/) override {
        return R"({"score":3.5,"confidence":0.9,"reasoning":"test-ok"})";
    }
};

// Build a PromptTemplateManager with a custom template that always
// produces a non-empty prompt (required by evaluateWithLLM).
PromptTemplateManager makeTemplateManager() {
    auto mgr = PromptTemplateManager::createDefault();
    // Override the FAITHFULNESS template with a trivial one so
    // generatePrompt() always returns a non-empty string.
    mgr.setTemplate(EvaluationDimension::FAITHFULNESS,
                    "Evaluate: {answer} vs {context}");
    return mgr;
}

EvaluationInput makeInput() {
    EvaluationInput in;
    in.query = "What is X?";
    in.generated_answer = "X is Y.";
    RetrievedDocument d;
    d.id = "doc-1";
    d.content = "X equals Y.";
    d.similarity_score = 1.0;
    in.documents.push_back(std::move(d));
    return in;
}

} // namespace

// ---------------------------------------------------------------------------
// JGI_MOCK_05 — is_mock field exists and defaults to false in ParsedResponse
// ---------------------------------------------------------------------------
TEST(JGI_MOCK, JGI_MOCK_05_ParsedResponseIsMockDefaultsFalse) {
    const ParsedResponse r{};
    EXPECT_FALSE(r.is_mock)
        << "ParsedResponse::is_mock must default to false";
}

// ---------------------------------------------------------------------------
// JGI_MOCK_01 — real engine: is_mock == false
// ---------------------------------------------------------------------------
TEST(JGI_MOCK, JGI_MOCK_01_RealEngineIsMockFalse) {
    FakeEngine engine;
    LLMJudgeIntegration integration(&engine);
    EXPECT_FALSE(integration.isMockMode());

    auto tmgr = makeTemplateManager();
    auto input = makeInput();
    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, input, tmgr);

    EXPECT_FALSE(result.is_mock)
        << "is_mock must be false when a real engine is injected";
}

// ---------------------------------------------------------------------------
// JGI_MOCK_02 — use_mock_mode == true → is_mock == true
// ---------------------------------------------------------------------------
TEST(JGI_MOCK, JGI_MOCK_02_UseMockModeSetsMockFlag) {
    LLMJudgeIntegration::Config cfg;
    cfg.use_mock_mode = true;
    cfg.warn_on_mock_mode = false;  // suppress spdlog output in test
    LLMJudgeIntegration integration(cfg);
    EXPECT_TRUE(integration.isMockMode());

    auto tmgr = makeTemplateManager();
    auto input = makeInput();
    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, input, tmgr);

    EXPECT_TRUE(result.is_mock)
        << "is_mock must be true when use_mock_mode == true";
}

// ---------------------------------------------------------------------------
// JGI_MOCK_03 — allow_mock + nullptr engine → is_mock == true
// ---------------------------------------------------------------------------
TEST(JGI_MOCK, JGI_MOCK_03_AllowMockNullptrEngineSetsMockFlag) {
    LLMJudgeIntegration::Config cfg;
    cfg.allow_mock = true;
    cfg.use_mock_mode = true;
    cfg.warn_on_mock_mode = false;
    LLMJudgeIntegration integration(nullptr, cfg);
    EXPECT_TRUE(integration.isMockMode());

    auto tmgr = makeTemplateManager();
    auto input = makeInput();
    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, input, tmgr);

    EXPECT_TRUE(result.is_mock)
        << "is_mock must be true when allow_mock=true and engine=nullptr";
}

// ---------------------------------------------------------------------------
// JGI_MOCK_04 — isMockMode() is consistent with result.is_mock
// ---------------------------------------------------------------------------
TEST(JGI_MOCK, JGI_MOCK_04_IsMockModeConsistentWithFlag) {
    {   // mock path
        LLMJudgeIntegration::Config cfg;
        cfg.use_mock_mode = true;
        cfg.warn_on_mock_mode = false;
        LLMJudgeIntegration integ(cfg);

        auto tmgr = makeTemplateManager();
        auto input = makeInput();
        const auto res = integ.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS, input, tmgr);

        EXPECT_EQ(integ.isMockMode(), res.is_mock)
            << "isMockMode() and result.is_mock must agree (mock case)";
    }
    {   // real engine path
        FakeEngine engine;
        LLMJudgeIntegration integ(&engine);

        auto tmgr = makeTemplateManager();
        auto input = makeInput();
        const auto res = integ.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS, input, tmgr);

        EXPECT_EQ(integ.isMockMode(), res.is_mock)
            << "isMockMode() and result.is_mock must agree (real engine case)";
    }
}

// ---------------------------------------------------------------------------
// JGI_MOCK_06 — allow_mock=true with real engine remains production path
// ---------------------------------------------------------------------------
TEST(JGI_MOCK, JGI_MOCK_06_AllowMockWithRealEngineIsNotMock) {
    LLMJudgeIntegration::Config cfg;
    cfg.allow_mock = true;  // permit nullptr fallback, but we inject a real engine
    cfg.warn_on_mock_mode = false;

    FakeEngine engine;
    LLMJudgeIntegration integration(&engine, cfg);
    EXPECT_FALSE(integration.isMockMode());

    auto tmgr = makeTemplateManager();
    auto input = makeInput();
    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, input, tmgr);

    EXPECT_FALSE(result.is_mock)
        << "is_mock must remain false when a real engine is injected, even if allow_mock=true";
}
