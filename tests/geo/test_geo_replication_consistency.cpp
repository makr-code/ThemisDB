/**
 * ThemisDB – Geo-Replication with Consistency Levels
 *
 * Unit tests for GeoReplicationManager (v1.7.0).
 * Covers all acceptance criteria:
 *   AC-1  Per-request consistency level
 *   AC-2  Consistency levels: STRONG, BOUNDED_STALENESS, SESSION, EVENTUAL
 *   AC-3  Automatic routing based on consistency requirements
 *   AC-4  Session tokens for read-your-writes guarantee
 *   AC-5  STRONG: Linearizable, up-to-date reads
 *   AC-6  BOUNDED_STALENESS: Stale by at most N seconds/versions
 *   AC-7  SESSION: Read-your-writes within session
 *   AC-8  EVENTUAL: No guarantee, best performance
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "replication/replication_manager.h"

#include <thread>
#include <chrono>
#include <limits>
#include <atomic>
#include <vector>

using namespace themisdb::replication;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GeoReplicationManager::GeoConfig makeGeoConfig(
    const std::string& local = "us-east-1",
    uint32_t max_staleness_ms = 5000)
{
    GeoReplicationManager::GeoConfig cfg;
    cfg.local_region        = local;
    cfg.regions             = {local, "eu-west-1", "ap-south-1"};
    cfg.replication_factor  = 3;
    cfg.local_replicas      = 2;
    cfg.global_replicas     = 1;
    cfg.default_consistency = ConsistencyLevel::SESSION;
    cfg.max_staleness_ms    = max_staleness_ms;
    cfg.session_token_ttl_ms = 30000;
    return cfg;
}

// ============================================================================
// 1. Construction & initial state
// ============================================================================

TEST(GeoReplicationConsistencyTest, ConstructionSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig());
    // Must not throw
}

TEST(GeoReplicationConsistencyTest, LocalRegionStartsFresh) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    EXPECT_EQ(mgr.getStaleness("us-east-1").count(), 0);
}

TEST(GeoReplicationConsistencyTest, PeerRegionStartsWithHighStaleness) {
    GeoReplicationManager mgr(makeGeoConfig());
    // Peer regions start with unknown (very high) staleness
    EXPECT_GT(mgr.getStaleness("eu-west-1").count(),
              static_cast<int64_t>(1e12));
}

TEST(GeoReplicationConsistencyTest, UnknownRegionReturnsMaxStaleness) {
    GeoReplicationManager mgr(makeGeoConfig());
    auto s = mgr.getStaleness("nonexistent");
    EXPECT_EQ(s.count(), std::numeric_limits<int64_t>::max());
}

// ============================================================================
// 2. Staleness updates
// ============================================================================

TEST(GeoReplicationConsistencyTest, UpdateRegionStalenessRefreshesLag) {
    GeoReplicationManager mgr(makeGeoConfig());
    mgr.updateRegionStaleness("eu-west-1", 200, 5);
    EXPECT_EQ(mgr.getStaleness("eu-west-1").count(), 200);
}

TEST(GeoReplicationConsistencyTest, UpdateRegionStalenessAddsNewRegion) {
    GeoReplicationManager mgr(makeGeoConfig());
    mgr.updateRegionStaleness("sa-east-1", 100, 3);
    EXPECT_EQ(mgr.getStaleness("sa-east-1").count(), 100);
}

TEST(GeoReplicationConsistencyTest, UpdateLocalRegionStaleness) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 50, 2);
    EXPECT_EQ(mgr.getStaleness("us-east-1").count(), 50);
}

// ============================================================================
// 3. Session tokens  (AC-4, AC-7)
// ============================================================================

TEST(GeoReplicationConsistencyTest, GetSessionTokenReturnsNonEmpty) {
    GeoReplicationManager mgr(makeGeoConfig());
    std::string tok = mgr.getSessionToken();
    EXPECT_FALSE(tok.empty());
}

TEST(GeoReplicationConsistencyTest, SessionTokenContainsSeqAndRegion) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1"));
    std::string tok = mgr.getSessionToken();
    EXPECT_NE(tok.find("seq="), std::string::npos);
    EXPECT_NE(tok.find("region=us-east-1"), std::string::npos);
}

TEST(GeoReplicationConsistencyTest, SessionTokenParsesZeroAtStart) {
    GeoReplicationManager mgr(makeGeoConfig());
    std::string tok = mgr.getSessionToken();
    uint64_t seq = mgr.parseSessionToken(tok);
    EXPECT_EQ(seq, 0u);
}

TEST(GeoReplicationConsistencyTest, SessionTokenAdvancesAfterWrite) {
    GeoReplicationManager mgr(makeGeoConfig());
    mgr.write("k1", "v1", ConsistencyLevel::SESSION);
    std::string tok = mgr.getSessionToken();
    uint64_t seq = mgr.parseSessionToken(tok);
    EXPECT_EQ(seq, 1u);
}

TEST(GeoReplicationConsistencyTest, ParseInvalidTokenReturnsZero) {
    GeoReplicationManager mgr(makeGeoConfig());
    EXPECT_EQ(mgr.parseSessionToken(""), 0u);
    EXPECT_EQ(mgr.parseSessionToken("garbage"), 0u);
}

// ============================================================================
// 4. Automatic routing  (AC-3)
// ============================================================================

TEST(GeoReplicationConsistencyTest, RoutingStrongEligibleWhenFresh) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    // Local region is fresh (staleness == 0)
    std::string region = mgr.selectReadRegion(ConsistencyLevel::STRONG);
    EXPECT_EQ(region, "us-east-1");
}

TEST(GeoReplicationConsistencyTest, RoutingStrongRejectWhenStale) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 100, 0);  // introduce lag
    std::string region = mgr.selectReadRegion(ConsistencyLevel::STRONG);
    EXPECT_TRUE(region.empty());
}

TEST(GeoReplicationConsistencyTest, RoutingBoundedStalenessEligibleWithinBound) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 3000, 1);  // within 5000ms bound
    std::string region = mgr.selectReadRegion(ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_EQ(region, "us-east-1");
}

TEST(GeoReplicationConsistencyTest, RoutingBoundedStalenessRejectWhenExceeded) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 6000, 0);  // exceeds 5000ms
    std::string region = mgr.selectReadRegion(ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_TRUE(region.empty());
}

TEST(GeoReplicationConsistencyTest, RoutingSessionEligibleWhenSequenceMet) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    // Write to advance local sequence to 1, then get token
    mgr.write("k", "v", ConsistencyLevel::SESSION);
    std::string tok = mgr.getSessionToken();
    std::string region = mgr.selectReadRegion(ConsistencyLevel::SESSION, tok);
    EXPECT_EQ(region, "us-east-1");
}

TEST(GeoReplicationConsistencyTest, RoutingEventualAlwaysSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    // Even with high local staleness, EVENTUAL always routes to local
    mgr.updateRegionStaleness("us-east-1", 100000, 0);
    std::string region = mgr.selectReadRegion(ConsistencyLevel::EVENTUAL);
    EXPECT_EQ(region, "us-east-1");
}

// SESSION with a garbage token: parseSessionToken returns 0 → required_seq=0
// → always satisfies local_applied_sequence >= 0 → routes to local region
TEST(GeoReplicationConsistencyTest, RoutingSessionGarbageTokenTreatedAsNoToken) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    std::string region = mgr.selectReadRegion(ConsistencyLevel::SESSION, "not_a_valid_token");
    EXPECT_EQ(region, "us-east-1");
}

// SESSION without a token at all: required_seq=0, always succeeds
TEST(GeoReplicationConsistencyTest, RoutingSessionNoTokenAlwaysSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    std::string region = mgr.selectReadRegion(ConsistencyLevel::SESSION, "");
    EXPECT_EQ(region, "us-east-1");
}

// STRONG write rejection does NOT increment writes_total_
TEST(GeoReplicationConsistencyTest, WritesCounterNotIncrementedOnStrongRejection) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 100, 0);  // make local stale
    bool ok = mgr.write("key", "val", ConsistencyLevel::STRONG);
    EXPECT_FALSE(ok);
    // Rejected write should NOT appear in Prometheus writes_total
    std::string m = mgr.exportPrometheusMetrics();
    EXPECT_NE(m.find("themisdb_geo_repl_writes_total{region=\"us-east-1\"} 0"),
              std::string::npos);
}


TEST(GeoReplicationConsistencyTest, WriteStrongSucceedsWhenFresh) {
    GeoReplicationManager mgr(makeGeoConfig());
    EXPECT_TRUE(mgr.write("key", "val", ConsistencyLevel::STRONG));
}

TEST(GeoReplicationConsistencyTest, WriteStrongFailsWhenStale) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 200, 0);  // introduce lag
    EXPECT_FALSE(mgr.write("key", "val", ConsistencyLevel::STRONG));
}

TEST(GeoReplicationConsistencyTest, WriteSessionAlwaysSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig());
    mgr.updateRegionStaleness("us-east-1", 9999, 0);  // high lag
    EXPECT_TRUE(mgr.write("k", "v", ConsistencyLevel::SESSION));
}

TEST(GeoReplicationConsistencyTest, WriteEventualAlwaysSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig());
    EXPECT_TRUE(mgr.write("k", "v", ConsistencyLevel::EVENTUAL));
}

TEST(GeoReplicationConsistencyTest, WriteBoundedStalenessAlwaysSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig());
    EXPECT_TRUE(mgr.write("k", "v", ConsistencyLevel::BOUNDED_STALENESS));
}

// ============================================================================
// 6. Read behaviour  (AC-1, AC-2)
// ============================================================================

TEST(GeoReplicationConsistencyTest, ReadEventualAlwaysSucceeds) {
    GeoReplicationManager mgr(makeGeoConfig());
    auto result = mgr.read("key", ConsistencyLevel::EVENTUAL);
    EXPECT_TRUE(result.has_value());
}

TEST(GeoReplicationConsistencyTest, ReadStrongSucceedsWhenFresh) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    // Local starts at staleness 0
    auto result = mgr.read("key", ConsistencyLevel::STRONG);
    EXPECT_TRUE(result.has_value());
}

TEST(GeoReplicationConsistencyTest, ReadStrongFailsWhenStale) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 10, 0);
    auto result = mgr.read("key", ConsistencyLevel::STRONG);
    EXPECT_FALSE(result.has_value());
}

TEST(GeoReplicationConsistencyTest, ReadBoundedStalenessWithinBound) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 2000, 1);
    auto result = mgr.read("key", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_TRUE(result.has_value());
}

TEST(GeoReplicationConsistencyTest, ReadBoundedStalenessExceedsBound) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 8000, 0);
    auto result = mgr.read("key", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_FALSE(result.has_value());
}

// AC-7: SESSION read-your-writes guarantee
TEST(GeoReplicationConsistencyTest, ReadSessionReadYourWrites) {
    GeoReplicationManager mgr(makeGeoConfig());
    mgr.write("key", "val", ConsistencyLevel::SESSION);
    std::string tok = mgr.getSessionToken();
    auto result = mgr.read("key", ConsistencyLevel::SESSION, tok);
    EXPECT_TRUE(result.has_value());
}

// ============================================================================
// 7. Prometheus metrics
// ============================================================================

TEST(GeoReplicationConsistencyTest, PrometheusMetricsContainsExpectedKeys) {
    GeoReplicationManager mgr(makeGeoConfig());
    mgr.write("k", "v");
    mgr.read("k", ConsistencyLevel::EVENTUAL);

    std::string m = mgr.exportPrometheusMetrics();
    EXPECT_NE(m.find("themisdb_geo_repl_writes_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_geo_repl_reads_total"),  std::string::npos);
    EXPECT_NE(m.find("themisdb_geo_repl_reads_rejected_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_geo_repl_region_staleness_ms"), std::string::npos);
}

TEST(GeoReplicationConsistencyTest, PrometheusCountersAccurate) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    mgr.write("k1", "v1");
    mgr.write("k2", "v2");
    mgr.read("k1", ConsistencyLevel::EVENTUAL);

    std::string m = mgr.exportPrometheusMetrics();
    EXPECT_NE(m.find("themisdb_geo_repl_writes_total{region=\"us-east-1\"} 2"),
              std::string::npos);
    EXPECT_NE(m.find("themisdb_geo_repl_reads_total{region=\"us-east-1\"} 1"),
              std::string::npos);
}

// ============================================================================
// 8. Thread safety
// ============================================================================

TEST(GeoReplicationConsistencyTest, ConcurrentWritesDoNotRace) {
    GeoReplicationManager mgr(makeGeoConfig());
    const int N = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&mgr, &successes] {
            if (mgr.write("k", "v", ConsistencyLevel::EVENTUAL)) {
              ++successes;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(successes.load(), N);
}

TEST(GeoReplicationConsistencyTest, ConcurrentReadsDoNotRace) {
    GeoReplicationManager mgr(makeGeoConfig());
    const int N = 100;
    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&mgr] {
            mgr.read("k", ConsistencyLevel::EVENTUAL);
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    // Must not crash
}

TEST(GeoReplicationConsistencyTest, ConcurrentStalenessUpdateAndReadDoNotRace) {
    GeoReplicationManager mgr(makeGeoConfig("us-east-1", 5000));
    std::atomic<bool> stop{false};
    std::thread updater([&] {
        for (int i = 0; i < 200 && !stop.load(); ++i) {
            mgr.updateRegionStaleness("us-east-1",
                                      static_cast<int64_t>(i * 10),
                                      static_cast<uint64_t>(i));
        }
        stop.store(true);
    });
    std::thread reader([&] {
        while (!stop.load()) {
            mgr.read("k", ConsistencyLevel::BOUNDED_STALENESS);
        }
    });
    updater.join();
    reader.join();
    // Must not crash or deadlock
}
