/**
 * @file test_llm_integration.cpp
 * @brief Unit tests for LLM Integration utilities
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include "rag/llm_integration.h"

using namespace themis::rag;

class LLMIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test data
    }
};

// ============================================================================
// PromptTemplate Tests
// ============================================================================

TEST_F(LLMIntegrationTest, PromptTemplateFormatBasic) {
    PromptTemplate tmpl;
    tmpl.user_template = "Query: {query}\nAnswer: {answer}";
    
    std::unordered_map<std::string, std::string> vars;
    vars["query"] = "What is AI?";
    vars["answer"] = "Artificial Intelligence";
    
    std::string formatted = tmpl.format(vars);
    
    EXPECT_TRUE(formatted.find("What is AI?") != std::string::npos);
    EXPECT_TRUE(formatted.find("Artificial Intelligence") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptTemplateWithSystemPrompt) {
    PromptTemplate tmpl;
    tmpl.system_prompt = "You are a helpful assistant.";
    tmpl.user_template = "Question: {question}";
    
    std::unordered_map<std::string, std::string> vars;
    vars["question"] = "Hello";
    
    std::string formatted = tmpl.format(vars);
    
    EXPECT_TRUE(formatted.find("You are a helpful assistant") != std::string::npos);
    EXPECT_TRUE(formatted.find("Hello") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptTemplateWithFewShot) {
    PromptTemplate tmpl;
    tmpl.few_shot_examples = "Example 1: Q: Hi A: Hello";
    tmpl.user_template = "Q: {q}";
    
    std::unordered_map<std::string, std::string> vars;
    vars["q"] = "Test";
    
    std::string formatted = tmpl.format(vars);
    
    EXPECT_TRUE(formatted.find("Example 1") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptTemplateWithOutputFormat) {
    PromptTemplate tmpl;
    tmpl.user_template = "Evaluate: {text}";
    tmpl.output_format_instruction = "Output as JSON";
    
    std::unordered_map<std::string, std::string> vars;
    vars["text"] = "Sample";
    
    std::string formatted = tmpl.format(vars);
    
    EXPECT_TRUE(formatted.find("Output as JSON") != std::string::npos);
}

// ============================================================================
// LLMIntegration Tests
// ============================================================================

TEST_F(LLMIntegrationTest, GenerateBasic) {
    // With no engine configured, generate() fails fast (no silent stub)
    LLMIntegration::setInferenceEngine(nullptr);
    LLMGenerationOptions options;
    options.temperature = 0.7;
    options.max_tokens = 100;
    
    EXPECT_THROW(LLMIntegration::generate("Test prompt", options), std::runtime_error);
}

TEST_F(LLMIntegrationTest, GenerateMultipleSamples) {
    // With no engine configured, each generate() call throws — so generateMultipleSamples throws too
    LLMIntegration::setInferenceEngine(nullptr);
    LLMGenerationOptions options;
    
    EXPECT_THROW(LLMIntegration::generateMultipleSamples("Test", 3, options), std::runtime_error);
}

TEST_F(LLMIntegrationTest, ParseEvaluationResponseJSON) {
    std::string response = R"({"score": 0.85, "confidence": 0.9, "explanation": "Good"})";
    
    auto parsed = LLMIntegration::parseEvaluationResponse(response);
    
    EXPECT_TRUE(parsed.parse_successful);
    EXPECT_DOUBLE_EQ(parsed.score, 0.85);
    EXPECT_DOUBLE_EQ(parsed.confidence, 0.9);
}

TEST_F(LLMIntegrationTest, ParseEvaluationResponseMalformed) {
    std::string response = "Invalid JSON";
    
    auto parsed = LLMIntegration::parseEvaluationResponse(response);
    
    EXPECT_FALSE(parsed.parse_successful);
    EXPECT_DOUBLE_EQ(parsed.score, 0.5); // Default score
}

TEST_F(LLMIntegrationTest, CalculatePerplexity) {
    std::vector<double> probs = {0.9, 0.8, 0.85, 0.7};
    
    double perplexity = LLMIntegration::calculatePerplexity(probs);
    
    EXPECT_GT(perplexity, 0.0);
    EXPECT_LT(perplexity, 100.0); // Reasonable range
}

TEST_F(LLMIntegrationTest, CalculatePerplexityEmpty) {
    std::vector<double> probs;
    
    double perplexity = LLMIntegration::calculatePerplexity(probs);
    
    EXPECT_DOUBLE_EQ(perplexity, 0.0);
}

TEST_F(LLMIntegrationTest, CalculateSemanticSimilarity) {
    std::string text1 = "This is a test.";
    std::string text2 = "This is also a test.";
    
    double similarity = LLMIntegration::calculateSemanticSimilarity(text1, text2);
    
    EXPECT_GE(similarity, 0.0);
    EXPECT_LE(similarity, 1.0);
}

TEST_F(LLMIntegrationTest, CalculateSemanticSimilarityEmpty) {
    double similarity = LLMIntegration::calculateSemanticSimilarity("", "text");
    
    EXPECT_DOUBLE_EQ(similarity, 0.0);
}

// ============================================================================
// PromptLibrary Tests
// ============================================================================

TEST_F(LLMIntegrationTest, PromptLibraryConfidenceEvaluation) {
    auto tmpl = PromptLibrary::getConfidenceEvaluationPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_FALSE(tmpl.user_template.empty());
    EXPECT_TRUE(tmpl.user_template.find("{query}") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryClaimVerification) {
    auto tmpl = PromptLibrary::getClaimVerificationPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_TRUE(tmpl.user_template.find("{claim}") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryConsistencyCheck) {
    auto tmpl = PromptLibrary::getConsistencyCheckPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_TRUE(tmpl.user_template.find("{response1}") != std::string::npos);
    EXPECT_TRUE(tmpl.user_template.find("{response2}") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryFaithfulness) {
    auto tmpl = PromptLibrary::getFaithfulnessEvaluationPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_FALSE(tmpl.few_shot_examples.empty());
    EXPECT_TRUE(tmpl.user_template.find("{answer}") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryRelevance) {
    auto tmpl = PromptLibrary::getRelevanceEvaluationPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_TRUE(tmpl.user_template.find("{query}") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryCompleteness) {
    auto tmpl = PromptLibrary::getCompletenessEvaluationPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_TRUE(tmpl.output_format_instruction.find("missing_aspects") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryCoherence) {
    auto tmpl = PromptLibrary::getCoherenceEvaluationPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_TRUE(tmpl.user_template.find("{answer}") != std::string::npos);
}

TEST_F(LLMIntegrationTest, PromptLibraryPairwiseComparison) {
    auto tmpl = PromptLibrary::getPairwiseComparisonPrompt();
    
    EXPECT_FALSE(tmpl.system_prompt.empty());
    EXPECT_TRUE(tmpl.user_template.find("{answer_a}") != std::string::npos);
    EXPECT_TRUE(tmpl.user_template.find("{answer_b}") != std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(LLMIntegrationTest, EndToEndPromptFormatting) {
    auto tmpl = PromptLibrary::getFaithfulnessEvaluationPrompt();
    
    std::unordered_map<std::string, std::string> vars;
    vars["query"] = "What is the capital of France?";
    vars["documents"] = "Document 1: Paris is the capital of France.";
    vars["answer"] = "The capital of France is Paris.";
    
    std::string formatted = tmpl.format(vars);
    
    EXPECT_TRUE(formatted.find("What is the capital of France?") != std::string::npos);
    EXPECT_TRUE(formatted.find("Paris is the capital") != std::string::npos);
    EXPECT_FALSE(formatted.find("{query}") != std::string::npos); // Variables replaced
}

TEST_F(LLMIntegrationTest, TokenProbabilityCallback) {
    bool callback_invoked = false;
    TokenProbability received_token;
    
    LLMGenerationOptions options;
    options.include_token_probabilities = true;
    options.token_callback = [&](const TokenProbability& tp) {
        callback_invoked = true;
        received_token = tp;
    };
    
    // Note: Callback won't be invoked until actual LLM integration
    // This test documents the API
    EXPECT_TRUE(options.token_callback != nullptr);
}

// ============================================================================
// Phase 2b: Inference Engine Integration Tests
// ============================================================================

TEST_F(LLMIntegrationTest, SetAndGetInferenceEngine) {
    // Test that we can set and retrieve the inference engine
    auto engine = LLMIntegration::getInferenceEngine();
    // Initially should be null or previous value
    
    // Test that setInferenceEngine doesn't crash
    LLMIntegration::setInferenceEngine(nullptr);
    EXPECT_EQ(LLMIntegration::getInferenceEngine(), nullptr);
}

TEST_F(LLMIntegrationTest, GenerateWithoutEngine) {
    // Ensure we set engine to null for this test
    LLMIntegration::setInferenceEngine(nullptr);
    
    LLMGenerationOptions options;
    // With no engine configured, generate() must fail fast (throw std::runtime_error)
    // rather than silently returning a placeholder stub.
    EXPECT_THROW(LLMIntegration::generate("Test prompt", options), std::runtime_error);
}

TEST_F(LLMIntegrationTest, ImprovedSemanticSimilarity) {
    // Test the improved semantic similarity calculation
    
    // Identical texts should have high similarity
    std::string text1 = "The quick brown fox jumps over the lazy dog";
    double sim1 = LLMIntegration::calculateSemanticSimilarity(text1, text1);
    EXPECT_GT(sim1, 0.9);
    
    // Similar texts should have moderate similarity
    std::string text2 = "The quick brown fox jumps over the dog";
    double sim2 = LLMIntegration::calculateSemanticSimilarity(text1, text2);
    EXPECT_GT(sim2, 0.7);
    EXPECT_LT(sim2, 1.0);
    
    // Different texts should have low similarity
    std::string text3 = "Artificial intelligence and machine learning";
    double sim3 = LLMIntegration::calculateSemanticSimilarity(text1, text3);
    EXPECT_LT(sim3, 0.3);
    
    // Empty text edge cases
    EXPECT_DOUBLE_EQ(LLMIntegration::calculateSemanticSimilarity("", text1), 0.0);
    EXPECT_DOUBLE_EQ(LLMIntegration::calculateSemanticSimilarity(text1, ""), 0.0);
}

TEST_F(LLMIntegrationTest, MultipleSamplesWithSeeds) {
    LLMIntegration::setInferenceEngine(nullptr);
    
    LLMGenerationOptions options;
    options.seeds = {123, 456, 789};
    options.temperature = 0.7;
    
    // With no engine configured, generateMultipleSamples throws on the first sample
    EXPECT_THROW(LLMIntegration::generateMultipleSamples("Test", 3, options), std::runtime_error);
}

TEST_F(LLMIntegrationTest, SemanticSimilarityLengthNormalization) {
    // Test that length differences are handled appropriately
    std::string short_text = "AI ML";
    std::string long_text = "AI ML deep learning neural networks data science";
    
    double similarity = LLMIntegration::calculateSemanticSimilarity(short_text, long_text);
    
    // Should have some similarity due to common words but penalized for length difference
    EXPECT_GT(similarity, 0.3);
    EXPECT_LT(similarity, 0.8);
}


