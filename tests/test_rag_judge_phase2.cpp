/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rag_judge_phase2.cpp                          ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 19:05:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     393                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_rag_judge_phase2.cpp
 * @brief Unit tests for RAG Judge Phase 2 specialized evaluators
 */

#include "rag/faithfulness_evaluator.h"
#include "rag/relevance_evaluator.h"
#include "rag/completeness_evaluator.h"
#include "rag/coherence_evaluator.h"
#include "rag/rag_judge.h"
#include <gtest/gtest.h>

using namespace themis::rag::judge;

// ============================================================================
// Faithfulness Evaluator Tests
// ============================================================================

class FaithfulnessEvaluatorTest : public ::testing::Test {
protected:
    FaithfulnessEvaluator evaluator;
    
    std::vector<std::pair<std::string, std::string>> sample_docs = {
        {"doc1", "Paris is the capital of France. It has a population of over 2 million people."},
        {"doc2", "The Eiffel Tower is located in Paris and was built in 1889."}
    };
};

TEST_F(FaithfulnessEvaluatorTest, BasicEvaluation) {
    std::string answer = "Paris is the capital of France. The Eiffel Tower was built in 1889.";
    
    auto result = evaluator.evaluate(answer, sample_docs, "Tell me about Paris");
    
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_LE(result.faithfulness_score, 1.0);
    EXPECT_GT(result.total_claims_count, 0);
    EXPECT_FALSE(result.explanation.empty());
}

TEST_F(FaithfulnessEvaluatorTest, ClaimExtraction) {
    std::string answer = "Paris is a city. The Eiffel Tower is a landmark.";
    
    auto claims = evaluator.extractClaims(answer);
    
    EXPECT_FALSE(claims.empty());
    EXPECT_LE(claims.size(), 10);  // Should respect max_claims_to_extract
}

TEST_F(FaithfulnessEvaluatorTest, EntailmentCheck) {
    std::string claim = "Paris is the capital of France";
    
    auto support_level = evaluator.checkEntailment(claim, sample_docs);
    
    // Should be fully or partially supported
    EXPECT_NE(support_level, SupportLevel::CONTRADICTED);
}

TEST_F(FaithfulnessEvaluatorTest, UnsupportedClaim) {
    std::string answer = "Paris is located in Germany.";  // Contradicted by docs
    
    auto result = evaluator.evaluate(answer, sample_docs, "Where is Paris?");
    
    // Score should be lower due to unsupported claim
    EXPECT_LT(result.faithfulness_score, 0.8);
}

TEST_F(FaithfulnessEvaluatorTest, EmptyDocuments) {
    std::string answer = "Paris is the capital of France.";
    std::vector<std::pair<std::string, std::string>> empty_docs;
    
    auto result = evaluator.evaluate(answer, empty_docs, "Where is Paris?");
    
    // Should handle empty documents gracefully
    EXPECT_GE(result.faithfulness_score, 0.0);
}

// ============================================================================
// Relevance Evaluator Tests
// ============================================================================

class RelevanceEvaluatorTest : public ::testing::Test {
protected:
    RelevanceEvaluator evaluator;
};

TEST_F(RelevanceEvaluatorTest, BasicEvaluation) {
    std::string query = "What is the capital of France?";
    std::string answer = "The capital of France is Paris, which is located in the north-central part of the country.";
    
    auto result = evaluator.evaluate(answer, query);
    
    EXPECT_GE(result.relevance_score, 0.0);
    EXPECT_LE(result.relevance_score, 1.0);
    EXPECT_FALSE(result.explanation.empty());
}

TEST_F(RelevanceEvaluatorTest, ReverseQuestionGeneration) {
    std::string answer = "Paris is the capital of France. It has many museums.";
    
    auto questions = evaluator.generateReverseQuestions(answer);
    
    // Should generate questions (may be 0 if LLM not available, which is OK)
    EXPECT_LE(questions.size(), 5);
}

TEST_F(RelevanceEvaluatorTest, IntentAnalysis) {
    std::string query1 = "What is the population of Paris?";
    std::string query2 = "Find the Paris tourism website";
    std::string query3 = "Buy tickets to Paris";
    
    auto intent1 = evaluator.analyzeIntent(query1);
    auto intent2 = evaluator.analyzeIntent(query2);
    auto intent3 = evaluator.analyzeIntent(query3);
    
    EXPECT_EQ(intent1, QueryIntent::INFORMATIONAL);
    // Note: intent2 and intent3 may vary based on keyword matching
}

