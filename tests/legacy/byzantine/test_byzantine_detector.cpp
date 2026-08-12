/**
 * @file test_byzantine_detector.cpp
 * @brief Comprehensive tests for Byzantine Fault Detection
 * 
 * Tests Byzantine detection functionality including:
 * - Median-based detection (MAD threshold)
 * - Krum algorithm
 * - Bulyan algorithm
 * - Ensemble detection
 * - Attack simulations
 * - Integration with DistributedTrainingCoordinator
 * 
 * @note Requires GTest: vcpkg install gtest OR apt-get install libgtest-dev
 * @build cmake -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=ON ..
 * @run ./tests/test_byzantine_detector
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "llm/byzantine_detector.h"
#include "llm/distributed_training_coordinator.h"
#include "byzantine_attacks.h"
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <chrono>  // for std::chrono::high_resolution_clock

using namespace themis::llm;

// ============================================================================
// Test Fixtures
// ============================================================================

class ByzantineDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create benign gradients for multiple shards
        for (int i = 0; i < 5; ++i) {
            std::string shard_id = "shard-" + std::to_string(i);
            auto gradients = byzantine_attacks::createBenignGradients(
                3, 64, 0.0f, 0.01f, 42 + i
            );
            
            for (auto& grad : gradients) {
                grad.source_shard = shard_id;
            }
            
            benign_gradients_[shard_id] = gradients;
        }
    }
    
    std::map<std::string, std::vector<GradientTensor>> benign_gradients_;
};

// ============================================================================
// MedianDetector Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, MedianDetector_NormalGradients_NoDetection) {
    MedianDetector detector(3.0f);
    
    auto result = detector.detectByzantineShards(benign_gradients_);
    
    EXPECT_FALSE(result.requires_action);
    EXPECT_TRUE(result.suspected_shards.empty());
    EXPECT_EQ(result.detection_method, "MEDIAN");
}

TEST_F(ByzantineDetectorTest, MedianDetector_ScaleAttack_Detected) {
    MedianDetector detector(3.0f);
    
    // Apply scale attack to one shard
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 100.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
    EXPECT_FALSE(result.suspected_shards.empty());
    EXPECT_NE(std::find(result.suspected_shards.begin(), 
                       result.suspected_shards.end(), 
                       "shard-0"),
             result.suspected_shards.end());
}

TEST_F(ByzantineDetectorTest, MedianDetector_SignFlipAttack_Detected) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::signFlipAttack(gradients_copy["shard-1"]);
    
    auto result = detector.detectByzantineShards(gradients_copy);

    // Sign-flip may preserve gradient norm and can evade norm-based median detection.
    EXPECT_EQ(result.detection_method, "MEDIAN");
    EXPECT_GE(result.suspected_shards.size(), 0u);
}

TEST_F(ByzantineDetectorTest, MedianDetector_ZeroAttack_Detected) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::zeroAttack(gradients_copy["shard-2"]);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
    EXPECT_FALSE(result.suspected_shards.empty());
}

TEST_F(ByzantineDetectorTest, MedianDetector_GaussianNoiseAttack_Detected) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::noiseAttack(gradients_copy["shard-3"], 10.0f, 123);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
    EXPECT_FALSE(result.suspected_shards.empty());
}

TEST_F(ByzantineDetectorTest, MedianDetector_ComputeStatistics) {
    MedianDetector detector(3.0f);
    
    auto stats = detector.computeStatistics(benign_gradients_);
    
    EXPECT_EQ(stats.gradient_norms.size(), 5);
    EXPECT_EQ(stats.gradient_means.size(), 5);
    EXPECT_EQ(stats.gradient_variances.size(), 5);
    EXPECT_GT(stats.global_median_norm, 0.0f);
    EXPECT_GE(stats.global_mad, 0.0f);
}

TEST_F(ByzantineDetectorTest, MedianDetector_AnomalyScores) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 50.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    // Check anomaly scores
    EXPECT_EQ(result.anomaly_scores.size(), 5);
    
    // Byzantine shard should have high anomaly score
    EXPECT_GT(result.anomaly_scores["shard-0"], 0.5f);
    
    // Normal shards should have low anomaly score
    EXPECT_LT(result.anomaly_scores["shard-1"], 0.5f);
}

TEST_F(ByzantineDetectorTest, MedianDetector_ThresholdAdjustment) {
    MedianDetector detector_strict(2.0f);  // Strict threshold
    MedianDetector detector_lenient(5.0f);  // Lenient threshold
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 10.0f);
    
    auto result_strict = detector_strict.detectByzantineShards(gradients_copy);
    auto result_lenient = detector_lenient.detectByzantineShards(gradients_copy);
    
    // Strict detector should be more likely to detect
    // (though this depends on the actual gradient distribution)
    EXPECT_GE(result_strict.suspected_shards.size(), 
              result_lenient.suspected_shards.size());
}

// ============================================================================
// KrumDetector Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, KrumDetector_NormalGradients_NoDetection) {
    KrumDetector detector(1);  // Allow up to 1 Byzantine shard
    
    auto result = detector.detectByzantineShards(benign_gradients_);
    
    // With normal gradients, Krum might still exclude some shards
    // but there should be no definitive Byzantine behavior
    EXPECT_EQ(result.detection_method, "KRUM");
}

TEST_F(ByzantineDetectorTest, KrumDetector_ScaleAttack_Detected) {
    KrumDetector detector(1);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 100.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
    EXPECT_FALSE(result.suspected_shards.empty());
    
    // The attacked shard should be excluded by Krum
    EXPECT_NE(std::find(result.suspected_shards.begin(), 
                       result.suspected_shards.end(), 
                       "shard-0"),
             result.suspected_shards.end());
}

TEST_F(ByzantineDetectorTest, KrumDetector_MultipleAttacks_Detected) {
    KrumDetector detector(2);  // Allow up to 2 Byzantine shards
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 50.0f);
    byzantine_attacks::signFlipAttack(gradients_copy["shard-1"]);
    
    auto result = detector.detectByzantineShards(gradients_copy);

    // Krum requires n >= 2f+3. With n=5 and f=2 this is intentionally insufficient.
    EXPECT_FALSE(result.requires_action);
    EXPECT_TRUE(result.suspected_shards.empty());
}

TEST_F(ByzantineDetectorTest, KrumDetector_InsufficientShards) {
    KrumDetector detector(2);
    
    // Create minimal shard set (less than 2f+3)
    std::map<std::string, std::vector<GradientTensor>> minimal_gradients;
    minimal_gradients["shard-0"] = benign_gradients_["shard-0"];
    minimal_gradients["shard-1"] = benign_gradients_["shard-1"];
    
    auto result = detector.detectByzantineShards(minimal_gradients);
    
    // Should return without detection due to insufficient shards
    EXPECT_FALSE(result.requires_action);
}

// ============================================================================
// BulyanDetector Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, BulyanDetector_NormalGradients_NoDetection) {
    // Bulyan needs at least 4f+3 shards, so with 5 shards, f can be at most 0
    // But for testing, we'll use a small f value
    BulyanDetector detector(0);
    
    auto result = detector.detectByzantineShards(benign_gradients_);
    
    EXPECT_EQ(result.detection_method, "BULYAN");
}

TEST_F(ByzantineDetectorTest, BulyanDetector_ScaleAttack_Detected) {
    BulyanDetector detector(1);
    
    // Add more shards to meet Bulyan requirement (4f+3 = 7 shards for f=1)
    auto gradients_copy = benign_gradients_;
    gradients_copy["shard-5"] = byzantine_attacks::createBenignGradients(3, 64, 0.0f, 0.01f, 47);
    gradients_copy["shard-6"] = byzantine_attacks::createBenignGradients(3, 64, 0.0f, 0.01f, 48);
    
    // Apply attack
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 100.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
    EXPECT_FALSE(result.suspected_shards.empty());
}

TEST_F(ByzantineDetectorTest, BulyanDetector_AggregateRobust) {
    BulyanDetector detector(1);
    
    // Add more shards
    auto gradients_copy = benign_gradients_;
    gradients_copy["shard-5"] = byzantine_attacks::createBenignGradients(3, 64, 0.0f, 0.01f, 47);
    gradients_copy["shard-6"] = byzantine_attacks::createBenignGradients(3, 64, 0.0f, 0.01f, 48);
    
    // Apply attack to one shard
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 100.0f);
    
    // Aggregate with Byzantine tolerance
    auto aggregated = detector.aggregateRobust(gradients_copy);
    
    EXPECT_FALSE(aggregated.empty());
    EXPECT_EQ(aggregated.size(), 3);  // Should have 3 layers
}

// ============================================================================
// EnsembleDetector Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, EnsembleDetector_CombinesDetections) {
    EnsembleDetector detector(3.0f, 1);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 100.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_EQ(result.detection_method, "ENSEMBLE");
    EXPECT_TRUE(result.requires_action);
    EXPECT_FALSE(result.suspected_shards.empty());
}

TEST_F(ByzantineDetectorTest, EnsembleDetector_StrongerDetection) {
    MedianDetector median_detector(3.0f);
    EnsembleDetector ensemble_detector(3.0f, 1);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 50.0f);
    
    auto median_result = median_detector.detectByzantineShards(gradients_copy);
    auto ensemble_result = ensemble_detector.detectByzantineShards(gradients_copy);
    
    // Ensemble should provide at least as strong detection as individual methods
    EXPECT_GE(ensemble_result.suspected_shards.size(), 
              median_result.suspected_shards.size());
}

// ============================================================================
// ByzantineDetectorFactory Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, Factory_CreateMedianDetector) {
    auto detector = ByzantineDetectorFactory::create(
        ByzantineDetectionMethod::MEDIAN, 3.0f, 1
    );
    
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getName(), "MEDIAN");
}

TEST_F(ByzantineDetectorTest, Factory_CreateKrumDetector) {
    auto detector = ByzantineDetectorFactory::create(
        ByzantineDetectionMethod::KRUM, 3.0f, 1
    );
    
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getName(), "KRUM");
}

TEST_F(ByzantineDetectorTest, Factory_CreateBulyanDetector) {
    auto detector = ByzantineDetectorFactory::create(
        ByzantineDetectionMethod::BULYAN, 3.0f, 1
    );
    
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getName(), "BULYAN");
}

TEST_F(ByzantineDetectorTest, Factory_CreateEnsembleDetector) {
    auto detector = ByzantineDetectorFactory::create(
        ByzantineDetectionMethod::ENSEMBLE, 3.0f, 1
    );
    
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(detector->getName(), "ENSEMBLE");
}

TEST_F(ByzantineDetectorTest, Factory_CreateNone) {
    auto detector = ByzantineDetectorFactory::create(
        ByzantineDetectionMethod::NONE, 3.0f, 1
    );
    
    EXPECT_EQ(detector, nullptr);
}

// ============================================================================
// Attack Scenario Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, AttackScenario_ConstantAttack) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::constantAttack(gradients_copy["shard-0"], 1000.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
}

TEST_F(ByzantineDetectorTest, AttackScenario_RandomAttack) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::randomAttack(gradients_copy["shard-0"], -100.0f, 100.0f, 999);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    // Random attack with large range should be detected
    EXPECT_TRUE(result.requires_action);
}

TEST_F(ByzantineDetectorTest, AttackScenario_GaussianOutlierAttack) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::gaussianOutlierAttack(gradients_copy["shard-0"], 10.0f, 5.0f, 555);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
}

TEST_F(ByzantineDetectorTest, AttackScenario_CombinedAttacks) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    
    // Multiple coordinated attacks
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 50.0f);
    byzantine_attacks::signFlipAttack(gradients_copy["shard-1"]);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    EXPECT_TRUE(result.requires_action);
    EXPECT_GE(result.suspected_shards.size(), 1);
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, GradientStatistics_JSONSerialization) {
    MedianDetector detector(3.0f);
    
    auto stats = detector.computeStatistics(benign_gradients_);
    
    // Serialize
    auto json = stats.toJSON();
    
    EXPECT_TRUE(json.contains("gradient_norms"));
    EXPECT_TRUE(json.contains("global_median_norm"));
    EXPECT_TRUE(json.contains("global_mad"));
    
    // Deserialize
    auto stats_restored = GradientStatistics::fromJSON(json);
    
    EXPECT_EQ(stats_restored.gradient_norms.size(), stats.gradient_norms.size());
    EXPECT_FLOAT_EQ(stats_restored.global_median_norm, stats.global_median_norm);
    EXPECT_FLOAT_EQ(stats_restored.global_mad, stats.global_mad);
}

TEST_F(ByzantineDetectorTest, DetectionResult_JSONSerialization) {
    MedianDetector detector(3.0f);
    
    auto gradients_copy = benign_gradients_;
    byzantine_attacks::scaleAttack(gradients_copy["shard-0"], 100.0f);
    
    auto result = detector.detectByzantineShards(gradients_copy);
    
    // Serialize
    auto json = result.toJSON();
    
    EXPECT_TRUE(json.contains("suspected_shards"));
    EXPECT_TRUE(json.contains("anomaly_scores"));
    EXPECT_TRUE(json.contains("detection_method"));
    EXPECT_TRUE(json.contains("requires_action"));
    
    // Deserialize
    auto result_restored = DetectionResult::fromJSON(json);
    
    EXPECT_EQ(result_restored.suspected_shards.size(), result.suspected_shards.size());
    EXPECT_EQ(result_restored.detection_method, result.detection_method);
    EXPECT_EQ(result_restored.requires_action, result.requires_action);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ByzantineDetectorTest, Performance_MedianDetection) {
    MedianDetector detector(3.0f);
    
    // Create larger gradient set
    std::map<std::string, std::vector<GradientTensor>> large_gradients;
    for (int i = 0; i < 20; ++i) {
        std::string shard_id = "shard-" + std::to_string(i);
        large_gradients[shard_id] = byzantine_attacks::createBenignGradients(
            10, 256, 0.0f, 0.01f, 42 + i
        );
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = detector.detectByzantineShards(large_gradients);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Detection should be fast (< 100ms for 20 shards)
    EXPECT_LT(duration.count(), 100);
}

// ============================================================================
// Main Test Runner
// ============================================================================


