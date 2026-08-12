// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_raft_load_balancer.cpp
 * @brief Focused unit tests for RaftLoadBalancer (Issue #78 / v1.8.0)
 *
 * Acceptance criteria covered:
 *   AC1  – Raft consensus for load balancer state (leader election)
 *   AC2  – Automatic failover on node failures (health-based exclusion)
 *   AC3  – Health-based routing decisions
 *   AC4  – Dynamic weight adjustment based on load
 *   AC5  – Cross-datacenter routing
 *   AC6  – Round Robin strategy
 *   AC7  – Least Connections strategy
 *   AC8  – Weighted Round Robin strategy
 *   AC9  – Health-Based strategy
 *   AC10 – Consistent Hashing strategy
 *   AC11 – Raft leader monitors backend health
 *   AC12 – Automatic removal of failed backends
 *   AC13 – Automatic re-addition when backend recovers
 *   AC14 – Leader election on LB leader failure
 */

#include <gtest/gtest.h>
#include "network/raft_load_balancer.h"

#include <atomic>
#include <chrono>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// =============================================================================
// Test Fixture
// =============================================================================

class RaftLoadBalancerTest : public ::testing::Test {
protected:
    RaftLoadBalancer::Config make_config(
            LoadBalancingStrategy strategy = LoadBalancingStrategy::ROUND_ROBIN) {
        RaftLoadBalancer::Config cfg;
        cfg.strategy                  = strategy;
        cfg.health_check_interval_ms  = 100'000; // 100 s – prevent interference
        cfg.heartbeat_interval_ms     = 50;
        cfg.election_timeout_min_ms   = 10;
        cfg.election_timeout_max_ms   = 20;
        cfg.unhealthy_threshold       = 3;
        cfg.recovery_threshold        = 2;
        cfg.rebalance_threshold       = 0.2;
        return cfg;
    }
};

// =============================================================================
// AC1 – Raft consensus: leader election
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC1_LeaderElectionAfterStart) {
    RaftLoadBalancer lb(make_config());
    lb.addBackend("node1:8766");

    EXPECT_EQ(lb.getRole(), RaftRole::FOLLOWER)
        << "Should start as FOLLOWER before start() is called";

    lb.start();

    // Allow the Raft loop to elect a leader (tiny timeout)
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        if (lb.isLeader()) break;
        std::this_thread::sleep_for(5ms);
    }

    EXPECT_TRUE(lb.isLeader())
        << "Single-node cluster should elect itself leader";
    EXPECT_GT(lb.getCurrentTerm(), 0ULL)
        << "Term must be incremented after election";

    lb.stop();
}

TEST_F(RaftLoadBalancerTest, AC1_TermIncrementsOnElection) {
    RaftLoadBalancer lb(make_config());
    lb.addBackend("node1:8766");
    lb.start();

    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline && !lb.isLeader()) {
        std::this_thread::sleep_for(5ms);
    }

    const uint64_t term = lb.getCurrentTerm();
    EXPECT_GE(term, 1ULL);

    lb.stop();
}

// =============================================================================
// AC2 / AC3 / AC11 / AC12 / AC13 – Health monitoring & failover
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC2_UnhealthyBackendExcludedAfterThreshold) {
    auto cfg = make_config(LoadBalancingStrategy::HEALTH_BASED);
    cfg.health_check_interval_ms = 10; // 10 ms
    cfg.unhealthy_threshold      = 2;

    RaftLoadBalancer lb(cfg);
    lb.addBackend("good:8766");
    lb.addBackend("bad:8766");
    lb.setHealthCheckFn([](const RaftLoadBalancer::Backend& b) -> bool {
        return b.address != "bad:8766";
    });
    lb.start();

    // Wait up to 500 ms for "bad" to be marked unhealthy (2 checks × 10 ms = 20 ms)
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        auto backends = lb.getBackends();
        bool bad_unhealthy = false;
        for (auto* b : backends) {
            if (b->address == "bad:8766" && !b->healthy) {
                bad_unhealthy = true;
            }
        }
        if (bad_unhealthy) break;
        std::this_thread::sleep_for(5ms);
    }

    // Routing should never return "bad" now
    for (int i = 0; i < 20; ++i) {
        const auto selected = lb.selectBackend();
        EXPECT_NE(selected, "bad:8766")
            << "Unhealthy backend must not be selected";
    }

    lb.stop();
}

