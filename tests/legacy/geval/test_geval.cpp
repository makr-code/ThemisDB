/**
 * @file test_geval.cpp
 * @brief Unit tests for G-Eval evaluator
 */

#include <gtest/gtest.h>
#include "rag/geval_evaluator.h"

using namespace themis::rag::judge;

class GEvalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.num_samples = 3;
        config_.aggregation = AggregationMethod::MEAN;
        config_.temperature = 0.7;
        config_.extract_reasoning = true;
        config_.confidence_threshold = 0.6;
    }
    
    GEvalEvaluator::Config config_;
    
    std::vector<std::pair<std::string, std::string>> createTestDocuments() {
        return {
            {"doc1", "Paris is the capital of France. It is located in Western Europe."},
            {"doc2", "France is a country in Western Europe with a population of about 67 million."},
            {"doc3", "The Eiffel Tower is a famous landmark in Paris."}
        };
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, DefaultConstructor) {
    GEvalEvaluator evaluator;
    ASSERT_NO_THROW({
        auto result = evaluator.evaluate(
            "What is the capital of France?",
            "The capital of France is Paris.",
            createTestDocuments(),
            "faithfulness"
        );
    });
}

TEST_F(GEvalTest, ConfigConstructor) {
    GEvalEvaluator evaluator(config_);
    ASSERT_NO_THROW({
        auto result = evaluator.evaluate(
            "What is the capital of France?",
            "The capital of France is Paris.",
            createTestDocuments(),
            "faithfulness"
        );
    });
}

// ═══════════════════════════════════════════════════════════
// Core Evaluation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, BasicEvaluation) {
    GEvalEvaluator evaluator(config_);
    
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "The capital of France is Paris, located in Western Europe.",
        createTestDocuments(),
        "faithfulness"
    );
    
    // Check result structure
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
    EXPECT_EQ(result.dimension, "faithfulness");
    EXPECT_EQ(result.token_probabilities.size(), 5);  // 5 levels (1-5)
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
    EXPECT_FALSE(result.reasoning.empty());
}

TEST_F(GEvalTest, DifferentDimensions) {
    GEvalEvaluator evaluator(config_);
    auto docs = createTestDocuments();
    
    std::vector<std::string> dimensions = {
        "faithfulness", "relevance", "completeness", "coherence"
    };
    
    for (const auto& dim : dimensions) {
        auto result = evaluator.evaluate(
            "What is the capital of France?",
            "Paris is the capital of France.",
            docs,
            dim
        );
        
        EXPECT_EQ(result.dimension, dim);
        EXPECT_GE(result.geval_score, 0.0);
        EXPECT_LE(result.geval_score, 1.0);
    }
}

TEST_F(GEvalTest, EmptyAnswer) {
    GEvalEvaluator evaluator(config_);
    
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "",
        createTestDocuments(),
        "faithfulness"
    );
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
}

TEST_F(GEvalTest, EmptyDocuments) {
    GEvalEvaluator evaluator(config_);
    
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "Paris is the capital of France.",
        {},
        "faithfulness"
    );
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
}

// ═══════════════════════════════════════════════════════════
// Score Computation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, ComputeGEvalScore_Uniform) {
    // Uniform distribution: equal probabilities for all levels
    std::vector<double> probs = {0.2, 0.2, 0.2, 0.2, 0.2};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    
    // Expected value: (1*0.2 + 2*0.2 + 3*0.2 + 4*0.2 + 5*0.2) = 3.0
    // Normalized: (3.0 - 1) / (5 - 1) = 0.5
    EXPECT_NEAR(score, 0.5, 0.01);
}

TEST_F(GEvalTest, ComputeGEvalScore_HighQuality) {
    // High quality: skewed toward level 5
    std::vector<double> probs = {0.0, 0.0, 0.1, 0.2, 0.7};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    
    // Expected value: (3*0.1 + 4*0.2 + 5*0.7) = 4.6
    // Normalized: (4.6 - 1) / 4 = 0.9
    EXPECT_NEAR(score, 0.9, 0.01);
}

TEST_F(GEvalTest, ComputeGEvalScore_LowQuality) {
    // Low quality: skewed toward level 1
    std::vector<double> probs = {0.7, 0.2, 0.1, 0.0, 0.0};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    
    // Expected value: (1*0.7 + 2*0.2 + 3*0.1) = 1.4
    // Normalized: (1.4 - 1) / 4 = 0.1
    EXPECT_NEAR(score, 0.1, 0.01);
}

TEST_F(GEvalTest, ComputeGEvalScore_InvalidSize) {
    std::vector<double> probs = {0.5, 0.5};  // Wrong size
    double score = GEvalEvaluator::computeGEvalScore(probs);
    
    // Should return default (middle)
    EXPECT_NEAR(score, 0.5, 0.01);
}

// ═══════════════════════════════════════════════════════════
// Confidence Computation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, ComputeConfidence_Concentrated) {
    // Concentrated distribution = high confidence
    std::vector<double> probs = {0.0, 0.0, 0.0, 0.1, 0.9};
    double confidence = GEvalEvaluator::computeConfidence(probs);
    
    EXPECT_GT(confidence, 0.8);  // Should be high
}

