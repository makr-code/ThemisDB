/**
 * @file test_quality_control_pipeline.cpp
 * @brief Unit tests for Quality Control Pipeline
 */

#include "rag/quality_control_pipeline.h"
#include "rag/rag_judge.h"
#include <gtest/gtest.h>

using namespace themis::rag::judge;

// ============================================================================
// Quality Control Pipeline Tests
// ============================================================================

class QCPipelineTest : public ::testing::Test {
protected:
    QualityControlPipeline pipeline;
    
    std::vector<RetrievedDocument> sample_docs = {
        {"doc1", "Paris is the capital of France. Population over 2 million.", 0.9},
        {"doc2", "The Eiffel Tower is located in Paris.", 0.8}
    };
};

TEST_F(QCPipelineTest, BasicConstruction) {
    // Pipeline should construct without issues
    EXPECT_NO_THROW({
        QualityControlPipeline p;
    });
}

TEST_F(QCPipelineTest, ConfigurationAccess) {
    auto config = pipeline.getConfig();
    
    EXPECT_GT(config.max_evaluation_time_ms, 0.0);
    EXPECT_TRUE(config.use_faithfulness);
    EXPECT_TRUE(config.use_relevance);
}

TEST_F(QCPipelineTest, ConfigurationUpdate) {
    QualityControlPipeline::Config new_config;
    new_config.max_evaluation_time_ms = 300.0;
    new_config.min_confidence = 0.8;
    new_config.use_completeness = false;
    
    pipeline.setConfig(new_config);
    
    auto retrieved_config = pipeline.getConfig();
    EXPECT_DOUBLE_EQ(retrieved_config.max_evaluation_time_ms, 300.0);
    EXPECT_DOUBLE_EQ(retrieved_config.min_confidence, 0.8);
    EXPECT_FALSE(retrieved_config.use_completeness);
}

TEST_F(QCPipelineTest, BasicEvaluation) {
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France. It has many famous landmarks.";
    
    auto result = pipeline.evaluate(query, sample_docs, answer);
    
    // Check result structure
    EXPECT_GE(result.evaluation.overall_score, 0.0);
    EXPECT_LE(result.evaluation.overall_score, 1.0);
    EXPECT_GT(result.metrics.total_time_ms, 0.0);
    EXPECT_FALSE(result.quality_summary.empty());
}

TEST_F(QCPipelineTest, PerformanceMetrics) {
    std::string query = "Tell me about Paris";
    std::string answer = "Paris is the capital of France with the Eiffel Tower.";
    
    auto result = pipeline.evaluate(query, sample_docs, answer);
    
    // Check metrics
    EXPECT_GT(result.metrics.total_time_ms, 0.0);
    EXPECT_GE(result.metrics.llm_calls_count, 0);
    EXPECT_GE(result.metrics.nli_calls_count, 0);
}

TEST_F(QCPipelineTest, QualityChecks) {
    std::string query = "What is Paris?";
    std::string answer = "Paris is the capital city of France.";
    
    auto result = pipeline.evaluate(query, sample_docs, answer);
    
    // Should have quality checks
    EXPECT_FALSE(result.quality_checks.empty());
    
    for (const auto& check : result.quality_checks) {
        EXPECT_FALSE(check.dimension.empty());
        EXPECT_GE(check.confidence, 0.0);
        EXPECT_LE(check.confidence, 1.0);
    }
}

TEST_F(QCPipelineTest, OverallQualityPassFail) {
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France.";
    
    auto result = pipeline.evaluate(query, sample_docs, answer);
    
    // Should have at least one quality check
    EXPECT_FALSE(result.quality_checks.empty());
    
    // If passed, all checks should pass
    if (result.overall_quality_passed) {
        for (const auto& check : result.quality_checks) {
            EXPECT_TRUE(check.passed);
        }
    }
}

TEST_F(QCPipelineTest, BatchEvaluation) {
    std::vector<std::tuple<
        std::string,
        std::vector<RetrievedDocument>,
        std::string
    >> evaluations = {
        {"Query 1", sample_docs, "Answer 1"},
        {"Query 2", sample_docs, "Answer 2"},
        {"Query 3", sample_docs, "Answer 3"}
    };
    
    auto results = pipeline.evaluateBatch(evaluations);
    
    EXPECT_EQ(results.size(), evaluations.size());
    
    for (const auto& result : results) {
        EXPECT_GE(result.evaluation.overall_score, 0.0);
        EXPECT_LE(result.evaluation.overall_score, 1.0);
        EXPECT_GT(result.metrics.total_time_ms, 0.0);
    }
}

TEST_F(QCPipelineTest, CachingFunctionality) {
    std::string query = "What is Paris?";
    std::string answer = "Paris is the capital of France.";
    
    // First evaluation
    auto result1 = pipeline.evaluate(query, sample_docs, answer);
    auto time1 = result1.metrics.total_time_ms;
    
    // Second evaluation with same inputs (should be cached)
    auto result2 = pipeline.evaluate(query, sample_docs, answer);
    auto time2 = result2.metrics.total_time_ms;
    
    // Results should be identical
    EXPECT_DOUBLE_EQ(result1.evaluation.overall_score, result2.evaluation.overall_score);
    
    // Second call might be faster due to caching
    // (But we don't strictly enforce this in case caching is disabled)
}

