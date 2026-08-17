// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_batch_a7b_integration.cpp
 * @brief Integration tests for Batch A-7b: Geo placement, async WAL shipping, and lag alerts.
 *
 * This test verifies that the three components work together to support
 * geographic replica placement, asynchronous WAL shipping with batching,
 * and replication lag monitoring with SLO enforcement.
 */

#include "replication/geo_placement.h"
#include "replication/async_wal_shipper.h"
#include "replication/lag_alert_manager.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

namespace themisdb {
namespace replication {

// ============================================================================
// Integration test fixture
// ============================================================================

class Batch_A7b_IntegrationTest : public ::testing::Test {
protected:
    GeoReplicaPlacementManager placement_;
    LagAlertManager lag_monitor_;

    // Helper to create a replica for placement testing
    ReplicaInfo makeReplica(const std::string& node_id,
                           const std::string& datacenter,
                           int64_t            lag_ms)
    {
        ReplicaInfo r;
        r.node_id = node_id;
        r.datacenter = datacenter;
        r.health_status = HealthStatus::HEALTHY;
        r.is_voting_member = true;
        r.priority = 0;
        r.last_applied_sequence = 100;
        return r;
    }
};

// ============================================================================
// SCENARIO 1: Geographic replica placement with multi-DC topology
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, MultiDCReplicaPlacement)
{
    // Simulate a 3-DC production topology:
    // - Primary in us-east-1a
    // - Replicas in us-west-1b and eu-west-1c
    // - Backup in ap-southeast-1d
    std::vector<ReplicaInfo> replicas = {
        makeReplica("primary", "us-east-1/az-1a", 0),
        makeReplica("replica-us-west", "us-west-1/az-2b", 50),
        makeReplica("replica-eu", "eu-west-1/az-3c", 200),
        makeReplica("backup-ap", "ap-southeast-1/az-4d", 500),
    };

    PlacementConstraints constraints;
    constraints.preferred_datacenters = {"eu-west-1", "us-west-1", "us-east-1"};
    constraints.forbidden_datacenters = {"ap-southeast-1"};  // Don't use AP for leader
    constraints.min_copies_per_dc = 1;
    constraints.require_voter = true;
    constraints.healthy_only = true;

    auto leader = placement_.selectLeaderCandidate(replicas, constraints);
    ASSERT_TRUE(leader.has_value());
    // Should select from eu-west-1 (first in preferred list)
    EXPECT_EQ(leader->datacenter, "eu-west-1/az-3c");

    // Validate the placement satisfies constraints
    auto validation = placement_.validatePlacement(replicas, constraints);
    EXPECT_TRUE(validation.is_valid);
}

// ============================================================================
// SCENARIO 2: Async WAL shipping with lag monitoring
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, AsyncWALShippingWithLagMonitoring)
{
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "dc-eu-west:9876";
    cfg.local_dc_id = "dc-us-east";
    cfg.max_lag_ms = 100;
    cfg.max_queue_depth = 1000;

    AsyncWalShipper shipper(cfg);

    // Set up lag monitoring
    SLOThresholds thresholds;
    thresholds.alert_threshold_ms = 50;
    thresholds.critical_threshold_ms = 100;

    lag_monitor_.setThresholds(thresholds);

    std::atomic<int> lag_alerts{0};
    lag_monitor_.setAlertCallback([&](const AlertEvent& evt) {
        lag_alerts++;
    });

    // Simulate WAL entries being shipped
    for (int i = 0; i < 10; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = std::string(100, 'x');
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-eu-west";

        EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
    }

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 10);
    EXPECT_EQ(stats.bytes_enqueued, 1000);

    // Verify metrics
    auto metrics = shipper.exportPrometheusMetrics();
    EXPECT_NE(metrics.find("replication_wal_lag_ms"), std::string::npos);

    shipper.stop();
}

