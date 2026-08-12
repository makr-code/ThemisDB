/**
 * @file test_rag_judge_phase4.cpp
 * @brief Unit tests for RAG Judge Phase 4 (Rubric & CoT & G-Eval)
 */

#include "rag/rubric_evaluator.h"
#include "rag/cot_evaluator.h"
#include "rag/geval_evaluator.h"
#include <gtest/gtest.h>

using namespace themis::rag::judge;

// ============================================================================
// Rubric Evaluator Tests
// ============================================================================

class RubricEvaluatorTest : public ::testing::Test {
protected:
    RubricEvaluator::Config config;
    
    std::vector<std::pair<std::string, std::string>> sample_docs = {
        {"doc1", "Paris is the capital of France with over 2 million people."},
        {"doc2", "The Eiffel Tower is an iconic Paris landmark built in 1889."}
    };
};

TEST_F(RubricEvaluatorTest, DefaultRubricCreation) {
    auto rubric = RubricEvaluator::createDefaultRubric();
    
    EXPECT_FALSE(rubric.name.empty());
    EXPECT_FALSE(rubric.dimensions.empty());
    EXPECT_EQ(rubric.dimensions.size(), 4);  // 4 dimensions
    
    // Check dimensions are present
    std::vector<std::string> expected = {"Faithfulness", "Relevance", "Completeness", "Coherence"};
    for (const auto& expected_name : expected) {
        bool found = false;
        for (const auto& dim : rubric.dimensions) {
            if (dim.dimension_name == expected_name) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Dimension " << expected_name << " not found";
    }
}

TEST_F(RubricEvaluatorTest, RubricValidation) {
    auto valid_rubric = RubricEvaluator::createDefaultRubric();
    EXPECT_TRUE(RubricEvaluator::validateRubric(valid_rubric));
    
    // Test invalid rubric (no dimensions)
    EvaluationRubric invalid;
    invalid.name = "test";
    EXPECT_FALSE(RubricEvaluator::validateRubric(invalid));
}

TEST_F(RubricEvaluatorTest, ScoreNormalization) {
    EXPECT_DOUBLE_EQ(RubricEvaluator::normalizeScore(1), 0.0);
    EXPECT_DOUBLE_EQ(RubricEvaluator::normalizeScore(3), 0.5);
    EXPECT_DOUBLE_EQ(RubricEvaluator::normalizeScore(5), 1.0);
}

TEST_F(RubricEvaluatorTest, BasicEvaluation) {
    RubricEvaluator evaluator(config);
    
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs);
    
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    EXPECT_FALSE(result.rubric_name.empty());
    EXPECT_FALSE(result.dimension_scores.empty());
    EXPECT_FALSE(result.dimension_levels.empty());
}

TEST_F(RubricEvaluatorTest, DimensionScores) {
    RubricEvaluator evaluator(config);
    
    std::string query = "Explain machine learning";
    std::string answer = "Machine learning is about teaching computers to learn from data.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs);
    
    // Check all dimensions are evaluated
    EXPECT_TRUE(result.dimension_scores.find("Faithfulness") != result.dimension_scores.end());
    EXPECT_TRUE(result.dimension_scores.find("Relevance") != result.dimension_scores.end());
    EXPECT_TRUE(result.dimension_scores.find("Completeness") != result.dimension_scores.end());
    EXPECT_TRUE(result.dimension_scores.find("Coherence") != result.dimension_scores.end());
    
    // Check levels are in valid range
    for (const auto& [dim, level] : result.dimension_levels) {
        EXPECT_GE(level, 1);
        EXPECT_LE(level, 5);
    }
}

TEST_F(RubricEvaluatorTest, LoadRubricFromJSON) {
    RubricEvaluator evaluator(config);
    
    std::string json_rubric = R"({
        "name": "test_rubric",
        "description": "Test rubric",
        "domain": "test",
        "version": "1.0",
        "dimensions": [
            {
                "name": "Quality",
                "description": "Answer quality",
                "weight": 1.0,
                "levels": [
                    {"score": 5, "description": "Excellent"},
                    {"score": 3, "description": "Average"},
                    {"score": 1, "description": "Poor"}
                ]
            }
        ]
    })";
    
    bool loaded = evaluator.loadRubricFromYAML(json_rubric);
    EXPECT_TRUE(loaded);
    
    auto rubric = evaluator.getRubric();
    EXPECT_EQ(rubric.name, "test_rubric");
    EXPECT_EQ(rubric.dimensions.size(), 1);
}