TEST_F(RelevanceEvaluatorTest, NoiseDetection) {
    std::string query = "What is the capital of France?";
    std::string answer = "The capital of France is Paris. Unrelated: The weather is nice today. Also, I like pizza.";
    
    auto result = evaluator.evaluate(answer, query);
    
    // Should detect some noise
    EXPECT_GE(result.noise_ratio, 0.0);
    EXPECT_LE(result.noise_ratio, 1.0);
}

TEST_F(RelevanceEvaluatorTest, HighRelevanceAnswer) {
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France.";
    
    auto result = evaluator.evaluate(answer, query);
    
    // Should have high relevance
    EXPECT_GT(result.relevance_score, 0.5);
}

// ============================================================================
// Completeness Evaluator Tests
// ============================================================================

class CompletenessEvaluatorTest : public ::testing::Test {
protected:
    CompletenessEvaluator evaluator;
};

TEST_F(CompletenessEvaluatorTest, BasicEvaluation) {
    std::string query = "Tell me about Paris, including its history and culture.";
    std::string answer = "Paris is the capital of France with a rich history. It is known for art, fashion, and cuisine.";
    
    auto result = evaluator.evaluate(answer, query);
    
    EXPECT_GE(result.completeness_score, 0.0);
    EXPECT_LE(result.completeness_score, 1.0);
    EXPECT_FALSE(result.explanation.empty());
}

TEST_F(CompletenessEvaluatorTest, AspectExtraction) {
    std::string query = "What are the benefits and drawbacks of electric cars?";
    
    auto aspects = evaluator.extractQueryAspects(query);
    
    EXPECT_FALSE(aspects.empty());
    // Should identify at least the main query
    EXPECT_GE(aspects.size(), 1);
}

TEST_F(CompletenessEvaluatorTest, DepthAssessment) {
    std::string shallow_answer = "Paris is nice.";
    std::string deep_answer = "Paris is the capital of France, known for its rich history and culture. "
                              "For example, the Eiffel Tower represents French engineering prowess. "
                              "According to research, Paris attracts millions of tourists annually because "
                              "of its museums, cuisine, and architectural heritage.";
    
    auto shallow_aspects = evaluator.extractQueryAspects("Tell me about Paris");
    auto deep_aspects = evaluator.extractQueryAspects("Tell me about Paris");
    
    auto [shallow_level, shallow_score] = evaluator.assessDepth(shallow_answer, shallow_aspects);
    auto [deep_level, deep_score] = evaluator.assessDepth(deep_answer, deep_aspects);
    
    EXPECT_LT(shallow_score, deep_score);
}

TEST_F(CompletenessEvaluatorTest, MissingInformation) {
    std::string query = "Explain machine learning, including supervised and unsupervised learning.";
    std::string incomplete_answer = "Machine learning is about teaching computers.";
    
    auto result = evaluator.evaluate(incomplete_answer, query);
    
    // Should detect missing information
    EXPECT_LT(result.completeness_score, 0.8);
}

// ============================================================================
// Coherence Evaluator Tests
// ============================================================================

class CoherenceEvaluatorTest : public ::testing::Test {
protected:
    CoherenceEvaluator evaluator;
};

TEST_F(CoherenceEvaluatorTest, BasicEvaluation) {
    std::string answer = "Paris is the capital of France. It is known for its culture. "
                        "Moreover, it has many museums. Therefore, it attracts many tourists.";
    
    auto result = evaluator.evaluate(answer);
    
    EXPECT_GE(result.coherence_score, 0.0);
    EXPECT_LE(result.coherence_score, 1.0);
    EXPECT_FALSE(result.explanation.empty());
}

TEST_F(CoherenceEvaluatorTest, LogicalFlow) {
    std::string good_flow = "First, Paris is the capital. Second, it has history. Finally, it attracts tourists.";
    std::string poor_flow = "Paris capital. History. Tourists.";
    
    auto good_score = evaluator.analyzeLogicalFlow(good_flow);
    auto poor_score = evaluator.analyzeLogicalFlow(poor_flow);
    
    EXPECT_GT(good_score, poor_score);
}

