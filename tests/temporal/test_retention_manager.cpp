/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_retention_manager.cpp                         ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     308                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for RetentionManager
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/retention_manager.h"
#include <thread>

using namespace themisdb::temporal;

class RetentionManagerTest : public ::testing::Test {
protected:
    RetentionManager mgr;

    void populateHistory(SystemVersionedTable& t, size_t n_updates) {
        t.insert("key1", {{"value", 0}});
        for (size_t i = 1; i <= n_updates; ++i) {
            t.update("key1", {{"value", static_cast<int>(i)}});
        }
    }
};

// ── Policy management ─────────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, SetGetPolicy_RoundTrips) {
    RetentionPolicy policy;
    policy.type             = RetentionType::VERSION_COUNT_BASED;
    policy.max_versions_per_key = 3;

    mgr.setPolicy("my_table", policy);

    auto retrieved = mgr.getPolicy("my_table");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->type, RetentionType::VERSION_COUNT_BASED);
    EXPECT_EQ(retrieved->max_versions_per_key, 3u);
}

TEST_F(RetentionManagerTest, GetPolicy_NonExistent_ReturnsNullopt) {
    EXPECT_FALSE(mgr.getPolicy("unknown_table").has_value());
}

// ── Enforcement – TIME_BASED ──────────────────────────────────────────────────

TEST_F(RetentionManagerTest, EnforceTimeBased_FutureRetention_DeletesNothing) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::hours(24 * 365 * 100); // 100 years

    auto stats = mgr.enforceRetention(t, policy);
    // All historical versions are within the far-future retention window
    EXPECT_EQ(stats.versions_deleted, 0u);
}

TEST_F(RetentionManagerTest, EnforceTimeBased_ZeroRetention_DeletesAllHistory) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0);

    auto stats = mgr.enforceRetention(t, policy);
    // 3 updates → 3 historical versions should be deleted
    EXPECT_EQ(stats.versions_deleted, 3u);
}

// ── Enforcement – VERSION_COUNT_BASED ────────────────────────────────────────

TEST_F(RetentionManagerTest, EnforceVersionCount_ExcessVersionsDeleted) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 5); // 5 updates → 5 historical versions

    RetentionPolicy policy;
    policy.type                 = RetentionType::VERSION_COUNT_BASED;
    policy.max_versions_per_key = 2;

    auto stats = mgr.enforceRetention(t, policy);
    // 5 historical − 2 kept = 3 deleted
    EXPECT_EQ(stats.versions_deleted, 3u);
}

TEST_F(RetentionManagerTest, EnforceVersionCount_WithinLimit_DeletesNothing) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 2); // 2 historical versions

    RetentionPolicy policy;
    policy.type                 = RetentionType::VERSION_COUNT_BASED;
    policy.max_versions_per_key = 5;

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 0u);
}

// ── Enforcement – CUSTOM ─────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, EnforceCustom_KeepEvenValues) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 4);

    RetentionPolicy policy;
    policy.type = RetentionType::CUSTOM;
    policy.should_keep = [](const VersionedDocument& v) {
        if (v.data.contains("value")) {
            return v.data["value"].get<int>() % 2 == 0;
        }
        return true;
    };

    auto stats = mgr.enforceRetention(t, policy);
    // Values 1, 3 (odd, not current) → 2 deleted
    // Values 0, 2 (even) → kept  (value 4 is current, always kept)
    EXPECT_EQ(stats.versions_deleted, 2u);
}

// ── Archive ──────────────────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, Archive_EnabledBeforeDelete_PopulatesArchive) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type                  = RetentionType::VERSION_COUNT_BASED;
    policy.max_versions_per_key  = 1;
    policy.archive_before_delete = true;
    policy.archive_tag           = "test_archive";

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_archived, 2u);

    auto archived = mgr.getArchivedRecords();
    EXPECT_EQ(archived.size(), 2u);
}

TEST_F(RetentionManagerTest, ClearArchive_EmptiesArchive) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 2);

    RetentionPolicy policy;
    policy.type                  = RetentionType::TIME_BASED;
    policy.retention_period      = std::chrono::milliseconds(0);
    policy.archive_before_delete = true;

    mgr.enforceRetention(t, policy);
    EXPECT_FALSE(mgr.getArchivedRecords().empty());

    mgr.clearArchive();
    EXPECT_TRUE(mgr.getArchivedRecords().empty());
}