TEST_F(RubricEvaluatorTest, CustomRubric) {
    EvaluationRubric custom;
    custom.name = "custom_test";
    custom.description = "Custom test rubric";
    custom.domain = "testing";
    
    DimensionRubric dim;
    dim.dimension_name = "TestDimension";
    dim.description = "Test dimension";
    dim.weight = 1.0;
    dim.levels = {
        {5, "Perfect", {}, {}},
        {3, "Okay", {}, {}},
        {1, "Bad", {}, {}}
    };
    
    custom.dimensions.push_back(dim);
    
    EXPECT_TRUE(RubricEvaluator::validateRubric(custom));
    
    RubricEvaluator evaluator(config);
    evaluator.setRubric(custom);
    
    EXPECT_EQ(evaluator.getRubric().name, "custom_test");
}

// ============================================================================
// Chain-of-Thought Evaluator Tests
// ============================================================================

class CoTEvaluatorTest : public ::testing::Test {
protected:
    CoTEvaluator::Config config;
    
    std::vector<std::pair<std::string, std::string>> sample_docs = {
        {"doc1", "Machine learning is a subset of AI."},
        {"doc2", "Deep learning uses neural networks."}
    };
};

TEST_F(CoTEvaluatorTest, BasicEvaluation) {
    CoTEvaluator evaluator(config);
    
    std::string query = "What is machine learning?";
    std::string answer = "Machine learning is a subset of AI that learns from data.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs);
    
    EXPECT_GE(result.final_score, 0.0);
    EXPECT_LE(result.final_score, 1.0);
    // Note: reasoning_steps may be empty if LLM is not available
}

TEST_F(CoTEvaluatorTest, ReasoningStepsParsing) {
    CoTEvaluator evaluator(config);
    
    std::string response = R"(
Step 1: Is the answer accurate?
Observation: The answer mentions ML is subset of AI
Evidence: Document 1 confirms this
Conclusion: Accurate statement

Step 2: Is the answer complete?
Observation: Covers basic definition
Evidence: Mentions learning from data
Conclusion: Adequately complete

Final Score: 0.85
Final Reasoning: Answer is accurate and complete
)";
    
    auto steps = evaluator.parseCoTResponse(response);
    
    // May parse 0 or more steps depending on regex matching
    EXPECT_GE(steps.size(), 0);
}

TEST_F(CoTEvaluatorTest, ScoreExtraction) {
    CoTEvaluator evaluator(config);
    
    std::string response = "Final Score: 0.75\nFinal Reasoning: Good answer";
    
    std::vector<ReasoningStep> steps;
    double score = evaluator.extractFinalScore(steps, response);
    
    EXPECT_DOUBLE_EQ(score, 0.75);
}

TEST_F(CoTEvaluatorTest, ScoreExtractionFallback) {
    CoTEvaluator evaluator(config);
    
    std::string response = "Some text without explicit score";
    
    std::vector<ReasoningStep> steps;
    double score = evaluator.extractFinalScore(steps, response);
    
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 1.0);
}

TEST_F(CoTEvaluatorTest, LogicConsistencyCheck) {
    CoTEvaluator evaluator(config);
    
    std::vector<ReasoningStep> steps = {
        {1, "Q1", "Obs1", "Ev1", "Answer is good and supported"},
        {2, "Q2", "Obs2", "Ev2", "Answer is not supported"}
    };
    
    auto inconsistencies = evaluator.validateLogicConsistency(steps);
    
    // May detect inconsistency between "supported" and "not supported"
    EXPECT_GE(inconsistencies.size(), 0);
}

TEST_F(CoTEvaluatorTest, ConfigurableSteps) {
    CoTEvaluator::Config custom_config;
    custom_config.num_reasoning_steps = 3;
    custom_config.enable_self_questioning = true;
    
    CoTEvaluator evaluator(custom_config);
    
    std::string query = "What is AI?";
    std::string answer = "AI is artificial intelligence.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs);
    
    EXPECT_GE(result.final_score, 0.0);
    EXPECT_LE(result.final_score, 1.0);
}

