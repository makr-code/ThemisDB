/**
 * @file test_rag_judge_phase1.cpp
 * @brief Unit tests for RAG Judge Phase 1 implementation
 */

#include "rag/rag_judge.h"
#include "rag/judge_config.h"
#include "rag/prompt_templates.h"
#include "rag/response_parser.h"
#include "rag/llm_judge_integration.h"
#include "../test_helpers_llm.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace themis::rag::judge;
using json = nlohmann::json;

// ============================================================================
// Configuration Tests
// ============================================================================

class JudgeConfigTest : public ::testing::Test {
protected:
    JudgeConfigManager config_manager;
};

TEST_F(JudgeConfigTest, LoadFromJSONString) {
    std::string json_config = R"({
        "enabled": true,
        "mode": "balanced",
        "scoring": {
            "faithfulness_weight": 0.4,
            "relevance_weight": 0.3,
            "completeness_weight": 0.2,
            "coherence_weight": 0.1
        },
        "quality_threshold": 0.7,
        "faithfulness_threshold": 0.8
    })";
    
    ASSERT_TRUE(config_manager.loadFromJSONString(json_config));
    
    EXPECT_EQ(config_manager.getString("mode"), "balanced");
    EXPECT_DOUBLE_EQ(config_manager.getDouble("scoring.faithfulness_weight"), 0.4);
    EXPECT_DOUBLE_EQ(config_manager.getDouble("quality_threshold"), 0.7);
    EXPECT_TRUE(config_manager.getBool("enabled"));
}

TEST_F(JudgeConfigTest, ConfigValidation) {
    std::string valid_config = R"({
        "scoring": {
            "faithfulness_weight": 0.4,
            "relevance_weight": 0.3,
            "completeness_weight": 0.2,
            "coherence_weight": 0.1
        },
        "quality_threshold": 0.7
    })";
    
    ASSERT_TRUE(config_manager.loadFromJSONString(valid_config));
    EXPECT_TRUE(config_manager.validate());
}

TEST_F(JudgeConfigTest, RuntimeConfigUpdate) {
    config_manager.updateConfig("quality_threshold", "0.8");
    EXPECT_DOUBLE_EQ(config_manager.getDouble("quality_threshold"), 0.8);
    
    config_manager.updateConfig("enabled", "false");
    EXPECT_FALSE(config_manager.getBool("enabled"));
}

TEST_F(JudgeConfigTest, ToJSON) {
    config_manager.updateConfig("mode", "fast");
    config_manager.updateConfig("enabled", "true");
    
    std::string json_str = config_manager.toJSON();
    EXPECT_FALSE(json_str.empty());
    
    // Parse to verify it's valid JSON
    ASSERT_NO_THROW({
        json j = json::parse(json_str);
        EXPECT_TRUE(j.contains("mode"));
    });
}

// ============================================================================
// Prompt Template Tests
// ============================================================================

class PromptTemplateTest : public ::testing::Test {
protected:
    PromptTemplateManager template_manager;
    EvaluationInput sample_input;
    
    void SetUp() override {
        template_manager = PromptTemplateManager::createDefault();
        
        sample_input.query = "What is the capital of France?";
        sample_input.generated_answer = "The capital of France is Paris.";
        sample_input.documents = {
            {"doc1", "Paris is the capital and most populous city of France.", 0.95, {}}
        };
    }
};

TEST_F(PromptTemplateTest, GenerateFaithfulnessPrompt) {
    std::string prompt = template_manager.generatePrompt(
        EvaluationDimension::FAITHFULNESS,
        sample_input
    );
    
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("faithfulness"), std::string::npos);
    EXPECT_NE(prompt.find(sample_input.query), std::string::npos);
    EXPECT_NE(prompt.find(sample_input.generated_answer), std::string::npos);
}

TEST_F(PromptTemplateTest, GenerateRelevancePrompt) {
    std::string prompt = template_manager.generatePrompt(
        EvaluationDimension::RELEVANCE,
        sample_input
    );
    
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("relevance"), std::string::npos);
    EXPECT_NE(prompt.find(sample_input.query), std::string::npos);
}

TEST_F(PromptTemplateTest, GenerateCompletenessPrompt) {
    std::string prompt = template_manager.generatePrompt(
        EvaluationDimension::COMPLETENESS,
        sample_input
    );
    
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("completeness"), std::string::npos);
}

TEST_F(PromptTemplateTest, GenerateCoherencePrompt) {
    std::string prompt = template_manager.generatePrompt(
        EvaluationDimension::COHERENCE,
        sample_input
    );
    
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("coherence"), std::string::npos);
}

TEST_F(PromptTemplateTest, PromptContainsFewShotExamples) {
    FewShotExample example{
        "Test query",
        "Test context",
        "Test answer",
        5.0,
        "Test explanation"
    };
    
    template_manager.setFewShotExamples(
        EvaluationDimension::FAITHFULNESS,
        {example}
    );
    
    std::string prompt = template_manager.generatePrompt(
        EvaluationDimension::FAITHFULNESS,
        sample_input
    );
    
    EXPECT_NE(prompt.find("Example"), std::string::npos);
}

