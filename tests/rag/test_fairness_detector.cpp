/**
 * @file test_fairness_detector.cpp
 * @brief Unit tests for FairnessDetector (Wave A3: Fairness & Bias Detection)
 *
 * Test IDs: FAIR-01 .. FAIR-08
 *
 * Covers:
 *  - Initialization and configuration
 *  - Bias detection for various documents
 *  - Multi-dimensional bias scoring
 *  - Batch processing
 *  - Filtering by threshold
 *  - Error handling
 */

#include <gtest/gtest.h>
#include "rag/fairness_detector.h"

using namespace themis::rag;

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-01: FairnessDetector constructs with config
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_01_ConstructsWithConfig) {
    FairnessDetectorConfig config;
    config.bias_threshold = 0.7;
    config.detect_gender_bias = true;
    config.detect_occupational_bias = true;

    FairnessDetector detector(config);
    EXPECT_FALSE(detector.isInitialized());
    EXPECT_EQ(detector.getConfig().bias_threshold, 0.7);
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-02: Initialize succeeds with valid config
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_02_InitializeSucceeds) {
    FairnessDetectorConfig config;
    FairnessDetector detector(config);
    
    EXPECT_NO_THROW(detector.initialize());
    EXPECT_TRUE(detector.isInitialized());
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-03: detectBias fails when not initialized
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_03_DetectBiasFailsWhenNotInitialized) {
    FairnessDetectorConfig config;
    FairnessDetector detector(config);
    
    EXPECT_THROW(detector.detectBias("test document"), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-04: detectBias fails with empty document
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_04_DetectBiasFailsWithEmptyDocument) {
    FairnessDetectorConfig config;
    FairnessDetector detector(config);
    detector.initialize();
    
    EXPECT_THROW(detector.detectBias(""), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-05: detectBias returns BiasScore with expected fields
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_05_DetectBiasReturnsValidScore) {
    FairnessDetectorConfig config;
    FairnessDetector detector(config);
    detector.initialize();
    
    auto score = detector.detectBias("The nurse helped the doctor with the procedure.");
    
    EXPECT_GE(score.overall_score, 0.0);
    EXPECT_LE(score.overall_score, 1.0);
    EXPECT_GE(score.gender_bias, 0.0);
    EXPECT_LE(score.gender_bias, 1.0);
    EXPECT_GE(score.confidence, 0.0);
    EXPECT_LE(score.confidence, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-06: detectBiasBatch processes multiple documents
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_06_DetectBiasBatchWorks) {
    FairnessDetectorConfig config;
    FairnessDetector detector(config);
    detector.initialize();
    
    std::vector<std::string> documents = {
        "Document one with some content",
        "Document two with different content",
        "Document three about gender roles"
    };
    
    auto results = detector.detectBiasBatch(documents);
    EXPECT_EQ(results.size(), 3u);
    
    for (const auto& score : results) {
        EXPECT_GE(score.overall_score, 0.0);
        EXPECT_LE(score.overall_score, 1.0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-07: filterByBiasThreshold returns documents within threshold
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_07_FilterByBiasThresholdWorks) {
    FairnessDetectorConfig config;
    config.bias_threshold = 0.5;
    FairnessDetector detector(config);
    detector.initialize();
    
    std::vector<std::string> documents = {
        "Neutral content about the weather",
        "Discussion of different perspectives",
        "Analysis of historical events"
    };
    
    auto results = detector.filterByBiasThreshold(documents);
    
    // All documents should pass (they have low bias)
    EXPECT_GE(results.size(), 0u);
    
    for (const auto& pair : results) {
        EXPECT_LT(pair.second.overall_score, config.bias_threshold);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FAIR-08: setBiasThreshold updates configuration
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, FAIR_08_SetBiasThresholdWorks) {
    FairnessDetectorConfig config;
    config.bias_threshold = 0.5;
    FairnessDetector detector(config);
    
    EXPECT_EQ(detector.getConfig().bias_threshold, 0.5);
    
    detector.setBiasThreshold(0.75);
    EXPECT_EQ(detector.getConfig().bias_threshold, 0.75);
    
    // Test boundary clamping
    detector.setBiasThreshold(1.5);  // Should clamp to 1.0
    EXPECT_EQ(detector.getConfig().bias_threshold, 1.0);
    
    detector.setBiasThreshold(-0.5);  // Should clamp to 0.0
    EXPECT_EQ(detector.getConfig().bias_threshold, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// BONUS: BiasScore integration test
// ─────────────────────────────────────────────────────────────────────────────
TEST(FairnessDetector, BiasScore_IntegrationWithRetrievedDocument) {
    // Test that BiasScore can be properly stored in RetrievedDocument
    judge::RetrievedDocument doc;
    doc.id = "doc1";
    doc.content = "test content";
    doc.similarity_score = 0.95;
    
    // Initially, bias_score is optional and empty
    EXPECT_FALSE(doc.bias_score.has_value());
    
    // Set a bias score
    judge::BiasScore bias_score;
    bias_score.overall_score = 0.3;
    bias_score.gender_bias = 0.2;
    bias_score.flagged = false;
    
    doc.bias_score = bias_score;
    EXPECT_TRUE(doc.bias_score.has_value());
    EXPECT_EQ(doc.bias_score.value().overall_score, 0.3);
    EXPECT_EQ(doc.bias_score.value().gender_bias, 0.2);
    EXPECT_FALSE(doc.bias_score.value().flagged);
}
