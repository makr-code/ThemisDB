// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_replication_chaos_failover_focused.cpp
 * @brief Chaos / fault-injection tests for geo-placement failover (CHAOS-01..08).
 *
 * @note CTest labels: replication;autogen;release_critical
 *
 * ## Test families
 *
 * ### CHAOS — Geo-Placement Failover Fault Injection
 *
 * | ID       | Description                                                          |
 * |----------|----------------------------------------------------------------------|
 * | CHAOS-01 | All preferred-DC replicas fail — must fall back to secondary DC       |
 * | CHAOS-02 | All replicas in one DC become unhealthy — multi-DC placement survives |
 * | CHAOS-03 | Single surviving replica promoted as leader under max-failure load    |
 * | CHAOS-04 | Forbidden-DC constraint respected during mass failure                 |
 * | CHAOS-05 | Concurrent failover candidate queries are deterministic               |
 * | CHAOS-06 | WAL shipper survives transport handler that always throws             |
 * | CHAOS-07 | WAL shipper survives transport handler that blocks until timeout       |
 * | CHAOS-08 | validatePlacement returns violations when quorum is lost              |
 *
 * ### Design constraints
 * - No external services required; all faults are injected via in-process mocks.
 * - Tests must be deterministic (no sleep-based timing assertions).
 * - No test modifies shared global state; all fixtures are per-test.
 */

#include <gtest/gtest.h>

#include "replication/async_wal_shipper.h"
#include "replication/geo_placement.h"
#include "replication/replication_manager.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themisdb::replication;
using namespace std::chrono_literals;

// ===========================================================================
// Test helpers
// ===========================================================================

namespace {

/// Build a replica with controllable health in the given datacenter.
ReplicaInfo makeReplica(const std::string& node_id,
                        const std::string& datacenter,
                        HealthStatus health = HealthStatus::HEALTHY,
                        int32_t priority   = 1,
                        uint64_t seq       = 0)
{
    ReplicaInfo r;
    r.node_id               = node_id;
    r.datacenter            = datacenter;
    r.priority              = priority;
    r.last_applied_sequence = seq;
    r.health_status         = health;
    r.is_voting_member      = true;
    r.role                  = ReplicationRole::FOLLOWER;
    r.last_heartbeat        = std::chrono::system_clock::now();
    return r;
}

/// Build a WalSegment pre-aged by the given duration so lag thresholds fire.
WalSegment makeAgedSegment(uint64_t seq,
                           std::chrono::milliseconds age,
                           std::string target_dc = "dc-remote")
{
    WalSegment s;
    s.sequence_number = seq;
    s.data            = "chaos-payload";
    s.enqueue_time    = std::chrono::steady_clock::now() - age;
    s.target_dc       = std::move(target_dc);
    return s;
}

/// Build a basic WalShippingConfig with an injected transport handler.
WalShippingConfig makeConfig(
    uint32_t max_lag_ms,
    AsyncWalShipper::TransportHandler handler)
{
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "chaos-dc:5432";
    cfg.local_dc_id        = "dc-local";
    cfg.max_lag_ms         = max_lag_ms;
    cfg.max_queue_depth    = 64;
    cfg.transport_handler  = std::move(handler);
    return cfg;
}

} // namespace

// ===========================================================================
// CHAOS-01: All preferred-DC replicas fail — fallback to secondary DC
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS01_AllPreferredDCReplicasFail_FallbackToSecondary)
{
    // Preferred DC us-east has only unhealthy replicas.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("n1", "us-east", HealthStatus::UNHEALTHY),
        makeReplica("n2", "us-east", HealthStatus::UNHEALTHY),
        makeReplica("n3", "eu-west", HealthStatus::HEALTHY),
        makeReplica("n4", "eu-west", HealthStatus::HEALTHY),
    };

    PlacementConstraints c;
    c.preferred_datacenters = {"us-east", "eu-west"};
    c.healthy_only          = true;

    GeoReplicaPlacementManager mgr;
    const auto candidate = mgr.selectLeaderCandidate(replicas, c);

    ASSERT_TRUE(candidate.has_value())
        << "Must find a leader in eu-west when us-east is fully unhealthy";
    EXPECT_EQ(candidate->datacenter, "eu-west")
        << "Fallback must select the first healthy DC from preferred list";
}

