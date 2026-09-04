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
constexpr int64_t kNowMs = 1'000'000'000'000LL; // arbitrary fixed "now"
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

// ──────────────────────────────────────────────────────────────────────────────
// TSS-21: Multi-shard quorum consensus with majority fresh.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS21_QuorumMajorityFresh) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    c.registerShard("s2");
    c.registerShard("s3");
    c.registerShard("s4");

    // Refresh 3 out of 4: quorum is 0.75, so 3/4 = 0.75 meets it.
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);
    ShardSummary s2 = makeFreshSummary("s2");
    c.refreshShard("s2", s2, kRecentMs);
    ShardSummary s3 = makeFreshSummary("s3");
    c.refreshShard("s3", s3, kRecentMs);

    const std::vector<std::string> all_shards = {"s1", "s2", "s3", "s4"};
    const auto consensus = c.checkFreshnessConsensus(all_shards, kNowMs);

    EXPECT_EQ(consensus.fresh_shards, 3u);
    EXPECT_EQ(consensus.stale_shards, 1u);
    EXPECT_TRUE(consensus.quorum_met);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-22: Multi-shard quorum consensus with minority fresh (quorum not met).
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS22_QuorumMinorityFresh) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    c.registerShard("s2");
    c.registerShard("s3");
    c.registerShard("s4");

    // Refresh only 2 out of 4: 2/4 = 0.5 < 0.75 quorum.
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);
    ShardSummary s2 = makeFreshSummary("s2");
    c.refreshShard("s2", s2, kRecentMs);

    const std::vector<std::string> all_shards = {"s1", "s2", "s3", "s4"};
    const auto consensus = c.checkFreshnessConsensus(all_shards, kNowMs);

    EXPECT_EQ(consensus.fresh_shards, 2u);
    EXPECT_EQ(consensus.stale_shards, 2u);
    EXPECT_FALSE(consensus.quorum_met);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-23: Empty shard list in consensus check returns quorum_met=false.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS23_EmptyConsensusCheck) {
    ShardSummaryCoordinator c;
    const std::vector<std::string> empty;
    const auto consensus = c.checkFreshnessConsensus(empty, kNowMs);
    EXPECT_FALSE(consensus.quorum_met);
    EXPECT_EQ(consensus.total_shards, 0u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-24: Routing escalates INVALID shards when skip_invalid_shards=false.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS24_InvalidShardEscalationWhenNotSkipped) {
    ShardSummaryCoordinator c;
    ShardSummaryCoordinator::Config cfg;
    cfg.skip_invalid_shards = false;  // Don't skip; escalate instead.
    c.setConfig(cfg);

    ShardSummary s = makeInvalidSummary("s1");
    const auto decisions = c.routeSummaryFirst({s}, AccuracyMode::ADVISORY, kNowMs);

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].include_shard);
    EXPECT_TRUE(decisions[0].escalate_to_exact);
    EXPECT_EQ(decisions[0].reason, "shard_summary_invalid_escalate");
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-25: Routing expires old summaries correctly.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS25_RoutingExpiresStaleSummary) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");

    ShardSummary s = makeFreshSummary("s1");
    // Refresh 2 hours ago (expiry with default 1-hour TTL)
    c.refreshShard("s1", s, kExpiredMs);

    const auto decisions = c.routeSummaryFirst({s}, AccuracyMode::ADVISORY, kNowMs);
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_EQ(decisions[0].summary_freshness, SummaryFreshnessState::STALE);
    EXPECT_TRUE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-26: Multiple refreshes on same shard increment generation counter.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS26_RefreshGenerationIncrement) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");

    ShardSummary s = makeFreshSummary("s1");
    const auto r1 = c.refreshShard("s1", s, kNowMs);
    EXPECT_EQ(r1.generation, 1u);

    const auto r2 = c.refreshShard("s1", s, kNowMs + 1000);
    EXPECT_EQ(r2.generation, 2u);

    const auto r3 = c.refreshShard("s1", s, kNowMs + 2000);
    EXPECT_EQ(r3.generation, 3u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-27: Routing with EXACT accuracy mode always escalates.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS27_ExactModeAlwaysEscalates) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");

    ShardSummary s = makeFreshSummary("s1");
    c.refreshShard("s1", s, kRecentMs);

    const auto decisions = c.routeSummaryFirst({s}, AccuracyMode::EXACT, kNowMs);

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].include_shard);
    EXPECT_TRUE(decisions[0].escalate_to_exact);
    EXPECT_EQ(decisions[0].reason, "accuracy_mode_exact_forced");
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-28: refreshAll processes multiple shards in bulk.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS28_RefreshAllBulkOperation) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    c.registerShard("s2");
    c.registerShard("s3");

    std::unordered_map<std::string, ShardSummary> summaries;
    summaries["s1"] = makeStaleSummary("s1");
    summaries["s2"] = makeStaleSummary("s2");
    summaries["s3"] = makeStaleSummary("s3");

    const auto results = c.refreshAll(summaries, kNowMs);

    EXPECT_EQ(results.size(), 3u);
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
        EXPECT_EQ(r.freshness_state, SummaryFreshnessState::FRESH);
    }
    EXPECT_EQ(summaries["s1"].freshness_state, SummaryFreshnessState::FRESH);
    EXPECT_EQ(summaries["s2"].freshness_state, SummaryFreshnessState::FRESH);
    EXPECT_EQ(summaries["s3"].freshness_state, SummaryFreshnessState::FRESH);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-29: Exact fetch without fetcher returns error gracefully.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS29_FetchWithoutFetcher) {
    ShardSummaryCoordinator c;  // No fetcher provided

    ExactFetchRequest req;
    req.shard_id = "s1";
    req.artifact_id = "art";

    const auto result = c.fetchExact(req);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_reason, "no_fetcher_configured");
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-30: Config modification changes routing behavior.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS30_ConfigChangeAffectsRouting) {
    ShardSummaryCoordinator c;
    ShardSummaryCoordinator::Config cfg;
    cfg.escalate_stale_shards = false;
    c.setConfig(cfg);

    // With escalate=false, stale shards should be skipped.
    ShardSummary stale = makeStaleSummary("s1");
    auto decisions = c.routeSummaryFirst({stale}, AccuracyMode::ADVISORY, kNowMs);
    EXPECT_FALSE(decisions[0].include_shard);

    // Change config to escalate.
    cfg.escalate_stale_shards = true;
    c.setConfig(cfg);
    decisions = c.routeSummaryFirst({stale}, AccuracyMode::ADVISORY, kNowMs);
    EXPECT_TRUE(decisions[0].include_shard);
    EXPECT_TRUE(decisions[0].escalate_to_exact);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-31: Freshness record correctly identifies unrefreshed shard.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS31_UnrefreshedShardRecord) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");

    const auto rec = c.getFreshnessRecord("s1");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->last_refresh_ms, 0);
    EXPECT_TRUE(rec->isExpired(kNowMs));
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-32: Routing decision includes correct advisory score from summary.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS32_RoutingDecisionAdvisoryScore) {
    ShardSummary s = makeFreshSummary("s1");
    s.shard_relevance = 0.85f;

    ShardSummaryCoordinator c;
    const auto decisions = c.routeSummaryFirst({s}, AccuracyMode::ADVISORY, kNowMs);

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FLOAT_EQ(decisions[0].advisory_score, 0.85f);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-33: Fetch result includes measured latency.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS33_FetchResultLatency) {
    auto fetcher = std::make_shared<StubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    ExactFetchRequest req;
    req.shard_id = "s1";
    req.artifact_id = "art";

    const auto result = c.fetchExact(req);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.fetch_latency_ms, 0.0f);
    // Latency should be reasonably small (stub is very fast).
    EXPECT_LT(result.fetch_latency_ms, 100.0f);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-34: Quorum threshold is correctly applied.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS34_QuorumThreshold) {
    ShardSummaryCoordinator c;
    ShardSummaryCoordinator::Config cfg;
    cfg.freshness_quorum_ratio = 0.5f;  // 50% instead of 75%
    c.setConfig(cfg);

    c.registerShard("s1");
    c.registerShard("s2");

    // Refresh only 1 out of 2: 1/2 = 0.5 meets 50% quorum.
    ShardSummary s1 = makeFreshSummary("s1");
    c.refreshShard("s1", s1, kRecentMs);

    const std::vector<std::string> all_shards = {"s1", "s2"};
    const auto consensus = c.checkFreshnessConsensus(all_shards, kNowMs);

    EXPECT_EQ(consensus.fresh_shards, 1u);
    EXPECT_EQ(consensus.stale_shards, 1u);
    EXPECT_TRUE(consensus.quorum_met);  // 0.5 >= 0.5
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-35: Stats accurately reflect multiple operations.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS35_StatsMultipleOperations) {
    auto fetcher = std::make_shared<StubFetcher>();
    ShardSummaryCoordinator c(fetcher);

    c.registerShard("s1");
    c.registerShard("s2");

    ShardSummary s1 = makeFreshSummary("s1");
    ShardSummary s2 = makeFreshSummary("s2");
    c.refreshShard("s1", s1, kNowMs);
    c.refreshShard("s2", s2, kNowMs);

    const auto decisions =
        c.routeSummaryFirst({s1, s2}, AccuracyMode::ADVISORY, kNowMs);
    c.fetchEscalated(decisions, "art");

    const auto stats = c.stats();
    EXPECT_GE(stats.total_refreshes, 2u);
    EXPECT_GE(stats.total_routing_decisions, 2u);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-36: Routing preserves shard order.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS36_RoutingPreservesOrder) {
    std::vector<ShardSummary> summaries = {};

    for (int i = 0; i < 5; ++i) {
        ShardSummary s = makeFreshSummary("shard-" + std::to_string(i));
        summaries.push_back(s);
    }

    ShardSummaryCoordinator c;
    const auto decisions =
        c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kNowMs);

    ASSERT_EQ(decisions.size(), 5u);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(decisions[i].shard_id, "shard-" + std::to_string(i));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-37: Mixed fresh/stale/invalid in single routing decision.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS37_MixedFreshnessRouting) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    c.registerShard("s2");
    c.registerShard("s3");

    ShardSummary fresh = makeFreshSummary("s1");
    c.refreshShard("s1", fresh, kRecentMs);

    ShardSummary stale = makeStaleSummary("s2");
    std::vector<ShardSummary> summaries = {fresh, stale, makeInvalidSummary("s3")};

    const auto decisions = c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kNowMs);

    ASSERT_EQ(decisions.size(), 3u);
    EXPECT_EQ(decisions[0].reason, "shard_summary_fresh_advisory");
    EXPECT_EQ(decisions[1].reason, "shard_summary_stale_escalate_to_exact");
    EXPECT_EQ(decisions[2].reason, "shard_summary_invalid_skipped");
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-38: Register/unregister cycle preserves idempotency.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS38_RegisterIdempotency) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");
    const auto rec1 = c.getFreshnessRecord("s1");

    // Register again with same ID.
    c.registerShard("s1");
    const auto rec2 = c.getFreshnessRecord("s1");

    // Should be the same record.
    EXPECT_EQ(rec1->shard_id, rec2->shard_id);
    EXPECT_EQ(rec1->last_refresh_ms, rec2->last_refresh_ms);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-39: fetchEscalated handles partial failures gracefully.
