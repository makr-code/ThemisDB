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

// ── Config / createVersionedTable ────────────────────────────────────────────

TEST(SystemVersionedTableConfigTest, DefaultConfig_HistoryTableNameDerived) {
    SystemVersionedTable tbl("orders");
    EXPECT_EQ(tbl.getConfig().history_table_name, "orders_history");
    EXPECT_TRUE(tbl.getConfig().compress_history);
    EXPECT_TRUE(tbl.getConfig().track_user_id);
    EXPECT_GT(tbl.getConfig().retention_period.count(), 0);
}

TEST(SystemVersionedTableConfigTest, ExplicitConfig_Stored) {
    SystemVersionedTable::Config cfg;
    cfg.history_table_name = "orders_hist";
    cfg.compress_history   = false;
    cfg.retention_period   = std::chrono::seconds{90 * 24 * 3600};
    cfg.track_user_id      = false;

    SystemVersionedTable tbl("orders", cfg, "node_x");
    EXPECT_EQ(tbl.getConfig().history_table_name, "orders_hist");
    EXPECT_FALSE(tbl.getConfig().compress_history);
    EXPECT_FALSE(tbl.getConfig().track_user_id);
    EXPECT_EQ(tbl.getConfig().retention_period, std::chrono::seconds{90 * 24 * 3600});
}

TEST(SystemVersionedTableConfigTest, CreateVersionedTable_SetsSchemaInStats) {
    Document schema = {
        {"columns", {
            {{"name", "id"},     {"type", "INTEGER"}, {"primary_key", true}},
            {{"name", "name"},   {"type", "VARCHAR"}},
            {{"name", "salary"}, {"type", "DECIMAL"}}
        }},
        {"system_time", {{"start", "sys_start"}, {"end", "sys_end"}}}
    };

    auto tbl = SystemVersionedTable::createVersionedTable(
        "employees", schema, {}, "node_a");

    auto stats = tbl.getStatistics();
    EXPECT_EQ(stats["table_name"], "employees");
    EXPECT_EQ(stats["history_table"], "employees_history");
    ASSERT_TRUE(stats.contains("schema"));
    EXPECT_EQ(stats["schema"]["columns"].size(), 3u);
}

TEST(SystemVersionedTableConfigTest, CreateVersionedTable_CustomConfig) {
    SystemVersionedTable::Config cfg;
    cfg.history_table_name = "emp_hist";

    auto tbl = SystemVersionedTable::createVersionedTable(
        "employees", {}, cfg, "node_b");

    EXPECT_EQ(tbl.getConfig().history_table_name, "emp_hist");
}

// ── Upsert ───────────────────────────────────────────────────────────────────

TEST(SystemVersionedTableUpsertTest, Upsert_NewKey_ActsAsInsert) {
    SystemVersionedTable tbl{"products"};
    bool was_insert = tbl.upsert("p1", {{"price", 9.99}});
    EXPECT_TRUE(was_insert);
    EXPECT_EQ(tbl.versionCount(), 1u);
    auto current = tbl.getCurrent("p1");
    ASSERT_TRUE(current.has_value());
    EXPECT_DOUBLE_EQ(current->data["price"].get<double>(), 9.99);
}

TEST(SystemVersionedTableUpsertTest, Upsert_ExistingKey_ActsAsUpdate) {
    SystemVersionedTable tbl{"products"};
    tbl.insert("p1", {{"price", 9.99}, {"stock", 100}});

    bool was_insert = tbl.upsert("p1", {{"price", 12.50}});
    EXPECT_FALSE(was_insert); // update, not insert
    EXPECT_EQ(tbl.versionCount(), 2u);

    auto current = tbl.getCurrent("p1");
    ASSERT_TRUE(current.has_value());
    EXPECT_DOUBLE_EQ(current->data["price"].get<double>(), 12.50);
    EXPECT_EQ(current->data["stock"], 100); // unchanged field preserved
}

TEST(SystemVersionedTableUpsertTest, Upsert_AfterDelete_ActsAsInsert) {
    SystemVersionedTable tbl{"products"};
    tbl.insert("p1", {{"price", 9.99}});
    tbl.deleteRow("p1");

    bool was_insert = tbl.upsert("p1", {{"price", 15.00}});
    EXPECT_TRUE(was_insert);

    auto current = tbl.getCurrent("p1");
    ASSERT_TRUE(current.has_value());
    EXPECT_DOUBLE_EQ(current->data["price"].get<double>(), 15.00);
}