// ============================================================================
// SCENARIO 3: Lag alert escalation (alert → critical → failover)
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, LagAlertEscalation)
{
    SLOThresholds thresholds;
    thresholds.alert_threshold_ms = 2'000;        // 2s
    thresholds.critical_threshold_ms = 5'000;     // 5s
    thresholds.failover_threshold_ms = 10'000;    // 10s
    thresholds.failover_duration_ms = 100;        // 100ms for testing

    lag_monitor_.setThresholds(thresholds);

    std::vector<AlertEvent> events;
    lag_monitor_.setAlertCallback([&](const AlertEvent& evt) {
        events.push_back(evt);
    });

    // Stage 1: Update lag below alert threshold
    lag_monitor_.updateReplicaLag("replica-1", 1'000);
    lag_monitor_.checkAndAlertLagViolations();
    EXPECT_EQ(events.size(), 0);  // No alert

    // Stage 2: Exceed alert threshold
    lag_monitor_.updateReplicaLag("replica-1", 2'500);
    lag_monitor_.checkAndAlertLagViolations();
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].level, AlertEvent::Level::ALERT);

    // Stage 3: Escalate to critical
    lag_monitor_.updateReplicaLag("replica-1", 5'500);
    lag_monitor_.checkAndAlertLagViolations();
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events[1].level, AlertEvent::Level::CRITICAL);

    // Stage 4: Wait for failover eligibility
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    lag_monitor_.checkAndAlertLagViolations();

    auto eligible = lag_monitor_.replicasEligibleForFailover();
    EXPECT_EQ(eligible.size(), 1);
    EXPECT_EQ(eligible[0], "replica-1");
}