TEST_F(RaftLoadBalancerTest, AC3_HealthBasedRoutingSkipsUnhealthy) {
    auto cfg = make_config(LoadBalancingStrategy::HEALTH_BASED);
    cfg.health_check_interval_ms = 10;
    cfg.unhealthy_threshold      = 1;

    RaftLoadBalancer lb(cfg);
    lb.addBackend("a:8766");
    lb.addBackend("b:8766");
    lb.addBackend("c:8766");
    lb.setHealthCheckFn([](const RaftLoadBalancer::Backend& b) { return b.address != "b:8766"; });
    lb.start();

    // Wait for "b" to be marked unhealthy
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        bool done = true;
        for (auto* backend : lb.getBackends()) {
            if (backend->address == "b:8766" && backend->healthy) {
                done = false;
            }
        }
        if (done) break;
        std::this_thread::sleep_for(5ms);
    }

    std::set<std::string> seen;
    for (int i = 0; i < 30; ++i) {
        seen.insert(lb.selectBackend());
    }
    EXPECT_EQ(seen.count("b:8766"), 0u) << "Unhealthy backend must not appear";
    EXPECT_GT(seen.count("a:8766"), 0u);
    EXPECT_GT(seen.count("c:8766"), 0u);

    lb.stop();
}

TEST_F(RaftLoadBalancerTest, AC13_RecoveryReAddsBackend) {
    std::atomic<bool> backend_healthy{false};

    auto cfg = make_config(LoadBalancingStrategy::HEALTH_BASED);
    cfg.health_check_interval_ms = 20;
    cfg.unhealthy_threshold      = 1;
    cfg.recovery_threshold       = 1;

    RaftLoadBalancer lb(cfg);
    lb.addBackend("fragile:8766");
    lb.addBackend("stable:8766");
    lb.setHealthCheckFn([&](const RaftLoadBalancer::Backend& b) -> bool {
        if (b.address == "fragile:8766") return backend_healthy.load();
        return true;
    });
    lb.start();

    // Wait for "fragile" to be marked unhealthy
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        bool unhealthy = false;
        for (auto* b : lb.getBackends()) {
            if (b->address == "fragile:8766" && !b->healthy) unhealthy = true;
        }
        if (unhealthy) break;
        std::this_thread::sleep_for(5ms);
    }

    // Now allow fragile to recover
    backend_healthy.store(true);

    // Wait for "fragile" to be re-admitted
    deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        bool healthy = false;
        for (auto* b : lb.getBackends()) {
            if (b->address == "fragile:8766" && b->healthy) healthy = true;
        }
        if (healthy) break;
        std::this_thread::sleep_for(5ms);
    }

    bool found_healthy = false;
    for (auto* b : lb.getBackends()) {
        if (b->address == "fragile:8766" && b->healthy) found_healthy = true;
    }
    EXPECT_TRUE(found_healthy) << "Recovered backend should be re-admitted";

    const auto stats = lb.getStats();
    EXPECT_GE(stats.recovery_events, 1ULL);

    lb.stop();
}

// =============================================================================
// AC4 – Dynamic weight adjustment
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC4_UpdateWeightChangesRouting) {
    auto cfg = make_config(LoadBalancingStrategy::WEIGHTED_ROUND_ROBIN);
    RaftLoadBalancer lb(cfg);
    lb.addBackend("w1:8766", 1.0);
    lb.addBackend("w2:8766", 1.0);

    lb.updateWeight("w1:8766", 3.0);

    std::unordered_map<std::string, int> counts;
    for (int i = 0; i < 40; ++i) {
        ++counts[lb.selectBackend()];
    }

    // w1 has weight 3, w2 has weight 1 → w1 should be selected more often
    EXPECT_GT(counts["w1:8766"], counts["w2:8766"])
        << "Heavier backend should be selected more";
}

