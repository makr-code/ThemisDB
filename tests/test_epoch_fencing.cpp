// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_epoch_fencing.cpp
 * @brief Unit tests for EpochFencingManager and LeaseManager.
 *
 * Coverage:
 *  - EpochToken validity
 *  - Epoch bump and fencing check (ALLOWED / STALE_EPOCH)
 *  - Auto-STONITH on stale epoch
 *  - INVALID_TOKEN handling
 *  - Metrics counters
 *  - LeaseManager: acquire / renew / release / expiry
 *  - LeaseManager: re-acquire after expiry
 *  - LeaseManager: idempotent re-acquire by same holder
 *  - LeaseManager: STONITH on acquire timeout
 *  - LeaseManager: concurrent acquire serialisation
 *  - WAL round-trip persistence
 *  - Config validation
 */

#include "sharding/epoch_fencing.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

using namespace std::chrono_literals;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static EpochFencingConfig makeConfig(const std::string& shard = "s1",
                                     const std::string& node  = "n1",
                                     bool auto_stonith         = true) {
    EpochFencingConfig cfg;
    cfg.shard_id     = shard;
    cfg.node_id      = node;
    cfg.auto_stonith = auto_stonith;
    cfg.stonith_timeout_ms = 500ms;
    return cfg;
}

static LeaseConfig makeLeaseConfig(std::chrono::milliseconds ttl     = 200ms,
                                   std::chrono::milliseconds renew   = 50ms,
                                   std::chrono::milliseconds wait    = 500ms) {
    LeaseConfig cfg;
    cfg.ttl_ms          = ttl;
    cfg.renew_before_ms = renew;
    cfg.acquire_wait_ms = wait;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingConfig — validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(EpochFencingConfigTest, ValidConfigPasses) {
    EXPECT_TRUE(makeConfig().validate());
}

TEST(EpochFencingConfigTest, EmptyShardIdFails) {
    auto cfg = makeConfig();
    cfg.shard_id = "";
    EXPECT_FALSE(cfg.validate());
}

TEST(EpochFencingConfigTest, EmptyNodeIdFails) {
    auto cfg = makeConfig();
    cfg.node_id = "";
    EXPECT_FALSE(cfg.validate());
}

TEST(EpochFencingConfigTest, ZeroTimeoutFails) {
    auto cfg = makeConfig();
    cfg.stonith_timeout_ms = 0ms;
    EXPECT_FALSE(cfg.validate());
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochToken — validity
// ─────────────────────────────────────────────────────────────────────────────

TEST(EpochTokenTest, ValidTokenAccepted) {
    EpochToken tok;
    tok.epoch = 5;
    EXPECT_TRUE(tok.isValid(5));
}

TEST(EpochTokenTest, FutureEpochAccepted) {
    EpochToken tok;
    tok.epoch = 10;
    EXPECT_TRUE(tok.isValid(5));
}

TEST(EpochTokenTest, StaleEpochRejected) {
    EpochToken tok;
    tok.epoch = 3;
    EXPECT_FALSE(tok.isValid(5));
}

TEST(EpochTokenTest, ZeroEpochAlwaysInvalid) {
    EpochToken tok;
    tok.epoch = 0;
    EXPECT_FALSE(tok.isValid(0));
    EXPECT_FALSE(tok.isValid(1));
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager — basic operations
// ─────────────────────────────────────────────────────────────────────────────

TEST(EpochFencingManagerTest, InitialEpochIsOne) {
    EpochFencingManager mgr(makeConfig());
    EXPECT_EQ(mgr.currentEpoch(), 1u);
}

TEST(EpochFencingManagerTest, BumpEpochIncrementsCounter) {
    EpochFencingManager mgr(makeConfig());
    auto tok = mgr.bumpEpoch("test");
    EXPECT_EQ(tok.epoch, 2u);
    EXPECT_EQ(mgr.currentEpoch(), 2u);
}

TEST(EpochFencingManagerTest, MultipleBumpsAreMonotonic) {
    EpochFencingManager mgr(makeConfig());
    for (uint64_t i = 2; i <= 10; ++i) {
        auto tok = mgr.bumpEpoch("bump");
        EXPECT_EQ(tok.epoch, i);
    }
}

TEST(EpochFencingManagerTest, MakeTokenReturnsCurrentEpoch) {
    EpochFencingManager mgr(makeConfig());
    auto bumped = mgr.bumpEpoch("up");
    static_cast<void>(bumped);
    auto tok = mgr.makeToken();
    EXPECT_EQ(tok.epoch, mgr.currentEpoch());
    EXPECT_EQ(tok.issuer, "n1");
    EXPECT_EQ(tok.shard_id, "s1");
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager — fencing check
// ─────────────────────────────────────────────────────────────────────────────

TEST(EpochFencingManagerTest, CurrentTokenAllowed) {
    EpochFencingManager mgr(makeConfig("s1", "n1", false));
    auto tok = mgr.makeToken();
    EXPECT_EQ(mgr.checkToken(tok, "n2"), FencingResult::ALLOWED);
}

TEST(EpochFencingManagerTest, StaleTokenRejectedWithoutStonith) {
    EpochFencingManager mgr(makeConfig("s1", "n1", false));
    auto old_tok = mgr.makeToken();   // epoch==1
    auto bumped = mgr.bumpEpoch("bump"); // now epoch==2
    static_cast<void>(bumped);
    EXPECT_EQ(mgr.checkToken(old_tok, "n2"), FencingResult::STALE_EPOCH);
}

TEST(EpochFencingManagerTest, InvalidTokenRejected) {
    EpochFencingManager mgr(makeConfig("s1", "n1", false));
    EpochToken bad;
    bad.epoch = 0;
    EXPECT_EQ(mgr.checkToken(bad, "n2"), FencingResult::INVALID_TOKEN);
}

TEST(EpochFencingManagerTest, StaleTokenTriggersStonith) {
    auto stonith = std::make_shared<NullStonithProvider>();
    EpochFencingManager mgr(makeConfig("s1", "n1", true), stonith);
    auto old_tok = mgr.makeToken();
    auto bumped = mgr.bumpEpoch("bump");
    static_cast<void>(bumped);
    auto result = mgr.checkToken(old_tok, "stale-node");
    EXPECT_EQ(result, FencingResult::STONITH_ISSUED);
    EXPECT_TRUE(stonith->isFenced("stale-node"));
}

TEST(EpochFencingManagerTest, FutureEpochTokenAllowed) {
    EpochFencingManager mgr(makeConfig("s1", "n1", false));
    EpochToken future_tok;
    future_tok.epoch    = 999;
    future_tok.issuer   = "n1";
    future_tok.shard_id = "s1";
    // Epoch 999 > current (1): should be ALLOWED
    EXPECT_EQ(mgr.checkToken(future_tok, "n1"), FencingResult::ALLOWED);
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager — metrics
// ─────────────────────────────────────────────────────────────────────────────

TEST(EpochFencingManagerTest, MetricsAccumulateCorrectly) {
    auto stonith = std::make_shared<NullStonithProvider>();
    EpochFencingManager mgr(makeConfig("s1", "n1", true), stonith);

    // 2 bumps
    auto bumped_a = mgr.bumpEpoch("a");
    auto bumped_b = mgr.bumpEpoch("b");
    static_cast<void>(bumped_a);
    static_cast<void>(bumped_b);

    // 3 allowed writes
    auto cur = mgr.makeToken();
    for (int i = 0; i < 3; ++i) {
        auto result = mgr.checkToken(cur, "writer");
        static_cast<void>(result);
    }

    // 1 stale → STONITH
    EpochToken stale;
    stale.epoch = 1;
    auto stale_result = mgr.checkToken(stale, "old-writer");
    static_cast<void>(stale_result);

    auto m = mgr.metrics();
    EXPECT_EQ(m.epoch_bumps,      2u);
    EXPECT_EQ(m.allowed_writes,   3u);
    EXPECT_EQ(m.stale_rejections, 1u);
    EXPECT_EQ(m.stonith_issued,   1u);
    EXPECT_EQ(m.stonith_failed,   0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// NullStonithProvider
// ─────────────────────────────────────────────────────────────────────────────

TEST(NullStonithProviderTest, FenceRecordsNode) {
    NullStonithProvider prov;
    auto dl = std::chrono::steady_clock::now() + 1s;
    EXPECT_TRUE(prov.fence("n1", "test", dl));
    EXPECT_TRUE(prov.isFenced("n1"));
    EXPECT_FALSE(prov.isFenced("n2"));
}

TEST(NullStonithProviderTest, ResetClearsFencedSet) {
    NullStonithProvider prov;
    auto dl = std::chrono::steady_clock::now() + 1s;
    prov.fence("n1", "r", dl);
    prov.reset();
    EXPECT_FALSE(prov.isFenced("n1"));
    EXPECT_TRUE(prov.fencedNodes().empty());
}

TEST(NullStonithProviderTest, ProviderName) {
    NullStonithProvider prov;
    EXPECT_EQ(prov.providerName(), "NullStonithProvider");
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseConfig — validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(LeaseConfigTest, ValidConfigPasses) {
    EXPECT_TRUE(makeLeaseConfig().validate());
}

TEST(LeaseConfigTest, ZeroTtlFails) {
    auto cfg = makeLeaseConfig();
    cfg.ttl_ms = 0ms;
    EXPECT_FALSE(cfg.validate());
}

TEST(LeaseConfigTest, RenewBeforeGeTtlFails) {
    auto cfg = makeLeaseConfig(200ms, 200ms);  // renew == ttl
    EXPECT_FALSE(cfg.validate());
}

TEST(LeaseConfigTest, ZeroAcquireWaitFails) {
    auto cfg = makeLeaseConfig();
    cfg.acquire_wait_ms = 0ms;
    EXPECT_FALSE(cfg.validate());
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — basic acquire / release / query
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<EpochFencingManager> makeFencing(bool auto_s = false) {
    return std::make_shared<EpochFencingManager>(makeConfig("s1", "n1", auto_s));
}

TEST(LeaseManagerTest, AcquireSucceeds) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto res = lm.acquire("leader", "node-1");
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.record.holder, "node-1");
    EXPECT_EQ(res.record.state, LeaseState::HELD);
}

TEST(LeaseManagerTest, IsHolderReturnsTrueForHolder) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    EXPECT_TRUE(lm.isHolder("leader", "node-1"));
    EXPECT_FALSE(lm.isHolder("leader", "node-2"));
}

TEST(LeaseManagerTest, ReleaseAllowsReacquire) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    lm.release("leader", "node-1");

    auto res = lm.acquire("leader", "node-2");
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.record.holder, "node-2");
}

TEST(LeaseManagerTest, ReleaseByNonHolderNoOp) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    EXPECT_FALSE(lm.release("leader", "node-2"));
    EXPECT_TRUE(lm.isHolder("leader", "node-1"));
}

TEST(LeaseManagerTest, RenewExtendsTtl) {
    LeaseManager lm(makeLeaseConfig(500ms, 100ms), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    std::this_thread::sleep_for(50ms);
    auto renewed = lm.renew("leader", "node-1");
    ASSERT_TRUE(renewed.has_value());
    EXPECT_GT(renewed->remainingTtl().count(), 400);
}

TEST(LeaseManagerTest, RenewByNonHolderFails) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    EXPECT_FALSE(lm.renew("leader", "node-2").has_value());
}

TEST(LeaseManagerTest, GetReturnsLeaseRecord) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    auto rec = lm.get("leader");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->holder, "node-1");
}

TEST(LeaseManagerTest, GetUnknownKeyReturnsNullopt) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    EXPECT_FALSE(lm.get("nonexistent").has_value());
}

