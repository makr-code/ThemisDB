// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file test_tensor_shard_summary.cpp
/// @brief Phase C ctest gate — shard summary refresh, summary-first routing
///        with escalation, and exact-on-demand tensor fetch.
///
/// Test IDs: TSS-01 .. TSS-20

#include <gtest/gtest.h>

#include "shard_summary_coordinator.h"
#include "tensor/tensor_summary_types.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace {

using namespace themis::distributed_tensor;
using namespace themis::tensor;

// ──────────────────────────────────────────────────────────────────────────────
// Test helpers
// ──────────────────────────────────────────────────────────────────────────────

/// Milliseconds of a notional "past" refresh time that is still within TTL.
constexpr int64_t kNowMs = 1_000_000'000'000LL; // arbitrary fixed "now"
constexpr int64_t kRecentMs = kNowMs - 60'000LL; // 60 s ago — within 1-hour TTL
constexpr int64_t kExpiredMs = kNowMs - 7'200'000LL; // 2 hours ago — expired

ShardSummary makeFreshSummary(const std::string& shard_id) {
    ShardSummary s;
    s.shard_id = shard_id;
    s.shard_relevance = 0.9f;
    s.shard_healthy = true;
    s.freshness_state = SummaryFreshnessState::FRESH;
    s.freshness_ttl_seconds = 3600;
    return s;
}

ShardSummary makeStaleSummary(const std::string& shard_id) {
    ShardSummary s = makeFreshSummary(shard_id);
    s.freshness_state = SummaryFreshnessState::STALE;
    return s;
}

ShardSummary makeInvalidSummary(const std::string& shard_id) {
    ShardSummary s = makeFreshSummary(shard_id);
    s.freshness_state = SummaryFreshnessState::INVALID;
    return s;
}

// Minimal stub IShardFetcher: returns success with synthetic payload.
class StubFetcher : public IShardFetcher {
public:
    ExactFetchResult fetch(const ExactFetchRequest& req) const noexcept override {
        ExactFetchResult r;
        r.shard_id = req.shard_id;
        r.artifact_id = req.artifact_id;
        r.success = true;
        r.fragment_data = {0x01, 0x02, 0x03};
        r.content_hash = "stub_hash";
        r.integrity_verified = true;
        return r;
    }
};

// Fetcher that always fails.
class FailingFetcher : public IShardFetcher {
public:
    ExactFetchResult fetch(const ExactFetchRequest& req) const noexcept override {
        ExactFetchResult r;
        r.shard_id = req.shard_id;
        r.artifact_id = req.artifact_id;
        r.success = false;
        r.error_reason = "shard_unreachable";
        return r;
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// TSS-01: default construction succeeds without fetcher or manifest store.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS01_DefaultConstruction) {
    ShardSummaryCoordinator coordinator;
    const auto s = coordinator.stats();
    EXPECT_EQ(s.total_refreshes, 0u);
    EXPECT_EQ(s.total_exact_fetches, 0u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-02: registerShard adds a record; unregisterShard removes it.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS02_RegisterUnregister) {
    ShardSummaryCoordinator c;
    c.registerShard("shard-a");
    EXPECT_TRUE(c.getFreshnessRecord("shard-a").has_value());

    c.unregisterShard("shard-a");
    EXPECT_FALSE(c.getFreshnessRecord("shard-a").has_value());
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-03: getFreshnessRecord returns nullopt for unknown shard.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS03_GetFreshnessUnknownShard) {
    ShardSummaryCoordinator c;
    EXPECT_FALSE(c.getFreshnessRecord("no-such-shard").has_value());
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-04: refreshShard marks shard FRESH and increments generation.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS04_RefreshMarksFresh) {
    ShardSummaryCoordinator c;
    c.registerShard("shard-a");

    ShardSummary summary = makeStaleSummary("shard-a");
    const auto result = c.refreshShard("shard-a", summary, kNowMs);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.shard_id, "shard-a");
    EXPECT_EQ(result.freshness_state, SummaryFreshnessState::FRESH);
    EXPECT_EQ(result.generation, 1u);
    EXPECT_EQ(result.refreshed_at_ms, kNowMs);

    EXPECT_EQ(summary.freshness_state, SummaryFreshnessState::FRESH);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-05: isFresh returns false for newly registered (never-refreshed) shard.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS05_NeverRefreshedNotFresh) {
    ShardSummaryCoordinator c;
    c.registerShard("shard-a");
    EXPECT_FALSE(c.isFresh("shard-a", kNowMs));
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-06: isFresh returns true after a recent refresh.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS06_FreshAfterRefresh) {
    ShardSummaryCoordinator c;
    c.registerShard("shard-a");
    ShardSummary s = makeStaleSummary("shard-a");
    c.refreshShard("shard-a", s, kRecentMs);
    EXPECT_TRUE(c.isFresh("shard-a", kNowMs));
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-07: isFresh returns false when TTL has expired.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS07_ExpiredAfterTTL) {
    ShardSummaryCoordinator c;
    c.registerShard("shard-a", /*ttl=*/3600u);
    ShardSummary s = makeFreshSummary("shard-a");
    c.refreshShard("shard-a", s, kExpiredMs); // refreshed 2 h ago
    EXPECT_FALSE(c.isFresh("shard-a", kNowMs));
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-08: refreshAll updates all shards in the map.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS08_RefreshAll) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    c.registerShard("s2");