TEST_F(PromptTemplateTest, CustomTemplateLoading) {
    std::string custom_template = "Custom template: {query} | {answer}";
    template_manager.setTemplate(EvaluationDimension::FAITHFULNESS, custom_template);
    
    std::string result = template_manager.getTemplate(EvaluationDimension::FAITHFULNESS);
    EXPECT_EQ(result, custom_template);
}

// ============================================================================
// Response Parser Tests
// ============================================================================

class ResponseParserTest : public ::testing::Test {};

TEST_F(ResponseParserTest, ParseValidJSON) {
    std::string response = R"({
        "score": 4.5,
        "confidence": 0.9,
        "reasoning": "The answer is well-supported by the context.",
        "supporting_claims": ["Paris is the capital"],
        "unsupported_claims": []
    })";
    
    auto parsed = ResponseParser::parseJSON(response);
    
    ASSERT_TRUE(parsed.success);
    ASSERT_TRUE(parsed.score.has_value());
    EXPECT_DOUBLE_EQ(*parsed.score, 4.5);
    EXPECT_DOUBLE_EQ(*parsed.confidence, 0.9);
    EXPECT_FALSE(parsed.reasoning.empty());
    EXPECT_EQ(parsed.supporting_claims.size(), 1);
}

TEST_F(ResponseParserTest, ParseJSONWithTextAround) {
    std::string response = R"(Here is my evaluation:
    {
        "score": 3.0,
        "reasoning": "Partially supported"
    }
    That's my assessment.)";
    
    auto parsed = ResponseParser::parseJSON(response);
    
    ASSERT_TRUE(parsed.success);
    ASSERT_TRUE(parsed.score.has_value());
    EXPECT_DOUBLE_EQ(*parsed.score, 3.0);
}

TEST_F(ResponseParserTest, ParseInvalidJSONFallbackToRegex) {
    std::string response = "Score: 4.5 out of 5\nReasoning: Good answer";
    
    auto parsed = ResponseParser::parse(response);
    
    ASSERT_TRUE(parsed.success);
    ASSERT_TRUE(parsed.score.has_value());
    // Should be normalized from 4.5/5 scale
}

TEST_F(ResponseParserTest, NormalizeScoreDifferentRanges) {
    // 1-5 scale to 0-1
    EXPECT_DOUBLE_EQ(ResponseParser::normalizeScore(5.0, 1.0, 5.0), 1.0);
    EXPECT_DOUBLE_EQ(ResponseParser::normalizeScore(1.0, 1.0, 5.0), 0.0);
    EXPECT_DOUBLE_EQ(ResponseParser::normalizeScore(3.0, 1.0, 5.0), 0.5);
    
    // 0-100 scale to 0-1
    EXPECT_DOUBLE_EQ(ResponseParser::normalizeScore(100.0, 0.0, 100.0), 1.0);
    EXPECT_DOUBLE_EQ(ResponseParser::normalizeScore(50.0, 0.0, 100.0), 0.5);
}

TEST_F(ResponseParserTest, ExtractScoreVariousFormats) {
    EXPECT_TRUE(ResponseParser::extractScore("score: 4.5").has_value());
    EXPECT_TRUE(ResponseParser::extractScore("rating: 85%").has_value());
    EXPECT_TRUE(ResponseParser::extractScore("4 out of 5").has_value());
    EXPECT_TRUE(ResponseParser::extractScore("3.5/5").has_value());
}

TEST_F(ResponseParserTest, ExtractExplanation) {
    std::string response = "reasoning: This is a detailed explanation of the score.";
    std::string explanation = ResponseParser::extractExplanation(response);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("explanation"), std::string::npos);
}

TEST_F(ResponseParserTest, ValidateSchema) {
    json valid_json = {
        {"score", 4.5},
        {"reasoning", "Good answer"}
    };
    EXPECT_TRUE(ResponseParser::validateSchema(valid_json));
    
    json invalid_json = {
        {"reasoning", "Good answer"}
        // Missing score
    };
    EXPECT_FALSE(ResponseParser::validateSchema(invalid_json));
}

// ============================================================================
// LLM Integration Tests (with mocking)
// ============================================================================

class LLMIntegrationTest : public ::testing::Test {
protected:
    LLMJudgeIntegration integration;
    PromptTemplateManager template_manager;
    EvaluationInput sample_input;
    
    void SetUp() override {
        template_manager = PromptTemplateManager::createDefault();
        
        sample_input.query = "What is the capital of France?";
        sample_input.generated_answer = "Paris is the capital of France.";
        sample_input.documents = {
            {"doc1", "Paris is the capital of France.", 0.95, {}}
        };
        
        // Set mock inference function
        integration.setInferenceFunction([](const std::string& /*prompt*/) {
            return R"({
                "score": 5.0,
                "confidence": 0.95,
                "reasoning": "Fully supported by context",
                "supporting_claims": ["Paris is the capital of France"],
                "unsupported_claims": []
            })";
        });
    }
};