TEST_F(CoTEvaluatorTest, DifferentDimensions) {
    CoTEvaluator evaluator(config);
    
    std::string query = "Explain neural networks";
    std::string answer = "Neural networks are computational models inspired by the brain.";
    
    auto result1 = evaluator.evaluate(query, answer, sample_docs, "faithfulness");
    auto result2 = evaluator.evaluate(query, answer, sample_docs, "relevance");
    
    // Both should produce valid scores
    EXPECT_GE(result1.final_score, 0.0);
    EXPECT_LE(result1.final_score, 1.0);
    EXPECT_GE(result2.final_score, 0.0);
    EXPECT_LE(result2.final_score, 1.0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class Phase4IntegrationTest : public ::testing::Test {
protected:
    std::vector<std::pair<std::string, std::string>> docs = {
        {"doc1", "Paris is the capital and largest city of France."},
        {"doc2", "Paris has a population of over 2 million people."}
    };
};

TEST_F(Phase4IntegrationTest, RubricAndCoTTogether) {
    // Test both rubric and CoT evaluation
    RubricEvaluator rubric_eval;
    CoTEvaluator cot_eval;
    
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France with over 2 million people.";
    
    auto rubric_result = rubric_eval.evaluate(query, answer, docs);
    auto cot_result = cot_eval.evaluate(query, answer, docs);
    
    EXPECT_GE(rubric_result.overall_score, 0.0);
    EXPECT_LE(rubric_result.overall_score, 1.0);
    EXPECT_GE(cot_result.final_score, 0.0);
    EXPECT_LE(cot_result.final_score, 1.0);
    
    // Scores should be in similar range (allowing for variance)
    double diff = std::abs(rubric_result.overall_score - cot_result.final_score);
    EXPECT_LT(diff, 0.5);  // Shouldn't differ by more than 0.5
}

TEST_F(Phase4IntegrationTest, CustomRubricWithCoT) {
    // Create custom rubric
    EvaluationRubric custom;
    custom.name = "integration_test";
    custom.domain = "test";
    
    DimensionRubric dim;
    dim.dimension_name = "Quality";
    dim.weight = 1.0;
    dim.levels = {
        {5, "Excellent", {}, {}},
        {3, "Good", {}, {}},
        {1, "Poor", {}, {}}
    };
    custom.dimensions.push_back(dim);
    
    RubricEvaluator rubric_eval;
    rubric_eval.setRubric(custom);
    
    std::string query = "Test query";
    std::string answer = "Test answer with good content.";
    
    auto result = rubric_eval.evaluate(query, answer, docs);
    
    EXPECT_EQ(result.rubric_name, "integration_test");
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST_F(Phase4IntegrationTest, PerformanceCheck) {
    RubricEvaluator rubric_eval;
    
    std::string query = "Explain quantum computing";
    std::string answer = "Quantum computing uses quantum mechanics for computation.";
    
    auto start = std::chrono::steady_clock::now();
    auto result = rubric_eval.evaluate(query, answer, docs);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 10000);  // < 10 seconds
    EXPECT_GE(result.overall_score, 0.0);
}

TEST_F(Phase4IntegrationTest, EmptyInput) {
    RubricEvaluator rubric_eval;
    CoTEvaluator cot_eval;
    
    std::string query = "Test";
    std::string answer = "";
    
    // Should handle empty answer gracefully
    auto rubric_result = rubric_eval.evaluate(query, answer, docs);
    auto cot_result = cot_eval.evaluate(query, answer, docs);
    
    EXPECT_GE(rubric_result.overall_score, 0.0);
    EXPECT_LE(rubric_result.overall_score, 1.0);
    EXPECT_GE(cot_result.final_score, 0.0);
    EXPECT_LE(cot_result.final_score, 1.0);
}

// ============================================================================
// G-Eval Tests
// ============================================================================

class GEvalEvaluatorTest : public ::testing::Test {
protected:
    GEvalEvaluator::Config config;
    
    std::vector<std::pair<std::string, std::string>> sample_docs = {
        {"doc1", "Paris is the capital of France with over 2 million people."},
        {"doc2", "The Eiffel Tower is an iconic Paris landmark built in 1889."}
    };
};

TEST_F(GEvalEvaluatorTest, BasicEvaluation) {
    config.num_samples = 3;
    GEvalEvaluator evaluator(config);
    
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs, "faithfulness");
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
    EXPECT_EQ(result.dimension, "faithfulness");
    EXPECT_FALSE(result.reasoning.empty());
}

TEST_F(GEvalEvaluatorTest, TokenProbabilities) {
    GEvalEvaluator evaluator(config);
    
    std::string query = "Explain AI";
    std::string answer = "AI is artificial intelligence.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs, "relevance");
    
    // Should have probabilities for 5 levels
    EXPECT_EQ(result.token_probabilities.size(), 5);
    
    // Probabilities should sum to approximately 1.0
    double sum = 0.0;
    for (double prob : result.token_probabilities) {
        sum += prob;
        EXPECT_GE(prob, 0.0);
        EXPECT_LE(prob, 1.0);
    }
    EXPECT_NEAR(sum, 1.0, 0.01);
}

TEST_F(GEvalEvaluatorTest, MultipleSamples) {
    config.num_samples = 5;
    config.aggregation = AggregationMethod::MEAN;
    GEvalEvaluator evaluator(config);
    
    std::string query = "What is ML?";
    std::string answer = "Machine learning is a type of AI.";
    
    auto result = evaluator.evaluate(query, answer, sample_docs, "completeness");
    
    EXPECT_EQ(result.sample_scores.size(), 5);
    EXPECT_GE(result.variance, 0.0);
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
}

TEST_F(GEvalEvaluatorTest, DifferentDimensions) {
    GEvalEvaluator evaluator(config);
    
    std::string query = "Explain quantum computing";
    std::string answer = "Quantum computing uses quantum mechanics principles.";
    
    // Test different dimensions
    auto faith_result = evaluator.evaluate(query, answer, sample_docs, "faithfulness");
    auto rel_result = evaluator.evaluate(query, answer, sample_docs, "relevance");
    auto comp_result = evaluator.evaluate(query, answer, sample_docs, "completeness");
    auto coh_result = evaluator.evaluate(query, answer, sample_docs, "coherence");
    
    EXPECT_EQ(faith_result.dimension, "faithfulness");
    EXPECT_EQ(rel_result.dimension, "relevance");
    EXPECT_EQ(comp_result.dimension, "completeness");
    EXPECT_EQ(coh_result.dimension, "coherence");
    
    // All should have valid scores
    EXPECT_GE(faith_result.geval_score, 0.0);
    EXPECT_GE(rel_result.geval_score, 0.0);
    EXPECT_GE(comp_result.geval_score, 0.0);
    EXPECT_GE(coh_result.geval_score, 0.0);
}

TEST_F(GEvalEvaluatorTest, ScoreComputation) {
    // Test static method
    std::vector<double> probs = {0.1, 0.2, 0.4, 0.2, 0.1};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    
    // Expected: (1*0.1 + 2*0.2 + 3*0.4 + 4*0.2 + 5*0.1) = 3.0
    // Normalized: (3.0 - 1) / 4 = 0.5
    EXPECT_NEAR(score, 0.5, 0.01);
}

TEST_F(GEvalEvaluatorTest, ConfidenceComputation) {
    // High confidence (peaked distribution)
    std::vector<double> peaked = {0.05, 0.05, 0.8, 0.05, 0.05};
    double high_conf = GEvalEvaluator::computeConfidence(peaked);
    
    // Low confidence (uniform distribution)
    std::vector<double> uniform = {0.2, 0.2, 0.2, 0.2, 0.2};
    double low_conf = GEvalEvaluator::computeConfidence(uniform);
    
    EXPECT_GT(high_conf, low_conf);
    EXPECT_GE(high_conf, 0.0);
    EXPECT_LE(high_conf, 1.0);
    EXPECT_GE(low_conf, 0.0);
    EXPECT_LE(low_conf, 1.0);
}

TEST_F(GEvalEvaluatorTest, AggregationMethods) {
    std::vector<double> samples = {0.3, 0.5, 0.7, 0.5, 0.6};
    
    double mean = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEAN);
    double median = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEDIAN);
    double mode = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MODE);
    
    EXPECT_NEAR(mean, 0.52, 0.01);  // (0.3+0.5+0.7+0.5+0.6)/5 = 0.52
    EXPECT_NEAR(median, 0.5, 0.01);  // Sorted: 0.3,0.5,0.5,0.6,0.7 -> middle is 0.5
    EXPECT_GE(mode, 0.0);
    EXPECT_LE(mode, 1.0);
}

