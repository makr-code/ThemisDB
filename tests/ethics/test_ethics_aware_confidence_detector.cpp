/*
 * ThemisDB | File: test_ethics_aware_confidence_detector.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_ethics_aware_confidence_detector.cpp
 * @brief Unit tests for Ethics-Aware Confidence Detector
 */

#include <gtest/gtest.h>
#include "llm/ethics_aware_confidence_detector.h"

using namespace themis::llm;

class EthicsAwareConfidenceDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.min_autonomy_respect = 0.7f;
        config_.min_transparency = 0.6f;
        config_.min_technical_confidence = 0.5f;
        
        detector_ = std::make_unique<EthicsAwareConfidenceDetector>(config_);
    }
    
    EthicsAwareConfidenceConfig config_;
    std::unique_ptr<EthicsAwareConfidenceDetector> detector_;
};

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, ConfigurationValidation) {
    EthicsAwareConfidenceConfig valid_config;
    valid_config.technical_weight = 0.4f;
    valid_config.autonomy_weight = 0.35f;
    valid_config.transparency_weight = 0.25f;
    EXPECT_TRUE(valid_config.validateWeights());
    
    EthicsAwareConfidenceConfig invalid_config;
    invalid_config.technical_weight = 0.5f;
    invalid_config.autonomy_weight = 0.5f;
    invalid_config.transparency_weight = 0.5f;
    EXPECT_FALSE(invalid_config.validateWeights());
}

TEST_F(EthicsAwareConfidenceDetectorTest, ConfigurationUpdate) {
    EthicsAwareConfidenceConfig new_config;
    new_config.min_autonomy_respect = 0.85f;
    new_config.min_transparency = 0.75f;
    
    detector_->setConfig(new_config);
    auto retrieved = detector_->getConfig();
    
    EXPECT_FLOAT_EQ(retrieved.min_autonomy_respect, 0.85f);
    EXPECT_FLOAT_EQ(retrieved.min_transparency, 0.75f);
}

// ═══════════════════════════════════════════════════════════
// Patronizing Language Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, DetectPatronizingEnglish) {
    std::string text = "You must do this immediately. You have to follow these instructions.";
    auto result = detector_->detectPatronizingLanguage(text);
    
    EXPECT_GT(result.size(), 0);
}

TEST_F(EthicsAwareConfidenceDetectorTest, DetectPatronizingGerman) {
    std::string text = "Sie müssen das sofort tun. Du musst diesen Anweisungen folgen.";
    auto result = detector_->detectPatronizingLanguage(text);
    
    EXPECT_GT(result.size(), 0);
}

TEST_F(EthicsAwareConfidenceDetectorTest, NoPatronizingLanguage) {
    std::string text = "You might consider doing this. Here are some options you could explore.";
    auto result = detector_->detectPatronizingLanguage(text);
    
    EXPECT_EQ(result.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Autonomy Respect Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, EvaluateAutonomyRespectHigh) {
    std::string text = "You could consider these options. Different approaches include...";
    float score = detector_->evaluateAutonomyRespect(text);
    
    EXPECT_GE(score, 0.7f);
}

TEST_F(EthicsAwareConfidenceDetectorTest, EvaluateAutonomyRespectLow) {
    std::string text = "You must do this. You have to follow my advice. You should immediately act.";
    float score = detector_->evaluateAutonomyRespect(text);
    
    EXPECT_LT(score, 0.7f);
}

// ═══════════════════════════════════════════════════════════
// Transparency Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, DetectUncertaintyAcknowledgment) {
    std::string text = "This may be true. It could possibly work. It seems that...";
    auto hedge_words = detector_->detectUncertaintyAcknowledgment(text);
    
    EXPECT_GT(hedge_words.size(), 0);
}

TEST_F(EthicsAwareConfidenceDetectorTest, EvaluateTransparencyHigh) {
    std::string text = "This may be the case, but I'm not entirely sure. "
                       "You should consult an expert for specific guidance.";
    float score = detector_->evaluateTransparency(text);
    
    EXPECT_GE(score, 0.6f);
}