// ===========================================================================
// CHAOS-02: Entire DC becomes unhealthy — multi-DC placement survives
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS02_EntireDCUnhealthy_MultiDCSurvives)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("n1", "dc-a", HealthStatus::UNHEALTHY),
        makeReplica("n2", "dc-a", HealthStatus::UNHEALTHY),
        makeReplica("n3", "dc-a", HealthStatus::UNHEALTHY),
        makeReplica("n4", "dc-b", HealthStatus::HEALTHY),
        makeReplica("n5", "dc-c", HealthStatus::HEALTHY),
    };

    PlacementConstraints c;
    c.preferred_datacenters = {"dc-a", "dc-b", "dc-c"};
    c.healthy_only          = true;

    GeoReplicaPlacementManager mgr;
    const auto candidate = mgr.selectLeaderCandidate(replicas, c);

    ASSERT_TRUE(candidate.has_value())
        << "Must find a leader even when the first DC is fully lost";
    EXPECT_NE(candidate->datacenter, "dc-a")
        << "Leader must not be selected from the failed DC";
}

// ===========================================================================
// CHAOS-03: Single surviving replica is promoted under maximum failure load
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS03_SingleSurvivingReplicaPromoted)
{
    // 7 nodes all unhealthy; 1 healthy survivor in dc-survivor.
    std::vector<ReplicaInfo> replicas;
    for (int i = 0; i < 7; ++i) {
        replicas.push_back(makeReplica("dead-" + std::to_string(i),
                                      "dc-main", HealthStatus::UNHEALTHY));
    }
    replicas.push_back(makeReplica("survivor", "dc-survivor",
                                   HealthStatus::HEALTHY, 10, 999));

    PlacementConstraints c;
    c.healthy_only = true;

    GeoReplicaPlacementManager mgr;
    const auto candidate = mgr.selectLeaderCandidate(replicas, c);

    ASSERT_TRUE(candidate.has_value())
        << "Must promote the single surviving replica";
    EXPECT_EQ(candidate->node_id, "survivor")
        << "The single healthy replica must be selected as leader";
}

// ===========================================================================
// CHAOS-04: Forbidden-DC constraint respected during mass failure
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS04_ForbiddenDCRespectedUnderMassFailure)
{
    // dc-restricted has healthy replicas but must be forbidden.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("n1", "dc-main",       HealthStatus::UNHEALTHY),
        makeReplica("n2", "dc-main",       HealthStatus::UNHEALTHY),
        makeReplica("n3", "dc-restricted", HealthStatus::HEALTHY),
        makeReplica("n4", "dc-allowed",    HealthStatus::HEALTHY),
    };

    PlacementConstraints c;
    c.healthy_only          = true;
    c.forbidden_datacenters = {"dc-restricted"};

    GeoReplicaPlacementManager mgr;
    const auto candidate = mgr.selectLeaderCandidate(replicas, c);

    ASSERT_TRUE(candidate.has_value())
        << "Must find candidate outside forbidden DC";
    EXPECT_NE(candidate->datacenter, "dc-restricted")
        << "Forbidden DC must never be selected even under failure";
    EXPECT_EQ(candidate->datacenter, "dc-allowed");
}

// ===========================================================================
// CHAOS-05: Concurrent failover candidate queries are deterministic
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS05_ConcurrentQueriesAreDeterministic)
{
    // Stable topology with a clear best candidate.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("best",  "dc-a", HealthStatus::HEALTHY, 10, 500),
        makeReplica("ok",    "dc-b", HealthStatus::HEALTHY,  5, 200),
        makeReplica("worst", "dc-c", HealthStatus::HEALTHY,  1, 100),
    };

    PlacementConstraints c;
    c.preferred_datacenters = {"dc-a", "dc-b", "dc-c"};
    c.healthy_only          = true;

    GeoReplicaPlacementManager mgr;

    // Run 8 concurrent threads each querying selectLeaderCandidate.
    constexpr int kThreads = 8;
    constexpr int kIter    = 50;

    std::vector<std::string> results(kThreads);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            std::string last;
            for (int i = 0; i < kIter; ++i) {
                const auto cand = mgr.selectLeaderCandidate(replicas, c);
                if (cand) {
                    if (!last.empty()) {
                        EXPECT_EQ(cand->node_id, last)
                            << "Concurrent queries must be deterministic (thread " << t << ")";
                    }
                    last = cand->node_id;
                }
            }
            results[t] = last;
        });
    }

    for (auto& th : threads) th.join();

    // All threads should have selected the same candidate.
    for (int t = 1; t < kThreads; ++t) {
        EXPECT_EQ(results[t], results[0])
            << "All threads must select the same leader candidate";
    }
}