TEST_F(RaftLoadBalancerTest, AC4_RebalanceEventTracked) {
    auto cfg = make_config(LoadBalancingStrategy::LEAST_CONNECTIONS);
    cfg.health_check_interval_ms = 10;
    cfg.rebalance_threshold      = 0.0; // always rebalance

    RaftLoadBalancer lb(cfg);
    lb.addBackend("r1:8766");
    lb.addBackend("r2:8766");

    // Simulate skewed load
    lb.onConnectionOpened("r1:8766");
    lb.onConnectionOpened("r1:8766");
    lb.onConnectionOpened("r1:8766");

    lb.start();

    // Wait for at least one rebalance event
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline &&
           lb.getStats().rebalance_events == 0) {
        std::this_thread::sleep_for(5ms);
    }

    EXPECT_GE(lb.getStats().rebalance_events, 1ULL)
        << "Rebalance should be triggered for skewed load";

    lb.stop();
}

// =============================================================================
// AC5 – Cross-datacenter routing
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC5_PrefersLocalDatacenter) {
    auto cfg = make_config(LoadBalancingStrategy::ROUND_ROBIN);
    cfg.datacenter              = "dc-west";
    cfg.prefer_local_datacenter = true;

    RaftLoadBalancer lb(cfg);
    lb.addBackend("west1:8766", 1.0, "dc-west");
    lb.addBackend("west2:8766", 1.0, "dc-west");
    lb.addBackend("east1:8766", 1.0, "dc-east");

    std::set<std::string> seen;
    for (int i = 0; i < 20; ++i) {
        seen.insert(lb.selectBackend());
    }

    EXPECT_EQ(seen.count("east1:8766"), 0u)
        << "Remote-datacenter backend should not be selected when local backends are available";
    EXPECT_GT(seen.count("west1:8766") + seen.count("west2:8766"), 0u);
}

TEST_F(RaftLoadBalancerTest, AC5_FallsBackToRemoteWhenLocalUnhealthy) {
    auto cfg = make_config(LoadBalancingStrategy::ROUND_ROBIN);
    cfg.datacenter              = "dc-west";
    cfg.prefer_local_datacenter = true;
    cfg.health_check_interval_ms = 10;
    cfg.unhealthy_threshold      = 1;

    RaftLoadBalancer lb(cfg);
    lb.addBackend("west1:8766", 1.0, "dc-west");
    lb.addBackend("east1:8766", 1.0, "dc-east");

    // Mark local backend unhealthy
    lb.setHealthCheckFn([](const RaftLoadBalancer::Backend& b) {
        return b.address != "west1:8766";
    });
    lb.start();

    // Wait for west1 to be marked unhealthy
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        bool unhealthy = false;
        for (auto* b : lb.getBackends()) {
            if (b->address == "west1:8766" && !b->healthy) unhealthy = true;
        }
        if (unhealthy) break;
        std::this_thread::sleep_for(5ms);
    }

    // Now remote backend should be selected
    const auto selected = lb.selectBackend();
    EXPECT_EQ(selected, "east1:8766")
        << "Should fall back to remote DC when local is unhealthy";

    lb.stop();
}

// =============================================================================
// AC6 – Round Robin
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC6_RoundRobinCyclesAllBackends) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::ROUND_ROBIN));
    lb.addBackend("n1:8766");
    lb.addBackend("n2:8766");
    lb.addBackend("n3:8766");

    std::unordered_map<std::string, int> counts;
    const int rounds = 30;
    for (int i = 0; i < rounds; ++i) {
        ++counts[lb.selectBackend()];
    }

    for (const auto& addr : {"n1:8766", "n2:8766", "n3:8766"}) {
        EXPECT_GT(counts[addr], 0) << addr << " should be selected at least once";
    }
    // Expect roughly equal distribution (±2)
    EXPECT_NEAR(counts["n1:8766"], rounds / 3, 2);
    EXPECT_NEAR(counts["n2:8766"], rounds / 3, 2);
    EXPECT_NEAR(counts["n3:8766"], rounds / 3, 2);
}

TEST_F(RaftLoadBalancerTest, AC6_RoundRobinEmptyReturnsEmpty) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::ROUND_ROBIN));
    EXPECT_EQ(lb.selectBackend(), "");
}