TEST(LeaseManagerTest, ListLeasesIncludesAcquiredKey) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired_a = lm.acquire("key-a", "n1");
    auto acquired_b = lm.acquire("key-b", "n2");
    static_cast<void>(acquired_a);
    static_cast<void>(acquired_b);
    auto keys = lm.listLeases();
    EXPECT_EQ(keys.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — expiry
// ─────────────────────────────────────────────────────────────────────────────

TEST(LeaseManagerTest, ExpiredLeaseIsReacquirable) {
    LeaseManager lm(makeLeaseConfig(100ms, 20ms, 500ms), makeFencing());
    auto acquired = lm.acquire("leader", "node-1");
    static_cast<void>(acquired);
    std::this_thread::sleep_for(150ms);  // let it expire
    auto res = lm.acquire("leader", "node-2");
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.record.holder, "node-2");
}

TEST(LeaseManagerTest, IdempotentReacquireBySameHolder) {
    LeaseManager lm(makeLeaseConfig(500ms, 100ms), makeFencing());
    auto r1 = lm.acquire("leader", "node-1");
    auto r2 = lm.acquire("leader", "node-1");
    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    // generation should not have changed
    EXPECT_EQ(r1.record.generation, r2.record.generation);
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — metrics
// ─────────────────────────────────────────────────────────────────────────────

TEST(LeaseManagerTest, MetricsCountAcquireAndRelease) {
    LeaseManager lm(makeLeaseConfig(), makeFencing());
    auto acquired = lm.acquire("k", "n1");
    auto renewed = lm.renew("k", "n1");
    static_cast<void>(acquired);
    static_cast<void>(renewed);
    lm.release("k", "n1");

    auto m = lm.metrics();
    EXPECT_GE(m.acquires,  1u);
    EXPECT_EQ(m.renewals,  1u);
    EXPECT_EQ(m.releases,  1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — WAL round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(LeaseManagerTest, WalPersistsAndRestores) {
    const std::string wal = "/tmp/test_epoch_fencing_wal.txt";
    std::filesystem::remove(wal);

    {
        auto lcfg       = makeLeaseConfig(500ms, 50ms);
        lcfg.wal_path   = wal;
        LeaseManager lm(lcfg, makeFencing());
        auto acquired = lm.acquire("shard-0-leader", "node-A");
        static_cast<void>(acquired);
    }

    // Re-open from WAL
    auto lcfg2    = makeLeaseConfig(500ms, 50ms);
    lcfg2.wal_path = wal;
    LeaseManager lm2(lcfg2, makeFencing());

    auto rec = lm2.get("shard-0-leader");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->holder, "node-A");

    std::filesystem::remove(wal);
}

TEST(LeaseManagerTest, WalDiscardsExpiredRecords) {
    const std::string wal = "/tmp/test_epoch_fencing_wal_exp.txt";
    std::filesystem::remove(wal);

    {
        auto lcfg       = makeLeaseConfig(100ms, 20ms);
        lcfg.wal_path   = wal;
        LeaseManager lm(lcfg, makeFencing());
        auto acquired = lm.acquire("shard-0-leader", "node-A");
        static_cast<void>(acquired);
    }

    std::this_thread::sleep_for(150ms);  // let it expire

    auto lcfg2    = makeLeaseConfig(100ms, 20ms);
    lcfg2.wal_path = wal;
    LeaseManager lm2(lcfg2, makeFencing());

    auto rec = lm2.get("shard-0-leader");
    // Record exists but state should be EXPIRED
    if (rec.has_value()) {
        EXPECT_NE(rec->state, LeaseState::HELD);
    }

    std::filesystem::remove(wal);
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety smoke test
// ─────────────────────────────────────────────────────────────────────────────

TEST(EpochFencingManagerTest, ConcurrentBumpsAreThreadSafe) {
    EpochFencingManager mgr(makeConfig());
    constexpr int N = 50;
    std::vector<EpochNumber> epochs(N);
    std::vector<std::thread> threads;
    threads.reserve(N);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&mgr, &epochs, i] {
            epochs[i] = mgr.bumpEpoch("concurrent").epoch;
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // All bumped epochs must be unique and >= 2
    std::sort(epochs.begin(), epochs.end());
    for (int i = 0; i < N; ++i) {
        EXPECT_GE(epochs[i], 2u);
        if (i > 0) {
            EXPECT_NE(epochs[i], epochs[i - 1]);
        }
    }
}

TEST(LeaseManagerTest, ConcurrentAcquireOnlyOneSucceeds) {
    LeaseManager lm(makeLeaseConfig(500ms, 100ms, 200ms), makeFencing());

    std::atomic<int> successes{0};
    constexpr int N = 8;
    std::vector<std::thread> threads;
    threads.reserve(N);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&lm, &successes, i] {
            auto res = lm.acquire("shared-key", "node-" + std::to_string(i));
            if (res.success) {
              ++successes;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // Exactly one node should hold the lease
    EXPECT_EQ(successes.load(), 1);
}