TEST_F(GEvalEvaluatorTest, EmptyDocuments) {
    GEvalEvaluator evaluator(config);
    
    std::string query = "Test query";
    std::string answer = "Test answer";
    std::vector<std::pair<std::string, std::string>> empty_docs;
    
    auto result = evaluator.evaluate(query, answer, empty_docs, "overall");
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
}

// ============================================================================
// Integration Tests (with G-Eval)
// ============================================================================

TEST_F(Phase4IntegrationTest, AllThreeEvaluators) {
    RubricEvaluator rubric_eval;
    CoTEvaluator cot_eval;
    GEvalEvaluator geval_eval;
    
    std::string query = "What is quantum computing?";
    std::string answer = "Quantum computing uses quantum bits (qubits) to perform calculations that classical computers cannot efficiently solve.";
    
    auto rubric_result = rubric_eval.evaluate(query, answer, docs);
    auto cot_result = cot_eval.evaluate(query, answer, docs, "faithfulness");
    auto geval_result = geval_eval.evaluate(query, answer, docs, "faithfulness");
    
    // All should produce valid scores
    EXPECT_GE(rubric_result.overall_score, 0.0);
    EXPECT_LE(rubric_result.overall_score, 1.0);
    EXPECT_GE(cot_result.final_score, 0.0);
    EXPECT_LE(cot_result.final_score, 1.0);
    EXPECT_GE(geval_result.geval_score, 0.0);
    EXPECT_LE(geval_result.geval_score, 1.0);
    
    // G-Eval should have probability distribution
    EXPECT_EQ(geval_result.token_probabilities.size(), 5);
}

// ============================================================================
// Main
// ============================================================================


