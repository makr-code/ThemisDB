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

// ── STORAGE_BASED ─────────────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, StorageBased_UnderLimit_DeletesNothing) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type              = RetentionType::STORAGE_BASED;
    policy.max_storage_bytes = 1024 * 1024; // 1 MB – far above test data size

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 0u);
}

TEST_F(RetentionManagerTest, StorageBased_OverLimit_DeletesOldestFirst) {
    SystemVersionedTable t{"tbl", "node_a"};
    // Insert 5 updates; sleep between updates to guarantee strictly increasing
    // sys_start timestamps (Timestamp resolution is 1 ms).
    t.insert("k1", {{"value", 0}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    t.update("k1", {{"value", 1}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    t.update("k1", {{"value", 2}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    t.update("k1", {{"value", 3}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    t.update("k1", {{"value", 4}}); // current

    // Capture all sys_start values in ascending order before enforcement.
    auto before = t.getHistory("k1");
    std::sort(before.begin(), before.end(), [](const VersionedDocument& a, const VersionedDocument& b) {
        return a.sys_time.start < b.sys_time.start;
    });
    ASSERT_EQ(before.size(), 5u); // 4 historical + 1 current

    // Allow only 1 historical version to survive besides the current row.
    // Compute max_storage_bytes that would hold at most 1 historical entry.
    // estimateVersionSize returns key.size() + data.dump().size() + 32 (overhead);
    // adding 10 gives a small margin so that exactly 1 entry fits within the limit.
    uint64_t one_entry_approx = before[0].key.size() + before[0].data.dump().size() + 32 + 10;

    RetentionPolicy policy;
    policy.type              = RetentionType::STORAGE_BASED;
    policy.max_storage_bytes = one_entry_approx;

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_GT(stats.versions_deleted, 0u);
    EXPECT_GT(stats.space_freed_bytes, 0u);
    // Current version must always survive
    EXPECT_GE(t.versionCount(), 1u);

    // Verify that the oldest historical version(s) were deleted first.
    // The remaining historical version should have a sys_start greater than
    // the ones that were removed.
    auto after = t.getHistory("k1");
    for (const auto& remaining : after) {
        if (remaining.isCurrent()) {
          continue;
        }
        // Every remaining non-current version must have a sys_start newer than
        // the oldest entry in 'before' that we expect to have been deleted.
        // before[0] is the oldest historical version; it must be gone.
        EXPECT_GT(remaining.sys_time.start, before[0].sys_time.start);
    }
}

TEST_F(RetentionManagerTest, StorageBased_ZeroLimit_IsNoop) {
    // max_storage_bytes == 0 means "no storage-based enforcement"
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type              = RetentionType::STORAGE_BASED;
    policy.max_storage_bytes = 0; // unlimited

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 0u);
    EXPECT_EQ(stats.space_freed_bytes, 0u);
}

TEST_F(RetentionManagerTest, StorageBased_ArchivesBeforeDelete) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 4);

    RetentionPolicy policy;
    policy.type                  = RetentionType::STORAGE_BASED;
    policy.max_storage_bytes     = 1; // force deletions
    policy.archive_before_delete = true;
    policy.archive_tag           = "cold_storage";

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_GT(stats.versions_archived, 0u);
    EXPECT_EQ(stats.versions_archived, stats.versions_deleted);
    auto archived = mgr.getArchivedRecords();
    EXPECT_EQ(archived.size(), stats.versions_archived);
}

// ── space_freed_bytes ─────────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, SpaceFreedBytes_NonZeroAfterDeletion) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0); // delete all history

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_GT(stats.space_freed_bytes, 0u);
}

TEST_F(RetentionManagerTest, SpaceFreedBytes_AccumulatesInCumulativeStats) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0);

    mgr.enforceRetention(t, policy);
    auto cum = mgr.getCumulativeStats();
    EXPECT_GT(cum["total_space_freed_bytes"].get<uint64_t>(), 0u);
}

// ── Compliance: minimum_retention_period ─────────────────────────────────────

TEST_F(RetentionManagerTest, MinimumRetention_ProtectsRecentVersions) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type                       = RetentionType::TIME_BASED;
    policy.retention_period           = std::chrono::milliseconds(0); // want to delete all
    // 100 years minimum keeps every version created in the test.
    // std::chrono::hours(24 * 365 * 100) ≈ 876 000 hours ≈ 100 years.
    policy.minimum_retention_period   = std::chrono::hours(24 * 365 * 100);

    auto stats = mgr.enforceRetention(t, policy);
    // All versions are within the 100-year minimum – nothing should be deleted
    EXPECT_EQ(stats.versions_deleted, 0u);
}

TEST_F(RetentionManagerTest, ComplianceTag_AppearsInArchivedRecords) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 2);

    RetentionPolicy policy;
    policy.type                  = RetentionType::TIME_BASED;
    policy.retention_period      = std::chrono::milliseconds(0);
    policy.compliance_tag        = "GDPR";
    policy.archive_before_delete = true;

    mgr.enforceRetention(t, policy);

    auto archived = mgr.getArchivedRecords();
    EXPECT_GT(archived.size(), 0u);
    // When archive_tag is empty the compliance_tag is combined with the table name
    // so that getArchivedRecords("<table>") can still locate these records.
    for (const auto& rec : archived) {
        EXPECT_EQ(rec.archive_tag, "tbl:GDPR");
    }

    // Verify that getArchivedRecords("tbl") returns the same records.
    auto by_table = mgr.getArchivedRecords("tbl");
    EXPECT_EQ(by_table.size(), archived.size());
}