TEST_F(EthicsAwareConfidenceDetectorTest, EvaluateTransparencyLow) {
    std::string text = "This is definitely the answer. There's no doubt about it.";
    float score = detector_->evaluateTransparency(text);
    
    EXPECT_LT(score, 0.8f);
}

// ═══════════════════════════════════════════════════════════
// Choice Preservation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, CheckChoicePreservationPositive) {
    std::string text = "You can decide what works best for you. "
                       "Your choice depends on your circumstances.";
    bool preserves = detector_->checkChoicePreservation(text);
    
    EXPECT_TRUE(preserves);
}

TEST_F(EthicsAwareConfidenceDetectorTest, CheckChoicePreservationNegative) {
    std::string text = "Do exactly as I say. Follow these steps precisely.";
    bool preserves = detector_->checkChoicePreservation(text);
    
    EXPECT_FALSE(preserves);
}

// ═══════════════════════════════════════════════════════════
// Full Confidence Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, DetectConfidenceGoodResponse) {
    std::string text = "Based on available information, you might consider these options. "
                       "Different approaches could work depending on your situation. "
                       "It may be helpful to consult with relevant experts.";
    
    auto result = detector_->detectConfidence(text);
    
    EXPECT_GT(result.autonomy_respect_score, 0.7f);
    EXPECT_GT(result.transparency_score, 0.6f);
    EXPECT_GT(result.combined_confidence, 0.6f);
    EXPECT_FALSE(result.has_patronizing_language);
    EXPECT_TRUE(result.acknowledges_uncertainty);
}

TEST_F(EthicsAwareConfidenceDetectorTest, DetectConfidencePoorResponse) {
    std::string text = "You must do this immediately. This is definitely the right answer. "
                       "You have to follow these instructions exactly.";
    
    auto result = detector_->detectConfidence(text);
    
    EXPECT_LT(result.autonomy_respect_score, 0.7f);
    EXPECT_TRUE(result.has_patronizing_language);
    EXPECT_GT(result.patronizing_phrases.size(), 0);
}

TEST_F(EthicsAwareConfidenceDetectorTest, DetectConfidenceWithTokenData) {
    std::string text = "This might be the answer.";
    
    // Mock token confidence data
    std::vector<TokenConfidence> tokens;
    {
        TokenConfidence tc;
        tc.token = "this"; tc.probability = 0.9f; tc.entropy = 0.1f; tc.position = 0;
        tokens.push_back(tc);
    }
    {
        TokenConfidence tc;
        tc.token = "might"; tc.probability = 0.8f; tc.entropy = 0.2f; tc.position = 1;
        tokens.push_back(tc);
    }
    {
        TokenConfidence tc;
        tc.token = "be"; tc.probability = 0.95f; tc.entropy = 0.05f; tc.position = 2;
        tokens.push_back(tc);
    }
    {
        TokenConfidence tc;
        tc.token = "the"; tc.probability = 0.92f; tc.entropy = 0.08f; tc.position = 3;
        tokens.push_back(tc);
    }
    {
        TokenConfidence tc;
        tc.token = "answer"; tc.probability = 0.85f; tc.entropy = 0.15f; tc.position = 4;
        tokens.push_back(tc);
    }
    
    auto result = detector_->detectConfidence(text, tokens);
    
    EXPECT_GT(result.technical_confidence, 0.0f);
    EXPECT_GT(result.avg_token_entropy, 0.0f);
}

// ═══════════════════════════════════════════════════════════
// Context-Aware Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, DetectConfidenceWithContext) {
    std::string text = "You should definitely do this.";
    std::string query = "Should I make this important life decision?";
    std::vector<std::string> context = {"I'm facing a difficult choice."};
    
    auto result = detector_->detectConfidenceWithContext(text, query, context);
    
    EXPECT_LT(result.autonomy_respect_score, 0.9f); // Should detect patronizing in this context
    EXPECT_GT(result.combined_confidence, 0.0f);
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, StatisticsTracking) {
    std::string text1 = "You must do this.";
    std::string text2 = "You might consider this option.";
    
    detector_->detectConfidence(text1);
    detector_->detectConfidence(text2);
    
    auto stats = detector_->getStatistics();
    
    EXPECT_EQ(stats.total_detections, 2);
    EXPECT_GE(stats.patronizing_detected, 1);
}

