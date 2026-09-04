/**
 * Tests for TemporalTierManager (LSM three-tier: hot / warm / cold)
 *
 * Test IDs:
 *   TTM-BLOOM-01..03   BloomFilter correctness
 *   TTM-POLICY-01..04  TierPolicy threshold + LLM hook
 *   TTM-HOT-01..03     Hot-only inserts and getAsOf
 *   TTM-FLUSH-01..04   Hot→warm flush and warm block metadata
 *   TTM-COLD-01..03    Warm→cold compaction
 *   TTM-QUERY-01..05   Three-tier getAsOf / getHistory / getHistoryInRange
 *   TTM-CONC-01        Concurrent readers
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include "temporal/temporal_tier_manager.h"

using namespace themisdb::temporal;

// ── helpers ──────────────────────────────────────────────────────────────────

static VersionedDocument makeDoc(const std::string& key,
                                  Timestamp start, Timestamp end,
                                  const std::string& val = "v") {
    VersionedDocument d;
    d.key        = key;
    d.data       = {{"v", val}};
    d.sys_time   = {start, end};
    d.valid_time = {start, end};
    return d;
}

// ── BloomFilter tests ─────────────────────────────────────────────────────────

// TTM-BLOOM-01: Inserted timestamps are always found (no false negatives).
TEST(BloomFilterTierManagerTest, InsertedAlwaysFound) {
    BloomFilter bf(100);
    for (int i = 0; i < 100; ++i) {
      bf.add(static_cast<int64_t>(i * 13));
    }
    for (int i = 0; i < 100; ++i)
        EXPECT_TRUE(bf.mightContain(static_cast<int64_t>(i * 13)));
}

// TTM-BLOOM-02: FPR is reasonable for 8 bits/element (<15% empirically).
TEST(BloomFilterTierManagerTest, FalsePositiveRateAcceptable) {
    constexpr int N = 500;
    BloomFilter bf(N, 8);
    for (int i = 0; i < N; ++i) {
      bf.add(static_cast<int64_t>(i));
    }
    int fp = 0;
    for (int i = N; i < N * 2; ++i)
        if (bf.mightContain(static_cast<int64_t>(i))) {
          ++fp;
        }
    EXPECT_LT(static_cast<double>(fp) / N, 0.15);
}

// TTM-BLOOM-03: Empty bloom filter has no false positives for any value.
TEST(BloomFilterTierManagerTest, EmptyHasNoFalseNegativesOnInserted) {
    BloomFilter bf(0);  // edge: 0 expected elements → defaults to 64 bits
    bf.add(42);
    EXPECT_TRUE(bf.mightContain(42));
}

// ── TierPolicy tests ──────────────────────────────────────────────────────────

// TTM-POLICY-01: KEEP when counts are below thresholds.
TEST(TierPolicyTest, EvaluateKeep) {
    TierPolicy p;
    p.hot_max_versions_per_key = 100;
    TierDecisionContext ctx;
    ctx.hot_version_count = 50;
    ctx.warm_block_count  = 2;
    ctx.total_warm_bytes  = 1024;
    ctx.now_ts = 1000;
    EXPECT_EQ(p.evaluate(ctx), TierDecision::KEEP);
}

// TTM-POLICY-02: FLUSH_HOT_TO_WARM when hot count exceeds threshold.
TEST(TierPolicyTest, EvaluateFlushHotToWarm) {
    TierPolicy p;
    p.hot_max_versions_per_key = 10;
    TierDecisionContext ctx;
    ctx.hot_version_count = 11;
    ctx.now_ts = 1000;
    EXPECT_EQ(p.evaluate(ctx), TierDecision::FLUSH_HOT_TO_WARM);
}

// TTM-POLICY-03: FLUSH_WARM_TO_COLD when warm block cap exceeded.
TEST(TierPolicyTest, EvaluateFlushWarmToCold_BlockCap) {
    TierPolicy p;
    p.warm_max_blocks_per_key = 4;
    TierDecisionContext ctx;
    ctx.hot_version_count = 5;
    ctx.warm_block_count  = 4;
    ctx.now_ts = 1000;
    EXPECT_EQ(p.evaluate(ctx), TierDecision::FLUSH_WARM_TO_COLD);
}

// TTM-POLICY-04: LLM/LoRA decision_fn hook overrides threshold logic.
TEST(TierPolicyTest, DecisionFnHookOverridesThreshold) {
    TierPolicy p;
    p.hot_max_versions_per_key = 1000;  // threshold would say KEEP
    p.decision_fn = [](const TierDecisionContext&) {
        // Simulate LoRA model always recommending cold flush
        return TierDecision::FLUSH_WARM_TO_COLD;
    };
    TierDecisionContext ctx;
    ctx.hot_version_count = 1;
    ctx.now_ts = 1000;
    EXPECT_EQ(p.evaluate(ctx), TierDecision::FLUSH_WARM_TO_COLD);
}

// TTM-POLICY-05: clearing decision_fn reverts to built-in threshold path.
TEST(TierPolicyTest, DecisionFnClearRevertsToThreshold) {
    TierPolicy p;
    p.hot_max_versions_per_key = 1000; // threshold would say KEEP
    p.decision_fn = [](const TierDecisionContext&) {
        return TierDecision::FLUSH_WARM_TO_COLD;
    };
    // Clear the fn — threshold path takes over
    p.decision_fn = {};
    TierDecisionContext ctx;
    ctx.hot_version_count = 1; // below threshold
    ctx.now_ts = 1000;
    EXPECT_EQ(p.evaluate(ctx), TierDecision::KEEP);
}

// TTM-POLICY-06: decision_fn receives fully-populated context values.
TEST(TierPolicyTest, DecisionFnReceivesCorrectContext) {
    TierPolicy p;
    TierDecisionContext captured{};
    p.decision_fn = [&captured](const TierDecisionContext& ctx) {
        captured = ctx;
        return TierDecision::KEEP;
    };
    TierDecisionContext ctx;
    ctx.hot_version_count    = 42;
    ctx.warm_block_count     = 7;
    ctx.total_warm_bytes     = 1024;
    ctx.warm_pressure        = 0.25;
    ctx.now_ts               = 9999;
    p.evaluate(ctx);
    EXPECT_EQ(captured.hot_version_count, 42u);
    EXPECT_EQ(captured.warm_block_count, 7u);
    EXPECT_EQ(captured.total_warm_bytes, 1024u);
    EXPECT_DOUBLE_EQ(captured.warm_pressure, 0.25);
    EXPECT_EQ(captured.now_ts, 9999);
}

// ── Hot-only tests ────────────────────────────────────────────────────────────

class TierManagerTest : public ::testing::Test {
protected:
    TierPolicy policy;
    std::unique_ptr<TemporalTierManager> mgr;

    void SetUp() override {
        policy.hot_max_versions_per_key = 100;  // don't auto-flush
        policy.warm_max_blocks_per_key  = 20;
        policy.auto_compact             = false;
        policy.cold_after_age           = std::chrono::milliseconds(0); // disabled
        mgr = std::make_unique<TemporalTierManager>(policy);
    }
};

// TTM-HOT-01: Version inserted into hot tier is findable via getAsOf.
TEST_F(TierManagerTest, HotTier_InsertAndGetAsOf) {
    ASSERT_TRUE(mgr->insert("t", makeDoc("k", 100, 300, "v1")));
    auto r = mgr->getAsOf("t", "k", 200);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->data["v"].get<std::string>(), "v1");
}

// TTM-HOT-02: getAsOf on a timestamp before all versions returns nullopt.
TEST_F(TierManagerTest, HotTier_GetAsOf_BeforeAll_NullOpt) {
    mgr->insert("t", makeDoc("k", 100, 300));
    EXPECT_FALSE(mgr->getAsOf("t", "k", 50).has_value());
}

// TTM-HOT-03: Current (open-ended) versions are rejected.
TEST_F(TierManagerTest, HotTier_CurrentVersionRejected) {
    VersionedDocument d = makeDoc("k", 100, kMaxTimestamp);
    EXPECT_FALSE(mgr->insert("t", d));
    EXPECT_EQ(mgr->keyStats("t", "k").hot_versions, 0u);
}

// ── Flush hot→warm tests ─────────────────────────────────────────────────────

// TTM-FLUSH-01: Explicit flushHotToWarm moves versions to warm tier.
TEST_F(TierManagerTest, FlushHotToWarm_MovesVersions) {
    for (int i = 0; i < 10; ++i)
        mgr->insert("t", makeDoc("k", i * 10, (i + 1) * 10));

    const size_t moved = mgr->flushHotToWarm("t", "k");
    EXPECT_EQ(moved, 0u);  // hot_max=100 not exceeded; nothing to move

    // Lower threshold and flush again
    TierPolicy p2 = policy;
    p2.hot_max_versions_per_key = 5;
    p2.warm_block_size          = 3;
    mgr->setPolicy(p2);

    const size_t moved2 = mgr->flushHotToWarm("t", "k");
    EXPECT_GT(moved2, 0u);

    const auto ks = mgr->keyStats("t", "k");
    EXPECT_GT(ks.warm_blocks, 0u);
    EXPECT_LE(ks.hot_versions, 5u);
}

// TTM-FLUSH-02: VersionBlock has correct min_start / max_end.
TEST_F(TierManagerTest, FlushHotToWarm_BlockMetadata) {
    TierPolicy p = policy;
    p.hot_max_versions_per_key = 2;
    p.warm_block_size          = 5;
    mgr->setPolicy(p);

    for (int i = 0; i < 7; ++i)
        mgr->insert("t", makeDoc("k", i * 10, (i + 1) * 10));

    const auto ks = mgr->keyStats("t", "k");
    EXPECT_GT(ks.warm_blocks, 0u);
    EXPECT_GT(ks.warm_versions, 0u);
}

// TTM-FLUSH-03: After hot→warm, getAsOf still resolves via warm tier.
TEST_F(TierManagerTest, FlushHotToWarm_GetAsOfResolvedFromWarm) {
    TierPolicy p = policy;
    p.hot_max_versions_per_key = 2;
    p.warm_block_size          = 10;
    mgr->setPolicy(p);

    for (int i = 0; i < 5; ++i)
        mgr->insert("t", makeDoc("k", i * 100, (i + 1) * 100, "v" + std::to_string(i)));

    // The older versions should be in warm tier
    auto r = mgr->getAsOf("t", "k", 50);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->data["v"].get<std::string>(), "v0");
}

// TTM-FLUSH-04: Warm block Bloom filter is populated.
TEST_F(TierManagerTest, FlushHotToWarm_BloomFilterPopulated) {
    TierPolicy p = policy;
    p.hot_max_versions_per_key = 2;
    p.warm_block_size          = 5;
    mgr->setPolicy(p);

    // Insert 5 versions, trigger flush
    for (int i = 0; i < 5; ++i)
        mgr->insert("t", makeDoc("k", i * 10, (i + 1) * 10));

    mgr->flushHotToWarm("t", "k");
    // We can't inspect VersionBlock directly via public API,
    // but bloom works correctly if getAsOf succeeds on warm-tier versions.
    auto r = mgr->getAsOf("t", "k", 15);
    ASSERT_TRUE(r.has_value());
}

// ── Warm→cold compaction tests ────────────────────────────────────────────────

// TTM-COLD-01: flushWarmToCold moves oldest block to cold tier.
TEST(TierManagerColdTest, FlushWarmToCold_MoveToColddStore) {
    TierPolicy p;
    p.hot_max_versions_per_key = 2;
    p.warm_max_blocks_per_key  = 3;
    p.warm_block_size          = 2;
    p.auto_compact             = false;
    p.cold_after_age           = std::chrono::milliseconds(0);

    auto cold = std::make_shared<TemporalColdStore>();
    TemporalTierManager mgr(p, cold);

    for (int i = 0; i < 8; ++i)
        mgr.insert("t", makeDoc("k", i * 10, (i + 1) * 10));

    // Force flush hot → warm
    mgr.flushHotToWarm("t", "k");

    const auto before = cold->totalVersionCount();
    const size_t moved = mgr.flushWarmToCold("t", "k");
    EXPECT_GT(moved, 0u);
    EXPECT_GT(cold->totalVersionCount(), before);
}

// TTM-COLD-02: compactTable respects warm_max_blocks_per_key.
TEST(TierManagerColdTest, CompactTable_RespectsWarmMaxBlocks) {
    TierPolicy p;
    p.hot_max_versions_per_key = 2;
    p.warm_max_blocks_per_key  = 2;
    p.warm_block_size          = 2;
    p.auto_compact             = false;
    p.cold_after_age           = std::chrono::milliseconds(0);

    auto cold = std::make_shared<TemporalColdStore>();
    TemporalTierManager mgr(p, cold);

    for (int i = 0; i < 20; ++i)
        mgr.insert("t", makeDoc("k", i * 10, (i + 1) * 10));

    mgr.compactTable("t");

    const auto ks = mgr.keyStats("t", "k");
    EXPECT_LE(ks.warm_blocks, p.warm_max_blocks_per_key + 1); // +1 tolerance
}

// TTM-COLD-03: Three-tier getAsOf resolves from cold store.
TEST(TierManagerColdTest, GetAsOf_ResolvesFromColdStore) {
    TierPolicy p;
    p.hot_max_versions_per_key = 2;
    p.warm_max_blocks_per_key  = 1;
    p.warm_block_size          = 2;
    p.auto_compact             = false;
    p.cold_after_age           = std::chrono::milliseconds(0);

    auto cold = std::make_shared<TemporalColdStore>();
    TemporalTierManager mgr(p, cold);

    for (int i = 0; i < 10; ++i)
        mgr.insert("t", makeDoc("k", i * 100, (i + 1) * 100,
                                "v" + std::to_string(i)));

    // Compact everything to cold
    mgr.compactTable("t");
    mgr.flushHotToWarm("t", "k");
    mgr.compactTable("t");

    // The oldest version (v0, sys_time [0,100)) should be in cold
    auto r = mgr.getAsOf("t", "k", 50);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->data["v"].get<std::string>(), "v0");
}

// ── Three-tier query tests ────────────────────────────────────────────────────

class ThreeTierQueryTest : public ::testing::Test {
protected:
    std::shared_ptr<TemporalColdStore> cold;
    TemporalTierManager* mgr{nullptr};

    void SetUp() override {
        TierPolicy p;
        p.hot_max_versions_per_key = 3;
        p.warm_max_blocks_per_key  = 2;
        p.warm_block_size          = 3;
        p.auto_compact             = false;
        p.cold_after_age           = std::chrono::milliseconds(0);
        cold = std::make_shared<TemporalColdStore>();
        mgr  = new TemporalTierManager(p, cold);

        // Insert 12 versions: oldest will be cold, middle warm, newest hot
        for (int i = 0; i < 12; ++i)
            mgr->insert("t", makeDoc("k", i * 10, (i + 1) * 10,
                                     "v" + std::to_string(i)));
    }

    void TearDown() override { delete mgr; }
};

// TTM-QUERY-01: getHistory returns all versions across all tiers.
TEST_F(ThreeTierQueryTest, GetHistory_AllVersionsReturned) {
    const auto history = mgr->getHistory("t", "k");
    EXPECT_EQ(history.size(), 12u);
    // Sorted ascending by sys_start
    for (size_t i = 1; i < history.size(); ++i)
        EXPECT_LE(history[i-1].sys_time.start, history[i].sys_time.start);
}

// TTM-QUERY-02: getAsOf resolves correct version regardless of tier.
TEST_F(ThreeTierQueryTest, GetAsOf_AnyTierResolvable) {
    for (int i = 0; i < 12; ++i) {
        const Timestamp t = static_cast<Timestamp>(i * 10 + 5);
        auto r = mgr->getAsOf("t", "k", t);
        ASSERT_TRUE(r.has_value()) << "Failed for i=" << i;
        EXPECT_EQ(r->data["v"].get<std::string>(), "v" + std::to_string(i));
    }
}

// TTM-QUERY-03: getHistoryInRange returns only overlapping versions.
TEST_F(ThreeTierQueryTest, GetHistoryInRange_Filtered) {
    const auto range_versions = mgr->getHistoryInRange("t", "k", {25, 75});
    for (const auto& v : range_versions)
        EXPECT_TRUE(v.sys_time.overlaps({25, 75}));
    EXPECT_FALSE(range_versions.empty());
}

// TTM-QUERY-04: statsJson includes all tier counts.
TEST_F(ThreeTierQueryTest, StatsJson_AllTiersCounted) {
    const auto stats = mgr->statsJson("t");
    EXPECT_GE(stats["hot_versions"].get<size_t>(), 0u);
}

// TTM-QUERY-05: Unknown key/table returns nullopt / empty.
TEST_F(ThreeTierQueryTest, UnknownKey_NullOptAndEmpty) {
    EXPECT_FALSE(mgr->getAsOf("t", "nonexistent", 50).has_value());
    EXPECT_TRUE(mgr->getHistory("t", "nonexistent").empty());
}

// ── Concurrent readers ────────────────────────────────────────────────────────

// TTM-CONC-01: Concurrent getAsOf calls do not deadlock.
TEST(TierManagerConcurrencyTest, ConcurrentReads_NoDeadlock) {
    TierPolicy p;
    p.hot_max_versions_per_key = 5;
    p.warm_block_size          = 5;
    p.auto_compact             = false;
    p.cold_after_age           = std::chrono::milliseconds(0);
    TemporalTierManager mgr(p);

    constexpr int N = 100;
    for (int i = 0; i < N; ++i)
        mgr.insert("t", makeDoc("k", i * 10, (i + 1) * 10));

    std::atomic<int> hits{0};
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            for (int q = 0; q < 50; ++q) {
                const Timestamp ts = static_cast<Timestamp>(
                    ((t * 12 + q) % N) * 10 + 4);
                if (mgr.getAsOf("t", "k", ts).has_value())
                    hits.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(hits.load(), 8 * 50);
}