TEST_F(GEvalTest, ComputeConfidence_Uniform) {
    // Uniform distribution = low confidence
    std::vector<double> probs = {0.2, 0.2, 0.2, 0.2, 0.2};
    double confidence = GEvalEvaluator::computeConfidence(probs);
    
    EXPECT_LT(confidence, 0.3);  // Should be low
}

TEST_F(GEvalTest, ComputeConfidence_Empty) {
    std::vector<double> probs;
    double confidence = GEvalEvaluator::computeConfidence(probs);
    
    EXPECT_DOUBLE_EQ(confidence, 0.0);
}

// ═══════════════════════════════════════════════════════════
// Aggregation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, AggregateScores_Mean) {
    std::vector<double> samples = {0.6, 0.7, 0.8};
    double result = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEAN);
    
    EXPECT_NEAR(result, 0.7, 0.01);
}

TEST_F(GEvalTest, AggregateScores_Median_Odd) {
    std::vector<double> samples = {0.5, 0.7, 0.9};
    double result = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEDIAN);
    
    EXPECT_DOUBLE_EQ(result, 0.7);
}

TEST_F(GEvalTest, AggregateScores_Median_Even) {
    std::vector<double> samples = {0.5, 0.6, 0.7, 0.8};
    double result = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEDIAN);
    
    EXPECT_NEAR(result, 0.65, 0.01);
}

TEST_F(GEvalTest, AggregateScores_Mode) {
    std::vector<double> samples = {0.7, 0.7, 0.7, 0.8, 0.9};
    double result = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MODE);
    
    // Mode should be close to 0.7
    EXPECT_NEAR(result, 0.7, 0.1);
}

TEST_F(GEvalTest, AggregateScores_Empty) {
    std::vector<double> samples;
    double result = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEAN);
    
    EXPECT_DOUBLE_EQ(result, 0.5);  // Default
}

TEST_F(GEvalTest, AggregateScores_Single) {
    std::vector<double> samples = {0.75};
    double result = GEvalEvaluator::aggregateScores(samples, AggregationMethod::MEAN);
    
    EXPECT_DOUBLE_EQ(result, 0.75);
}

// ═══════════════════════════════════════════════════════════
// Multiple Samples Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, MultipleSamples_Consistency) {
    config_.num_samples = 5;
    GEvalEvaluator evaluator(config_);
    
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "Paris is the capital of France.",
        createTestDocuments(),
        "faithfulness"
    );
    
    EXPECT_EQ(result.sample_scores.size(), 5);
    EXPECT_GE(result.variance, 0.0);  // Variance should be non-negative
    
    // All sample scores should be in valid range
    for (double score : result.sample_scores) {
        EXPECT_GE(score, 0.0);
        EXPECT_LE(score, 1.0);
    }
}

TEST_F(GEvalTest, DifferentAggregationMethods) {
    std::vector<AggregationMethod> methods = {
        AggregationMethod::MEAN,
        AggregationMethod::MEDIAN,
        AggregationMethod::MODE
    };
    
    for (auto method : methods) {
        config_.aggregation = method;
        GEvalEvaluator evaluator(config_);
        
        auto result = evaluator.evaluate(
            "What is the capital of France?",
            "Paris is the capital of France.",
            createTestDocuments(),
            "faithfulness"
        );
        
        EXPECT_GE(result.geval_score, 0.0);
        EXPECT_LE(result.geval_score, 1.0);
    }
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, VeryLongAnswer) {
    GEvalEvaluator evaluator(config_);
    
    std::string long_answer = std::string(5000, 'a');  // 5000 character answer
    
    auto result = evaluator.evaluate(
        "Question",
        long_answer,
        createTestDocuments(),
        "faithfulness"
    );
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
}

TEST_F(GEvalTest, SpecialCharacters) {
    GEvalEvaluator evaluator(config_);
    
    auto result = evaluator.evaluate(
        "What about émojis? 😀",
        "Spëcial çharacters: @#$%^&*()[]{}",
        createTestDocuments(),
        "faithfulness"
    );
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
}

TEST_F(GEvalTest, TokenProbabilities_SumToOne) {
    GEvalEvaluator evaluator(config_);
    
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "Paris is the capital of France.",
        createTestDocuments(),
        "faithfulness"
    );
    
    // Token probabilities should sum to approximately 1.0
    double sum = 0.0;
    for (double p : result.token_probabilities) {
        sum += p;
    }
    
    EXPECT_NEAR(sum, 1.0, 0.01);
}

// ═══════════════════════════════════════════════════════════
// Performance Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GEvalTest, PerformanceReasonable) {
    GEvalEvaluator evaluator(config_);
    
    auto start = std::chrono::steady_clock::now();
    
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "Paris is the capital of France.",
        createTestDocuments(),
        "faithfulness"
    );
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (stub implementation should be fast)
    EXPECT_LT(duration.count(), 1000);  // Less than 1 second
}