// ──────────────────────────────────────────────────────────────────────────────
class PartialFetcher : public IShardFetcher {
public:
    ExactFetchResult fetch(const ExactFetchRequest& req) const noexcept override {
        ExactFetchResult r;
        r.shard_id = req.shard_id;
        r.artifact_id = req.artifact_id;
        // Fail on "s-fail", succeed otherwise
        if (req.shard_id == "s-fail") {
            r.success = false;
            r.error_reason = "shard_failed";
        } else {
            r.success = true;
            r.fragment_data = {0x42};
            r.content_hash = "partial_hash";
            r.integrity_verified = true;
        }
        return r;
    }
};

TEST(TensorShardSummaryTest, TSS39_PartialFetchFailure) {
    auto fetcher = std::make_shared<PartialFetcher>();
    ShardSummaryCoordinator c(fetcher);

    std::vector<RoutingDecision> decisions;
    decisions.push_back({.shard_id = "s-ok", .escalate_to_exact = true});
    decisions.push_back({.shard_id = "s-fail", .escalate_to_exact = true});

    const auto results = c.fetchEscalated(decisions, "art");

    EXPECT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0].success);
    EXPECT_FALSE(results[1].success);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-40: Freshness record error field is populated on refresh failure.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS40_FreshnessRecordErrorField) {
    ShardSummaryCoordinator c;
    c.registerShard("s1");

    ShardSummary s = makeFreshSummary("s1");
    c.refreshShard("s1", s, kNowMs);

    const auto rec = c.getFreshnessRecord("s1");
    EXPECT_TRUE(rec->last_refresh_error.empty());  // Success clears error
    EXPECT_EQ(rec->freshness_state, SummaryFreshnessState::FRESH);
}

// ──────────────────────────────────────────────────────────────────────────────
// TSS-41: Concurrent registration and freshness queries are safe.
// ──────────────────────────────────────────────────────────────────────────────
TEST(TensorShardSummaryTest, TSS41_ConcurrentRegistrationAndQuery) {
    ShardSummaryCoordinator c;

    // Register multiple shards
    for (int i = 0; i < 10; ++i) {
        c.registerShard("shard-" + std::to_string(i));
    }

    // Query concurrently-safe checks
    for (int i = 0; i < 10; ++i) {
        const auto rec = c.getFreshnessRecord("shard-" + std::to_string(i));
        EXPECT_TRUE(rec.has_value());
        const bool fresh = c.isFresh("shard-" + std::to_string(i), kNowMs);
        EXPECT_FALSE(fresh);  // Not refreshed yet
    }
}

} // namespace