// ============================================================================
// SCENARIO 4: Multi-replica monitoring with different lag profiles
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, MultiReplicaLagMonitoring)
{
    SLOThresholds thresholds;
    thresholds.alert_threshold_ms = 5'000;
    thresholds.critical_threshold_ms = 15'000;

    lag_monitor_.setThresholds(thresholds);

    // Simulate a 3-replica cluster with varying lag
    std::map<std::string, int64_t> initial_lags;
    initial_lags["replica-us-west"] = 2'000;   // Healthy
    initial_lags["replica-eu"] = 8'000;        // Alert
    initial_lags["replica-ap"] = 20'000;       // Critical

    lag_monitor_.updateReplicaLags(initial_lags);
    lag_monitor_.checkAndAlertLagViolations();

    // Verify state tracking
    auto in_alert = lag_monitor_.replicasInAlert();
    EXPECT_GE(in_alert.size(), 1);

    auto in_critical = lag_monitor_.replicasInCritical();
    EXPECT_GE(in_critical.size(), 1);

    // Verify metrics
    auto all_lags = lag_monitor_.allReplicaLags();
    EXPECT_EQ(all_lags.size(), 3);
    EXPECT_EQ(all_lags["replica-us-west"], 2'000);
    EXPECT_EQ(all_lags["replica-eu"], 8'000);
    EXPECT_EQ(all_lags["replica-ap"], 20'000);
}

// ============================================================================
// SCENARIO 5: Failover candidate selection based on lag and placement
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, FailoverCandidateWithPlacementConstraints)
{
    // Create a cluster where primary has failed
    std::vector<ReplicaInfo> replicas = {
        makeReplica("primary", "us-east-1/az-1a", 0),
        makeReplica("replica-us-west", "us-west-1/az-2b", 50),
        makeReplica("replica-eu", "eu-west-1/az-3c", 100),
    };

    PlacementConstraints failover_constraints;
    failover_constraints.preferred_datacenters = {"eu-west-1", "us-west-1"};
    failover_constraints.forbidden_datacenters = {"ap-southeast-1"};
    failover_constraints.require_voter = true;

    // Select failover candidate (excluding failed primary)
    auto candidate = placement_.selectFailoverCandidate(
        replicas, failover_constraints, "primary");

    ASSERT_TRUE(candidate.has_value());
    // Should be from preferred list, highest priority
    EXPECT_NE(candidate->node_id, "primary");
    EXPECT_TRUE(
        candidate->datacenter == "eu-west-1/az-3c" ||
        candidate->datacenter == "us-west-1/az-2b");

    // Monitor lag for the new leader
    lag_monitor_.updateReplicaLag(candidate->node_id, 100);
    lag_monitor_.checkAndAlertLagViolations();

    // Verify lag is being tracked
    EXPECT_EQ(lag_monitor_.getReplicaLag(candidate->node_id), 100);
}

// ============================================================================
// SCENARIO 6: Recovery from alert state
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, RecoveryFromAlertState)
{
    SLOThresholds thresholds;
    thresholds.alert_threshold_ms = 5'000;
    thresholds.critical_threshold_ms = 15'000;

    lag_monitor_.setThresholds(thresholds);

    std::vector<AlertEvent> events;
    lag_monitor_.setAlertCallback([&](const AlertEvent& evt) {
        events.push_back(evt);
    });

    // Trigger alert
    lag_monitor_.updateReplicaLag("replica-1", 6'000);
    lag_monitor_.checkAndAlertLagViolations();
    int initial_count = events.size();
    EXPECT_GT(initial_count, 0);

    // Recovery: lag drops below threshold
    lag_monitor_.updateReplicaLag("replica-1", 3'000);
    lag_monitor_.checkAndAlertLagViolations();

    // Alert should clear (no new events)
    EXPECT_EQ(events.size(), initial_count);

    auto in_alert = lag_monitor_.replicasInAlert();
    EXPECT_FALSE(std::any_of(
        in_alert.begin(), in_alert.end(),
        [](const auto& id) { return id == "replica-1"; }));
}

// ============================================================================
// SCENARIO 7: Concurrent updates and checks (thread safety)
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, ConcurrentReplicaUpdates)
{
    lag_monitor_.setThresholds(SLOThresholds());

    std::vector<std::thread> threads;

    // Writer threads: continuously update lag
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([this, i]() {
            const std::string replica_id = "replica-" + std::to_string(i);
            for (int j = 0; j < 50; ++j) {
                lag_monitor_.updateReplicaLag(replica_id, j * 1000);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // Reader/checker thread: check for alerts
    threads.emplace_back([this]() {
        for (int i = 0; i < 50; ++i) {
            lag_monitor_.checkAndAlertLagViolations();
            auto all = lag_monitor_.allReplicaLags();
            // Just verify we can read without crashes
            (void)all;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    for (auto& t : threads) {
        t.join();
    }

    // All replicas should be tracked
    auto all_lags = lag_monitor_.allReplicaLags();
    EXPECT_EQ(all_lags.size(), 3);
}

// ============================================================================
// SCENARIO 8: Prometheus metrics completeness
// ============================================================================

TEST_F(Batch_A7b_IntegrationTest, PrometheusMetricsContent)
{
    lag_monitor_.setThresholds(SLOThresholds());

    lag_monitor_.updateReplicaLag("replica-1", 5'000);
    lag_monitor_.updateReplicaLag("replica-2", 15'000);
    lag_monitor_.checkAndAlertLagViolations();

    auto metrics = lag_monitor_.exportPrometheusMetrics();

    // Verify all required metrics are present
    EXPECT_NE(metrics.find("replication_lag_ms"), std::string::npos);
    EXPECT_NE(metrics.find("lag_alert_triggered_total"), std::string::npos);
    EXPECT_NE(metrics.find("lag_critical_triggered_total"), std::string::npos);
    EXPECT_NE(metrics.find("failover_initiated_total"), std::string::npos);
    EXPECT_NE(metrics.find("replica-1"), std::string::npos);
    EXPECT_NE(metrics.find("replica-2"), std::string::npos);
}

}  // namespace replication
}  // namespace themisdb
