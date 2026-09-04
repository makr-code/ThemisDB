// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_replication_geo_placement_wal_shipping_focused.cpp
 * @brief Focused tests for Geographic Replica Placement Policies (GEO-01..08)
 *        and Async Cross-Region WAL Shipping (WAL-01..08).
 *
 * ## Test families
 *
 * ### GEO — Geographic Replica Placement
 * | ID     | Description                                                       |
 * |--------|-------------------------------------------------------------------|
 * | GEO-01 | selectLeaderCandidate — preferred DC respected                    |
 * | GEO-02 | selectLeaderCandidate — forbidden DC excluded                     |
 * | GEO-03 | selectLeaderCandidate — falls back when preferred DC unhealthy    |
 * | GEO-04 | selectLeaderCandidate — zone affinity respected                   |
 * | GEO-05 | selectFailoverCandidate — failed node excluded                    |
 * | GEO-06 | selectFailoverCandidate — constraint + failed node interaction     |
 * | GEO-07 | validatePlacement — required DC violation detected                |
 * | GEO-08 | validatePlacement — min_copies_per_dc violation detected          |
 *
 * ### WAL — Async Cross-Region WAL Shipping
 * | ID     | Description                                                       |
 * |--------|-------------------------------------------------------------------|
 * | WAL-01 | enqueueSegment — segment accepted and shipped                     |
 * | WAL-02 | enqueueSegment — back-pressure on full queue                      |
 * | WAL-03 | lag alert fires when lag exceeds max_lag_ms                       |
 * | WAL-04 | lag alert does NOT fire when lag is within limit                  |
 * | WAL-05 | stats accounting — enqueued/shipped/dropped counters correct      |
 * | WAL-06 | exportPrometheusMetrics — required metric names present           |
 * | WAL-07 | currentLagMs — returns 0 on empty queue                           |
 * | WAL-08 | stop() — drains queue gracefully, no crash on double-stop         |
 */

#include <gtest/gtest.h>

#include <algorithm>

#include "replication/async_wal_shipper.h"
#include "replication/geo_placement.h"
#include "replication/replication_manager.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themisdb::replication;
using namespace std::chrono_literals;

// ===========================================================================
// Helpers
// ===========================================================================

namespace {

/// Build a healthy voting replica in the given datacenter.
ReplicaInfo makeReplica(const std::string& node_id,
                        const std::string& datacenter,
                        int32_t            priority = 1,
                        uint64_t           seq      = 0,
                        HealthStatus       health   = HealthStatus::HEALTHY)
{
    ReplicaInfo r;
    r.node_id              = node_id;
    r.datacenter           = datacenter;
    r.priority             = priority;
    r.last_applied_sequence = seq;
    r.health_status        = health;
    r.is_voting_member     = true;
    r.role                 = ReplicationRole::FOLLOWER;
    r.last_heartbeat       = std::chrono::system_clock::now();
    return r;
}

/// Build a WalSegment with current enqueue_time.
WalSegment makeSegment(uint64_t seq, std::string data = "payload",
                       std::string target_dc = "dc-eu")
{
    WalSegment s;
    s.sequence_number = seq;
    s.data            = std::move(data);
    s.enqueue_time    = std::chrono::steady_clock::now();
    s.target_dc       = std::move(target_dc);
    return s;
}

} // anonymous namespace

// ===========================================================================
// GEO test fixture
// ===========================================================================

class GeoPlacementTest : public ::testing::Test {
protected:
    GeoReplicaPlacementManager mgr;
};

// ===========================================================================
// GEO-01: selectLeaderCandidate — preferred DC respected
// ===========================================================================
TEST_F(GeoPlacementTest, GEO01_PreferredDCRespected)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("node-eu", "eu-west-1", 1),
        makeReplica("node-us", "us-east-1", 2), // higher priority but not preferred
    };

    PlacementConstraints c;
    c.preferred_datacenters = {"eu-west-1"};

    const auto result = mgr.selectLeaderCandidate(replicas, c);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->node_id, "node-eu")
        << "eu-west-1 is in preferred_datacenters; should beat higher priority in us-east-1";
}