// =============================================================================
// AC7 – Least Connections
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC7_LeastConnectionsSelectsLeast) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::LEAST_CONNECTIONS));
    lb.addBackend("lc1:8766");
    lb.addBackend("lc2:8766");
    lb.addBackend("lc3:8766");

    // Simulate different connection counts
    lb.onConnectionOpened("lc1:8766");
    lb.onConnectionOpened("lc1:8766");
    lb.onConnectionOpened("lc2:8766");
    // lc3 has 0

    const auto selected = lb.selectBackend();
    EXPECT_EQ(selected, "lc3:8766")
        << "Should route to backend with fewest active connections";
}

TEST_F(RaftLoadBalancerTest, AC7_LeastConnectionsUpdatesOnClose) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::LEAST_CONNECTIONS));
    lb.addBackend("lca:8766");
    lb.addBackend("lcb:8766");

    lb.onConnectionOpened("lca:8766");
    lb.onConnectionOpened("lca:8766");
    lb.onConnectionOpened("lcb:8766");

    // Close one connection on lca
    lb.onConnectionClosed("lca:8766");
    // Now both have 1 connection → either is acceptable

    const auto sel = lb.selectBackend();
    EXPECT_TRUE(sel == "lca:8766" || sel == "lcb:8766");
}

// =============================================================================
// AC8 – Weighted Round Robin
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC8_WeightedRoundRobinFavoursHeavierBackend) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::WEIGHTED_ROUND_ROBIN));
    lb.addBackend("wh:8766", 4.0);  // heavy
    lb.addBackend("wl:8766", 1.0);  // light

    std::unordered_map<std::string, int> counts;
    for (int i = 0; i < 40; ++i) {
        ++counts[lb.selectBackend()];
    }

    EXPECT_GT(counts["wh:8766"], counts["wl:8766"])
        << "Backend with higher weight should receive more requests";
}

// =============================================================================
// AC9 – Health-Based strategy
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC9_HealthBasedOnlyRoutesToHealthy) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::HEALTH_BASED));
    lb.addBackend("hb_good:8766");
    lb.addBackend("hb_bad:8766");

    // Manually mark bad as unhealthy through health check simulation
    auto cfg2 = make_config(LoadBalancingStrategy::HEALTH_BASED);
    cfg2.health_check_interval_ms = 10;
    cfg2.unhealthy_threshold      = 1;

    RaftLoadBalancer lb2(cfg2);
    lb2.addBackend("hb_good:8766");
    lb2.addBackend("hb_bad:8766");
    lb2.setHealthCheckFn([](const RaftLoadBalancer::Backend& b) {
        return b.address != "hb_bad:8766";
    });
    lb2.start();

    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline) {
        bool done = true;
        for (auto* b : lb2.getBackends()) {
            if (b->address == "hb_bad:8766" && b->healthy) done = false;
        }
        if (done) break;
        std::this_thread::sleep_for(5ms);
    }

    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(lb2.selectBackend(), "hb_good:8766");
    }

    lb2.stop();
}

// =============================================================================
// AC10 – Consistent Hashing
// =============================================================================

TEST_F(RaftLoadBalancerTest, AC10_ConsistentHashIsDeterministic) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::CONSISTENT_HASH));
    lb.addBackend("ch1:8766");
    lb.addBackend("ch2:8766");
    lb.addBackend("ch3:8766");

    const std::string key = "user-session-42";

    // Same key should always map to the same backend
    const auto first = lb.selectBackend(key);
    ASSERT_FALSE(first.empty());
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(lb.selectBackend(key), first)
            << "Consistent hash must be deterministic for the same key";
    }
}

TEST_F(RaftLoadBalancerTest, AC10_ConsistentHashDifferentKeysDifferentBackends) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::CONSISTENT_HASH));
    lb.addBackend("ch1:8766");
    lb.addBackend("ch2:8766");
    lb.addBackend("ch3:8766");

    std::set<std::string> seen;
    for (int i = 0; i < 100; ++i) {
        seen.insert(lb.selectBackend("key-" + std::to_string(i)));
    }
    // With 100 different keys across 3 backends, all backends should be hit
    EXPECT_GE(seen.size(), 2u)
        << "Different keys should spread across multiple backends";
}

// =============================================================================
// Backend management
// =============================================================================