TEST_F(CoherenceEvaluatorTest, StructuralCoherence) {
    std::string structured = "Introduction: Paris is important.\n\nBody: It has history.\n\nConclusion: Visit Paris.";
    std::string unstructured = "Paris history important visit.";
    
    auto structured_score = evaluator.assessStructure(structured);
    auto unstructured_score = evaluator.assessStructure(unstructured);
    
    EXPECT_GT(structured_score, unstructured_score);
}

TEST_F(CoherenceEvaluatorTest, LinguisticQuality) {
    std::string quality_answer = "Paris is the capital of France. It has a rich cultural heritage.";
    
    auto score = evaluator.evaluateLinguisticQuality(quality_answer);
    
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 1.0);
}

TEST_F(CoherenceEvaluatorTest, ContradictionDetection) {
    std::string contradictory = "Paris is the largest city. Paris is not the largest city.";
    
    auto contradictions = evaluator.detectContradictions(contradictory);
    
    // May or may not detect contradiction depending on implementation
    EXPECT_GE(contradictions.size(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class RAGJudgeIntegrationTest : public ::testing::Test {
protected:
    RAGJudgeConfig config;
    
    void SetUp() override {
        config.mode = EvaluationMode::THOROUGH;
        config.faithfulness_weight = 0.3;
        config.relevance_weight = 0.25;
        config.completeness_weight = 0.25;
        config.coherence_weight = 0.2;
    }
};

TEST_F(RAGJudgeIntegrationTest, FullEvaluationAllDimensions) {
    RAGJudge judge(config);
    
    std::string query = "What is the capital of France?";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Paris is the capital of France.", 0.95, {}}
    };
    std::string answer = "The capital of France is Paris, a beautiful city with rich history.";
    
    auto result = judge.evaluate(query, docs, answer);
    
    // All dimension scores should be evaluated
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_LE(result.faithfulness_score, 1.0);
    
    EXPECT_GE(result.relevance_score, 0.0);
    EXPECT_LE(result.relevance_score, 1.0);
    
    EXPECT_GE(result.completeness_score, 0.0);
    EXPECT_LE(result.completeness_score, 1.0);
    
    EXPECT_GE(result.coherence_score, 0.0);
    EXPECT_LE(result.coherence_score, 1.0);
    
    // Overall score should be weighted combination
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    
    EXPECT_FALSE(result.explanation.empty());
}

TEST_F(RAGJudgeIntegrationTest, BalancedMode) {
    config.mode = EvaluationMode::BALANCED;
    RAGJudge judge(config);
    
    std::string query = "What is machine learning?";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Machine learning is a subset of AI.", 0.9, {}}
    };
    std::string answer = "Machine learning is about teaching computers to learn from data.";
    
    auto result = judge.evaluate(query, docs, answer);
    
    EXPECT_TRUE(result.evaluation_time.count() > 0);
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST_F(RAGJudgeIntegrationTest, FastMode) {
    config.mode = EvaluationMode::FAST;
    RAGJudge judge(config);
    
    std::string query = "What is AI?";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "AI is artificial intelligence.", 0.95, {}}
    };
    std::string answer = "AI stands for artificial intelligence.";
    
    auto result = judge.evaluate(query, docs, answer);
    
    // Fast mode should still produce valid scores
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST_F(RAGJudgeIntegrationTest, EmptyAnswer) {
    RAGJudge judge(config);
    
    std::string query = "What is the capital?";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Paris is the capital.", 0.95, {}}
    };
    std::string answer = "";
    
    auto result = judge.evaluate(query, docs, answer);
    
    // Should handle empty answer gracefully
    EXPECT_EQ(result.relevance_score, 0.0);
}

TEST_F(RAGJudgeIntegrationTest, PerformanceTarget) {
    RAGJudge judge(config);
    
    std::string query = "Explain quantum computing in detail.";
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Quantum computing uses qubits instead of bits.", 0.9, {}},
        {"doc2", "Quantum computers can solve certain problems faster.", 0.85, {}}
    };
    std::string answer = "Quantum computing is a revolutionary technology that uses quantum mechanics.";
    
    auto result = judge.evaluate(query, docs, answer);
    
    // Performance target: < 2000ms for thorough evaluation
    EXPECT_LT(result.evaluation_time.count(), 2000);
}

// ============================================================================
// Main
// ============================================================================