TEST_F(LLMIntegrationTest, EvaluateWithMockedLLM) {
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        sample_input,
        template_manager
    );
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.score.has_value());
    EXPECT_DOUBLE_EQ(*result.score, 5.0);
    EXPECT_DOUBLE_EQ(*result.confidence, 0.95);
}

TEST_F(LLMIntegrationTest, ConfigurationUpdate) {
    LLMJudgeIntegration::Config config;
    config.model_name = "test-model";
    config.temperature = 0.5;
    config.max_retries = 5;
    
    integration.setConfig(config);
    
    auto retrieved_config = integration.getConfig();
    EXPECT_EQ(retrieved_config.model_name, "test-model");
    EXPECT_DOUBLE_EQ(retrieved_config.temperature, 0.5);
    EXPECT_EQ(retrieved_config.max_retries, 5);
}

// ============================================================================
// Integration Test: End-to-End
// ============================================================================

class RAGJudgeIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<RAGJudge> judge;
    
    void SetUp() override {
        RAGJudgeConfig config;
        config.mode = EvaluationMode::BALANCED;
        config.use_chain_of_thought = true;
        config.enable_claim_verification = false; // Disable for basic tests
        
        judge = std::make_unique<RAGJudge>(config);
    }
};

TEST_F(RAGJudgeIntegrationTest, BasicEvaluation) {
    std::string query = "What is the capital of France?";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Paris is the capital of France.", 0.95, {}}
    };
    std::string answer = "The capital of France is Paris.";
    
    auto result = judge->evaluate(query, docs, answer);
    const bool has_real_models = themis::test::hasRealModels();

    if (result.explanation.empty()) {
        if (has_real_models) {
            FAIL() << "Lokale Modelle sind verfuegbar, aber RAGJudge lieferte keine Erklaerung. "
                   << "Bitte LLM-Testkonfiguration/Plugin-Verdrahtung pruefen.";
        }
        GTEST_SKIP() << "No LLM model available for RAG judge integration in this environment.";
    }
    
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_GE(result.relevance_score, 0.0);
    EXPECT_FALSE(result.explanation.empty());
}

TEST_F(RAGJudgeIntegrationTest, EmptyDocumentsLowFaithfulness) {
    std::string query = "What is the capital of France?";
    std::vector<RetrievedDocument> docs; // Empty
    std::string answer = "The capital of France is Paris.";
    
    auto result = judge->evaluate(query, docs, answer);
    
    // Should have low faithfulness with no supporting documents
    EXPECT_LT(result.faithfulness_score, 0.5);
}

TEST_F(RAGJudgeIntegrationTest, CacheEvaluation) {
    std::string query = "Test query";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Test content", 0.9, {}}
    };
    std::string answer = "Test answer";
    
    // First evaluation
    auto result1 = judge->evaluate(query, docs, answer);
    
    // Second evaluation (should hit cache)
    auto result2 = judge->evaluate(query, docs, answer);
    
    // Results should be identical (from cache)
    EXPECT_DOUBLE_EQ(result1.overall_score, result2.overall_score);
}

TEST_F(RAGJudgeIntegrationTest, PairwiseComparison) {
    std::string query = "What is AI?";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "AI is artificial intelligence.", 0.9, {}}
    };
    std::string answer_a = "AI stands for artificial intelligence.";
    std::string answer_b = "AI is about robots.";
    
    auto comparison = judge->compare(query, docs, answer_a, answer_b);
    
    // answer_a should be better as it's more accurate
    EXPECT_NE(comparison.winner, ComparisonResult::Winner::ANSWER_B);
}

// ============================================================================
// Factory Tests
// ============================================================================

TEST(RAGJudgeFactoryTest, CreateFastMode) {
    auto judge = RAGJudgeFactory::createFast();
    ASSERT_NE(judge, nullptr);
    
    auto config = judge->getConfig();
    EXPECT_EQ(config.mode, EvaluationMode::FAST);
    EXPECT_FALSE(config.enable_claim_verification);
}

TEST(RAGJudgeFactoryTest, CreateBalancedMode) {
    auto judge = RAGJudgeFactory::createBalanced();
    ASSERT_NE(judge, nullptr);
    
    auto config = judge->getConfig();
    EXPECT_EQ(config.mode, EvaluationMode::BALANCED);
}

TEST(RAGJudgeFactoryTest, CreateThoroughMode) {
    auto judge = RAGJudgeFactory::createThorough();
    ASSERT_NE(judge, nullptr);
    
    auto config = judge->getConfig();
    EXPECT_EQ(config.mode, EvaluationMode::THOROUGH);
    EXPECT_TRUE(config.enable_claim_verification);
    EXPECT_TRUE(config.use_chain_of_thought);
}

TEST(RAGJudgeFactoryTest, CreateEnsemble) {
    auto ensemble = RAGJudgeFactory::createEnsemble(3, VotingStrategy::WEIGHTED_AVERAGE);
    ASSERT_NE(ensemble, nullptr);
}


