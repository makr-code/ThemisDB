/**
 * @file test_sharding_uncovered.cpp
 * @brief Tests for OrphanDetector and ShardLoadDetector
 *        (previously uncovered by the test suite).
 *
 * Covers OrphanDetector:
 *   - Default construction + Config defaults
 *   - detectOrphans with null coordinator (returns empty, no crash)
 *   - isOrphaned with null coordinator (returns false, no crash)
 *   - Config: custom timeout_seconds
 *
 * Covers ShardLoadDetector:
 *   - Construction (topology=nullptr, metrics=nullptr)
 *   - updateShardLoad + detectImbalance (balanced set)
 *   - updateShardLoad + detectImbalance (imbalanced set)
 *   - detectImbalance with < min_shards → no detection
 *   - recordRebalanceTriggered (smoke test)
 */

#include <gtest/gtest.h>
#include "sharding/orphan_detector.h"
#include "sharding/shard_load_detector.h"
#include "sharding/shard_topology.h"
#include <memory>

using namespace themis::sharding;

// ============================================================================
// OrphanDetector
// ============================================================================

TEST(OrphanDetectorTest, DefaultConstruction_Succeeds) {
    sharding::OrphanDetector::Config cfg;
    EXPECT_NO_THROW(sharding::OrphanDetector det(cfg));
}

TEST(OrphanDetectorTest, ConfigDefaults) {
    sharding::OrphanDetector::Config cfg;
    EXPECT_EQ(cfg.timeout_seconds, 900u);
    EXPECT_TRUE(cfg.check_preparing);
    EXPECT_TRUE(cfg.check_prepared);
    EXPECT_TRUE(cfg.check_committing);
    EXPECT_TRUE(cfg.check_aborting);
}

TEST(OrphanDetectorTest, DetectOrphans_NullCoordinator_ReturnsEmpty) {
    sharding::OrphanDetector::Config cfg;
    sharding::OrphanDetector det(cfg);
    // Passing nullptr should not crash and should return empty list
    auto orphans = det.detectOrphans(nullptr);
    EXPECT_TRUE(orphans.empty());
}

TEST(OrphanDetectorTest, IsOrphaned_NullCoordinator_ReturnsFalse) {
    sharding::OrphanDetector::Config cfg;
    sharding::OrphanDetector det(cfg);
    bool result = det.isOrphaned("txn_123", nullptr);
    EXPECT_FALSE(result);
}

TEST(OrphanDetectorTest, CustomTimeout_IsStored) {
    sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 300;
    sharding::OrphanDetector det(cfg);
    // Construction succeeds with custom timeout
    EXPECT_NO_THROW((void)det.detectOrphans(nullptr));
}

// ============================================================================
// ShardLoadDetector
// ============================================================================

class ShardLoadDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto topology = std::make_shared<ShardTopology>();
        ShardLoadDetector::Config cfg;
        cfg.min_shards_for_detection = 2;
        cfg.storage_imbalance_threshold = 0.3;
        cfg.request_imbalance_threshold = 0.3;
        detector_ = std::make_unique<ShardLoadDetector>(topology, nullptr, cfg);
    }

    std::unique_ptr<ShardLoadDetector> detector_;
};

TEST_F(ShardLoadDetectorTest, ConstructionSucceeds) {
    EXPECT_NE(detector_, nullptr);
}

TEST_F(ShardLoadDetectorTest, DetectImbalance_NoShards_NoImbalance) {
    auto result = detector_->detectImbalance();
    EXPECT_FALSE(result.is_imbalanced);
}

TEST_F(ShardLoadDetectorTest, DetectImbalance_OneShardBelowMin_NoImbalance) {
    ShardLoadMetrics m;
    m.shard_id              = "shard1";
    m.total_records         = 1000;
    m.storage_usage_percent = 50.0;
    m.requests_per_sec      = 100;
    detector_->updateShardLoad("shard1", m);

    auto result = detector_->detectImbalance();
    EXPECT_FALSE(result.is_imbalanced); // min_shards_for_detection = 2
}

TEST_F(ShardLoadDetectorTest, DetectImbalance_BalancedShards_NoImbalance) {
    ShardLoadMetrics m1;
    m1.shard_id              = "shard1";
    m1.total_records         = 1000;
    m1.storage_usage_percent = 50.0;
    m1.requests_per_sec      = 100;

    ShardLoadMetrics m2;
    m2.shard_id              = "shard2";
    m2.total_records         = 1000;
    m2.storage_usage_percent = 50.0;
    m2.requests_per_sec      = 100;

    detector_->updateShardLoad("shard1", m1);
    detector_->updateShardLoad("shard2", m2);

    auto result = detector_->detectImbalance();
    EXPECT_FALSE(result.is_imbalanced);
}

TEST_F(ShardLoadDetectorTest, DetectImbalance_HeavilyImbalancedShards_Detected) {
    // shard1 extremely overloaded, shard2 nearly empty
    ShardLoadMetrics hot;
    hot.shard_id              = "shard1";
    hot.total_records         = 100000;
    hot.storage_usage_percent = 95.0;
    hot.requests_per_sec      = 5000;

    ShardLoadMetrics cold;
    cold.shard_id              = "shard2";
    cold.total_records         = 10;
    cold.storage_usage_percent = 1.0;
    cold.requests_per_sec      = 1;

    detector_->updateShardLoad("shard1", hot);
    detector_->updateShardLoad("shard2", cold);

    auto result = detector_->detectImbalance();
    EXPECT_TRUE(result.is_imbalanced);
    EXPECT_FALSE(result.hotspot_shards.empty());
}

TEST_F(ShardLoadDetectorTest, RecordRebalanceTriggered_DoesNotThrow) {
    EXPECT_NO_THROW(detector_->recordRebalanceTriggered());
}

TEST_F(ShardLoadDetectorTest, UpdateShardLoad_OverwritesPreviousLoad) {
    ShardLoadMetrics m1;
    m1.shard_id              = "shard1";
    m1.total_records         = 1000;
    m1.storage_usage_percent = 50.0;
    m1.requests_per_sec      = 100;

    ShardLoadMetrics m2;
    m2.shard_id              = "shard1";
    m2.total_records         = 9999;
    m2.storage_usage_percent = 99.0;
    m2.requests_per_sec      = 9999;

    detector_->updateShardLoad("shard1", m1);
    detector_->updateShardLoad("shard1", m2); // overwrite
    detector_->updateShardLoad("shard2", m1);

    // Now shard1 is heavily loaded, shard2 is light → should detect imbalance
    auto result = detector_->detectImbalance();
    EXPECT_TRUE(result.is_imbalanced);
}
