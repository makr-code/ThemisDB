/**
 * @file test_llm_judge_is_mock.cpp
 * @brief Compatibility tests for ParsedResponse::is_mock after mock removal.
 */

#include <gtest/gtest.h>
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "rag/prompt_templates.h"
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

namespace {

struct FakeEngine : ILLMInferenceEngine {
    std::string generate(const std::string& /*prompt*/) override {
        return R"({"score":3.5,"confidence":0.9,"reasoning":"test-ok"})";
    }
};

PromptTemplateManager makeTemplateManager() {
    auto mgr = PromptTemplateManager::createDefault();
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

TEST(JGI_MOCK, JGI_MOCK_01_ParsedResponseIsMockDefaultsFalse) {
    const ParsedResponse r{};
    EXPECT_FALSE(r.is_mock);
}

TEST(JGI_MOCK, JGI_MOCK_02_RealEngineLeavesMockFlagFalse) {
    FakeEngine engine;
    LLMJudgeIntegration integration(&engine);
    EXPECT_FALSE(integration.isMockMode());

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, makeInput(), makeTemplateManager());

    EXPECT_FALSE(result.is_mock);
}

TEST(JGI_MOCK, JGI_MOCK_03_UnavailablePathLeavesMockFlagFalse) {
    LLMJudgeIntegration integration;
    EXPECT_FALSE(integration.isMockMode());

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, makeInput(), makeTemplateManager());

    EXPECT_FALSE(result.is_mock);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reasoning, "llm_unavailable");
}

TEST(JGI_MOCK, JGI_MOCK_04_GateDisabledLeavesMockFlagFalse) {
    FakeEngine engine;
    LLMJudgeIntegration::Config cfg;
    cfg.enable_llm_judge = false;

    LLMJudgeIntegration integration(&engine, cfg);
    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, makeInput(), makeTemplateManager());

    EXPECT_FALSE(result.is_mock);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reasoning, "llm_unavailable");
}

TEST(JGI_MOCK, JGI_MOCK_05_InjectedFunctionLeavesMockFlagFalse) {
    LLMJudgeIntegration integration;
    integration.setInferenceFunction([](const std::string&) {
        return R"({"score":4.0,"confidence":0.8,"reasoning":"ok"})";
    });

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS, makeInput(), makeTemplateManager());

    EXPECT_FALSE(integration.isMockMode());
    EXPECT_FALSE(result.is_mock);
    EXPECT_TRUE(result.success);
}