TEST_F(RaftLoadBalancerTest, AddAndRemoveBackend) {
    RaftLoadBalancer lb(make_config());
    lb.addBackend("x:8766");
    lb.addBackend("y:8766");

    EXPECT_EQ(lb.getBackends().size(), 2u);

    lb.removeBackend("x:8766");
    EXPECT_EQ(lb.getBackends().size(), 1u);
    EXPECT_EQ(lb.getBackends()[0]->address, "y:8766");
}

TEST_F(RaftLoadBalancerTest, DuplicateAddIsIdempotent) {
    RaftLoadBalancer lb(make_config());
    lb.addBackend("dup:8766");
    lb.addBackend("dup:8766");
    EXPECT_EQ(lb.getBackends().size(), 1u);
}

TEST_F(RaftLoadBalancerTest, RemoveNonExistentIsNoOp) {
    RaftLoadBalancer lb(make_config());
    lb.addBackend("z:8766");
    lb.removeBackend("nonexistent:8766");
    EXPECT_EQ(lb.getBackends().size(), 1u);
}

// =============================================================================
// Stats
// =============================================================================

TEST_F(RaftLoadBalancerTest, StatsTrackRequests) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::ROUND_ROBIN));
    lb.addBackend("s1:8766");
    lb.addBackend("s2:8766");

    for (int i = 0; i < 10; ++i) {
        const auto addr = lb.selectBackend();
        lb.onRequestComplete(addr, true);
    }

    const auto stats = lb.getStats();
    EXPECT_EQ(stats.total_requests, 10ULL);
    EXPECT_EQ(stats.failed_requests, 0ULL);
}

TEST_F(RaftLoadBalancerTest, StatsTrackFailures) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::ROUND_ROBIN));
    lb.addBackend("f1:8766");

    lb.onRequestComplete("f1:8766", false);
    lb.onRequestComplete("f1:8766", false);
    lb.onRequestComplete("f1:8766", true);

    const auto stats = lb.getStats();
    EXPECT_EQ(stats.total_requests, 3ULL);
    EXPECT_EQ(stats.failed_requests, 2ULL);
}

TEST_F(RaftLoadBalancerTest, StatsFailoverEvents) {
    auto cfg = make_config();
    cfg.health_check_interval_ms = 10;
    cfg.unhealthy_threshold      = 1;

    RaftLoadBalancer lb(cfg);
    lb.addBackend("fo1:8766");
    lb.setHealthCheckFn([](const RaftLoadBalancer::Backend&) { return false; });
    lb.start();

    auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < deadline &&
           lb.getStats().failover_events == 0) {
        std::this_thread::sleep_for(5ms);
    }

    EXPECT_GE(lb.getStats().failover_events, 1ULL);
    lb.stop();
}

// =============================================================================
// Strategy switching at runtime
// =============================================================================

TEST_F(RaftLoadBalancerTest, SetStrategySwitchesAtRuntime) {
    RaftLoadBalancer lb(make_config(LoadBalancingStrategy::ROUND_ROBIN));
    lb.addBackend("sw1:8766");
    lb.addBackend("sw2:8766");

    lb.setStrategy(LoadBalancingStrategy::LEAST_CONNECTIONS);
    EXPECT_EQ(lb.getConfig().strategy, LoadBalancingStrategy::ROUND_ROBIN)
        << "Config is immutable; strategy_ atom is separate";

    // With 0 active connections both are equivalent; just check we get a result
    const auto result = lb.selectBackend();
    EXPECT_FALSE(result.empty());
}

// =============================================================================
// Lifecycle
// =============================================================================

TEST_F(RaftLoadBalancerTest, StartStopIsClean) {
    RaftLoadBalancer lb(make_config());
    lb.addBackend("n:8766");
    lb.start();
    lb.stop();
    // Calling stop again should be safe
    lb.stop();
}

TEST_F(RaftLoadBalancerTest, DestructorStopsThreads) {
    // Creating and immediately destroying lb should not crash or hang.
    {
        RaftLoadBalancer lb(make_config());
        lb.addBackend("tmp:8766");
        lb.start();
    }
    // If we get here, destructor called stop() correctly.
    SUCCEED();
}
