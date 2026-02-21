/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_system_versioned_table.cpp                    ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     265                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for SystemVersionedTable
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/system_versioned_table.h"
#include <thread>

using namespace themisdb::temporal;

class SystemVersionedTableTest : public ::testing::Test {
protected:
    SystemVersionedTable table{"employees", "node_a"};
};

// ── Insert ───────────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, Insert_NewKey_Succeeds) {
    EXPECT_TRUE(table.insert("emp1", {{"name", "Alice"}, {"salary", 50000}}));
    EXPECT_EQ(table.keyCount(), 1u);
    EXPECT_EQ(table.versionCount(), 1u);
}

TEST_F(SystemVersionedTableTest, Insert_DuplicateKey_Fails) {
    table.insert("emp1", {{"name", "Alice"}});
    EXPECT_FALSE(table.insert("emp1", {{"name", "Bob"}}));
    EXPECT_EQ(table.versionCount(), 1u);
}

// ── GetCurrent ───────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, GetCurrent_ExistingKey_ReturnsCurrentVersion) {
    table.insert("emp1", {{"name", "Alice"}, {"salary", 50000}});
    auto current = table.getCurrent("emp1");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data["name"], "Alice");
    EXPECT_EQ(current->sys_time.end, kMaxTimestamp);
}

TEST_F(SystemVersionedTableTest, GetCurrent_MissingKey_ReturnsNullopt) {
    EXPECT_FALSE(table.getCurrent("nonexistent").has_value());
}

// ── Update ───────────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, Update_ExistingKey_CreatesNewVersion) {
    table.insert("emp1", {{"name", "Alice"}, {"salary", 50000}});
    EXPECT_TRUE(table.update("emp1", {{"salary", 55000}}));

    // Two versions: historical + current
    EXPECT_EQ(table.versionCount(), 2u);

    auto current = table.getCurrent("emp1");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data["salary"], 55000);
    EXPECT_EQ(current->data["name"], "Alice"); // Unchanged field preserved
}

TEST_F(SystemVersionedTableTest, Update_MissingKey_ReturnsFalse) {
    EXPECT_FALSE(table.update("nonexistent", {{"salary", 50000}}));
}

TEST_F(SystemVersionedTableTest, Update_ClosesOldVersion) {
    table.insert("emp1", {{"name", "Alice"}, {"salary", 50000}});
    auto history_before = table.getHistory("emp1");
    ASSERT_EQ(history_before.size(), 1u);
    EXPECT_EQ(history_before[0].sys_time.end, kMaxTimestamp);

    table.update("emp1", {{"salary", 55000}});

    auto history_after = table.getHistory("emp1");
    ASSERT_EQ(history_after.size(), 2u);

    // First version should be closed now
    EXPECT_NE(history_after[0].sys_time.end, kMaxTimestamp);
    // Second (current) version should be open
    EXPECT_EQ(history_after[1].sys_time.end, kMaxTimestamp);
}

// ── Delete ───────────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, Delete_ExistingKey_ClosesVersion) {
    table.insert("emp1", {{"name", "Alice"}});
    EXPECT_TRUE(table.deleteRow("emp1"));

    // Row deleted → no current version
    EXPECT_FALSE(table.getCurrent("emp1").has_value());

    // But history still accessible
    auto history = table.getHistory("emp1");
    ASSERT_EQ(history.size(), 1u);
    EXPECT_NE(history[0].sys_time.end, kMaxTimestamp);
}

TEST_F(SystemVersionedTableTest, Delete_MissingKey_ReturnsFalse) {
    EXPECT_FALSE(table.deleteRow("nonexistent"));
}

// ── GetAsOf ──────────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, GetAsOf_AfterInsert_ReturnsVersion) {
    Timestamp before = now();
    table.insert("emp1", {{"name", "Alice"}, {"salary", 50000}});
    Timestamp after = now();

    auto result = table.getAsOf("emp1", (before + after) / 2);
    // The sys_start of the inserted version should be <= mid and sys_end = MAX
    // Only holds if we query at a time after the insert
    // Use 'after' to be sure
    auto result2 = table.getAsOf("emp1", after);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2->data["name"], "Alice");
}

