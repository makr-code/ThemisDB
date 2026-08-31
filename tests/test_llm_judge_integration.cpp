/**
 * @file test_llm_judge_integration.cpp
 * @brief Unit tests for fail-closed LLMJudgeIntegration production paths.
 */

#include "rag/llm_judge_integration.h"
#include "rag/prompt_templates.h"
#include "rag/rag_judge.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

using namespace themis::rag::judge;

namespace {

PromptTemplateManager makeTemplateManager() {
    auto manager = PromptTemplateManager::createDefault();
    manager.setTemplate(EvaluationDimension::FAITHFULNESS,
                        "Question: {question}\nAnswer: {answer}\nContext: {context}");
    return manager;
}

EvaluationInput makeSampleInput() {
    EvaluationInput input;
    input.query = "What is the capital of France?";
    input.generated_answer = "Paris is the capital of France.";
    input.documents = {
        {"doc1", "Paris is the capital of France.", 0.95, {}}
    };
    return input;
}

std::string makeJudgeResponse(double score,
                              double confidence,
                              const std::string& reasoning) {
    return std::string{"{\"score\":"} + std::to_string(score) +
           ",\"confidence\":" + std::to_string(confidence) +
           ",\"reasoning\":\"" + reasoning +
           "\",\"supporting_claims\":[],\"unsupported_claims\":[]}";
}

struct FixedScoreEngine : ILLMInferenceEngine {
    explicit FixedScoreEngine(double score) : score_(score) {}

    std::string generate(const std::string&) override {
        return makeJudgeResponse(score_, 0.9, "fixed");
    }

    double score_;
};

struct RandomScoreEngine : ILLMInferenceEngine {
    explicit RandomScoreEngine(unsigned int seed = 42) : rng_(seed), dist_(1.0, 5.0) {}

    std::string generate(const std::string&) override {
        return makeJudgeResponse(dist_(rng_), 0.8, "random");
    }

    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
};

} // namespace

class LLMJudgeIntegrationAvailabilityTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager = makeTemplateManager();
    EvaluationInput sample_input = makeSampleInput();
};

TEST_F(LLMJudgeIntegrationAvailabilityTest, DefaultConstructorReturnsUnavailableWithoutBackend) {
    LLMJudgeIntegration integration;

    EXPECT_FALSE(integration.isMockMode());

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reasoning, "llm_unavailable");
    EXPECT_NE(result.error_message.find("llm_unavailable"), std::string::npos);
}

TEST_F(LLMJudgeIntegrationAvailabilityTest, ConfigOnlyConstructorReturnsUnavailableWithoutBackend) {
    LLMJudgeIntegration::Config config;
    config.model_name = "judge-prod";
    config.max_retries = 1;

    LLMJudgeIntegration integration(config);

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reasoning, "llm_unavailable");
}

TEST_F(LLMJudgeIntegrationAvailabilityTest, ErrorMessageProvidesBackendGuidance) {
    LLMJudgeIntegration integration;

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("setInferenceFunction"), std::string::npos);
    EXPECT_EQ(result.error_message.find("allow_mock"), std::string::npos);
}

class LLMJudgeIntegrationConfigTest : public ::testing::Test {};

TEST_F(LLMJudgeIntegrationConfigTest, ConfigDefaultValues) {
    LLMJudgeIntegration::Config config;

    EXPECT_EQ(config.model_name, "default");
    EXPECT_DOUBLE_EQ(config.temperature, 0.3);
    EXPECT_EQ(config.max_tokens, 1024);
    EXPECT_EQ(config.max_retries, 3);
    EXPECT_EQ(config.timeout_ms, 30000);
    EXPECT_TRUE(config.use_json_mode);
}

TEST_F(LLMJudgeIntegrationConfigTest, ConfigCanBeUpdated) {
    LLMJudgeIntegration::Config config;
    config.model_name = "gpt-4";
    config.temperature = 0.7;
    config.max_tokens = 2048;

    LLMJudgeIntegration integration(config);
    const auto retrieved = integration.getConfig();

    EXPECT_EQ(retrieved.model_name, "gpt-4");
    EXPECT_DOUBLE_EQ(retrieved.temperature, 0.7);
    EXPECT_EQ(retrieved.max_tokens, 2048);
}

TEST_F(LLMJudgeIntegrationConfigTest, ConfigCanBeChangedAtRuntime) {
    LLMJudgeIntegration integration;

    LLMJudgeIntegration::Config new_config;
    new_config.model_name = "custom-model";
    new_config.max_retries = 5;

    integration.setConfig(new_config);

    const auto retrieved = integration.getConfig();
    EXPECT_EQ(retrieved.model_name, "custom-model");
    EXPECT_EQ(retrieved.max_retries, 5);
}

class LLMJudgeIntegrationInferenceFunctionTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager = makeTemplateManager();
    EvaluationInput sample_input = makeSampleInput();
};

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, CustomInferenceFunctionCalled) {
    LLMJudgeIntegration integration;

    int call_count = 0;
    integration.setInferenceFunction([&call_count](const std::string&) {
        ++call_count;
        return makeJudgeResponse(3.5, 0.75, "Test response");
    });

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_EQ(call_count, 1);
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.score.has_value());
    EXPECT_DOUBLE_EQ(*result.score, 3.5);
}

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, InferenceFunctionReceivesPrompt) {
    LLMJudgeIntegration integration;

    std::string received_prompt;
    integration.setInferenceFunction([&received_prompt](const std::string& prompt) {
        received_prompt = prompt;
        return makeJudgeResponse(4.5, 0.9, "Test");
    });

    integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_FALSE(received_prompt.empty());
    EXPECT_NE(received_prompt.find("France"), std::string::npos);
}

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, RetriesOnFailure) {
    LLMJudgeIntegration::Config config;
    config.max_retries = 3;

    LLMJudgeIntegration integration(config);

    int attempt_count = 0;
    integration.setInferenceFunction([&attempt_count](const std::string&) {
        ++attempt_count;
        if (attempt_count < 2) {
            throw std::runtime_error("Simulated failure");
        }
        return makeJudgeResponse(4.0, 0.8, "Success after retry");
    });

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_GE(attempt_count, 2);
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, FailsAfterMaxRetries) {
    LLMJudgeIntegration::Config config;
    config.max_retries = 2;

    LLMJudgeIntegration integration(config);
    integration.setInferenceFunction([](const std::string&) -> std::string {
        throw std::runtime_error("Always fails");
    });

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("backend did not return a response"), std::string::npos);
}

class LLMJudgeIntegrationDimensionTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager = makeTemplateManager();
    EvaluationInput sample_input = [] {
        EvaluationInput input;
        input.query = "What is quantum computing?";
        input.generated_answer = "Quantum computing uses quantum mechanics principles.";
        input.documents = {
            {"doc1", "Quantum computing leverages quantum mechanics.", 0.9, {}}
        };
        return input;
    }();
};

TEST_F(LLMJudgeIntegrationDimensionTest, EvaluateSupportedDimensionsViaInjectedFunction) {
    const std::vector<EvaluationDimension> dimensions = {
        EvaluationDimension::FAITHFULNESS,
        EvaluationDimension::RELEVANCE,
        EvaluationDimension::COMPLETENESS,
        EvaluationDimension::COHERENCE,
    };

    for (const auto dimension : dimensions) {
        LLMJudgeIntegration integration;
        integration.setInferenceFunction([](const std::string&) {
            return makeJudgeResponse(4.1, 0.82, "dimension-ok");
        });

        const auto result = integration.evaluateWithLLM(
            dimension,
            sample_input,
            template_manager);

        EXPECT_TRUE(result.success);
    }
}

class LLMJudgeIntegrationEngineInjectionTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager = makeTemplateManager();
    EvaluationInput sample_input = [] {
        EvaluationInput input;
        input.query = "What is the capital of Germany?";
        input.generated_answer = "Berlin is the capital of Germany.";
        input.documents = {
            {"doc1", "Berlin is the capital and largest city of Germany.", 0.97, {}}
        };
        return input;
    }();
};

TEST_F(LLMJudgeIntegrationEngineInjectionTest, ConstructWithNullEngineThrows) {
    LLMJudgeIntegration::Config config;
    EXPECT_THROW((LLMJudgeIntegration(nullptr, config)), std::invalid_argument);
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, ConstructWithRealEngineProducesExpectedScore) {
    FixedScoreEngine engine(3.7);
    LLMJudgeIntegration integration(&engine);

    EXPECT_FALSE(integration.isMockMode());

    const auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager);

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.score.has_value());
    EXPECT_NEAR(*result.score, 3.7, 0.01);
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, RandomEngineProducesScoreVariance) {
    RandomScoreEngine engine(12345);
    LLMJudgeIntegration integration(&engine);

    constexpr int kSamples = 8;
    std::vector<double> scores;
    scores.reserve(kSamples);

    for (int i = 0; i < kSamples; ++i) {
        const auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            sample_input,
            template_manager);
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(result.score.has_value());
        scores.push_back(*result.score);
    }

    const auto minmax = std::minmax_element(scores.begin(), scores.end());
    EXPECT_GT(*minmax.second - *minmax.first, 0.0);
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, IsMockModeAlwaysFalseForProductionPaths) {
    FixedScoreEngine engine(4.0);
    LLMJudgeIntegration integration(&engine);
    EXPECT_FALSE(integration.isMockMode());

    LLMJudgeIntegration fn_integration;
    fn_integration.setInferenceFunction([](const std::string&) {
        return makeJudgeResponse(4.0, 0.9, "fn");
    });
    EXPECT_FALSE(fn_integration.isMockMode());
}
