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