TEST_F(RetentionManagerTest, ComplianceTag_ExplicitArchiveTagTakesPrecedence) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 2);

    RetentionPolicy policy;
    policy.type                  = RetentionType::TIME_BASED;
    policy.retention_period      = std::chrono::milliseconds(0);
    policy.compliance_tag        = "HIPAA";
    policy.archive_tag           = "explicit_tag";
    policy.archive_before_delete = true;

    mgr.enforceRetention(t, policy);

    auto archived = mgr.getArchivedRecords();
    EXPECT_GT(archived.size(), 0u);
    for (const auto& rec : archived) {
        EXPECT_EQ(rec.archive_tag, "explicit_tag");
    }
}

// ── Incremental batch enforcement ────────────────────────────────────────────

TEST_F(RetentionManagerTest, IncrementalBatch_LimitsVersionsDeleted) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 5); // 5 historical versions

    RetentionPolicy policy;
    policy.type                   = RetentionType::TIME_BASED;
    policy.retention_period       = std::chrono::milliseconds(0);
    policy.incremental_batch_size = 2; // delete at most 2 per run

    auto stats = mgr.enforceRetention(t, policy);
    // At most 2 versions should be deleted per run
    EXPECT_LE(stats.versions_deleted, 2u);
    // 3 historical + current still present
    EXPECT_GE(t.versionCount(), 4u);
}

TEST_F(RetentionManagerTest, IncrementalBatch_ZeroMeansUnlimited) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 5);

    RetentionPolicy policy;
    policy.type                   = RetentionType::TIME_BASED;
    policy.retention_period       = std::chrono::milliseconds(0);
    policy.incremental_batch_size = 0; // unlimited

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 5u);
    EXPECT_EQ(t.versionCount(), 1u);
}

TEST_F(RetentionManagerTest, IncrementalBatch_VersionCount_LimitsDeleted) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 5); // 1 current + 5 historical

    RetentionPolicy policy;
    policy.type                   = RetentionType::VERSION_COUNT_BASED;
    policy.max_versions_per_key   = 1; // keep only 1 historical
    policy.incremental_batch_size = 2; // but delete at most 2 per run

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_LE(stats.versions_deleted, 2u);
}

// ── Retry on errors ───────────────────────────────────────────────────────────

TEST_F(RetentionManagerTest, Retry_SucceedsEvenWithoutErrors) {
    // max_retries is set but there are no errors → should still work normally
    SystemVersionedTable t{"tbl", "node_a"};
    populateHistory(t, 3);

    RetentionPolicy policy;
    policy.type             = RetentionType::TIME_BASED;
    policy.retention_period = std::chrono::milliseconds(0);
    policy.max_retries      = 3;

    auto stats = mgr.enforceRetention(t, policy);
    EXPECT_EQ(stats.versions_deleted, 3u);
    EXPECT_TRUE(stats.errors.empty());
}

// ── RetentionRule operator== / operator< (RR-01..06) ─────────────────────────

TEST(RetentionRuleTest, RR_01_EqualityReflexive) {
    auto r = RetentionRule::timeBased(std::chrono::hours(24), "GDPR");
    EXPECT_EQ(r, r);
}

TEST(RetentionRuleTest, RR_02_EqualitySymmetric) {
    auto r1 = RetentionRule::timeBased(std::chrono::hours(24), "GDPR");
    auto r2 = RetentionRule::timeBased(std::chrono::hours(24), "GDPR");
    EXPECT_EQ(r1, r2);
    EXPECT_EQ(r2, r1);
}

TEST(RetentionRuleTest, RR_03_InequalityDifferentPeriod) {
    auto r1 = RetentionRule::timeBased(std::chrono::hours(24));
    auto r2 = RetentionRule::timeBased(std::chrono::hours(48));
    EXPECT_NE(r1, r2);
}

TEST(RetentionRuleTest, RR_04_InequalityDifferentType) {
    auto r1 = RetentionRule::timeBased(std::chrono::hours(1));
    auto r2 = RetentionRule::versionCount(5);
    EXPECT_NE(r1, r2);
}

TEST(RetentionRuleTest, RR_05_LessThanism_TotalOrder) {
    auto r1 = RetentionRule::timeBased(std::chrono::hours(1));
    auto r2 = RetentionRule::timeBased(std::chrono::hours(2));
    EXPECT_LT(r1, r2);
    EXPECT_FALSE(r2 < r1);
    EXPECT_FALSE(r1 < r1);
}

TEST(RetentionRuleTest, RR_06_UsableInStdSet) {
    std::set<RetentionRule> rule_set;
    rule_set.insert(RetentionRule::timeBased(std::chrono::hours(24), "GDPR"));
    rule_set.insert(RetentionRule::versionCount(10, "HIPAA"));
    rule_set.insert(RetentionRule::storageBased(1024 * 1024));
    EXPECT_EQ(rule_set.size(), 3u);

    // Inserting a duplicate should not grow the set.
    rule_set.insert(RetentionRule::timeBased(std::chrono::hours(24), "GDPR"));
    EXPECT_EQ(rule_set.size(), 3u);
}
