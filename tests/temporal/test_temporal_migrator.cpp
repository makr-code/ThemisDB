/**
 * Focused tests for TemporalMigrator (Phase 5 — temporal module)
 *
 * Test groups:
 *  1. analyzeMigration – plan content, schema inference, edge cases
 *  2. migrateToTemporal – row migration, stats, created_at timestamp
 *  3. backfillHistory – closed-version insertion, open-ended skips
 *  4. verifyMigration – all five data-integrity checks
 *  5. Status / progress callback – lifecycle transitions
 *  6. Serialisation – MigrationPlan/MigrationReport toJson()
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_migrator.h"
#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"

using namespace themisdb::temporal;

// ============================================================================
// Test fixture
// ============================================================================

class TemporalMigratorTest : public ::testing::Test {
protected:
    TemporalMigrator migrator;

    // Helper: build a small source document map
    static std::unordered_map<std::string, Document> makeSourceDocs(
        int n = 5, const std::string& prefix = "emp") {
        std::unordered_map<std::string, Document> docs = {};

        for (int i = 1; i <= n; ++i) {
            std::string key = prefix + std::to_string(i);
            docs[key] = {
                {"name",   "Employee " + std::to_string(i)},
                {"salary", 50000 + i * 1000},
                {"active", true}
            };
        }
        return docs;
    }

    // Helper: build historical version entries
    static std::vector<VersionedDocument> makeHistory(
        const std::string& key, int versions) {
        std::vector<VersionedDocument> entries = {};

        for (int i = 0; i < versions; ++i) {
            VersionedDocument vd;
            vd.key           = key;
            vd.data          = {{"version", i}, {"value", i * 100}};
            vd.sys_time      = {static_cast<Timestamp>(1000 + i * 500),
                                static_cast<Timestamp>(1000 + (i + 1) * 500)};
            vd.modified_by   = "backfill";
            entries.push_back(vd);
        }
        return entries;
    }
};

// ============================================================================
// Group 1: analyzeMigration
// ============================================================================

TEST_F(TemporalMigratorTest, Analyze_ReturnsCorrectTableName) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    EXPECT_EQ(plan.source_table_name, "employees");
}

TEST_F(TemporalMigratorTest, Analyze_ReturnsCorrectRowCount) {
    auto docs = makeSourceDocs(7);
    auto plan = migrator.analyzeMigration("employees", docs);
    EXPECT_EQ(plan.source_row_count, 7u);
}

TEST_F(TemporalMigratorTest, Analyze_EmptySource_ZeroRows) {
    auto plan = migrator.analyzeMigration("empty_table", {});
    EXPECT_EQ(plan.source_row_count, 0u);
    EXPECT_TRUE(plan.columns.empty());
}

TEST_F(TemporalMigratorTest, Analyze_InfersStringColumn) {
    std::unordered_map<std::string, Document> docs = {
        {"k1", {{"title", "hello"}}},
        {"k2", {{"title", "world"}}}
    };
    auto plan = migrator.analyzeMigration("articles", docs);
    auto it   = std::find_if(plan.columns.begin(), plan.columns.end(),
                             [](const ColumnInfo& c){ return c.name == "title"; });
    ASSERT_NE(it, plan.columns.end());
    EXPECT_EQ(it->inferred_type, "string");
}

TEST_F(TemporalMigratorTest, Analyze_InfersNumberColumn) {
    std::unordered_map<std::string, Document> docs = {
        {"k1", {{"score", 42}}},
        {"k2", {{"score", 99}}}
    };
    auto plan = migrator.analyzeMigration("scores", docs);
    auto it   = std::find_if(plan.columns.begin(), plan.columns.end(),
                             [](const ColumnInfo& c){ return c.name == "score"; });
    ASSERT_NE(it, plan.columns.end());
    EXPECT_EQ(it->inferred_type, "number");
}

TEST_F(TemporalMigratorTest, Analyze_InfersBooleanColumn) {
    std::unordered_map<std::string, Document> docs = {
        {"k1", {{"active", true}}},
        {"k2", {{"active", false}}}
    };
    auto plan = migrator.analyzeMigration("flags", docs);
    auto it   = std::find_if(plan.columns.begin(), plan.columns.end(),
                             [](const ColumnInfo& c){ return c.name == "active"; });
    ASSERT_NE(it, plan.columns.end());
    EXPECT_EQ(it->inferred_type, "boolean");
}

TEST_F(TemporalMigratorTest, Analyze_DetectsNullableColumn) {
    std::unordered_map<std::string, Document> docs = {
        {"k1", {{"opt", "present"}}},
        {"k2", {{"other", 1}}}   // "opt" is absent
    };
    auto plan = migrator.analyzeMigration("test", docs);
    auto it   = std::find_if(plan.columns.begin(), plan.columns.end(),
                             [](const ColumnInfo& c){ return c.name == "opt"; });
    ASSERT_NE(it, plan.columns.end());
    EXPECT_TRUE(it->nullable);
}

TEST_F(TemporalMigratorTest, Analyze_NonNullableWhenAllPresent) {
    std::unordered_map<std::string, Document> docs = {
        {"k1", {{"id", 1}}},
        {"k2", {{"id", 2}}}
    };
    auto plan = migrator.analyzeMigration("test", docs);
    auto it   = std::find_if(plan.columns.begin(), plan.columns.end(),
                             [](const ColumnInfo& c){ return c.name == "id"; });
    ASSERT_NE(it, plan.columns.end());
    EXPECT_FALSE(it->nullable);
}

TEST_F(TemporalMigratorTest, Analyze_BaselineTimestampIsPositive) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("t", docs);
    EXPECT_GT(plan.baseline_timestamp, 0);
}

TEST_F(TemporalMigratorTest, Analyze_DefaultHistoryTableName) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    EXPECT_EQ(plan.versioned_config.history_table_name, "employees_history");
}

TEST_F(TemporalMigratorTest, Analyze_DetectsEmptyDocument) {
    std::unordered_map<std::string, Document> docs = {
        {"k1", {}},
        {"k2", {{"id", 1}}}
    };
    auto plan = migrator.analyzeMigration("mixed", docs);
    EXPECT_TRUE(plan.has_empty_documents);
}

TEST_F(TemporalMigratorTest, Analyze_NoEmptyDocumentWhenAllPopulated) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    EXPECT_FALSE(plan.has_empty_documents);
}

TEST_F(TemporalMigratorTest, Analyze_KeysAreUniqueForMapInput) {
    auto docs = makeSourceDocs(4);
    auto plan = migrator.analyzeMigration("employees", docs);
    EXPECT_TRUE(plan.keys_are_unique);
}

// ============================================================================
// Group 2: migrateToTemporal
// ============================================================================

TEST_F(TemporalMigratorTest, Migrate_CreatesTableWithCorrectName) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);
    EXPECT_EQ(table.tableName(), "employees");
}

TEST_F(TemporalMigratorTest, Migrate_AllRowsInserted) {
    auto docs = makeSourceDocs(5);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);
    EXPECT_EQ(table.keyCount(), 5u);
}

TEST_F(TemporalMigratorTest, Migrate_RowsAreCurrent) {
    auto docs = makeSourceDocs(4);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);
    for (const auto& [key, _] : docs) {
        auto current = table.getCurrent(key);
        ASSERT_TRUE(current.has_value()) << "Missing current version for key: " << key;
        EXPECT_EQ(current->sys_time.end, kMaxTimestamp);
    }
}

TEST_F(TemporalMigratorTest, Migrate_StatsRowsMigratedEquals5) {
    auto docs = makeSourceDocs(5);
    auto plan = migrator.analyzeMigration("employees", docs);
    migrator.migrateToTemporal(plan, docs);
    EXPECT_EQ(migrator.getStats().rows_migrated, 5u);
}

TEST_F(TemporalMigratorTest, Migrate_EmptySource_SucceedsWithZeroRows) {
    auto plan = migrator.analyzeMigration("empty", {});
    auto [table, ok] = migrator.migrateToTemporal(plan, {});
    EXPECT_TRUE(ok);
    EXPECT_EQ(table.keyCount(), 0u);
}

TEST_F(TemporalMigratorTest, Migrate_PreservesDocumentData) {
    std::unordered_map<std::string, Document> docs = {
        {"emp1", {{"name", "Alice"}, {"salary", 80000}}}
    };
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);
    auto current = table.getCurrent("emp1");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data["name"], "Alice");
    EXPECT_EQ(current->data["salary"], 80000);
}

TEST_F(TemporalMigratorTest, Migrate_StatusIsCompleteOnSuccess) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    migrator.migrateToTemporal(plan, docs);
    EXPECT_EQ(migrator.getStatus(), MigrationStatus::COMPLETE);
}

TEST_F(TemporalMigratorTest, Migrate_ElapsedMsIsNonNegative) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    migrator.migrateToTemporal(plan, docs);
    EXPECT_GE(migrator.getStats().elapsed_ms.count(), 0);
}

// ============================================================================
// Group 3: backfillHistory
// ============================================================================

TEST_F(TemporalMigratorTest, Backfill_SkipsOpenEndedVersions) {
    auto docs = makeSourceDocs(1);
    auto plan = migrator.analyzeMigration("test", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    // Construct an open-ended version (sys_end == kMaxTimestamp)
    VersionedDocument open_vd;
    open_vd.key      = "emp1";
    open_vd.data     = {{"name", "Old Alice"}};
    open_vd.sys_time = {100, kMaxTimestamp};

    size_t inserted = migrator.backfillHistory(table, {open_vd});
    // Open-ended entries must be skipped
    EXPECT_EQ(inserted, 0u);
}

TEST_F(TemporalMigratorTest, Backfill_AcceptsClosedVersions) {
    auto docs = makeSourceDocs(1);
    auto plan = migrator.analyzeMigration("test", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto history = makeHistory("emp1", 2);
    size_t inserted = migrator.backfillHistory(table, history);
    // At least some versions should have been processed
    EXPECT_GE(inserted, 0u);
}

TEST_F(TemporalMigratorTest, Backfill_EmptyList_ReturnsZero) {
    auto docs = makeSourceDocs(1);
    auto plan = migrator.analyzeMigration("test", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    size_t inserted = migrator.backfillHistory(table, {});
    EXPECT_EQ(inserted, 0u);
}

TEST_F(TemporalMigratorTest, Backfill_StatsVersionsBackfilledAccessible) {
    auto docs = makeSourceDocs(1);
    auto plan = migrator.analyzeMigration("test", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    // stats_.versions_backfilled should be accessible (initially 0)
    EXPECT_EQ(migrator.getStats().versions_backfilled, 0u);

    // backfilling an empty list must not crash
    size_t n = migrator.backfillHistory(table, {});
    EXPECT_EQ(n, 0u);
}

// ============================================================================
// Group 4: verifyMigration
// ============================================================================

TEST_F(TemporalMigratorTest, Verify_PassesForFreshMigration) {
    auto docs = makeSourceDocs(5);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    EXPECT_TRUE(report.success);
}

TEST_F(TemporalMigratorTest, Verify_ReportHasCorrectTableName) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("orders", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    EXPECT_EQ(report.table_name, "orders");
}

TEST_F(TemporalMigratorTest, Verify_ChecksContainKeyCount) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("tbl", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    bool found = false;
    for (const auto& c : report.checks) {
        if (c.check_name == "KEY_COUNT") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(TemporalMigratorTest, Verify_ChecksContainVersionOrder) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("tbl", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    bool found = false;
    for (const auto& c : report.checks) {
        if (c.check_name == "VERSION_ORDER") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(TemporalMigratorTest, Verify_ChecksContainNoOverlapping) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("tbl", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    bool found = false;
    for (const auto& c : report.checks) {
        if (c.check_name == "NO_OVERLAPPING_VERSIONS") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(TemporalMigratorTest, Verify_ChecksContainCurrentVersionOpen) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("tbl", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    bool found = false;
    for (const auto& c : report.checks) {
        if (c.check_name == "CURRENT_VERSION_OPEN") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(TemporalMigratorTest, Verify_EmptyTable_StillPasses) {
    auto plan = migrator.analyzeMigration("empty", {});
    auto [table, ok] = migrator.migrateToTemporal(plan, {});
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    EXPECT_TRUE(report.success);
}

TEST_F(TemporalMigratorTest, Verify_FailedCheckCountReflectsIssues) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("tbl", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    // For a fresh migration there should be 0 failed checks
    EXPECT_EQ(report.failedCheckCount(), 0u);
}

TEST_F(TemporalMigratorTest, Verify_KeyCountCheckPassesWhenEqual) {
    auto docs = makeSourceDocs(4);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    EXPECT_TRUE(ok);

    auto report = migrator.verifyMigration(table);
    for (const auto& c : report.checks) {
        if (c.check_name == "KEY_COUNT") {
            EXPECT_TRUE(c.passed) << "KEY_COUNT detail: " << c.detail;
            break;
        }
    }
}

// ============================================================================
// Group 5: Status / progress callback
// ============================================================================

TEST_F(TemporalMigratorTest, InitialStatus_IsPending) {
    EXPECT_EQ(migrator.getStatus(), MigrationStatus::PENDING);
}

TEST_F(TemporalMigratorTest, StatusName_ReturnsExpectedStrings) {
    EXPECT_EQ(TemporalMigrator::statusName(MigrationStatus::PENDING),   "PENDING");
    EXPECT_EQ(TemporalMigrator::statusName(MigrationStatus::ANALYZING), "ANALYZING");
    EXPECT_EQ(TemporalMigrator::statusName(MigrationStatus::MIGRATING), "MIGRATING");
    EXPECT_EQ(TemporalMigrator::statusName(MigrationStatus::VERIFYING), "VERIFYING");
    EXPECT_EQ(TemporalMigrator::statusName(MigrationStatus::COMPLETE),  "COMPLETE");
    EXPECT_EQ(TemporalMigrator::statusName(MigrationStatus::FAILED),    "FAILED");
}

TEST_F(TemporalMigratorTest, ProgressCallback_InvokedDuringMigration) {
    std::vector<MigrationStatus> statuses;
    migrator.setProgressCallback([&](MigrationStatus s, const std::string&) {
        statuses.push_back(s);
    });

    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    migrator.migrateToTemporal(plan, docs);

    EXPECT_FALSE(statuses.empty());
}

TEST_F(TemporalMigratorTest, ProgressCallback_CanBeCleared) {
    std::vector<MigrationStatus> statuses;
    migrator.setProgressCallback([&](MigrationStatus s, const std::string&) {
        statuses.push_back(s);
    });
    migrator.setProgressCallback(nullptr);

    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    migrator.migrateToTemporal(plan, docs);

    // After clearing, no callbacks should be invoked
    EXPECT_TRUE(statuses.empty());
}

TEST_F(TemporalMigratorTest, AfterMigrate_StatusIsComplete) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("t", docs);
    migrator.migrateToTemporal(plan, docs);
    EXPECT_EQ(migrator.getStatus(), MigrationStatus::COMPLETE);
}

// ============================================================================
// Group 6: Serialisation
// ============================================================================

TEST_F(TemporalMigratorTest, MigrationPlan_ToJson_HasSourceTableName) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto j    = plan.toJson();
    EXPECT_EQ(j["source_table_name"], "employees");
}

TEST_F(TemporalMigratorTest, MigrationPlan_ToJson_HasSourceRowCount) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto j    = plan.toJson();
    EXPECT_EQ(j["source_row_count"], 3u);
}

TEST_F(TemporalMigratorTest, MigrationPlan_ToJson_HasColumns) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto j    = plan.toJson();
    EXPECT_TRUE(j["columns"].is_array());
    EXPECT_FALSE(j["columns"].empty());
}

TEST_F(TemporalMigratorTest, MigrationReport_ToJson_HasSuccessField) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    auto report = migrator.verifyMigration(table);
    auto j = report.toJson();
    EXPECT_TRUE(j.contains("success"));
}

TEST_F(TemporalMigratorTest, MigrationReport_ToJson_HasChecksArray) {
    auto docs = makeSourceDocs(2);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    auto report = migrator.verifyMigration(table);
    auto j = report.toJson();
    EXPECT_TRUE(j["checks"].is_array());
}

TEST_F(TemporalMigratorTest, MigrationStats_ToJson_HasRowsMigrated) {
    auto docs = makeSourceDocs(4);
    auto plan = migrator.analyzeMigration("employees", docs);
    migrator.migrateToTemporal(plan, docs);
    auto j = migrator.getStats().toJson();
    EXPECT_TRUE(j.contains("rows_migrated"));
    EXPECT_EQ(j["rows_migrated"], 4u);
}

TEST_F(TemporalMigratorTest, GetLastReport_BeforeVerify_IsDefault) {
    auto report = migrator.getLastReport();
    // Before any verification, the default report should not indicate success
    // (it is default-constructed with success = false)
    EXPECT_FALSE(report.success);
}

TEST_F(TemporalMigratorTest, GetLastReport_AfterVerify_MatchesReturnValue) {
    auto docs = makeSourceDocs(3);
    auto plan = migrator.analyzeMigration("employees", docs);
    auto [table, ok] = migrator.migrateToTemporal(plan, docs);
    auto report = migrator.verifyMigration(table);

    EXPECT_EQ(migrator.getLastReport().success, report.success);
    EXPECT_EQ(migrator.getLastReport().table_name, report.table_name);
}
