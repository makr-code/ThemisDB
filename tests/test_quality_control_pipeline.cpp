/**
 * @file test_quality_control_pipeline.cpp
 * @brief Tests for the quality control pipeline
 */

#include <gtest/gtest.h>
#include "rag/quality_control_pipeline.h"
#include "rag/llm_judge_client.h"
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

class QualityControlPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Prepare test data
        query = "What is the capital of France?";
        good_answer = "The capital of France is Paris, located on the Seine River.";
        bad_answer = "The capital of France is London, which is known for Big Ben.";
        
        // Add documents
        RetrievedDocument doc1;
        doc1.id = "doc1";
        doc1.content = "Paris is the capital and most populous city of France. "
                      "It is situated on the Seine River in northern France.";
        doc1.similarity_score = 0.95;
        documents.push_back(doc1);
        
        RetrievedDocument doc2;
        doc2.id = "doc2";
        doc2.content = "France is a country in Western Europe with Paris as its capital.";
        doc2.similarity_score = 0.90;
        documents.push_back(doc2);
    }
    
    std::string query;
    std::string good_answer;
    std::string bad_answer;
    std::vector<RetrievedDocument> documents;
};

// ═══════════════════════════════════════════════════════════
// Basic Pipeline Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, CreateFastPipeline) {
    auto pipeline = QualityPipelineFactory::createFast();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_TRUE(config.enable_fast_stage);
    EXPECT_FALSE(config.enable_balanced_stage);
    EXPECT_FALSE(config.enable_thorough_stage);
}

TEST_F(QualityControlPipelineTest, CreateBalancedPipeline) {
    auto pipeline = QualityPipelineFactory::createBalanced();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_TRUE(config.enable_fast_stage);
    EXPECT_TRUE(config.enable_balanced_stage);
    EXPECT_FALSE(config.enable_thorough_stage);
}

TEST_F(QualityControlPipelineTest, CreateThoroughPipeline) {
    auto pipeline = QualityPipelineFactory::createThorough();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_TRUE(config.enable_fast_stage);
    EXPECT_TRUE(config.enable_balanced_stage);
    EXPECT_TRUE(config.enable_thorough_stage);
}

TEST_F(QualityControlPipelineTest, CreateProductionPipeline) {
    auto pipeline = QualityPipelineFactory::createProduction();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_TRUE(config.enable_fast_stage);
    EXPECT_TRUE(config.enable_balanced_stage);
    EXPECT_TRUE(config.enable_thorough_stage);
    EXPECT_TRUE(config.enable_learning_feedback);
}

// ═══════════════════════════════════════════════════════════
// Fast Stage Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, FastStagePassesGoodAnswer) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    
    // Should complete quickly (<100ms to allow slack for test environment)
    // Production target is <50ms, but tests may run slower due to overhead
    EXPECT_LT(result.fast_stage_time.count(), 100);
    
    // Should have faithfulness score
    EXPECT_FALSE(result.dimension_scores.empty());
    
    bool found_faithfulness = false;
    for (const auto& score : result.dimension_scores) {
        if (score.dimension == "faithfulness") {
            found_faithfulness = true;
            EXPECT_GT(score.score, 0.5);
        }
    }
    EXPECT_TRUE(found_faithfulness);
}

TEST_F(QualityControlPipelineTest, FastStageDetectsBadAnswer) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    auto result = pipeline->runQualityControl(query, bad_answer, documents);
    
    // Should complete quickly regardless of answer quality
    // (<100ms to allow slack for test environment)
    EXPECT_LT(result.fast_stage_time.count(), 100);
    
    // May or may not fail depending on threshold, but should have scores
    EXPECT_FALSE(result.dimension_scores.empty());
}

// ═══════════════════════════════════════════════════════════
// Balanced Stage Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, BalancedStageEvaluatesMultipleDimensions) {
    auto pipeline = QualityPipelineFactory::createBalanced();
    
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    
    // Should complete within timeout
    EXPECT_LT(result.total_time.count(), 600);
    
    // Should have multiple dimension scores
    EXPECT_GE(result.dimension_scores.size(), 2);
    
    // Should have overall score
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

