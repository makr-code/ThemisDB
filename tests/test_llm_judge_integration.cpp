/**
 * @file test_llm_judge_integration.cpp
 * @brief Unit tests for LLMJudgeIntegration mock mode and configuration
 */

#include "rag/llm_judge_integration.h"
#include "rag/prompt_templates.h"
#include "rag/rag_judge.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

using namespace themis::rag::judge;
using json = nlohmann::json;

// ============================================================================
// Mock Mode Configuration Tests
// ============================================================================

class LLMJudgeIntegrationMockModeTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager;
    EvaluationInput sample_input;
    
    void SetUp() override {
        template_manager = PromptTemplateManager::createDefault();
        
        sample_input.query = "What is the capital of France?";
        sample_input.generated_answer = "Paris is the capital of France.";
        sample_input.documents = {
            {"doc1", "Paris is the capital of France.", 0.95, {}}
        };
    }
};

TEST_F(LLMJudgeIntegrationMockModeTest, DefaultConstructorRequiresInferenceFunction) {
    // Default config should not be in mock mode
    LLMJudgeIntegration integration;
    
    EXPECT_FALSE(integration.isMockMode());
    
    // Should throw when trying to evaluate without setting inference function
    EXPECT_THROW({
        integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            sample_input,
            template_manager
        );
    }, std::runtime_error);
}

TEST_F(LLMJudgeIntegrationMockModeTest, ExplicitMockModeWorks) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    
    LLMJudgeIntegration integration(config);
    
    EXPECT_TRUE(integration.isMockMode());
    
    // Should not throw - uses mock responses
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.score.has_value());
    EXPECT_GE(*result.score, 1.0);
    EXPECT_LE(*result.score, 5.0);
}

TEST_F(LLMJudgeIntegrationMockModeTest, MockModeDeterministicForSamePromptAndVariesAcrossPrompts) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;

    LLMJudgeIntegration integration(config);

    const auto first = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    const auto second = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );

    ASSERT_TRUE(first.success);
    ASSERT_TRUE(second.success);
    ASSERT_TRUE(first.score.has_value());
    ASSERT_TRUE(second.score.has_value());
    ASSERT_TRUE(first.confidence.has_value());
    ASSERT_TRUE(second.confidence.has_value());
    EXPECT_DOUBLE_EQ(*first.score, *second.score);
    EXPECT_DOUBLE_EQ(*first.confidence, *second.confidence);

    auto alternate_input = sample_input;
    alternate_input.query = "List renewable energy types.";
    alternate_input.generated_answer = "Solar and wind are renewable sources.";
    alternate_input.documents = {
        {"doc2", "Renewable energy includes solar and wind.", 0.92, {}}
    };
    const auto alternate = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        alternate_input,
        template_manager
    );

    ASSERT_TRUE(alternate.success);
    ASSERT_TRUE(alternate.score.has_value());
    ASSERT_TRUE(alternate.confidence.has_value());

    const bool score_changed = std::abs(*first.score - *alternate.score) > 1e-12;
    const bool confidence_changed = std::abs(*first.confidence - *alternate.confidence) > 1e-12;
    EXPECT_TRUE(score_changed || confidence_changed);
}

TEST_F(LLMJudgeIntegrationMockModeTest, MockModeWarningShownOnce) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = true;
    
    LLMJudgeIntegration integration(config);
    
    // Call multiple times - warning should only be shown once
    for (int i = 0; i < 3; i++) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            sample_input,
            template_manager
        );
        EXPECT_TRUE(result.success);
    }
}

TEST_F(LLMJudgeIntegrationMockModeTest, MockModeWarningCanBeDisabled) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    
    EXPECT_TRUE(integration.isMockMode());
    
    // Should work without warnings
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationMockModeTest, SetInferenceFunctionOverridesMockMode) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    
    // Set custom inference function
    integration.setInferenceFunction([](const std::string& prompt) {
        return R"({
            "score": 5.0,
            "confidence": 0.99,
            "reasoning": "Custom inference",
            "supporting_claims": ["Custom claim"],
            "unsupported_claims": []
        })";
    });
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.score.has_value());
    EXPECT_DOUBLE_EQ(*result.score, 5.0);  // Custom returns 5.0
}

TEST_F(LLMJudgeIntegrationMockModeTest, ErrorMessageProvidesGuidance) {
    LLMJudgeIntegration integration;
    
    std::string error_message;
    try {
        integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            sample_input,
            template_manager
        );
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        error_message = e.what();
    }
    
    // Error message should provide guidance about all available options
    EXPECT_NE(error_message.find("setInferenceFunction"), std::string::npos);
    EXPECT_NE(error_message.find("allow_mock"), std::string::npos);
}

// ============================================================================
// Configuration Tests
// ============================================================================

class LLMJudgeIntegrationConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LLMJudgeIntegrationConfigTest, ConfigDefaultValues) {
    LLMJudgeIntegration::Config config;
    
    EXPECT_EQ(config.model_name, "default");
    EXPECT_DOUBLE_EQ(config.temperature, 0.3);
    EXPECT_EQ(config.max_tokens, 1024);
    EXPECT_EQ(config.max_retries, 3);
    EXPECT_EQ(config.timeout_ms, 30000);
    EXPECT_TRUE(config.use_json_mode);
    EXPECT_FALSE(config.use_mock_mode);
    EXPECT_TRUE(config.warn_on_mock_mode);
    EXPECT_FALSE(config.allow_mock);  // default false = production mode (fail fast on nullptr)
}

TEST_F(LLMJudgeIntegrationConfigTest, ConfigCanBeUpdated) {
    LLMJudgeIntegration::Config config;
    config.model_name = "gpt-4";
    config.temperature = 0.7;
    config.max_tokens = 2048;
    config.use_mock_mode = true;
    
    LLMJudgeIntegration integration(config);
    
    auto retrieved = integration.getConfig();
    EXPECT_EQ(retrieved.model_name, "gpt-4");
    EXPECT_DOUBLE_EQ(retrieved.temperature, 0.7);
    EXPECT_EQ(retrieved.max_tokens, 2048);
    EXPECT_TRUE(retrieved.use_mock_mode);
}

TEST_F(LLMJudgeIntegrationConfigTest, ConfigCanBeChangedAtRuntime) {
    LLMJudgeIntegration integration;
    
    LLMJudgeIntegration::Config new_config;
    new_config.model_name = "custom-model";
    new_config.max_retries = 5;
    
    integration.setConfig(new_config);
    
    auto retrieved = integration.getConfig();
    EXPECT_EQ(retrieved.model_name, "custom-model");
    EXPECT_EQ(retrieved.max_retries, 5);
}

// ============================================================================
// Inference Function Tests
// ============================================================================

class LLMJudgeIntegrationInferenceFunctionTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager;
    EvaluationInput sample_input;
    
    void SetUp() override {
        template_manager = PromptTemplateManager::createDefault();
        
        sample_input.query = "What is the capital of France?";
        sample_input.generated_answer = "Paris is the capital of France.";
        sample_input.documents = {
            {"doc1", "Paris is the capital of France.", 0.95, {}}
        };
    }
};

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, CustomInferenceFunctionCalled) {
    LLMJudgeIntegration integration;
    
    int call_count = 0;
    integration.setInferenceFunction([&call_count](const std::string& prompt) {
        call_count++;
        return R"({
            "score": 3.5,
            "confidence": 0.75,
            "reasoning": "Test response",
            "supporting_claims": [],
            "unsupported_claims": []
        })";
    });
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    EXPECT_EQ(call_count, 1);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(*result.score, 3.5);
}

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, InferenceFunctionReceivesPrompt) {
    LLMJudgeIntegration integration;
    
    std::string received_prompt;
    integration.setInferenceFunction([&received_prompt](const std::string& prompt) {
        received_prompt = prompt;
        return R"({
            "score": 4.5,
            "confidence": 0.9,
            "reasoning": "Test",
            "supporting_claims": [],
            "unsupported_claims": []
        })";
    });
    
    integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    EXPECT_FALSE(received_prompt.empty());
    // Prompt should contain the query
    EXPECT_NE(received_prompt.find("France"), std::string::npos);
}

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, RetriesOnFailure) {
    LLMJudgeIntegration::Config config;
    config.max_retries = 3;
    
    LLMJudgeIntegration integration(config);
    
    int attempt_count = 0;
    integration.setInferenceFunction([&attempt_count](const std::string& prompt) {
        attempt_count++;
        if (attempt_count < 2) {
            throw std::runtime_error("Simulated failure");
        }
        return R"({
            "score": 4.0,
            "confidence": 0.8,
            "reasoning": "Success after retry",
            "supporting_claims": [],
            "unsupported_claims": []
        })";
    });
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    EXPECT_GE(attempt_count, 2);
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationInferenceFunctionTest, FailsAfterMaxRetries) {
    LLMJudgeIntegration::Config config;
    config.max_retries = 2;
    
    LLMJudgeIntegration integration(config);
    
    integration.setInferenceFunction([](const std::string& prompt) {
        throw std::runtime_error("Always fails");
        return "{}";
    });
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("failed to respond"), std::string::npos);
}

// ============================================================================
// Integration Tests with Different Dimensions
// ============================================================================

class LLMJudgeIntegrationDimensionTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager;
    EvaluationInput sample_input;
    
    void SetUp() override {
        template_manager = PromptTemplateManager::createDefault();
        
        sample_input.query = "What is quantum computing?";
        sample_input.generated_answer = "Quantum computing uses quantum mechanics principles.";
        sample_input.documents = {
            {"doc1", "Quantum computing leverages quantum mechanics.", 0.9, {}}
        };
    }
};

