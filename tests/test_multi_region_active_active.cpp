/**
 * ThemisDB – Multi-Region Active-Active with Bounded Staleness
 *
 * Unit tests for MultiRegionActiveActiveManager.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "replication/replication_manager.h"

#include <thread>
#include <chrono>

using namespace themisdb::replication;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static MultiRegionActiveActiveConfig makeConfig(
    const std::string& local_region = "us-east-1",
    uint32_t max_staleness_ms = 5000)
{
    MultiRegionActiveActiveConfig cfg;
    cfg.local_region_id    = local_region;
    cfg.peer_region_ids    = {"eu-west-1", "ap-south-1"};
    cfg.default_consistency = ConsistencyLevel::BOUNDED_STALENESS;
    cfg.max_staleness_ms   = max_staleness_ms;
    cfg.session_token_ttl_ms = 30000;
    cfg.conflict_strategy  = ConflictResolution::LAST_WRITE_WINS;
    return cfg;
}

// ============================================================================
// 1. Construction & initial state
// ============================================================================

TEST(MultiRegionActiveActiveTest, ConstructionSucceeds) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    // Should not throw
}

TEST(MultiRegionActiveActiveTest, InitialLocalRegionIsAlwaysFresh) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));
    // Local region starts at 0 staleness
    EXPECT_EQ(mgr.getStaleness("us-east-1").count(), 0);
    EXPECT_TRUE(mgr.isWithinStalenessBound("us-east-1"));
}

TEST(MultiRegionActiveActiveTest, InitialPeerRegionIsUnknown) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    // Peer regions start with very high staleness (unknown)
    EXPECT_GT(mgr.getStaleness("eu-west-1").count(), static_cast<int64_t>(1e12));
    EXPECT_FALSE(mgr.isWithinStalenessBound("eu-west-1"));
}

TEST(MultiRegionActiveActiveTest, UnknownRegionReturnsMaxStaleness) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto stale = mgr.getStaleness("nonexistent-region");
    EXPECT_EQ(stale.count(), std::numeric_limits<int64_t>::max());
}

TEST(MultiRegionActiveActiveTest, GetAllRegionStalenessCoversLocalAndPeers) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1"));
    auto all = mgr.getAllRegionStaleness();
    // Should have local + 2 peers = 3 entries
    EXPECT_EQ(all.size(), 3u);
    std::vector<std::string> ids = {};

    for (const auto& info : all) {
      ids.push_back(info.region_id);
    }
    EXPECT_NE(std::find(ids.begin(), ids.end(), "us-east-1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "eu-west-1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "ap-south-1"), ids.end());
}

// ============================================================================
// 2. updateRegionStaleness
// ============================================================================

TEST(MultiRegionActiveActiveTest, UpdateRegionStalenessRefreshesInfo) {
    MultiRegionActiveActiveManager mgr(makeConfig());

    mgr.updateRegionStaleness("eu-west-1", 1234, 42);
    EXPECT_EQ(mgr.getStaleness("eu-west-1").count(), 1234);
}

TEST(MultiRegionActiveActiveTest, UpdateRegionStalenessAddsNewRegion) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    mgr.updateRegionStaleness("new-region", 500, 10);
    EXPECT_EQ(mgr.getStaleness("new-region").count(), 500);
}

TEST(MultiRegionActiveActiveTest, RegionWithinBoundAfterUpdate) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("eu-west-1", 3000, 5);
    EXPECT_TRUE(mgr.isWithinStalenessBound("eu-west-1"));
}

TEST(MultiRegionActiveActiveTest, RegionOutOfBoundWhenStalenessExceedsMax) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("eu-west-1", 6000, 5);
    EXPECT_FALSE(mgr.isWithinStalenessBound("eu-west-1"));
}

// ============================================================================
// 3. Write
// ============================================================================

TEST(MultiRegionActiveActiveTest, WriteSucceedsAndReturnsWriteId) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto result = mgr.write("users", "u1", "INSERT", R"({"name":"Alice"})");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.write_id.empty());
    EXPECT_EQ(result.region_id, "us-east-1");
    EXPECT_GT(result.sequence_number, 0u);
}

TEST(MultiRegionActiveActiveTest, WriteProducesMonotonicallyIncreasingSequence) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto r1 = mgr.write("col", "d1", "INSERT", "{}");
    auto r2 = mgr.write("col", "d2", "INSERT", "{}");
    EXPECT_GT(r2.sequence_number, r1.sequence_number);
}

TEST(MultiRegionActiveActiveTest, WriteReturnsSessionToken) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto result = mgr.write("col", "d1", "INSERT", "{}",
                            ConsistencyLevel::SESSION);
    EXPECT_FALSE(result.session_token.empty());
    // Token should embed the sequence number
    EXPECT_NE(result.session_token.find("seq="), std::string::npos);
}

TEST(MultiRegionActiveActiveTest, WriteUpdatesLocalRegionToZeroStaleness) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    // Manually inject some lag first
    mgr.updateRegionStaleness("us-east-1", 100, 1);
    EXPECT_EQ(mgr.getStaleness("us-east-1").count(), 100);

    mgr.write("col", "d1", "INSERT", "{}");
    // After a local write, local staleness should be reset to 0
    EXPECT_EQ(mgr.getStaleness("us-east-1").count(), 0);
}

// ============================================================================
// 4. Read – BOUNDED_STALENESS
// ============================================================================

TEST(MultiRegionActiveActiveTest, BoundedStalenessReadSucceedsWhenFresh) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));
    // Local region is fresh (0ms staleness)
    auto result = mgr.read("users", "u1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.served_at, ConsistencyLevel::BOUNDED_STALENESS);
}

TEST(MultiRegionActiveActiveTest, BoundedStalenessReadSucceedsWhenWithinBound) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 4999, 10);

    auto result = mgr.read("users", "u1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.staleness_ms, 4999);
}

TEST(MultiRegionActiveActiveTest, BoundedStalenessReadFailsWhenStalenessExceedsBound) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));
    mgr.updateRegionStaleness("us-east-1", 6000, 10);

    auto result = mgr.read("users", "u1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.staleness_ms, 6000);
}

TEST(MultiRegionActiveActiveTest, BoundedStalenessReadFailsAtExactBoundPlusOne) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 1000));
    mgr.updateRegionStaleness("us-east-1", 1001, 5);

    auto result = mgr.read("col", "d1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_FALSE(result.success);
}

TEST(MultiRegionActiveActiveTest, BoundedStalenessReadSucceedsAtExactBound) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 1000));
    mgr.updateRegionStaleness("us-east-1", 1000, 5);

    auto result = mgr.read("col", "d1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_TRUE(result.success);
}

// ============================================================================
// 5. Read – STRONG consistency
// ============================================================================

TEST(MultiRegionActiveActiveTest, StrongReadSucceedsWhenLocalIsFresh) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1"));
    // Local starts at 0 staleness
    auto result = mgr.read("col", "d1", ConsistencyLevel::STRONG);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.served_at, ConsistencyLevel::STRONG);
}

TEST(MultiRegionActiveActiveTest, StrongReadFailsWhenLocalHasAnyLag) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1"));
    mgr.updateRegionStaleness("us-east-1", 1, 10);  // 1ms lag

    auto result = mgr.read("col", "d1", ConsistencyLevel::STRONG);
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 6. Read – SESSION consistency (read-your-writes)
// ============================================================================

TEST(MultiRegionActiveActiveTest, SessionReadSucceedsWithoutToken) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto result = mgr.read("col", "d1", ConsistencyLevel::SESSION, "");
    EXPECT_TRUE(result.success);
}

TEST(MultiRegionActiveActiveTest, SessionReadSucceedsWhenLocalSeqSufficient) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto w = mgr.write("col", "d1", "INSERT", "{}");
    // Now read back with the session token from the write
    auto result = mgr.read("col", "d1", ConsistencyLevel::SESSION, w.session_token);
    EXPECT_TRUE(result.success);
}

TEST(MultiRegionActiveActiveTest, SessionReadFailsWhenTokenRequiresHigherSeq) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    // Build a session token that embeds a very high sequence (not yet reached)
    // Directly craft a token with seq=99999
    std::string high_seq_token = "seq=99999;region=us-east-1;exp=9999999999999";
    auto result = mgr.read("col", "d1", ConsistencyLevel::SESSION, high_seq_token);
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 7. Read – EVENTUAL consistency
// ============================================================================

TEST(MultiRegionActiveActiveTest, EventualReadAlwaysSucceeds) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 100));
    // Even with extreme staleness
    mgr.updateRegionStaleness("us-east-1", 99999999, 0);

    auto result = mgr.read("col", "d1", ConsistencyLevel::EVENTUAL);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.served_at, ConsistencyLevel::EVENTUAL);
}

TEST(MultiRegionActiveActiveTest, EventualReadSucceedsWithHighStaleness) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 0));  // 0 = no tolerance
    mgr.updateRegionStaleness("us-east-1", 1000000, 0);

    auto result = mgr.read("col", "d1", ConsistencyLevel::EVENTUAL);
    EXPECT_TRUE(result.success);
}

// ============================================================================
// 8. Session tokens
// ============================================================================

TEST(MultiRegionActiveActiveTest, CreateSessionTokenReturnsNonEmptyString) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    std::string token = mgr.createSessionToken();
    EXPECT_FALSE(token.empty());
}

TEST(MultiRegionActiveActiveTest, ValidateSessionTokenSucceedsForZeroRequirement) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    std::string token = mgr.createSessionToken();
    EXPECT_TRUE(mgr.validateSessionToken(token, 0));
}

TEST(MultiRegionActiveActiveTest, ValidateSessionTokenSucceedsWhenSeqMet) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    auto w = mgr.write("col", "d1", "INSERT", "{}");  // seq = 1
    EXPECT_TRUE(mgr.validateSessionToken(w.session_token, w.sequence_number));
}

TEST(MultiRegionActiveActiveTest, ValidateSessionTokenFailsForEmptyToken) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    EXPECT_FALSE(mgr.validateSessionToken("", 0));
}

TEST(MultiRegionActiveActiveTest, ValidateSessionTokenFailsForHighRequirement) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    std::string token = mgr.createSessionToken();  // embeds current (low) sequence
    EXPECT_FALSE(mgr.validateSessionToken(token, 999999));
}

TEST(MultiRegionActiveActiveTest, ValidateSessionTokenFailsForExpiredToken) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    // Craft a token that expired in the past
    std::string expired = "seq=0;region=us-east-1;exp=1";  // epoch ms = 1
    EXPECT_FALSE(mgr.validateSessionToken(expired, 0));
}

// ============================================================================
// 9. Prometheus metrics
// ============================================================================

TEST(MultiRegionActiveActiveTest, PrometheusMetricsContainExpectedKeys) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));

    mgr.write("col", "d1", "INSERT", "{}");
    mgr.read("col", "d1", ConsistencyLevel::BOUNDED_STALENESS);

    std::string m = mgr.exportPrometheusMetrics();

    EXPECT_NE(m.find("themisdb_mraaa_writes_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_mraaa_reads_total"),  std::string::npos);
    EXPECT_NE(m.find("themisdb_mraaa_staleness_rejections_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_mraaa_bounded_staleness_reads_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_mraaa_strong_reads_total"), std::string::npos);
    EXPECT_NE(m.find("themisdb_mraaa_region_staleness_ms"), std::string::npos);
}

TEST(MultiRegionActiveActiveTest, PrometheusMetricsCountersAreAccurate) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));

    mgr.write("col", "d1", "INSERT", "{}");
    mgr.write("col", "d2", "INSERT", "{}");
    mgr.read("col", "d1", ConsistencyLevel::EVENTUAL);

    std::string m = mgr.exportPrometheusMetrics();

    // writes_total = 2
    EXPECT_NE(m.find("themisdb_mraaa_writes_total{region=\"us-east-1\"} 2"), std::string::npos);
    // reads_total = 1
    EXPECT_NE(m.find("themisdb_mraaa_reads_total{region=\"us-east-1\"} 1"), std::string::npos);
}

TEST(MultiRegionActiveActiveTest, PrometheusMetricsReflectStalenessRejection) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 100));
    mgr.updateRegionStaleness("us-east-1", 200, 0);  // exceed bound

    mgr.read("col", "d1", ConsistencyLevel::BOUNDED_STALENESS);  // rejected

    std::string m = mgr.exportPrometheusMetrics();
    EXPECT_NE(m.find("themisdb_mraaa_staleness_rejections_total{region=\"us-east-1\"} 1"),
              std::string::npos);
}

// ============================================================================
// 10. Thread safety
// ============================================================================

TEST(MultiRegionActiveActiveTest, ConcurrentWritesDoNotRace) {
    MultiRegionActiveActiveManager mgr(makeConfig());

    const int N = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&mgr, &successes, i] {
            auto r = mgr.write("col", "doc-" + std::to_string(i), "INSERT", "{}");
            if (r.success) {
              ++successes;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(successes.load(), N);
}

TEST(MultiRegionActiveActiveTest, ConcurrentReadsDoNotRace) {
    MultiRegionActiveActiveManager mgr(makeConfig());
    mgr.write("col", "d1", "INSERT", "{}");

    const int N = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&mgr] {
            mgr.read("col", "d1", ConsistencyLevel::EVENTUAL);
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    // Must not crash or race
}

TEST(MultiRegionActiveActiveTest, ConcurrentUpdateAndReadDoNotRace) {
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 5000));

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
            mgr.read("col", "d1", ConsistencyLevel::BOUNDED_STALENESS);
        }
    });

    updater.join();
    reader.join();
    // Must not crash or deadlock
}

// ============================================================================
// 11. Per-collection consistency overrides
// ============================================================================

static MultiRegionActiveActiveConfig makeConfigWithOverrides(
    const std::string& local_region = "us-east-1",
    uint32_t max_staleness_ms = 5000)
{
    auto cfg = makeConfig(local_region, max_staleness_ms);
    cfg.collection_consistency_overrides["critical_orders"] = ConsistencyLevel::STRONG;
    cfg.collection_consistency_overrides["analytics_events"] = ConsistencyLevel::EVENTUAL;
    return cfg;
}

TEST(MultiRegionActiveActiveTest, GetEffectiveConsistencyReturnsDefaultForUnknownCollection) {
    MultiRegionActiveActiveManager mgr(makeConfigWithOverrides());
    EXPECT_EQ(mgr.getEffectiveConsistency("unknown_col"),
              ConsistencyLevel::BOUNDED_STALENESS);
}

TEST(MultiRegionActiveActiveTest, GetEffectiveConsistencyReturnsOverrideForConfiguredCollection) {
    MultiRegionActiveActiveManager mgr(makeConfigWithOverrides());
    EXPECT_EQ(mgr.getEffectiveConsistency("critical_orders"),
              ConsistencyLevel::STRONG);
    EXPECT_EQ(mgr.getEffectiveConsistency("analytics_events"),
              ConsistencyLevel::EVENTUAL);
}

TEST(MultiRegionActiveActiveTest, ReadAppliesCollectionConsistencyOverride) {
    // analytics_events is overridden to EVENTUAL; a very stale read must still succeed.
    MultiRegionActiveActiveManager mgr(makeConfigWithOverrides("us-east-1", 100));
    mgr.updateRegionStaleness("us-east-1", 99999, 0); // far beyond BOUNDED_STALENESS bound

    // Without override, BOUNDED_STALENESS would fail
    auto bounded_result = mgr.read("unknown_col", "d1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_FALSE(bounded_result.success);

    // With EVENTUAL override, same staleness succeeds
    auto override_result = mgr.read("analytics_events", "d1", ConsistencyLevel::BOUNDED_STALENESS);
    EXPECT_TRUE(override_result.success);
}

TEST(MultiRegionActiveActiveTest, WriteAppliesCollectionConsistencyOverride) {
    // critical_orders is overridden to STRONG; without a leader config this still writes.
    MultiRegionActiveActiveConfig cfg = makeConfigWithOverrides("us-east-1");
    // No leader_region_id set → STRONG write allowed on any region
    MultiRegionActiveActiveManager mgr(cfg);
    auto result = mgr.write("critical_orders", "ord-1", "INSERT", "{}");
    EXPECT_TRUE(result.success);
}

// ============================================================================
// 12. Leader region fencing for STRONG cross-region writes
// ============================================================================

static MultiRegionActiveActiveConfig makeConfigWithLeader(
    const std::string& local_region,
    const std::string& leader_region)
{
    auto cfg = makeConfig(local_region, 5000);
    cfg.leader_region_id = leader_region;
    return cfg;
}

TEST(MultiRegionActiveActiveTest, StrongWriteSucceedsOnLeaderRegion) {
    auto cfg = makeConfigWithLeader("eu-west-1", "eu-west-1");
    MultiRegionActiveActiveManager mgr(cfg);
    auto result = mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::STRONG);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.is_leader_region);
}

TEST(MultiRegionActiveActiveTest, StrongWriteRejectedOnNonLeaderRegion) {
    // Local is us-east-1, leader is eu-west-1 → STRONG write must be rejected
    auto cfg = makeConfigWithLeader("us-east-1", "eu-west-1");
    MultiRegionActiveActiveManager mgr(cfg);
    auto result = mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::STRONG);
    EXPECT_FALSE(result.success);
}

TEST(MultiRegionActiveActiveTest, EventualWriteAllowedOnNonLeaderRegion) {
    // Non-STRONG writes are never gated by the leader fence
    auto cfg = makeConfigWithLeader("us-east-1", "eu-west-1");
    MultiRegionActiveActiveManager mgr(cfg);
    auto result = mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::EVENTUAL);
    EXPECT_TRUE(result.success);
}

TEST(MultiRegionActiveActiveTest, SessionWriteAllowedOnNonLeaderRegion) {
    auto cfg = makeConfigWithLeader("us-east-1", "eu-west-1");
    MultiRegionActiveActiveManager mgr(cfg);
    auto result = mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::SESSION);
    EXPECT_TRUE(result.success);
}

TEST(MultiRegionActiveActiveTest, WriteIsLeaderRegionFalseWhenNotLeader) {
    // Even for non-STRONG writes the flag reflects the actual leader status
    auto cfg = makeConfigWithLeader("us-east-1", "eu-west-1");
    MultiRegionActiveActiveManager mgr(cfg);
    auto result = mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::SESSION);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.is_leader_region);
}

TEST(MultiRegionActiveActiveTest, WriteIsLeaderRegionTrueWhenNoLeaderConfigured) {
    // Without a leader_region_id every region is "leader"
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1"));
    auto result = mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::STRONG);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.is_leader_region);
}

TEST(MultiRegionActiveActiveTest, LeaderWriteRejectionCountedInMetrics) {
    auto cfg = makeConfigWithLeader("us-east-1", "eu-west-1");
    MultiRegionActiveActiveManager mgr(cfg);
    mgr.write("col", "d1", "INSERT", "{}", ConsistencyLevel::STRONG); // rejected
    mgr.write("col", "d2", "INSERT", "{}", ConsistencyLevel::STRONG); // rejected

    const std::string m = mgr.exportPrometheusMetrics();
    EXPECT_NE(m.find("themisdb_mraaa_leader_write_rejections_total{region=\"us-east-1\"} 2"),
              std::string::npos);
}

// ============================================================================
// 13. Split-brain detection
// ============================================================================

static MultiRegionActiveActiveConfig makeConfigWithSplitBrainDetection(
    const std::string& local_region = "us-east-1",
    uint32_t max_staleness_ms = 5000)
{
    auto cfg = makeConfig(local_region, max_staleness_ms);
    cfg.split_brain_detection_enabled = true;
    return cfg;
}

TEST(MultiRegionActiveActiveTest, SplitBrainFalseWhenDetectionDisabled) {
    // Detection is off by default; all peers unhealthy → still returns false
    MultiRegionActiveActiveManager mgr(makeConfig("us-east-1", 100));
    // Peers start unhealthy (max staleness) but detection is disabled
    EXPECT_FALSE(mgr.isSplitBrain());
}

TEST(MultiRegionActiveActiveTest, SplitBrainFalseWhenAtLeastOnePeerHealthy) {
    MultiRegionActiveActiveManager mgr(makeConfigWithSplitBrainDetection());
    // Make eu-west-1 healthy
    mgr.updateRegionStaleness("eu-west-1", 1000, 5);
    EXPECT_FALSE(mgr.isSplitBrain());
}

TEST(MultiRegionActiveActiveTest, SplitBrainTrueWhenAllPeersUnhealthy) {
    // Both peers remain at max staleness (initial state) → split-brain
    MultiRegionActiveActiveManager mgr(makeConfigWithSplitBrainDetection());
    EXPECT_TRUE(mgr.isSplitBrain());
}

TEST(MultiRegionActiveActiveTest, SplitBrainRecoveredWhenPeerBecomesHealthy) {
    MultiRegionActiveActiveManager mgr(makeConfigWithSplitBrainDetection());
    EXPECT_TRUE(mgr.isSplitBrain()); // initially all peers unhealthy

    // One peer recovers
    mgr.updateRegionStaleness("eu-west-1", 3000, 10);
    EXPECT_FALSE(mgr.isSplitBrain());
}

TEST(MultiRegionActiveActiveTest, SplitBrainFalseWhenNoPeers) {
    MultiRegionActiveActiveConfig cfg;
    cfg.local_region_id            = "standalone";
    cfg.peer_region_ids            = {};  // no peers
    cfg.split_brain_detection_enabled = true;
    MultiRegionActiveActiveManager mgr(cfg);
    EXPECT_FALSE(mgr.isSplitBrain()); // no peers → no split-brain possible
}