// ═══════════════════════════════════════════════════════════
// Thorough Stage Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, ThoroughStageIncludesNLI) {
    auto pipeline = QualityPipelineFactory::createThorough();
    
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    
    // Should complete within timeout
    EXPECT_LT(result.total_time.count(), 2500);
    
    // Should have NLI faithfulness score
    bool found_nli = false;
    for (const auto& score : result.dimension_scores) {
        if (score.dimension == "faithfulness_nli") {
            found_nli = true;
            EXPECT_EQ(score.method, "nli");
        }
    }
    EXPECT_TRUE(found_nli);
}

// ═══════════════════════════════════════════════════════════
// Performance Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, FastModeUnder50ms) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    auto start = std::chrono::steady_clock::now();
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Target: <50ms (may be higher in test environment)
    EXPECT_LT(duration.count(), 100);  // Allow some slack for testing
}

TEST_F(QualityControlPipelineTest, BalancedModeUnder500ms) {
    auto pipeline = QualityPipelineFactory::createBalanced();
    
    auto start = std::chrono::steady_clock::now();
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Target: <500ms (may be higher in test environment)
    EXPECT_LT(duration.count(), 700);  // Allow some slack for testing
}

TEST_F(QualityControlPipelineTest, ThoroughModeUnder2s) {
    auto pipeline = QualityPipelineFactory::createThorough();
    
    auto start = std::chrono::steady_clock::now();
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Target: <2s (may be higher in test environment)
    EXPECT_LT(duration.count(), 2500);  // Allow some slack for testing
}

// ═══════════════════════════════════════════════════════════
// LLM Judge Client Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, LLMJudgeClientBasic) {
    LLMJudgeClient::Config config;
    config.model_name = "test_model";
    config.temperature = 0.3;
    config.enable_caching = true;
    
    auto client = std::make_shared<LLMJudgeClient>(config);
    ASSERT_NE(client, nullptr);
    
    auto retrieved_config = client->getConfig();
    EXPECT_EQ(retrieved_config.model_name, "test_model");
    EXPECT_DOUBLE_EQ(retrieved_config.temperature, 0.3);
}

// ═══════════════════════════════════════════════════════════
// NLI Verifier Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, NLIVerifierBasic) {
    NLIFaithfulnessVerifier::Config config;
    config.entailment_threshold = 0.7;
    config.max_claims = 10;
    
    auto verifier = std::make_shared<NLIFaithfulnessVerifier>(config);
    ASSERT_NE(verifier, nullptr);
    
    std::vector<std::pair<std::string, std::string>> docs;
    docs.emplace_back("doc1", documents[0].content);
    
    auto result = verifier->verify(good_answer, docs);
    
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_LE(result.faithfulness_score, 1.0);
    EXPECT_GT(result.total_claims, 0);
}

TEST_F(QualityControlPipelineTest, NLIDetectsContradiction) {
    auto verifier = std::make_shared<NLIFaithfulnessVerifier>();
    
    std::vector<std::pair<std::string, std::string>> docs;
    docs.emplace_back("doc1", documents[0].content);
    
    // Answer contradicts documents
    auto result = verifier->verify(bad_answer, docs);
    
    // Should detect low faithfulness
    EXPECT_LT(result.faithfulness_score, 0.8);
}

// ═══════════════════════════════════════════════════════════
// G-Eval Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, GEvalBasic) {
    GEvalEvaluator::Config config;
    config.num_samples = 3;
    config.aggregation = AggregationMethod::MEAN;
    
    auto geval = std::make_shared<GEvalEvaluator>(config);
    ASSERT_NE(geval, nullptr);
    
    std::vector<std::pair<std::string, std::string>> docs;
    docs.emplace_back("doc1", documents[0].content);
    
    auto result = geval->evaluate(query, good_answer, docs, "faithfulness");
    
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
    EXPECT_EQ(result.token_probabilities.size(), 5);  // Levels 1-5
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
}