// ===========================================================================
// GEO-02: selectLeaderCandidate — forbidden DC excluded
// ===========================================================================
TEST_F(GeoPlacementTest, GEO02_ForbiddenDCExcluded)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("node-ap", "ap-southeast-1", 5),  // highest priority but forbidden
        makeReplica("node-eu", "eu-west-1", 1),
    };

    PlacementConstraints c;
    c.forbidden_datacenters = {"ap-southeast-1"};

    const auto result = mgr.selectLeaderCandidate(replicas, c);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->node_id, "node-eu");
}

// ===========================================================================
// GEO-03: selectLeaderCandidate — falls back when preferred DC unhealthy
// ===========================================================================
TEST_F(GeoPlacementTest, GEO03_FallbackWhenPreferredDCUnhealthy)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("node-eu", "eu-west-1", 1, 0, HealthStatus::DEGRADED),
        makeReplica("node-us", "us-east-1", 1),
    };

    PlacementConstraints c;
    c.preferred_datacenters = {"eu-west-1"};
    c.healthy_only          = true;

    const auto result = mgr.selectLeaderCandidate(replicas, c);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->node_id, "node-us")
        << "eu-west-1 replica is unhealthy; fallback to us-east-1 required";
}

// ===========================================================================
// GEO-04: selectLeaderCandidate — zone affinity respected
// ===========================================================================
TEST_F(GeoPlacementTest, GEO04_ZoneAffinityRespected)
{
    // datacenter encodes zone as "<dc>/<zone>"
    std::vector<ReplicaInfo> replicas{
        makeReplica("node-a", "eu-west-1/zone-a", 1, 100),
        makeReplica("node-b", "eu-west-1/zone-b", 1, 200), // higher seq but wrong zone
    };

    PlacementConstraints c;
    c.preferred_datacenters = {"eu-west-1/zone-a", "eu-west-1/zone-b"};
    c.zone_affinity_zone    = "zone-a"; // prefer zone-a over zone-b

    const auto result = mgr.selectLeaderCandidate(replicas, c);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->node_id, "node-a")
        << "zone-a affinity should beat higher sequence in zone-b";
}

// ===========================================================================
// GEO-05: selectFailoverCandidate — failed node excluded
// ===========================================================================
TEST_F(GeoPlacementTest, GEO05_FailoverExcludesFailedNode)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("leader", "eu-west-1", 10, 999),
        makeReplica("follower-1", "eu-west-1", 5, 800),
        makeReplica("follower-2", "us-east-1", 3, 700),
    };

    PlacementConstraints c;

    const auto result = mgr.selectFailoverCandidate(replicas, c, "leader");
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->node_id, "leader")
        << "Failed leader must never be returned as failover candidate";
    EXPECT_EQ(result->node_id, "follower-1")
        << "follower-1 has highest priority among remaining candidates";
}

// ===========================================================================
// GEO-06: selectFailoverCandidate — constraint + failed node interaction
// ===========================================================================
TEST_F(GeoPlacementTest, GEO06_FailoverRespectsDCConstraint)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("leader", "eu-west-1", 10, 999),
        makeReplica("follower-eu", "eu-west-1", 5, 800),
        makeReplica("follower-us", "us-east-1", 3, 700),
    };

    PlacementConstraints c;
    c.preferred_datacenters  = {"us-east-1"}; // prefer US for DR
    c.forbidden_datacenters  = {};

    const auto result = mgr.selectFailoverCandidate(replicas, c, "leader");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->node_id, "follower-us")
        << "DC preference must select us-east-1 over eu-west-1 in failover";
}

// ===========================================================================
// GEO-07: validatePlacement — required DC violation
// ===========================================================================
TEST_F(GeoPlacementTest, GEO07_ValidatePlacement_RequiredDCMissing)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("node-eu", "eu-west-1"),
    };

    PlacementConstraints c;
    c.required_datacenters = {"eu-west-1", "ap-southeast-1"}; // ap has no replicas

    const auto v = mgr.validatePlacement(replicas, c);
    EXPECT_FALSE(v.is_valid);
    EXPECT_FALSE(v.violations.empty());
    EXPECT_TRUE(std::any_of(v.violations.begin(), v.violations.end(),
                            [](const std::string& s) {
                                return s.find("ap-southeast-1") != std::string::npos;
                            }))
        << "Violation must mention the missing required DC";
}

