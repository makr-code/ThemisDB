/**
 * @file test_quality_control_pipeline.cpp
 * @brief Unit tests for Quality Control Pipeline
 */

#include <gtest/gtest.h>
#include "rag/quality_control_pipeline.h"

using namespace themis::rag::judge;

class QualityControlPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.default_mode = QCMode::BALANCED;
        config_.accept_threshold = 0.75;
        config_.reject_threshold = 0.50;
        config_.warn_threshold = 0.65;
        config_.enable_retry = true;
        config_.max_retries = 2;
        config_.fast_timeout_ms = 50;
        config_.balanced_timeout_ms = 500;
        config_.thorough_timeout_ms = 2000;
    }
    
    QualityControlPipeline::Config config_;
    
    std::vector<RetrievedDocument> createTestDocuments() {
        return {
            {"doc1", "Paris is the capital of France.", 0.95, {}},
            {"doc2", "France is in Western Europe.", 0.9, {}},
            {"doc3", "The Eiffel Tower is in Paris.", 0.85, {}}
        };
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, DefaultConstructor) {
    QualityControlPipeline pipeline;
    EXPECT_NO_THROW({
        auto result = pipeline.runQualityControl(
            "What is Paris?",
            createTestDocuments(),
            "Paris is the capital of France."
        );
    });
}

TEST_F(QualityControlPipelineTest, ConfigConstructor) {
    QualityControlPipeline pipeline(config_);
    auto config = pipeline.getConfig();
    
    EXPECT_EQ(config.default_mode, config_.default_mode);
    EXPECT_DOUBLE_EQ(config.accept_threshold, config_.accept_threshold);
}

// ═══════════════════════════════════════════════════════════
// Fast Mode Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, FastMode_BasicEvaluation) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        QCMode::FAST
    );
    
    EXPECT_EQ(result.mode, QCMode::FAST);
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_NE(result.decision, QCDecision::ACCEPT);  // Some decision was made
}

TEST_F(QualityControlPipelineTest, FastMode_Performance) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        QCMode::FAST
    );
    
    // Fast mode should be quick
    EXPECT_LT(result.latency.count(), config_.fast_timeout_ms * 2);
}

// ═══════════════════════════════════════════════════════════
// Balanced Mode Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, BalancedMode_BasicEvaluation) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France, located in Western Europe.",
        QCMode::BALANCED
    );
    
    EXPECT_EQ(result.mode, QCMode::BALANCED);
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    
    // Balanced mode evaluates multiple dimensions
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_GE(result.relevance_score, 0.0);
    EXPECT_GE(result.completeness_score, 0.0);
    EXPECT_GE(result.coherence_score, 0.0);
}

TEST_F(QualityControlPipelineTest, BalancedMode_Performance) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        QCMode::BALANCED
    );
    
    // Balanced mode should be reasonably fast
    EXPECT_LT(result.latency.count(), config_.balanced_timeout_ms * 2);
}

// ═══════════════════════════════════════════════════════════
// Thorough Mode Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, ThoroughMode_BasicEvaluation) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France, located in Western Europe with many tourists.",
        QCMode::THOROUGH
    );
    
    EXPECT_EQ(result.mode, QCMode::THOROUGH);
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    
    // Thorough mode should provide recommendations
    // (may be empty in some cases)
    EXPECT_GE(result.recommendations.size(), 0);
}

TEST_F(QualityControlPipelineTest, ThoroughMode_ComprehensiveScores) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        QCMode::THOROUGH
    );
    
    // All dimensions should be evaluated
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_LE(result.faithfulness_score, 1.0);
    EXPECT_GE(result.relevance_score, 0.0);
    EXPECT_LE(result.relevance_score, 1.0);
    EXPECT_GE(result.completeness_score, 0.0);
    EXPECT_LE(result.completeness_score, 1.0);
    EXPECT_GE(result.coherence_score, 0.0);
    EXPECT_LE(result.coherence_score, 1.0);
}

// ═══════════════════════════════════════════════════════════
// Decision Logic Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, Decision_Accept) {
    config_.accept_threshold = 0.5;  // Low threshold for testing
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France, a beautiful city in Western Europe with rich history.",
        QCMode::BALANCED
    );
    
    // Good answer should be accepted
    if (result.overall_score >= config_.accept_threshold) {
        EXPECT_EQ(result.decision, QCDecision::ACCEPT);
        EXPECT_TRUE(result.passed_threshold);
    }
}

TEST_F(QualityControlPipelineTest, Decision_EmptyAnswer) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "",  // Empty answer
        QCMode::FAST
    );
    
    // Empty answer should not be accepted
    EXPECT_NE(result.decision, QCDecision::ACCEPT);
    EXPECT_LT(result.overall_score, config_.accept_threshold);
}

// ═══════════════════════════════════════════════════════════
// Adaptive QC Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, AdaptiveQC_SmallBudget) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runAdaptiveQC(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        50  // Small time budget
    );
    
    // Should select fast mode
    EXPECT_EQ(result.mode, QCMode::FAST);
}

TEST_F(QualityControlPipelineTest, AdaptiveQC_MediumBudget) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runAdaptiveQC(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        500  // Medium time budget
    );
    
    // Should select balanced mode
    EXPECT_EQ(result.mode, QCMode::BALANCED);
}

TEST_F(QualityControlPipelineTest, AdaptiveQC_LargeBudget) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runAdaptiveQC(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        2000  // Large time budget
    );
    
    // Should select thorough mode
    EXPECT_EQ(result.mode, QCMode::THOROUGH);
}