// ── GetHistoryInRange ─────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, GetHistoryInRange_OverlapsInsertTime) {
    // Use the full time range to avoid timing precision issues
    table.insert("emp1", {{"name", "Alice"}});
    table.update("emp1", {{"name", "Alicia"}});

    // Query from the beginning of time to ensure both versions are included
    TimeRange range{kMinTimestamp, kMaxTimestamp};
    auto history = table.getHistoryInRange("emp1", range);
    EXPECT_EQ(history.size(), 2u);
}

// ── Scan ─────────────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, Scan_Default_ReturnCurrentRows) {
    table.insert("emp1", {{"name", "Alice"}});
    table.insert("emp2", {{"name", "Bob"}});
    table.deleteRow("emp1");

    auto current = table.scan();
    EXPECT_EQ(current.size(), 1u);
    EXPECT_EQ(current[0].data["name"], "Bob");
}

TEST_F(SystemVersionedTableTest, Scan_AsOf_ReturnsHistoricalRows) {
    table.insert("emp1", {{"name", "Alice"}});
    // Capture a timestamp after the insert so we can use it as the AS-OF point.
    // We add a 1 ms sleep to ensure the update receives a strictly later
    // sys_start timestamp, which is the minimal sleep needed.
    Timestamp before_update = now();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    table.update("emp1", {{"name", "Alicia"}});

    // Query at before_update → the version current at that instant is Alice
    auto rows_at_before = table.scan(before_update);
    ASSERT_EQ(rows_at_before.size(), 1u);
    EXPECT_EQ(rows_at_before[0].data["name"], "Alice");
}

// ── Statistics ───────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, Statistics_ReflectsState) {
    table.insert("emp1", {{"name", "Alice"}});
    table.update("emp1", {{"salary", 50000}});

    auto stats = table.getStatistics();
    EXPECT_EQ(stats["table_name"], "employees");
    EXPECT_EQ(stats["key_count"], 1);
    EXPECT_EQ(stats["current_rows"], 1);
    EXPECT_EQ(stats["historical_rows"], 1);
    EXPECT_EQ(stats["total_versions"], 2);
}

// ── getAllKeys ────────────────────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, GetAllKeys_IncludesDeletedKeys) {
    table.insert("emp1", {{"name", "Alice"}});
    table.insert("emp2", {{"name", "Bob"}});
    table.deleteRow("emp2"); // fully deleted

    auto keys = table.getAllKeys();
    EXPECT_EQ(keys.size(), 2u); // both keys must be present
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "emp1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "emp2") != keys.end());
}

TEST_F(SystemVersionedTableTest, GetAllKeys_EmptyTable_ReturnsEmpty) {
    EXPECT_TRUE(table.getAllKeys().empty());
}

// ── purgeHistoricalVersions ───────────────────────────────────────────────────

TEST_F(SystemVersionedTableTest, PurgeHistorical_RemovesMatchingVersions) {
    table.insert("emp1", {{"name", "Alice"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    table.update("emp1", {{"name", "Alicia"}});

    // Two versions; only one historical
    EXPECT_EQ(table.versionCount(), 2u);

    // Purge everything that is closed
    size_t removed = table.purgeHistoricalVersions(
        "emp1",
        [](const VersionedDocument&) { return true; });

    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(table.versionCount(), 1u); // current still present
    auto current = table.getCurrent("emp1");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data["name"], "Alicia");
}

TEST_F(SystemVersionedTableTest, PurgeHistorical_CurrentVersionNeverRemoved) {
    table.insert("emp1", {{"name", "Alice"}});

    // Attempt to purge even current versions via predicate
    size_t removed = table.purgeHistoricalVersions(
        "emp1",
        [](const VersionedDocument&) { return true; });

    EXPECT_EQ(removed, 0u); // current version is protected
    EXPECT_EQ(table.versionCount(), 1u);
}

TEST_F(SystemVersionedTableTest, PurgeHistorical_AllKeys_GlobalPurge) {
    table.insert("emp1", {{"name", "Alice"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    table.update("emp1", {{"name", "Alicia"}});
    table.insert("emp2", {{"name", "Bob"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    table.update("emp2", {{"name", "Bobby"}});

    // 4 total versions, 2 historical
    EXPECT_EQ(table.versionCount(), 4u);

    size_t removed = table.purgeHistoricalVersions(
        [](const VersionedDocument&) { return true; });

    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(table.versionCount(), 2u);
}