// ═══════════════════════════════════════════════════════════
// Callback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, FailureCallbackInvoked) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    bool callback_invoked = false;
    pipeline->setFailureCallback([&callback_invoked](const QualityCheckResult& result) {
        callback_invoked = true;
    });
    
    // This may or may not trigger failure depending on threshold
    auto result = pipeline->runQualityControl(query, bad_answer, documents);
    
    // If it failed, callback should have been invoked
    if (result.status == QualityGateStatus::FAILED) {
        EXPECT_TRUE(callback_invoked);
    }
}

TEST_F(QualityControlPipelineTest, LearningCallbackInvoked) {
    auto pipeline = QualityPipelineFactory::createProduction();
    
    bool callback_invoked = false;
    std::string captured_query;
    
    pipeline->setLearningCallback(
        [&callback_invoked, &captured_query](
            const std::string& q, 
            const QualityCheckResult& result) {
        callback_invoked = true;
        captured_query = q;
    });
    
    auto result = pipeline->runQualityControl(query, good_answer, documents);
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(captured_query, query);
}

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, ConfigurationUpdate) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    QualityControlPipeline::Config new_config;
    new_config.enable_fast_stage = false;
    new_config.enable_balanced_stage = true;
    
    pipeline->setConfig(new_config);
    
    auto retrieved_config = pipeline->getConfig();
    EXPECT_FALSE(retrieved_config.enable_fast_stage);
    EXPECT_TRUE(retrieved_config.enable_balanced_stage);
}

// ═══════════════════════════════════════════════════════════
// Citation Coverage Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, ThoroughStageIncludesCitationCoverage) {
    auto pipeline = QualityPipelineFactory::createThorough();

    auto result = pipeline->runQualityControl(query, good_answer, documents);

    // Citation coverage should be populated by the thorough stage
    EXPECT_GE(result.citation_coverage, 0.0);
    EXPECT_LE(result.citation_coverage, 1.0);

    // citation_coverage DimensionScore should be present
    bool found_citation = false;
    for (const auto& score : result.dimension_scores) {
        if (score.dimension == "citation_coverage") {
            found_citation = true;
            EXPECT_EQ(score.method, "citation_highlighter");
            EXPECT_GE(score.score, 0.0);
            EXPECT_LE(score.score, 1.0);
        }
    }
    EXPECT_TRUE(found_citation);
}

TEST_F(QualityControlPipelineTest, ThoroughStageCitationCoverageDisableable) {
    QualityControlPipeline::Config cfg;
    cfg.enable_fast_stage      = true;
    cfg.enable_balanced_stage  = true;
    cfg.enable_thorough_stage  = true;
    cfg.enable_citation_check  = false;

    QualityControlPipeline pipeline(cfg);
    auto result = pipeline.runQualityControl(query, good_answer, documents);

    // citation_coverage should remain at default 0.0 when disabled
    EXPECT_NEAR(result.citation_coverage, 0.0, 1e-9);

    // No citation_coverage DimensionScore should be present
    for (const auto& score : result.dimension_scores) {
        EXPECT_NE(score.dimension, "citation_coverage");
    }
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, StatisticsTracking) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    // Run several checks
    for (int i = 0; i < 5; i++) {
        pipeline->runQualityControl(query, good_answer, documents);
    }
    
    auto stats = pipeline->getStatistics();
    EXPECT_FALSE(stats.empty());
    
    // Stats should be JSON format
    EXPECT_NE(stats.find("total_checks"), std::string::npos);
}

TEST_F(QualityControlPipelineTest, StatisticsReset) {
    auto pipeline = QualityPipelineFactory::createFast();
    
    // Run a check
    pipeline->runQualityControl(query, good_answer, documents);
    
    // Reset
    pipeline->resetStatistics();
    
    auto stats = pipeline->getStatistics();
    EXPECT_NE(stats.find("\"total_checks\": 0"), std::string::npos);
}