TEST_F(EthicsAwareConfidenceDetectorTest, StatisticsReset) {
    std::string text = "You must do this.";
    detector_->detectConfidence(text);
    
    detector_->resetStatistics();
    auto stats = detector_->getStatistics();
    
    EXPECT_EQ(stats.total_detections, 0);
    EXPECT_EQ(stats.patronizing_detected, 0);
}

// ═══════════════════════════════════════════════════════════
// Cache Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, CacheHit) {
    std::string text = "You might consider this.";
    
    // First detection
    auto result1 = detector_->detectConfidence(text);
    auto stats1 = detector_->getStatistics();
    
    // Second detection (should hit cache)
    auto result2 = detector_->detectConfidence(text);
    auto stats2 = detector_->getStatistics();
    
    EXPECT_GT(stats2.cache_hits, stats1.cache_hits);
}

TEST_F(EthicsAwareConfidenceDetectorTest, CacheClear) {
    std::string text = "You might consider this.";
    detector_->detectConfidence(text);
    
    detector_->clearCache();
    
    // After clear, should be cache miss
    detector_->detectConfidence(text);
    auto stats = detector_->getStatistics();
    
    EXPECT_GT(stats.cache_misses, 0);
}

// ═══════════════════════════════════════════════════════════
// Factory Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, FactoryDefault) {
    auto detector = ConfidenceDetectorFactory::createDefault();
    ASSERT_NE(detector, nullptr);
    
    auto config = detector->getConfig();
    EXPECT_FLOAT_EQ(config.min_autonomy_respect, 0.7f);
}

TEST_F(EthicsAwareConfidenceDetectorTest, FactoryStrict) {
    auto detector = ConfidenceDetectorFactory::createStrict();
    ASSERT_NE(detector, nullptr);
    
    auto config = detector->getConfig();
    EXPECT_GT(config.min_autonomy_respect, 0.8f);
}

TEST_F(EthicsAwareConfidenceDetectorTest, FactoryLenient) {
    auto detector = ConfidenceDetectorFactory::createLenient();
    ASSERT_NE(detector, nullptr);
    
    auto config = detector->getConfig();
    EXPECT_LT(config.min_autonomy_respect, 0.7f);
}

// ═══════════════════════════════════════════════════════════
// Multilingual Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, MultilingualPatronizing) {
    // English
    std::string text_en = "You must do this. You have to follow.";
    auto result_en = detector_->detectConfidence(text_en);
    EXPECT_TRUE(result_en.has_patronizing_language);
    
    // German
    std::string text_de = "Sie müssen das tun. Du musst folgen.";
    auto result_de = detector_->detectConfidence(text_de);
    EXPECT_TRUE(result_de.has_patronizing_language);
}

TEST_F(EthicsAwareConfidenceDetectorTest, MultilingualHedgeWords) {
    // English
    std::string text_en = "This might be true. It could work.";
    auto result_en = detector_->detectConfidence(text_en);
    EXPECT_TRUE(result_en.acknowledges_uncertainty);
    
    // German  
    std::string text_de = "Das könnte wahr sein. Es ist möglich.";
    auto result_de = detector_->detectConfidence(text_de);
    EXPECT_TRUE(result_de.acknowledges_uncertainty);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(EthicsAwareConfidenceDetectorTest, EmptyText) {
    std::string text = "";
    auto result = detector_->detectConfidence(text);
    
    EXPECT_GE(result.combined_confidence, 0.0f);
    EXPECT_LE(result.combined_confidence, 1.0f);
}

TEST_F(EthicsAwareConfidenceDetectorTest, VeryLongText) {
    std::string text(10000, 'x'); // Very long text
    auto result = detector_->detectConfidence(text);
    
    EXPECT_GE(result.combined_confidence, 0.0f);
    EXPECT_LE(result.combined_confidence, 1.0f);
}
