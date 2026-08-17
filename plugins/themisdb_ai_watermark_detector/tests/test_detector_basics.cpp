/**
 * @file test_detector_basics.cpp
 * @brief Basic unit tests for WatermarkDetector interface.
 *
 * Tests Phase 1 API contract: interface creation, configuration, basic detection.
 */

#include <gtest/gtest.h>
#include "../include/detector_interface.h"
#include "../include/detection_config.h"
#include "../include/detection_result.h"
#include <iostream>

using namespace themisdb::watermark;

/**
 * @class WatermarkDetectorTest
 * @brief Test suite for WatermarkDetector Phase 1 API.
 */
class WatermarkDetectorTest : public ::testing::Test {
protected:
    std::unique_ptr<WatermarkDetector> detector_;

    void SetUp() override {
        try {
            detector_ = WatermarkDetectorFactory::create();
            ASSERT_TRUE(detector_ != nullptr);
            ASSERT_TRUE(detector_->is_initialized());
        } catch (const std::exception& e) {
            FAIL() << "Failed to create WatermarkDetector: " << e.what();
        }
    }

    void TearDown() override {
        if (detector_) {
            detector_->clear_cache();
        }
    }
};

// ============================================================================
// Test 1: Detector Creation
// ============================================================================

TEST_F(WatermarkDetectorTest, CreateDefaultDetector) {
    EXPECT_TRUE(detector_ != nullptr);
    EXPECT_TRUE(detector_->is_initialized());
}

TEST_F(WatermarkDetectorTest, GetVersion) {
    std::string version = detector_->get_version();
    EXPECT_FALSE(version.empty());
    EXPECT_NE(version.find("WatermarkDetector"), std::string::npos);
}

TEST_F(WatermarkDetectorTest, GetSupportedModels) {
    auto models = detector_->get_supported_models();
    EXPECT_FALSE(models.empty());
    // Phase 1: at least Claude
    EXPECT_NE(models.end(),
              std::find(models.begin(), models.end(), AIModelFamily::Claude));
}

// ============================================================================
// Test 2: Configuration
// ============================================================================

TEST_F(WatermarkDetectorTest, ConfigureWithDefaults) {
    DetectionConfig config;
    EXPECT_NO_THROW(detector_->configure(config));
}

TEST_F(WatermarkDetectorTest, ConfigureValidation) {
    DetectionConfig config;
    config.confidence_threshold = 1.5f;  // Invalid: > 1.0
    EXPECT_THROW(detector_->configure(config), std::invalid_argument);
}

TEST_F(WatermarkDetectorTest, ConfigureWithCustomThreshold) {
    DetectionConfig config;
    config.confidence_threshold = 0.75f;
    EXPECT_NO_THROW(detector_->configure(config));
}

TEST_F(WatermarkDetectorTest, ConfigureWithCaching) {
    DetectionConfig config;
    config.enable_caching = true;
    config.cache_max_entries = 5000;
    EXPECT_NO_THROW(detector_->configure(config));
}

// ============================================================================
// Test 3: Basic Detection
// ============================================================================

TEST_F(WatermarkDetectorTest, DetectEmptyText) {
    DetectionConfig config;
    config.confidence_threshold = 0.85f;
    detector_->configure(config);

    DetectionResult result = detector_->detect_text("");
    // Empty text should fail validation
    EXPECT_NE(result.status, DetectionStatus::Success);
    EXPECT_EQ(result.confidence_score, 0.5f);
}

TEST_F(WatermarkDetectorTest, DetectSimpleText) {
    DetectionConfig config;
    config.confidence_threshold = 0.85f;
    detector_->configure(config);

    DetectionResult result = detector_->detect_text("Hello world. This is a test.");
    EXPECT_GE(result.confidence_score, 0.0f);
    EXPECT_LE(result.confidence_score, 1.0f);
}

TEST_F(WatermarkDetectorTest, DetectLongerText) {
    DetectionConfig config;
    config.confidence_threshold = 0.85f;
    detector_->configure(config);

    std::string text =
        "This is a longer piece of text that should be analyzed by the watermark "
        "detector. It contains multiple sentences and ideas to provide enough context "
        "for the detector to make meaningful decisions. The detector should be able to "
        "analyze this text and produce a confidence score.";

    DetectionResult result = detector_->detect_text(text);
    EXPECT_GE(result.confidence_score, 0.0f);
    EXPECT_LE(result.confidence_score, 1.0f);
    EXPECT_EQ(result.text_length_chars, text.size());
}

TEST_F(WatermarkDetectorTest, DetectWithSourceId) {
    DetectionConfig config;
    config.confidence_threshold = 0.85f;
    detector_->configure(config);

    DetectionResult result = detector_->detect_text("Test text", "source_123");
    EXPECT_EQ(result.source_id, "source_123");
    EXPECT_GE(result.detection_duration_ms, 0UL);
}

// ============================================================================
// Test 4: Confidence Score Range
// ============================================================================

TEST_F(WatermarkDetectorTest, ConfidenceScoreRange) {
    DetectionConfig config;
    config.confidence_threshold = 0.5f;  // Lower threshold
    detector_->configure(config);

    std::vector<std::string> texts = {
        "Short.",
        "This is a medium length text with multiple sentences and ideas.",
        "This is a very long piece of text that contains much more content and should "
        "provide better confidence scoring due to its length and complexity. It covers "
        "multiple topics and provides enough information for thorough analysis. The "
        "detector should find this text suitable for reliable scoring."};

    for (const auto& text : texts) {
        DetectionResult result = detector_->detect_text(text);
        EXPECT_GE(result.confidence_score, 0.0f) << "Text: " << text;
        EXPECT_LE(result.confidence_score, 1.0f) << "Text: " << text;
    }
}