    std::unordered_map<std::string, ShardSummary> summaries;
    summaries["s1"] = makeStaleSummary("s1");
    summaries["s2"] = makeStaleSummary("s2");

    const auto results = c.refreshAll(summaries, kNowMs);

    EXPECT_EQ(results.size(), 2u);
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
        EXPECT_EQ(r.freshness_state, SummaryFreshnessState::FRESH);
    }
    EXPECT_TRUE(c.isFresh("s1", kNowMs));
    EXPECT_TRUE(c.isFresh("s2", kNowMs));
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-09: freshness consensus quorum met when all shards are fresh.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS09_ConsensusQuorumMet) {
    ShardSummaryCoordinator c;
    for (const auto& id : {"s1", "s2", "s3", "s4"}) {
        c.registerShard(id);
        ShardSummary s = makeFreshSummary(id);
        c.refreshShard(id, s, kRecentMs);
    }

    const auto res = c.checkFreshnessConsensus({"s1", "s2", "s3", "s4"}, kNowMs);
    EXPECT_TRUE(res.quorum_met);
    EXPECT_EQ(res.fresh_shards, 4u);
    EXPECT_EQ(res.stale_shards, 0u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-10: freshness consensus quorum not met when majority stale.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS10_ConsensusMajorityStale) {
    ShardSummaryCoordinator c;
    // Refresh s1 recently; s2/s3/s4 use expired timestamps.
    c.registerShard("s1");
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);

    for (const auto& id : {"s2", "s3", "s4"}) {
        c.registerShard(id);
        ShardSummary s = makeFreshSummary(id);
        c.refreshShard(id, s, kExpiredMs); // expired
    }

    const auto res = c.checkFreshnessConsensus({"s1", "s2", "s3", "s4"}, kNowMs);
    // Only 1/4 = 25 % fresh, below 75 % quorum.
    EXPECT_FALSE(res.quorum_met);
    EXPECT_EQ(res.fresh_shards, 1u);
    EXPECT_EQ(res.stale_shards, 3u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-11: routeSummaryFirst includes fresh shards without escalation.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS11_RouteFreshNoEscalation) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);

    const auto decisions = c.routeSummaryFirst({s1}, AccuracyMode::ADVISORY, kNowMs);
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].include_shard);
    EXPECT_FALSE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-12: routeSummaryFirst escalates stale shards.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS12_RouteStaleEscalates) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kExpiredMs); // now expired → STALE

    const auto decisions = c.routeSummaryFirst({s1}, AccuracyMode::ADVISORY, kNowMs);
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].include_shard);
    EXPECT_TRUE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-13: routeSummaryFirst skips INVALID shards by default.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS13_RouteInvalidSkipped) {
    ShardSummaryCoordinator c;
    ShardSummary inv = makeInvalidSummary("s-bad");

    const auto decisions = c.routeSummaryFirst({inv}, AccuracyMode::ADVISORY, kNowMs);
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].include_shard);
    EXPECT_FALSE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-14: AccuracyMode::EXACT forces escalation even for fresh summaries.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS14_ExactModeForcesEscalation) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);

    const auto decisions = c.routeSummaryFirst({s1}, AccuracyMode::EXACT, kNowMs);
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].include_shard);
    EXPECT_TRUE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-15: fetchExact fails gracefully when no fetcher is configured.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS15_FetchNoFetcherFails) {
    ShardSummaryCoordinator c; // no fetcher
    ExactFetchRequest req;
    req.shard_id = "s1";
    req.artifact_id = "art-1";

    const auto result = c.fetchExact(req);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_reason, "no_fetcher_configured");
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-16: fetchExact succeeds with a stub fetcher.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS16_FetchWithStubFetcher) {
    auto fetcher = std::make_shared<StubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    ExactFetchRequest req;
    req.shard_id = "s1";
    req.artifact_id = "art-1";

    const auto result = c.fetchExact(req);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.shard_id, "s1");
    EXPECT_EQ(result.content_hash, "stub_hash");
    EXPECT_FALSE(result.fragment_data.empty());
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-17: fetchEscalated only fetches shards where escalate_to_exact is true.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS17_FetchEscalatedFiltersDecisions) {
    auto fetcher = std::make_shared<StubFetcher>();
    ShardSummaryCoordinator c(fetcher);
    c.registerShard("s1");
    c.registerShard("s2");

    // s1 fresh (no escalation), s2 stale (escalation)
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);

    ShardSummary s2 = makeFreshSummary("s2");
    c.refreshShard("s2", s2, kExpiredMs);

    const auto decisions = c.routeSummaryFirst({s1, s2}, AccuracyMode::ADVISORY, kNowMs);
    const auto fetches = c.fetchEscalated(decisions, "artifact-x");

    // Only s2 should have been fetched.
    ASSERT_EQ(fetches.size(), 1u);
    EXPECT_EQ(fetches[0].shard_id, "s2");
    EXPECT_TRUE(fetches[0].success);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-18: stats are updated correctly after refresh and fetch.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS18_StatsUpdated) {
    auto fetcher = std::make_shared<StubFetcher>();
    ShardSummaryCoordinator c(fetcher);
    c.registerShard("s1");

    ShardSummary s = makeStaleSummary("s1");
    c.refreshShard("s1", s, kNowMs);

    ExactFetchRequest req;
    req.shard_id = "s1";
    req.artifact_id = "art";
    c.fetchExact(req);

    const auto st = c.stats();
    EXPECT_GE(st.total_refreshes, 1u);
    EXPECT_GE(st.total_exact_fetches, 1u);
    EXPECT_GE(st.total_exact_fetch_successes, 1u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-19: setConfig is reflected in subsequent routing decisions.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS19_SetConfigEscalateStaleDisabled) {
    ShardSummaryCoordinator c;
    ShardSummaryCoordinator::Config cfg;
    cfg.escalate_stale_shards = false; // skip stale shards entirely
    c.setConfig(cfg);

    ShardSummary s = makeStaleSummary("s1");
    const auto decisions = c.routeSummaryFirst({s}, AccuracyMode::ADVISORY, kNowMs);
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].include_shard);
    EXPECT_FALSE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-20: failing fetcher records are counted in stats, result is not success.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS20_FailingFetcherStats) {
    auto fetcher = std::make_shared<FailingFetcher>();
    ShardSummaryCoordinator c(fetcher);

    ExactFetchRequest req;
    req.shard_id = "s-fail";
    req.artifact_id = "art";

    const auto result = c.fetchExact(req);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_reason, "shard_unreachable");

    const auto st = c.stats();
    EXPECT_GE(st.total_exact_fetches, 1u);
    EXPECT_EQ(st.total_exact_fetch_successes, 0u);
}

} // namespace