// ── No policy registered ─────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, EnforceWithoutPolicy_ReturnsError) {
    SystemVersionedTable t{"unregistered", "node_a"};
    t.insert("k1", {{"v", 1}});

    auto stats = mgr.enforceRetention(t);
    EXPECT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.versions_deleted, 0u);
}

// ── Cumulative statistics ─────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, CumulativeStats_AccumulateAcrossRuns) {
    SystemVersionedTable t1{"tbl1", "node_a"};
    SystemVersionedTable t2{"tbl2", "node_a"};
    populateHistory(t1, 3);
    populateHistory(t2, 3);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0);

    mgr.enforceRetention(t1, policy);
    mgr.enforceRetention(t2, policy);

    auto cum = mgr.getCumulativeStats();
    EXPECT_EQ(cum["total_deleted"], 6); // 3 + 3
}

// ── Physical purge (real deletion) ────────────────────────────────────────────

TEST_F(RetentionManagerTest, EnforceTimeBased_ActuallyDeletesVersions) {
    SystemVersionedTable t{"employees", "node_a"};
    populateHistory(t, 3);  // 1 current + 3 historical

    EXPECT_EQ(t.versionCount(), 4u);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0); // keep nothing

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 3u);

    // Verify the versions are gone from the table itself
    EXPECT_EQ(t.versionCount(), 1u); // only current remains
}

TEST_F(RetentionManagerTest, EnforceVersionCount_ActuallyDeletesVersions) {
    SystemVersionedTable t{"employees", "node_a"};
    populateHistory(t, 4);  // 1 current + 4 historical, keep 2

    RetentionPolicy policy;
    policy.type                 = RetentionType::VERSION_COUNT_BASED;
    policy.max_versions_per_key = 2;

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 2u);
    EXPECT_EQ(t.versionCount(), 3u); // current + 2 historical
}

TEST_F(RetentionManagerTest, EnforceRetention_IncludesDeletedKeyHistory) {
    // After deleteRow the key has no current row, but historical versions
    // should still be enumerable and purgeable via getAllKeys().
    //
    // Sequence: insert → update → deleteRow
    //   insert:    v0 = [T0, ∞)                   → 1 version
    //   update:    v0 = [T0, T1), v1 = [T1, ∞)    → 2 versions
    //   deleteRow: v0 = [T0, T1), v1 = [T1, T2)   → 2 versions, 0 current
    SystemVersionedTable t{"employees", "node_a"};
    t.insert("emp1", {{"name", "Alice"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    t.update("emp1", {{"name", "Alicia"}});
    t.deleteRow("emp1"); // closes current; now 2 historical versions, 0 current

    EXPECT_EQ(t.versionCount(), 2u); // both versions are historical

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0);

    auto stats = mgr.enforceRetention(t, policy);
    // Both historical versions must be examined and deleted
    EXPECT_EQ(stats.versions_examined, 2u);
    EXPECT_EQ(stats.versions_deleted,  2u);
    EXPECT_EQ(t.versionCount(), 0u); // all versions purged
}

// ── Background Scheduler ──────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, Scheduler_StartsAndStops) {
    EXPECT_FALSE(mgr.schedulerRunning());
    mgr.startScheduler();
    EXPECT_TRUE(mgr.schedulerRunning());
    mgr.stopScheduler();
    EXPECT_FALSE(mgr.schedulerRunning());
}

TEST_F(RetentionManagerTest, Scheduler_StartTwice_IsNoop) {
    mgr.startScheduler();
    mgr.startScheduler(); // second call is a no-op
    EXPECT_TRUE(mgr.schedulerRunning());
    mgr.stopScheduler();
}

TEST_F(RetentionManagerTest, Scheduler_EnforcesRetentionInBackground) {
    SystemVersionedTable t{"employees", "node_a"};
    populateHistory(t, 3); // 1 current + 3 historical

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0);
    mgr.setPolicy("employees", policy);

    // Schedule with very short interval so it fires quickly
    mgr.scheduleTable(t, std::chrono::milliseconds(20));
    mgr.startScheduler();

    // Wait long enough for the scheduler to run at least once
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    mgr.stopScheduler();

    // Retention should have been enforced: only current version remains
    EXPECT_EQ(t.versionCount(), 1u);
}