// ============================================================================
// Test 5: Caching
// ============================================================================

TEST_F(WatermarkDetectorTest, CachingDisabled) {
    DetectionConfig config;
    config.enable_caching = false;
    detector_->configure(config);

    DetectionResult result1 = detector_->detect_text("Test text");
    EXPECT_FALSE(result1.from_cache);

    DetectionResult result2 = detector_->detect_text("Test text");
    EXPECT_FALSE(result2.from_cache);  // Should not be cached
}

TEST_F(WatermarkDetectorTest, CachingEnabled) {
    DetectionConfig config;
    config.enable_caching = true;
    config.cache_max_entries = 100;
    detector_->configure(config);

    DetectionResult result1 = detector_->detect_text("Test text");
    EXPECT_FALSE(result1.from_cache);  // First time

    DetectionResult result2 = detector_->detect_text("Test text");
    // Second call might be cached (if tokenization not stubbed)
    // For now, just check consistency
    EXPECT_EQ(result1.confidence_score, result2.confidence_score);
}

TEST_F(WatermarkDetectorTest, ClearCache) {
    DetectionConfig config;
    config.enable_caching = true;
    detector_->configure(config);

    detector_->detect_text("Test text 1");
    detector_->detect_text("Test text 2");

    std::string stats_before = detector_->get_cache_stats();
    EXPECT_FALSE(stats_before.empty());

    detector_->clear_cache();

    std::string stats_after = detector_->get_cache_stats();
    EXPECT_NE(stats_before, stats_after);
}

// ============================================================================
// Test 6: Batch Detection
// ============================================================================

TEST_F(WatermarkDetectorTest, BatchDetectionEmpty) {
    DetectionConfig config;
    detector_->configure(config);

    std::vector<std::string> texts;
    auto results = detector_->detect_batch(texts);
    EXPECT_TRUE(results.empty());
}

TEST_F(WatermarkDetectorTest, BatchDetectionMultiple) {
    DetectionConfig config;
    detector_->configure(config);

    std::vector<std::string> texts = {
        "Text one.",
        "Text two with more content.",
        "Text three is the longest and should provide reasonable confidence scoring "
        "due to its greater length."};

    auto results = detector_->detect_batch(texts);
    EXPECT_EQ(results.size(), texts.size());

    for (const auto& result : results) {
        EXPECT_GE(result.confidence_score, 0.0f);
        EXPECT_LE(result.confidence_score, 1.0f);
    }
}

TEST_F(WatermarkDetectorTest, BatchDetectionWithSourceIds) {
    DetectionConfig config;
    detector_->configure(config);

    std::vector<std::string> texts = {"Text 1", "Text 2"};
    std::vector<std::string> source_ids = {"src_1", "src_2"};

    auto results = detector_->detect_batch(texts, source_ids);
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].source_id, "src_1");
    EXPECT_EQ(results[1].source_id, "src_2");
}

TEST_F(WatermarkDetectorTest, BatchDetectionSourceIdMismatch) {
    DetectionConfig config;
    detector_->configure(config);

    std::vector<std::string> texts = {"Text 1", "Text 2"};
    std::vector<std::string> source_ids = {"src_1"};  // Mismatch: 1 vs 2

    EXPECT_THROW(detector_->detect_batch(texts, source_ids),
                 std::invalid_argument);
}

// ============================================================================
// Test 7: Detection Result Structure
// ============================================================================

TEST_F(WatermarkDetectorTest, DetectionResultToJson) {
    DetectionConfig config;
    detector_->configure(config);

    DetectionResult result = detector_->detect_text("Test text");
    std::string json = result.to_json();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("confidence_score"), std::string::npos);
}

TEST_F(WatermarkDetectorTest, DetectionResultIsReliable) {
    DetectionConfig config;
    config.confidence_threshold = 0.85f;
    detector_->configure(config);

    DetectionResult result = detector_->detect_text(
        "This is a reasonably long text that should produce a valid confidence score "
        "with the detector. It has enough content to be analyzed properly.");

    // Reliability depends on implementation details (Phase 2+)
    // Just test the method doesn't crash
    EXPECT_NO_THROW(result.is_reliable());
}

// ============================================================================
// Test 8: Thread Safety (Basic)
// ============================================================================

TEST_F(WatermarkDetectorTest, ConcurrentDetection) {
    DetectionConfig config;
    config.enable_caching = true;
    detector_->configure(config);

    std::string text1 = "First text for concurrent testing.";
    std::string text2 = "Second text for concurrent testing.";

    // Simple sequential simulation (true concurrent test in Phase 5)
    DetectionResult result1 = detector_->detect_text(text1);
    DetectionResult result2 = detector_->detect_text(text2);

    EXPECT_GE(result1.confidence_score, 0.0f);
    EXPECT_LE(result1.confidence_score, 1.0f);
    EXPECT_GE(result2.confidence_score, 0.0f);
    EXPECT_LE(result2.confidence_score, 1.0f);
}

// ============================================================================
// Test 9: Reset & Reinitialization
// ============================================================================

TEST_F(WatermarkDetectorTest, ResetDetector) {
    DetectionConfig config;
    config.enable_caching = true;
    detector_->configure(config);

    detector_->detect_text("Text 1");
    detector_->detect_text("Text 2");

    std::string stats_before = detector_->get_cache_stats();

    detector_->reset();

    std::string stats_after = detector_->get_cache_stats();
    // After reset, cache should be cleared
    EXPECT_NE(stats_before, stats_after);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