// ===========================================================================
// GEO-08: validatePlacement — min_copies_per_dc violation
// ===========================================================================
TEST_F(GeoPlacementTest, GEO08_ValidatePlacement_MinCopiesPerDCViolation)
{
    std::vector<ReplicaInfo> replicas{
        makeReplica("node-eu-1", "eu-west-1"),
        makeReplica("node-us-1", "us-east-1"),
        // us-east-1 only has 1 replica, but we require 2
    };

    PlacementConstraints c;
    c.min_copies_per_dc = 2;

    const auto v = mgr.validatePlacement(replicas, c);
    EXPECT_FALSE(v.is_valid);
    EXPECT_FALSE(v.violations.empty());
    EXPECT_TRUE(std::ranges::any_of(v.violations, [](const std::string& s) {
                    return s.find("required") != std::string::npos ||
                           s.find("require") != std::string::npos;
                }))
        << "Violation must reference the min_copies_per_dc requirement";
}

// ===========================================================================
// WAL test fixture
// ===========================================================================

class AsyncWalShipperTest : public ::testing::Test {
protected:
    WalShippingConfig defaultConfig()
    {
        WalShippingConfig cfg;
        cfg.local_dc_id        = "dc-us-east";
        cfg.remote_dc_endpoint = "dc-eu-west:9876";
        cfg.max_lag_ms         = 500;
        cfg.max_queue_depth    = 64;
        return cfg;
    }
};

// ===========================================================================
// WAL-01: segment accepted and shipped
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL01_SegmentAcceptedAndShipped)
{
    AsyncWalShipper shipper(defaultConfig());

    std::atomic<bool> shipped{false};
    shipper.setShipHandler([&](const WalSegment&) -> bool {
        shipped.store(true);
        return true;
    });

    const bool ok = shipper.enqueueSegment(makeSegment(1));
    EXPECT_TRUE(ok) << "enqueueSegment should accept the segment";

    // Allow background thread to process
    for (int i = 0; i < 50 && !shipped.load(); ++i)
        std::this_thread::sleep_for(10ms);

    EXPECT_TRUE(shipped.load()) << "Background thread must dispatch the segment";

    const auto s = shipper.stats();
    EXPECT_GE(s.segments_enqueued, uint64_t{1});
}

// ===========================================================================
// WAL-02: back-pressure on full queue
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL02_BackPressureOnFullQueue)
{
    WalShippingConfig cfg = defaultConfig();
    cfg.max_queue_depth   = 2;

    // Use a blocking ship handler so the queue fills up
    std::mutex block_mutex;
    block_mutex.lock(); // released in teardown

    AsyncWalShipper shipper(cfg);
    shipper.setShipHandler([&](const WalSegment&) -> bool {
        std::lock_guard<std::mutex> lk(block_mutex); // will block
        return true;
    });

    // Fill the queue beyond capacity
    shipper.enqueueSegment(makeSegment(1));
    shipper.enqueueSegment(makeSegment(2));

    // Next enqueue should fail (queue full)
    const bool rejected = !shipper.enqueueSegment(makeSegment(3));
    EXPECT_TRUE(rejected) << "Queue at capacity; third enqueue must fail";

    block_mutex.unlock(); // let ship handler proceed
    shipper.stop();

    EXPECT_GE(shipper.stats().segments_dropped, uint64_t{1});
}

