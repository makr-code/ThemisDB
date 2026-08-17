// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lag_alert_manager.cpp
 * @brief Unit tests for LagAlertManager.
 */

#include "replication/lag_alert_manager.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>

namespace themisdb {
namespace replication {

// Test fixture
class LagAlertManagerTest : public ::testing::Test {
protected:
    LagAlertManager manager_;
    std::vector<AlertEvent> captured_alerts_;

    void SetUp() override
    {
        manager_.setAlertCallback([this](const AlertEvent& evt) {
            captured_alerts_.push_back(evt);
        });
    }

    void TearDown() override
    {
        captured_alerts_.clear();
        manager_.clearAllReplicas();
    }
};

// ============================================================================
// Configuration tests
// ============================================================================

TEST_F(LagAlertManagerTest, DefaultThresholds)
{
    const auto thresholds = manager_.thresholds();
    EXPECT_EQ(thresholds.alert_threshold_ms, 10'000);
    EXPECT_EQ(thresholds.critical_threshold_ms, 30'000);
    EXPECT_EQ(thresholds.failover_threshold_ms, 60'000);
    EXPECT_EQ(thresholds.failover_duration_ms, 5 * 60'000);
}

TEST_F(LagAlertManagerTest, SetCustomThresholds)
{
    SLOThresholds custom;
    custom.alert_threshold_ms = 5'000;
    custom.critical_threshold_ms = 15'000;
    custom.failover_threshold_ms = 45'000;
    custom.failover_duration_ms = 3 * 60'000;

    manager_.setThresholds(custom);

    const auto retrieved = manager_.thresholds();
    EXPECT_EQ(retrieved.alert_threshold_ms, 5'000);
    EXPECT_EQ(retrieved.critical_threshold_ms, 15'000);
}

// ============================================================================
// Replica lag update tests
// ============================================================================

TEST_F(LagAlertManagerTest, UpdateSingleReplica)
{
    manager_.updateReplicaLag("replica-1", 5'000);
    EXPECT_EQ(manager_.getReplicaLag("replica-1"), 5'000);
}

TEST_F(LagAlertManagerTest, UpdateMultipleReplicas)
{
    std::map<std::string, int64_t> lags;
    lags["replica-1"] = 3'000;
    lags["replica-2"] = 8'000;
    lags["replica-3"] = 15'000;

    manager_.updateReplicaLags(lags);

    EXPECT_EQ(manager_.getReplicaLag("replica-1"), 3'000);
    EXPECT_EQ(manager_.getReplicaLag("replica-2"), 8'000);
    EXPECT_EQ(manager_.getReplicaLag("replica-3"), 15'000);
}

TEST_F(LagAlertManagerTest, UpdateNonexistentReplica)
{
    EXPECT_EQ(manager_.getReplicaLag("unknown"), 0);
}

TEST_F(LagAlertManagerTest, RemoveReplica)
{
    manager_.updateReplicaLag("replica-1", 5'000);
    EXPECT_EQ(manager_.getReplicaLag("replica-1"), 5'000);

    manager_.removeReplica("replica-1");
    EXPECT_EQ(manager_.getReplicaLag("replica-1"), 0);
}

TEST_F(LagAlertManagerTest, ClearAllReplicas)
{
    manager_.updateReplicaLag("replica-1", 1'000);
    manager_.updateReplicaLag("replica-2", 2'000);

    manager_.clearAllReplicas();

    EXPECT_EQ(manager_.getReplicaLag("replica-1"), 0);
    EXPECT_EQ(manager_.getReplicaLag("replica-2"), 0);
}

// ============================================================================
// Alert threshold tests
// ============================================================================

TEST_F(LagAlertManagerTest, AlertThresholdTriggers)
{
    manager_.updateReplicaLag("replica-1", 5'000);
    manager_.checkAndAlertLagViolations();
    EXPECT_EQ(captured_alerts_.size(), 0);  // Below threshold

    manager_.updateReplicaLag("replica-1", 10'001);  // Exceed alert threshold
    manager_.checkAndAlertLagViolations();
    EXPECT_EQ(captured_alerts_.size(), 1);
    EXPECT_EQ(captured_alerts_[0].level, AlertEvent::Level::ALERT);
    EXPECT_EQ(captured_alerts_[0].replica_id, "replica-1");
    EXPECT_EQ(captured_alerts_[0].lag_ms, 10'001);
}

TEST_F(LagAlertManagerTest, AlertThresholdRecovery)
{
    // Trigger alert
    manager_.updateReplicaLag("replica-1", 10'001);
    manager_.checkAndAlertLagViolations();
    EXPECT_EQ(captured_alerts_.size(), 1);

    // Drop back below threshold
    manager_.updateReplicaLag("replica-1", 5'000);
    manager_.checkAndAlertLagViolations();
    // Alert should clear, no new events
    EXPECT_EQ(captured_alerts_.size(), 1);

    auto in_alert = manager_.replicasInAlert();
    EXPECT_TRUE(in_alert.empty());
}

TEST_F(LagAlertManagerTest, CriticalThresholdTriggers)
{
    manager_.updateReplicaLag("replica-1", 30'001);  // Exceed critical
    manager_.checkAndAlertLagViolations();
    EXPECT_EQ(captured_alerts_.size(), 1);
    EXPECT_EQ(captured_alerts_[0].level, AlertEvent::Level::CRITICAL);
    EXPECT_EQ(captured_alerts_[0].lag_ms, 30'001);
}

TEST_F(LagAlertManagerTest, MultipleReplicasAlert)
{
    manager_.updateReplicaLag("replica-1", 10'500);
    manager_.updateReplicaLag("replica-2", 25'000);
    manager_.updateReplicaLag("replica-3", 35'000);

    manager_.checkAndAlertLagViolations();

    // Should have 3 alerts (alert + critical)
    EXPECT_GE(captured_alerts_.size(), 2);

    const auto in_alert = manager_.replicasInAlert();
    EXPECT_EQ(in_alert.size(), 1);  // replica-1 (alert but not critical)

    const auto in_critical = manager_.replicasInCritical();
    EXPECT_EQ(in_critical.size(), 2);  // replica-2 and replica-3
}

// ============================================================================
// Replica state tracking tests
// ============================================================================

TEST_F(LagAlertManagerTest, ReplicasInAlert)
{
    manager_.updateReplicaLag("replica-1", 10'500);
    manager_.updateReplicaLag("replica-2", 5'000);
    manager_.checkAndAlertLagViolations();

    const auto in_alert = manager_.replicasInAlert();
    EXPECT_EQ(in_alert.size(), 1);
    EXPECT_EQ(in_alert[0], "replica-1");
}

TEST_F(LagAlertManagerTest, ReplicasInCritical)
{
    manager_.updateReplicaLag("replica-1", 30'500);
    manager_.updateReplicaLag("replica-2", 15'000);
    manager_.checkAndAlertLagViolations();

    const auto in_critical = manager_.replicasInCritical();
    EXPECT_EQ(in_critical.size(), 1);
    EXPECT_EQ(in_critical[0], "replica-1");
}

// ============================================================================
// Failover tests
// ============================================================================

TEST_F(LagAlertManagerTest, FailoverEligibilityAfterSustainedCritical)
{
    // Set short thresholds for testing
    SLOThresholds custom;
    custom.alert_threshold_ms = 1'000;
    custom.critical_threshold_ms = 2'000;
    custom.failover_threshold_ms = 3'000;
    custom.failover_duration_ms = 100;  // 100ms for testing

    manager_.setThresholds(custom);

    // Put replica in critical state
    manager_.updateReplicaLag("replica-1", 2'500);
    manager_.checkAndAlertLagViolations();

    auto eligible = manager_.replicasEligibleForFailover();
    EXPECT_EQ(eligible.size(), 0);  // Not enough time has passed

    // Wait for failover duration
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    manager_.checkAndAlertLagViolations();

    eligible = manager_.replicasEligibleForFailover();
    EXPECT_EQ(eligible.size(), 1);
    EXPECT_EQ(eligible[0], "replica-1");
}

TEST_F(LagAlertManagerTest, FailoverEventEmitted)
{
    // Set very short thresholds
    SLOThresholds custom;
    custom.critical_threshold_ms = 1'000;
    custom.failover_duration_ms = 50;

    manager_.setThresholds(custom);

    manager_.updateReplicaLag("replica-1", 1'500);
    manager_.checkAndAlertLagViolations();
    
    const int initial_alerts = captured_alerts_.size();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    manager_.checkAndAlertLagViolations();

    // Should have a failover event eventually
    bool found_failover = false;
    for (const auto& evt : captured_alerts_) {
        if (evt.level == AlertEvent::Level::FAILOVER) {
            found_failover = true;
            break;
        }
    }
    EXPECT_TRUE(found_failover);
}

// ============================================================================
// Metrics tests
// ============================================================================

TEST_F(LagAlertManagerTest, AllReplicaLags)
{
    manager_.updateReplicaLag("replica-1", 5'000);
    manager_.updateReplicaLag("replica-2", 8'000);

    const auto all_lags = manager_.allReplicaLags();
    EXPECT_EQ(all_lags.size(), 2);
    EXPECT_EQ(all_lags.at("replica-1"), 5'000);
    EXPECT_EQ(all_lags.at("replica-2"), 8'000);
}

TEST_F(LagAlertManagerTest, PrometheusMetrics)
{
    manager_.updateReplicaLag("replica-1", 5'000);
    manager_.updateReplicaLag("replica-2", 15'000);
    manager_.checkAndAlertLagViolations();

    const auto metrics = manager_.exportPrometheusMetrics();
    
    EXPECT_NE(metrics.find("replication_lag_ms"), std::string::npos);
    EXPECT_NE(metrics.find("replica-1"), std::string::npos);
    EXPECT_NE(metrics.find("replica-2"), std::string::npos);
    EXPECT_NE(metrics.find("lag_alert_triggered_total"), std::string::npos);
}

TEST_F(LagAlertManagerTest, AlertStats)
{
    manager_.updateReplicaLag("replica-1", 10'500);
    manager_.checkAndAlertLagViolations();

    auto [alerts, critical, failover] = manager_.getAlertStats("replica-1");
    EXPECT_EQ(alerts, 1);
    EXPECT_EQ(critical, 0);
    EXPECT_EQ(failover, 0);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(LagAlertManagerTest, DisabledThresholds)
{
    SLOThresholds disabled;
    disabled.alert_threshold_ms = 0;
    disabled.critical_threshold_ms = 0;

    manager_.setThresholds(disabled);
    manager_.updateReplicaLag("replica-1", 100'000);
    manager_.checkAndAlertLagViolations();

    EXPECT_EQ(captured_alerts_.size(), 0);
}

TEST_F(LagAlertManagerTest, ThreadSafety)
{
    // Simple thread-safety test: concurrent updates and checks
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 100; ++j) {
                const std::string replica_id = "replica-" + std::to_string(i);
                manager_.updateReplicaLag(replica_id, j * 1000);
                manager_.checkAndAlertLagViolations();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    const auto all_lags = manager_.allReplicaLags();
    EXPECT_EQ(all_lags.size(), 5);
}

}  // namespace replication
}  // namespace themisdb