TEST_F(QCPipelineTest, ClearCache) {
    // Evaluate to populate cache
    pipeline.evaluate("query", sample_docs, "answer");
    
    // Clear cache - should not throw
    EXPECT_NO_THROW(pipeline.clearCache());
    
    // Evaluate again should work
    auto result = pipeline.evaluate("query", sample_docs, "answer");
    EXPECT_GE(result.evaluation.overall_score, 0.0);
}

TEST_F(QCPipelineTest, AggregateMetrics) {
    // Run multiple evaluations
    for (int i = 0; i < 5; i++) {
        std::string query = "Query " + std::to_string(i);
        std::string answer = "Answer " + std::to_string(i);
        pipeline.evaluate(query, sample_docs, answer);
    }
    
    auto aggregate = pipeline.getAggregateMetrics();
    
    // Should have averaged metrics
    EXPECT_GT(aggregate.total_time_ms, 0.0);
}

TEST_F(QCPipelineTest, EmptyDocuments) {
    std::string query = "What is Paris?";
    std::string answer = "Paris is the capital of France.";
    std::vector<RetrievedDocument> empty_docs;
    
    // Should handle empty documents gracefully
    auto result = pipeline.evaluate(query, empty_docs, answer);
    
    EXPECT_GE(result.evaluation.overall_score, 0.0);
    EXPECT_FALSE(result.quality_summary.empty());
}

TEST_F(QCPipelineTest, EmptyAnswer) {
    std::string query = "What is Paris?";
    std::string answer = "";
    
    // Should handle empty answer gracefully
    auto result = pipeline.evaluate(query, sample_docs, answer);
    
    EXPECT_GE(result.evaluation.overall_score, 0.0);
    EXPECT_FALSE(result.quality_summary.empty());
}

// ============================================================================
// Component Integration Tests
// ============================================================================

TEST_F(QCPipelineTest, LLMClientIntegration) {
    auto client = std::make_shared<LLMJudgeClient>();
    
    // Should be able to set client
    EXPECT_NO_THROW(pipeline.setLLMClient(client));
    
    // Evaluation should still work
    auto result = pipeline.evaluate("test query", sample_docs, "test answer");
    EXPECT_GE(result.evaluation.overall_score, 0.0);
}

TEST_F(QCPipelineTest, NLIVerifierIntegration) {
    auto verifier = std::make_shared<NLIFaithfulnessVerifier>();
    
    // Should be able to set verifier
    EXPECT_NO_THROW(pipeline.setNLIVerifier(verifier));
    
    // Evaluation should still work
    auto result = pipeline.evaluate("test query", sample_docs, "test answer");
    EXPECT_GE(result.evaluation.overall_score, 0.0);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(QCPipelineTest, MeetsTimeTarget) {
    QualityControlPipeline::Config config;
    config.max_evaluation_time_ms = 500.0;
    pipeline.setConfig(config);
    
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France.";
    
    auto start = std::chrono::steady_clock::now();
    auto result = pipeline.evaluate(query, sample_docs, answer);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // Should meet time target
    EXPECT_LT(duration_ms, 500);
    EXPECT_TRUE(result.metrics.met_time_target);
}

TEST_F(QCPipelineTest, BatchPerformance) {
    std::vector<std::tuple<
        std::string,
        std::vector<RetrievedDocument>,
        std::string
    >> evaluations;
    
    for (int i = 0; i < 10; i++) {
        evaluations.push_back({
            "Query " + std::to_string(i),
            sample_docs,
            "Answer " + std::to_string(i)
        });
    }
    
    auto start = std::chrono::steady_clock::now();
    auto results = pipeline.evaluateBatch(evaluations);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    EXPECT_EQ(results.size(), evaluations.size());
    
    // 10 evaluations at ~100ms each should be ~1000ms
    // Allow some overhead
    EXPECT_LT(duration_ms, 5000);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(QCPipelineTest, VeryLongAnswer) {
    std::string query = "Tell me about Paris.";
    
    // Create very long answer (> 1000 words)
    std::string answer;
    for (int i = 0; i < 100; i++) {
        answer += "Paris is the capital of France. It has many museums. ";
    }
    
    // Should handle long answers
    auto result = pipeline.evaluate(query, sample_docs, answer);
    EXPECT_GE(result.evaluation.overall_score, 0.0);
}

TEST_F(QCPipelineTest, SpecialCharacters) {
    std::string query = "What is Paris?";
    std::string answer = "Paris is 'the' capital of France! It's amazing (truly).";
    
    // Should handle special characters
    auto result = pipeline.evaluate(query, sample_docs, answer);
    EXPECT_GE(result.evaluation.overall_score, 0.0);
}