// ===========================================================================
// WAL-03: lag alert fires when lag exceeds max_lag_ms
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL03_LagAlertFires)
{
    WalShippingConfig cfg = defaultConfig();
    cfg.max_lag_ms        = 1; // 1 ms — easy to exceed in test

    AsyncWalShipper shipper(cfg);

    std::atomic<uint64_t> alert_lag{0};
    shipper.setAlertCallback([&](uint64_t lag_ms) {
        alert_lag.store(lag_ms);
    });

    // Build a segment whose enqueue_time is well in the past
    WalSegment seg      = makeSegment(1);
    seg.enqueue_time    = std::chrono::steady_clock::now() - 200ms;

    shipper.enqueueSegment(std::move(seg));

    for (int i = 0; i < 50 && alert_lag.load() == 0; ++i)
        std::this_thread::sleep_for(10ms);

    EXPECT_GT(alert_lag.load(), uint64_t{0})
        << "Alert callback must fire when lag exceeds max_lag_ms";

    EXPECT_GE(shipper.stats().lag_alerts_fired, uint64_t{1});
}

// ===========================================================================
// WAL-04: lag alert does NOT fire when lag is within limit
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL04_NoAlertWithinLagLimit)
{
    WalShippingConfig cfg = defaultConfig();
    cfg.max_lag_ms        = 60000; // 60 s — impossible to exceed in test

    AsyncWalShipper shipper(cfg);

    std::atomic<int> alert_count{0};
    shipper.setAlertCallback([&](uint64_t) { ++alert_count; });

    shipper.enqueueSegment(makeSegment(1));

    // Wait for dispatch
    for (int i = 0; i < 50; ++i)
        std::this_thread::sleep_for(5ms);

    EXPECT_EQ(alert_count.load(), 0) << "No alert must fire for fresh segments";
}

// ===========================================================================
// WAL-05: stats accounting
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL05_StatsAccounting)
{
    AsyncWalShipper shipper(defaultConfig());

    constexpr int kSegments = 5;
    for (int i = 0; i < kSegments; ++i)
        shipper.enqueueSegment(makeSegment(static_cast<uint64_t>(i), "hello"));

    // Wait for all to dispatch
    for (int wait = 0; wait < 100; ++wait) {
        if (shipper.stats().segments_shipped >= kSegments) {
          break;
        }
        std::this_thread::sleep_for(5ms);
    }

    const auto s = shipper.stats();
    EXPECT_EQ(s.segments_enqueued, uint64_t{kSegments});
    EXPECT_EQ(s.segments_shipped,  uint64_t{kSegments});
    EXPECT_EQ(s.segments_dropped,  uint64_t{0});
    EXPECT_EQ(s.bytes_enqueued,    uint64_t{kSegments * 5}); // "hello" = 5 bytes
}

// ===========================================================================
// WAL-06: exportPrometheusMetrics — required metric names present
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL06_PrometheusMetricsPresent)
{
    AsyncWalShipper shipper(defaultConfig());
    shipper.enqueueSegment(makeSegment(1));

    for (int i = 0; i < 50; ++i) {
        if (shipper.stats().segments_shipped >= 1) {
          break;
        }
        std::this_thread::sleep_for(5ms);
    }

    const std::string metrics = shipper.exportPrometheusMetrics();

    EXPECT_NE(metrics.find("replication_wal_lag_ms"), std::string::npos)
        << "replication_wal_lag_ms histogram must be present";
    EXPECT_NE(metrics.find("replication_wal_segments_enqueued_total"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_segments_shipped_total"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_segments_dropped_total"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_lag_alerts_total"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_bytes_shipped_total"), std::string::npos);
}

// ===========================================================================
// WAL-07: currentLagMs returns 0 on empty queue
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL07_CurrentLagMsZeroOnEmptyQueue)
{
    AsyncWalShipper shipper(defaultConfig());
    EXPECT_EQ(shipper.currentLagMs(), 0)
        << "currentLagMs must return 0 when the queue is empty";
}

// ===========================================================================
// WAL-08: stop() drains gracefully, double-stop is safe
// ===========================================================================
TEST_F(AsyncWalShipperTest, WAL08_GracefulStopAndDoubleSafe)
{
    AsyncWalShipper shipper(defaultConfig());

    for (int i = 0; i < 10; ++i)
        shipper.enqueueSegment(makeSegment(static_cast<uint64_t>(i)));

    // First stop — should drain and join background thread
    EXPECT_NO_THROW(shipper.stop());
    // Second stop — must be idempotent (no crash, no deadlock)
    EXPECT_NO_THROW(shipper.stop());
}