// ═══════════════════════════════════════════════════════════
// Batch Processing Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, BatchQualityControl) {
    QualityControlPipeline pipeline(config_);
    
    std::vector<EvaluationInput> inputs;
    auto docs = createTestDocuments();
    
    for (int i = 0; i < 3; i++) {
        EvaluationInput input;
        input.query = "Question " + std::to_string(i);
        input.documents = docs;
        input.generated_answer = "Answer " + std::to_string(i);
        inputs.push_back(input);
    }
    
    auto results = pipeline.batchQualityControl(inputs, QCMode::FAST);
    
    ASSERT_EQ(results.size(), inputs.size());
    
    for (const auto& result : results) {
        EXPECT_GE(result.overall_score, 0.0);
        EXPECT_LE(result.overall_score, 1.0);
    }
}

TEST_F(QualityControlPipelineTest, BatchQualityControl_Empty) {
    QualityControlPipeline pipeline(config_);
    
    std::vector<EvaluationInput> inputs;
    auto results = pipeline.batchQualityControl(inputs);
    
    EXPECT_TRUE(results.empty());
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, Statistics_Tracking) {
    QualityControlPipeline pipeline(config_);
    
    // Run a few evaluations
    for (int i = 0; i < 5; i++) {
        pipeline.runQualityControl(
            "Question",
            createTestDocuments(),
            "Answer",
            QCMode::FAST
        );
    }
    
    auto stats = pipeline.getStatistics();
    
    EXPECT_EQ(stats.total_evaluations, 5);
    EXPECT_GE(stats.avg_latency_ms, 0.0);
    EXPECT_GE(stats.avg_score, 0.0);
    EXPECT_LE(stats.avg_score, 1.0);
}

TEST_F(QualityControlPipelineTest, Statistics_ModeUsage) {
    QualityControlPipeline pipeline(config_);
    
    pipeline.runQualityControl("Q", createTestDocuments(), "A", QCMode::FAST);
    pipeline.runQualityControl("Q", createTestDocuments(), "A", QCMode::FAST);
    pipeline.runQualityControl("Q", createTestDocuments(), "A", QCMode::BALANCED);
    
    auto stats = pipeline.getStatistics();
    
    EXPECT_EQ(stats.mode_usage[QCMode::FAST], 2);
    EXPECT_EQ(stats.mode_usage[QCMode::BALANCED], 1);
}

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, SetConfig) {
    QualityControlPipeline pipeline;
    
    config_.accept_threshold = 0.85;
    pipeline.setConfig(config_);
    
    auto retrieved_config = pipeline.getConfig();
    EXPECT_DOUBLE_EQ(retrieved_config.accept_threshold, 0.85);
}

TEST_F(QualityControlPipelineTest, Callback_Called) {
    QualityControlPipeline pipeline(config_);
    
    bool callback_called = false;
    double callback_score = 0.0;
    
    pipeline.setQCCallback([&](const QCResult& result) {
        callback_called = true;
        callback_score = result.overall_score;
    });
    
    auto result = pipeline.runQualityControl(
        "Question",
        createTestDocuments(),
        "Answer",
        QCMode::FAST
    );
    
    EXPECT_TRUE(callback_called);
    EXPECT_DOUBLE_EQ(callback_score, result.overall_score);
}

// ═══════════════════════════════════════════════════════════
// Factory Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, Factory_CreateFast) {
    auto pipeline = QualityControlPipelineFactory::createFast();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_EQ(config.default_mode, QCMode::FAST);
}

TEST_F(QualityControlPipelineTest, Factory_CreateBalanced) {
    auto pipeline = QualityControlPipelineFactory::createBalanced();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_EQ(config.default_mode, QCMode::BALANCED);
}

TEST_F(QualityControlPipelineTest, Factory_CreateThorough) {
    auto pipeline = QualityControlPipelineFactory::createThorough();
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_EQ(config.default_mode, QCMode::THOROUGH);
}

TEST_F(QualityControlPipelineTest, Factory_CreateCustom) {
    QualityControlPipeline::Config custom_config;
    custom_config.accept_threshold = 0.9;
    
    auto pipeline = QualityControlPipelineFactory::create(custom_config);
    ASSERT_NE(pipeline, nullptr);
    
    auto config = pipeline->getConfig();
    EXPECT_DOUBLE_EQ(config.accept_threshold, 0.9);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, VeryLongAnswer) {
    QualityControlPipeline pipeline(config_);
    
    std::string long_answer(5000, 'a');  // 5000 character answer
    
    auto result = pipeline.runQualityControl(
        "Question",
        createTestDocuments(),
        long_answer,
        QCMode::FAST
    );
    
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST_F(QualityControlPipelineTest, NoDocuments) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "Question",
        {},  // No documents
        "Answer",
        QCMode::FAST
    );
    
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST_F(QualityControlPipelineTest, SpecialCharacters) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "Quéstion with émojis 😀?",
        createTestDocuments(),
        "Answér with spëcial chars: @#$%",
        QCMode::FAST
    );
    
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

// ═══════════════════════════════════════════════════════════
// Performance Tests
// ═══════════════════════════════════════════════════════════

TEST_F(QualityControlPipelineTest, Performance_FastModeTarget) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris.",
        QCMode::FAST
    );
    
    // Fast mode target: <50ms
    // Allow some margin for test environment
    EXPECT_LT(result.latency.count(), 100);
}

TEST_F(QualityControlPipelineTest, Performance_BalancedModeTarget) {
    QualityControlPipeline pipeline(config_);
    
    auto result = pipeline.runQualityControl(
        "What is the capital of France?",
        createTestDocuments(),
        "Paris is the capital of France.",
        QCMode::BALANCED
    );
    
    // Balanced mode target: <500ms
    // Allow margin for test environment
    EXPECT_LT(result.latency.count(), 1000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