// ===========================================================================
// CHAOS-06: WAL shipper survives transport handler that always throws
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS06_WalShipperSurvivesAlwaysThrowingTransport)
{
    std::atomic<int> attempts{0};

    auto throwing_transport = [&](const WalSegment&) {
        ++attempts;
        throw std::runtime_error("chaos: simulated transport failure");
    };

    AsyncWalShipper shipper(makeConfig(5000, std::move(throwing_transport)));

    // Enqueue several segments — shipper must not crash or deadlock.
    for (int i = 0; i < 5; ++i) {
        shipper.enqueueSegment(makeAgedSegment(static_cast<uint64_t>(i), 0ms));
    }

    // Give the background worker a moment to attempt shipping.
    for (int i = 0; i < 100; ++i) {
        if (attempts.load() >= 1) break;
        std::this_thread::sleep_for(5ms);
    }

    // Shipper must still be alive and stoppable without deadlock.
    EXPECT_NO_THROW(shipper.stop())
        << "Shipper must be stoppable after repeated transport failures";
    EXPECT_GE(attempts.load(), 1)
        << "Transport handler must have been called at least once";
}

// ===========================================================================
// CHAOS-07: WAL shipper survives transport handler that blocks until timeout
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS07_WalShipperSurvivesBlockingTransport)
{
    // Transport that blocks for 150 ms — short enough not to hang the test,
    // long enough to exercise the timeout path in executeFallback().
    std::atomic<int> completed{0};

    auto blocking_transport = [&](const WalSegment&) {
        std::this_thread::sleep_for(150ms);
        ++completed;
    };

    WalShippingConfig cfg = makeConfig(50, std::move(blocking_transport));
    cfg.io_timeout_ms     = 100;  // timeout shorter than handler sleep

    AsyncWalShipper shipper(cfg);
    shipper.enqueueSegment(makeAgedSegment(1, 0ms));

    // Allow enough wall time for at least one attempt.
    std::this_thread::sleep_for(400ms);

    // Shipper must be stoppable; deadlock here is a test failure.
    EXPECT_NO_THROW(shipper.stop())
        << "Shipper must be stoppable after transport timeout";
}

// ===========================================================================
// CHAOS-08: validatePlacement returns violations when quorum is lost
// ===========================================================================

TEST(ChaosFailoverTest, CHAOS08_ValidatePlacementDetectsQuorumLoss)
{
    // Three required DCs; only two are populated with healthy replicas.
    std::vector<ReplicaInfo> replicas = {
        makeReplica("n1", "dc-a", HealthStatus::HEALTHY),
        makeReplica("n2", "dc-b", HealthStatus::HEALTHY),
        // dc-required has no replicas at all
    };

    PlacementConstraints c;
    c.required_datacenters  = {"dc-a", "dc-b", "dc-required"};
    c.healthy_only          = true;
    c.min_copies_per_dc     = 1;

    GeoReplicaPlacementManager mgr;
    const auto violations = mgr.validatePlacement(replicas, c);

    EXPECT_FALSE(violations.empty())
        << "validatePlacement must report violations when a required DC is absent";

    bool found_dc_required = false;
    for (const auto& v : violations) {
        if (v.find("dc-required") != std::string::npos) {
            found_dc_required = true;
            break;
        }
    }
    EXPECT_TRUE(found_dc_required)
        << "Violation message must reference the missing required datacenter";
}