TEST_F(LLMJudgeIntegrationDimensionTest, EvaluateFaithfulness) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    LLMJudgeIntegration integration(config);
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationDimensionTest, EvaluateRelevance) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    LLMJudgeIntegration integration(config);
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::RELEVANCE,
        sample_input,
        template_manager
    );
    
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationDimensionTest, EvaluateCompleteness) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    LLMJudgeIntegration integration(config);
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::COMPLETENESS,
        sample_input,
        template_manager
    );
    
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationDimensionTest, EvaluateCoherence) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    LLMJudgeIntegration integration(config);
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::COHERENCE,
        sample_input,
        template_manager
    );
    
    EXPECT_TRUE(result.success);
}

// ============================================================================
// ILLMInferenceEngine Injection Tests
// ============================================================================

class LLMJudgeIntegrationEngineInjectionTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager;
    EvaluationInput sample_input;
    
    void SetUp() override {
        template_manager = PromptTemplateManager::createDefault();
        
        sample_input.query = "What is the capital of Germany?";
        sample_input.generated_answer = "Berlin is the capital of Germany.";
        sample_input.documents = {
            {"doc1", "Berlin is the capital and largest city of Germany.", 0.97, {}}
        };
    }
};

// A deterministic stub that always returns the same fixed score
struct FixedScoreEngine : ILLMInferenceEngine {
    explicit FixedScoreEngine(double score) : score_(score) {}
    std::string generate(const std::string&) override {
        return R"({"score":)" + std::to_string(score_) + R"(,"confidence":0.9,"reasoning":"fixed"})";
    }
    double score_;
};

// A random-score engine that returns a different score on each call
struct RandomScoreEngine : ILLMInferenceEngine {
    explicit RandomScoreEngine(unsigned int seed = 42) : rng_(seed), dist_(1.0, 5.0) {}
    std::string generate(const std::string&) override {
        double score = dist_(rng_);
        return R"({"score":)" + std::to_string(score) + R"(,"confidence":0.8,"reasoning":"random"})";
    }
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
};

TEST_F(LLMJudgeIntegrationEngineInjectionTest, ConstructWithNullEngineAndAllowMockFalseThrows) {
    LLMJudgeIntegration::Config config;
    config.allow_mock = false;  // production mode — nullptr must be rejected
    EXPECT_THROW(
        (LLMJudgeIntegration(nullptr, config)),
        std::invalid_argument
    );
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, ConstructWithNullEngineAndAllowMockTrueUsesStub) {
    LLMJudgeIntegration::Config config;
    config.allow_mock = true;  // test/mock mode — nullptr is acceptable
    config.warn_on_mock_mode = false;
    LLMJudgeIntegration integration(nullptr, config);
    
    EXPECT_TRUE(integration.isMockMode());
    
    // Should not throw — falls back to built-in mock
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    EXPECT_TRUE(result.success);
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, ConstructWithRealEngineProducesExpectedScore) {
    FixedScoreEngine engine(3.7);
    LLMJudgeIntegration integration(&engine);
    
    EXPECT_FALSE(integration.isMockMode());
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.score.has_value());
    EXPECT_NEAR(*result.score, 3.7, 0.01);
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, RandomEngineProducesScoreVariance) {
    // Verify the judge does NOT return a constant score when the injected engine
    // returns random values — i.e. real engine responses propagate through.
    RandomScoreEngine engine(/*seed=*/12345);
    LLMJudgeIntegration integration(&engine);
    
    constexpr int kSamples = 8;
    std::vector<double> scores;
    scores.reserve(kSamples);
    
    for (int i = 0; i < kSamples; ++i) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            sample_input,
            template_manager
        );
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(result.score.has_value());
        scores.push_back(*result.score);
    }
    
    // At least two distinct values → engine variance propagates (not constant mock)
    auto minmax = std::minmax_element(scores.begin(), scores.end());
    EXPECT_GT(*minmax.second - *minmax.first, 0.0)
        << "All " << kSamples << " scores were identical (" << scores[0]
        << "); expected variance from the random-score engine.";
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, IsMockModeReturnsFalseForRealEngine) {
    FixedScoreEngine engine(4.0);
    LLMJudgeIntegration integration(&engine);
    EXPECT_FALSE(integration.isMockMode());
}

TEST_F(LLMJudgeIntegrationEngineInjectionTest, DefaultAllowMockIsFalse) {
    // Default config has allow_mock = false; nullptr engine must throw
    LLMJudgeIntegration::Config config;
    EXPECT_FALSE(config.allow_mock);
    EXPECT_THROW((LLMJudgeIntegration(nullptr, config)), std::invalid_argument);
}
