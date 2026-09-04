/**
 * Tests for MVCCChainPruner
 *
 * Test IDs:
 *   MCP-01  pruneKey migrates versions below horizon into tier manager
 *   MCP-02  pruneKey respects min_versions_to_keep (never prunes the N newest)
 *   MCP-03  pruneKey with no versions below horizon is a no-op
 *   MCP-04  pruneKey sets correct sys_time (start = cur.ts, end = next.ts)
 *   MCP-05  pruneKey handles non-JSON binary values (hex fallback)
 *   MCP-06  pruneAll migrates multiple keys in a single call
 *   MCP-07  setSafeHorizon is monotone; pruneAllSafe respects safe horizon
 *   MCP-08  pruneAllSafe is a no-op when safe horizon is zero
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <vector>

#include "storage/hlc.h"
#include "storage/mvcc_chain_pruner.h"
#include "storage/mvcc_store.h"
#include "storage/rocksdb_wrapper.h"
#include "temporal/temporal_tier_manager.h"
#include "temporal/temporal_types.h"

using namespace themis;
using namespace themisdb::temporal;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class MVCCChainPrunerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_mvcc_chain_pruner";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = false;
        rocksdb_       = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(rocksdb_->open());

        clock_ = std::make_shared<HybridLogicalClock>();
        mvcc_  = std::make_shared<MVCCStore>(rocksdb_, clock_);

        // TemporalTierManager with a small hot-tier cap to keep tests simple.
        TierPolicy policy;
        policy.hot_max_versions_per_key = 64;
        policy.warm_max_blocks_per_key  = 4;
        tier_ = std::make_shared<TemporalTierManager>(policy);

        pruner_ = std::make_unique<MVCCChainPruner>(mvcc_, tier_);
    }

    void TearDown() override {
        pruner_.reset();
        tier_.reset();
        mvcc_.reset();
        rocksdb_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    // Helper: write N sequential versions of `key` and return their timestamps.
    std::vector<HLCTimestamp> writeVersions(
        const std::string& key,
        const std::vector<std::string>& values
    ) {
        std::vector<HLCTimestamp> tss = {};

        for (const auto& v : values) {
            tss.push_back(mvcc_->put(key, {v.begin(), v.end()}));
        }
        return tss;
    }

    // Helper: count MVCC versions remaining for `key`.
    size_t countMvccVersions(const std::string& key) {
        size_t n = 0;
        mvcc_->scanVersions(key, [&](const MVCCStore::VersionEntry&) -> bool {
            ++n;
            return true;
        });
        return n;
    }

    // Helper: count tier entries for (table, key).
    size_t countTierVersions(const std::string& table,
                              const std::string& key) {
        return tier_->getHistory(table, key).size();
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>      rocksdb_;
    std::shared_ptr<HybridLogicalClock>  clock_;
    std::shared_ptr<MVCCStore>           mvcc_;
    std::shared_ptr<TemporalTierManager> tier_;
    std::unique_ptr<MVCCChainPruner>     pruner_;
};

// ─────────────────────────────────────────────────────────────────────────────
// MCP-01: pruneKey migrates versions below horizon into tier manager
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneKeyMigratesVersionsBelowHorizon) {
    // Write 4 versions: v0, v1, v2, v3.
    auto tss = writeVersions("k1", {"v0", "v1", "v2", "v3"});
    ASSERT_EQ(tss.size(), 4u);

    // Set horizon = ts(v2), keep 1 newest → v0 and v1 are eligible.
    MVCCChainPruner::Config cfg;
    cfg.min_versions_to_keep = 1;
    cfg.table_name           = "t1";

    // Horizon is ts(v2): versions v0, v1 are strictly below.
    auto stats = pruner_->pruneKey("k1", tss[2], cfg);

    EXPECT_EQ(stats.keys_scanned,      1u);
    EXPECT_EQ(stats.keys_pruned,       1u);
    EXPECT_EQ(stats.versions_migrated, 2u);  // v0, v1
    EXPECT_EQ(stats.versions_deleted,  2u);

    // MVCC store should now hold only v2, v3 (2 versions).
    EXPECT_EQ(countMvccVersions("k1"), 2u);

    // Tier manager should hold 2 migrated documents.
    EXPECT_EQ(countTierVersions("t1", "k1"), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-02: pruneKey respects min_versions_to_keep
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneKeyRespectsMinVersionsToKeep) {
    auto tss = writeVersions("k2", {"a", "b", "c", "d", "e"});  // 5 versions

    MVCCChainPruner::Config cfg;
    cfg.min_versions_to_keep = 3;  // keep newest 3: c, d, e
    cfg.table_name           = "t2";

    // Horizon beyond all versions → everything is a candidate.
    HLCTimestamp beyond{tss.back().value + 1};
    auto stats = pruner_->pruneKey("k2", beyond, cfg);

    // max_deletable = 5 - 3 = 2; eligible = 5 (all below horizon).
    EXPECT_EQ(stats.versions_migrated, 2u);  // a, b
    EXPECT_EQ(stats.versions_deleted,  2u);
    EXPECT_EQ(countMvccVersions("k2"),   3u);  // c, d, e remain
    EXPECT_EQ(countTierVersions("t2", "k2"), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-03: pruneKey with no versions below horizon is a no-op
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneKeyNoOpWhenHorizonBeforeAllVersions) {
    auto tss = writeVersions("k3", {"x", "y", "z"});
    (void)tss;

    // Horizon is before the oldest version — nothing to prune.
    HLCTimestamp too_old{0};
    auto stats = pruner_->pruneKey("k3", too_old, {});

    EXPECT_EQ(stats.versions_migrated, 0u);
    EXPECT_EQ(stats.versions_deleted,  0u);
    EXPECT_EQ(stats.keys_pruned,       0u);
    EXPECT_EQ(countMvccVersions("k3"),  3u);
    EXPECT_EQ(countTierVersions("mvcc", "k3"), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-04: pruneKey sets correct sys_time (start=cur.ts, end=next.ts)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneKeySetsSysTimeCorrectly) {
    auto tss = writeVersions("k4", {"p", "q", "r"});  // v0, v1, v2

    MVCCChainPruner::Config cfg;
    cfg.min_versions_to_keep = 1;
    cfg.table_name           = "t4";

    // Prune v0, v1 (horizon = ts(v2)).
    pruner_->pruneKey("k4", tss[2], cfg);

    auto history = tier_->getHistory("t4", "k4");
    ASSERT_EQ(history.size(), 2u);

    // History is sorted ascending by sys_start.
    // v0: [tss[0], tss[1])
    // v1: [tss[1], tss[2])
    const auto& h0 = history[0];
    const auto& h1 = history[1];

    EXPECT_EQ(h0.sys_time.start,
              static_cast<Timestamp>(tss[0].value));
    EXPECT_EQ(h0.sys_time.end,
              static_cast<Timestamp>(tss[1].value));

    EXPECT_EQ(h1.sys_time.start,
              static_cast<Timestamp>(tss[1].value));
    EXPECT_EQ(h1.sys_time.end,
              static_cast<Timestamp>(tss[2].value));
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-05: pruneKey handles non-JSON binary values (hex fallback)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneKeyHandlesBinaryValues) {
    // Write binary (non-UTF-8, non-JSON) value.
    const std::vector<uint8_t> binary_val = {0x00, 0xFF, 0xAB, 0xCD};
    HLCTimestamp ts0 = mvcc_->put("k5", binary_val);
    // Write a second version so ts0 can be pruned (keeps ts1).
    HLCTimestamp ts1 = mvcc_->put("k5", {0x01});

    MVCCChainPruner::Config cfg;
    cfg.min_versions_to_keep = 1;
    cfg.table_name           = "t5";

    auto stats = pruner_->pruneKey("k5", ts1, cfg);
    EXPECT_EQ(stats.versions_migrated, 1u);

    auto history = tier_->getHistory("t5", "k5");
    ASSERT_EQ(history.size(), 1u);

    // The document should carry a "_raw" field with the hex-encoded bytes.
    ASSERT_TRUE(history[0].data.contains("_raw"));
    EXPECT_EQ(history[0].data["_raw"].get<std::string>(), "00ffabcd");
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-06: pruneAll migrates multiple keys in a single call
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneAllMigratesMultipleKeys) {
    auto tss_a = writeVersions("a", {"a0", "a1", "a2"});
    auto tss_b = writeVersions("b", {"b0", "b1"});

    // Horizon past all versions; keep 1 newest each.
    HLCTimestamp beyond{tss_b.back().value + 1};
    MVCCChainPruner::Config cfg;
    cfg.min_versions_to_keep = 1;
    cfg.table_name           = "tall";

    auto stats = pruner_->pruneAll(beyond, cfg);

    EXPECT_EQ(stats.keys_scanned,      2u);
    EXPECT_EQ(stats.keys_pruned,       2u);
    // a: 3 total, keep 1 → 2 migrated; b: 2 total, keep 1 → 1 migrated
    EXPECT_EQ(stats.versions_migrated, 3u);
    EXPECT_EQ(stats.versions_deleted,  3u);

    EXPECT_EQ(countMvccVersions("a"), 1u);
    EXPECT_EQ(countMvccVersions("b"), 1u);

    EXPECT_EQ(countTierVersions("tall", "a"), 2u);
    EXPECT_EQ(countTierVersions("tall", "b"), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-07: setSafeHorizon is monotone; pruneAllSafe respects it
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, SetSafeHorizonIsMonotoneAndPruneAllSafeUsesIt) {
    auto tss = writeVersions("m", {"m0", "m1", "m2", "m3"});

    // Set horizon to ts(m2).
    pruner_->setSafeHorizon(tss[2]);
    EXPECT_EQ(pruner_->safeHorizon().value, tss[2].value);

    // Attempt to move horizon backward — must be silently ignored.
    pruner_->setSafeHorizon(tss[0]);
    EXPECT_EQ(pruner_->safeHorizon().value, tss[2].value);

    // pruneAllSafe uses the safe horizon (tss[2]) to prune m0, m1.
    MVCCChainPruner::Config cfg;
    cfg.min_versions_to_keep = 1;
    cfg.table_name           = "tsafe";
    auto stats = pruner_->pruneAllSafe(cfg);

    EXPECT_EQ(stats.versions_migrated, 2u);  // m0, m1
    EXPECT_EQ(countMvccVersions("m"),  2u);  // m2, m3 remain
}

// ─────────────────────────────────────────────────────────────────────────────
// MCP-08: pruneAllSafe is a no-op when safe horizon is zero
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCChainPrunerTest, PruneAllSafeIsNoOpWithZeroHorizon) {
    writeVersions("n", {"n0", "n1"});

    // Default safe_horizon is 0 → pruneAllSafe should do nothing.
    auto stats = pruner_->pruneAllSafe({});

    EXPECT_EQ(stats.keys_scanned,      0u);
    EXPECT_EQ(stats.versions_migrated, 0u);
    EXPECT_EQ(countMvccVersions("n"),  2u);
}