// ── track_user_id ─────────────────────────────────────────────────────────────

TEST(SystemVersionedTableConfigTest, TrackUserIdTrue_SetsModifiedBy) {
    SystemVersionedTable::Config cfg;
    cfg.track_user_id = true;
    SystemVersionedTable tbl("t", cfg, "admin_node");

    tbl.insert("k1", {{"v", 1}});
    auto doc = tbl.getCurrent("k1");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->modified_by, "admin_node");
}

TEST(SystemVersionedTableConfigTest, TrackUserIdFalse_ModifiedByEmpty) {
    SystemVersionedTable::Config cfg;
    cfg.track_user_id = false;
    SystemVersionedTable tbl("t", cfg, "admin_node");

    tbl.insert("k1", {{"v", 1}});
    auto doc = tbl.getCurrent("k1");
    ASSERT_TRUE(doc.has_value());
    EXPECT_TRUE(doc->modified_by.empty());
}

// ── enforceRetentionPolicy ───────────────────────────────────────────────────

TEST(SystemVersionedTableRetentionTest, EnforceRetention_YoungVersionsNotPurged) {
    // Use a non-zero retention period to exercise the retention logic.
    // Without advancing time, recently closed versions are "young" and must
    // not be removed.
    SystemVersionedTable::Config cfg;
    cfg.retention_period = std::chrono::milliseconds{50};
    SystemVersionedTable tbl("t", cfg, "node");

    tbl.insert("k1", {{"v", 1}});
    tbl.update("k1", {{"v", 2}});  // closes old version
    tbl.update("k1", {{"v", 3}});  // closes second version

    // 3 total versions; 2 are historical, but they are too "young" to match
    // the retention window when we enforce immediately.
    EXPECT_EQ(tbl.versionCount(), 3u);

    size_t removed = tbl.enforceRetentionPolicy();
    // Without advancing time, no versions should be removed yet.
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(tbl.versionCount(), 3u);
}

TEST(SystemVersionedTableRetentionTest, EnforceRetention_ZeroPeriod_IsNoop) {
    SystemVersionedTable::Config cfg;
    cfg.retention_period = std::chrono::milliseconds{0};
    SystemVersionedTable tbl("t", cfg, "node");

    tbl.insert("k1", {{"v", 1}});
    tbl.update("k1", {{"v", 2}});
    EXPECT_EQ(tbl.versionCount(), 2u);

    size_t removed = tbl.enforceRetentionPolicy();
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(tbl.versionCount(), 2u);
}

TEST(SystemVersionedTableRetentionTest, EnforceRetention_CurrentNeverPurged) {
    // The current (open-ended) version must survive even when a large
    // retention window is configured: sys_end == kMaxTimestamp is always
    // ≥ any finite cutoff, so the predicate never matches it.
    SystemVersionedTable::Config cfg;
    cfg.retention_period = std::chrono::milliseconds{50};
    SystemVersionedTable tbl("t", cfg, "node");

    tbl.insert("k1", {{"v", 1}});
    // Only one version exists (current); enforcing retention must be a no-op.
    size_t removed = tbl.enforceRetentionPolicy();
    EXPECT_EQ(removed, 0u);
    EXPECT_TRUE(tbl.getCurrent("k1").has_value());
}

// ── Statistics includes config ────────────────────────────────────────────────

TEST(SystemVersionedTableConfigTest, Statistics_IncludesConfigFields) {
    SystemVersionedTable::Config cfg;
    cfg.history_table_name = "emp_hist";
    cfg.compress_history   = false;
    cfg.track_user_id      = true;
    cfg.retention_period   = std::chrono::milliseconds{60000}; // 60 seconds in ms

    SystemVersionedTable tbl("emp", cfg);
    tbl.insert("e1", {{"name", "Alice"}});

    auto stats = tbl.getStatistics();
    EXPECT_EQ(stats["table_name"],   "emp");
    EXPECT_EQ(stats["history_table"], "emp_hist");
    EXPECT_EQ(stats["compress_history"], false);
    EXPECT_EQ(stats["track_user_id"],    true);
    EXPECT_EQ(stats["retention_period_ms"], 60000);
}
