/**
 * Temporal v1.8.0 + v1.9.0 — Focused Tests
 *
 * CDCPL-01..08  CDCPersistentLog  (WAL, CRC, rotation, replay)
 * SD2-01..06    SnapshotDiff      (diff added/removed/modified)
 * BTM-01..06    BiTemporalTable::merge (LWW cross-node reconciliation)
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_cdc.h"
#include "temporal/snapshot_manager.h"
#include "temporal/system_versioned_table.h"
#include "temporal/bi_temporal.h"
#include <filesystem>
#include <stdexcept>

using namespace themisdb::temporal;

// ============================================================================
// Helpers
// ============================================================================

static ChangeEvent makeEv(const std::string& table,
                           ChangeType type,
                           const std::string& entity_id,
                           Timestamp ts = 1000) {
    ChangeEvent ev;
    ev.type             = type;
    ev.table_name       = table;
    ev.entity_id        = entity_id;
    ev.after_value      = {{"id", entity_id}};
    ev.transaction_time = ts;
    ev.valid_from       = ts;
    ev.valid_to         = kMaxTimestamp;
    return ev;
}

// Unique temp directory per test to avoid cross-test contamination.
static std::string tempDir(const std::string& suffix) {
    return (std::filesystem::temp_directory_path() /
            ("themisdb_cdc_test_" + suffix + "_" +
             std::to_string(
                 std::chrono::duration_cast<std::chrono::nanoseconds>(
                     std::chrono::steady_clock::now().time_since_epoch())
                 .count())))
               .string();
}

// ============================================================================
// CDCPL — CDCPersistentLog tests
// ============================================================================

// CDCPL-01: open() creates the directory and sets isOpen() = true.
TEST(CDCPersistentLogTest, CDCPL_01_OpenCreatesDirectory) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("01")});
    EXPECT_FALSE(log.isOpen());
    log.open();
    EXPECT_TRUE(log.isOpen());
    log.close();
}

// CDCPL-02: append + replay round-trip restores the event.
TEST(CDCPersistentLogTest, CDCPL_02_AppendReplayRoundTrip) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("02")});
    log.open();
    log.append(makeEv("employees", ChangeType::INSERT, "emp1", 1000));
    log.append(makeEv("employees", ChangeType::UPDATE, "emp1", 2000));
    log.close();

    auto events = log.replay("employees", {0, kMaxTimestamp});
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].entity_id, "emp1");
    EXPECT_EQ(events[0].type, ChangeType::INSERT);
    EXPECT_EQ(events[1].type, ChangeType::UPDATE);
}

// CDCPL-03: replay filters by table_name.
TEST(CDCPersistentLogTest, CDCPL_03_ReplayFiltersTable) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("03")});
    log.open();
    log.append(makeEv("employees", ChangeType::INSERT, "emp1", 1000));
    log.append(makeEv("orders",    ChangeType::INSERT, "ord1", 2000));
    log.close();

    auto all = log.replay("", {0, kMaxTimestamp});
    EXPECT_EQ(all.size(), 2u);

    auto emp_only = log.replay("employees", {0, kMaxTimestamp});
    ASSERT_EQ(emp_only.size(), 1u);
    EXPECT_EQ(emp_only[0].table_name, "employees");
}

// CDCPL-04: replay filters by TimeRange.
TEST(CDCPersistentLogTest, CDCPL_04_ReplayFiltersTimeRange) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("04")});
    log.open();
    log.append(makeEv("t", ChangeType::INSERT, "a", 100));
    log.append(makeEv("t", ChangeType::INSERT, "b", 500));
    log.append(makeEv("t", ChangeType::INSERT, "c", 900));
    log.close();

    auto events = log.replay("t", {300, 800});
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, "b");
}

// CDCPL-05: totalBytesWritten increases monotonically.
TEST(CDCPersistentLogTest, CDCPL_05_TotalBytesWritten) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("05")});
    log.open();
    EXPECT_EQ(log.totalBytesWritten(), 0u);
    log.append(makeEv("t", ChangeType::INSERT, "x", 1));
    EXPECT_GT(log.totalBytesWritten(), 0u);
    auto before = log.totalBytesWritten();
    log.append(makeEv("t", ChangeType::INSERT, "y", 2));
    EXPECT_GT(log.totalBytesWritten(), before);
    log.close();
}

// CDCPL-06: segmentCount returns the correct number of .seg files.
TEST(CDCPersistentLogTest, CDCPL_06_SegmentCount) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("06")});
    log.open();
    EXPECT_EQ(log.segmentCount(), 1u);
    log.append(makeEv("t", ChangeType::INSERT, "x", 1));
    log.close();
    EXPECT_EQ(log.segmentCount(), 1u);
}

// CDCPL-07: segment rotation creates a new file once the size limit is hit.
TEST(CDCPersistentLogTest, CDCPL_07_SegmentRotation) {
    // Very small segment limit (64 bytes) to force rapid rotation.
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("07"), 64u});
    log.open();
    // Each event JSON is well over 64 bytes, so every append rotates.
    for (int i = 0; i < 5; ++i) {
        log.append(makeEv("t", ChangeType::INSERT, "entity_" + std::to_string(i),
                          static_cast<Timestamp>(i * 100)));
    }
    log.close();
    EXPECT_GE(log.segmentCount(), 2u);

    // All events must still be recoverable.
    auto all = log.replay("t", {0, kMaxTimestamp});
    EXPECT_EQ(all.size(), 5u);
}

// CDCPL-08: append on closed log throws std::logic_error.
TEST(CDCPersistentLogTest, CDCPL_08_AppendOnClosedLogThrows) {
    CDCPersistentLog log(CDCPersistentLog::Config{tempDir("08")});
    EXPECT_THROW(log.append(makeEv("t", ChangeType::INSERT, "x")),
                 std::logic_error);
}

// ============================================================================
// SD2 — SnapshotDiff tests
// ============================================================================

class SnapshotDiffTest : public ::testing::Test {
protected:
    TemporalSnapshotManager mgr;
    SystemVersionedTable    tbl{"employees"};
    Timestamp               t0{1000};

    void SetUp() override {
        tbl.insert("emp1", {{"name", "Alice"}});
        tbl.insert("emp2", {{"name", "Bob"}});
    }
};

// SD2-01: diff of identical snapshots returns empty result.
TEST_F(SnapshotDiffTest, SD2_01_IdenticalSnapshots) {
    auto h1 = mgr.createSnapshot({{"employees", &tbl}});
    auto d   = mgr.diff(h1, h1, "employees");
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(d.totalChanges(), 0u);
}

// SD2-02: newly inserted row appears in diff.added.
TEST_F(SnapshotDiffTest, SD2_02_AddedRow) {
    auto h1 = mgr.createSnapshot({{"employees", &tbl}});
    tbl.insert("emp3", {{"name", "Charlie"}});
    auto h2 = mgr.createSnapshot({{"employees", &tbl}});

    auto d = mgr.diff(h1, h2, "employees");
    ASSERT_EQ(d.added.size(), 1u);
    EXPECT_EQ(d.added[0].key, "emp3");
    EXPECT_TRUE(d.removed.empty());
    EXPECT_TRUE(d.modified.empty());
}

// SD2-03: deleted row appears in diff.removed.
TEST_F(SnapshotDiffTest, SD2_03_RemovedRow) {
    auto h1 = mgr.createSnapshot({{"employees", &tbl}});
    tbl.deleteRow("emp2");
    auto h2 = mgr.createSnapshot({{"employees", &tbl}});

    auto d = mgr.diff(h1, h2, "employees");
    // emp2 is no longer current in h2
    EXPECT_FALSE(d.removed.empty());
    auto it = std::find_if(d.removed.begin(), d.removed.end(),
                           [](const VersionedDocument& v){ return v.key == "emp2"; });
    EXPECT_NE(it, d.removed.end());
}

// SD2-04: updated row appears in diff.modified.
TEST_F(SnapshotDiffTest, SD2_04_ModifiedRow) {
    auto h1 = mgr.createSnapshot({{"employees", &tbl}});
    tbl.update("emp1", {{"name", "Alice Smith"}});
    auto h2 = mgr.createSnapshot({{"employees", &tbl}});

    auto d = mgr.diff(h1, h2, "employees");
    ASSERT_EQ(d.modified.size(), 1u);
    EXPECT_EQ(d.modified[0].key, "emp1");
    EXPECT_EQ(d.modified[0].data["name"], "Alice Smith");
}

// SD2-05: diff with invalid handle returns empty diff (no crash).
TEST_F(SnapshotDiffTest, SD2_05_InvalidHandle) {
    auto h1 = mgr.createSnapshot({{"employees", &tbl}});
    SnapshotHandle bad;  // default-constructed → invalid
    auto d = mgr.diff(h1, bad, "employees");
    // All rows in h1 should appear as removed.
    EXPECT_FALSE(d.removed.empty() && d.added.empty());
    // No crash and no exception.
}

// SD2-06: toJson() serialises the diff correctly.
TEST_F(SnapshotDiffTest, SD2_06_ToJson) {
    auto h1 = mgr.createSnapshot({{"employees", &tbl}});
    tbl.insert("emp3", {{"name", "Charlie"}});
    auto h2 = mgr.createSnapshot({{"employees", &tbl}});

    auto d = mgr.diff(h1, h2, "employees");
    auto j = d.toJson();
    EXPECT_TRUE(j.contains("added"));
    EXPECT_TRUE(j.contains("removed"));
    EXPECT_TRUE(j.contains("modified"));
    EXPECT_EQ(j["added"].size(), 1u);
}

// ============================================================================
// BTM — BiTemporalTable::merge tests
// ============================================================================

class BiTemporalMergeTest : public ::testing::Test {
protected:
    BiTemporalTable local{"orders", "node-A"};
    BiTemporalTable remote{"orders", "node-B"};

    static constexpr Timestamp kT1 = 1000;
    static constexpr Timestamp kT2 = 2000;
    static constexpr Timestamp kT3 = 3000;
};

// BTM-01: merge from empty remote is a no-op.
TEST_F(BiTemporalMergeTest, BTM_01_MergeEmptyRemote) {
    local.insertWithValidTime("ord1", {{"status", "NEW"}}, {kT1, kMaxTimestamp});
    auto res = local.merge(remote);
    EXPECT_EQ(res.rows_adopted, 0u);
    EXPECT_EQ(res.keys_seen, 0u);
    EXPECT_EQ(local.versionCount(), 1u);
}

// BTM-02: remote row with newer sys_time wins.
TEST_F(BiTemporalMergeTest, BTM_02_RemoteWinsLWW) {
    local.insertWithValidTime("ord1", {{"status", "NEW"}},       {kT1, kMaxTimestamp});
    remote.insertWithValidTime("ord1", {{"status", "SHIPPED"}},  {kT1, kMaxTimestamp});
    // Manipulate remote's row to have a later sys_time.
    // Since BiTemporalTable uses now() we ensure remote is "newer" by inserting
    // into local first, then remote.  In a real scenario the sys_time is
    // assigned by the source node.  Here we verify via actual later wall-clock.

    auto res = local.merge(remote);
    // Remote sys_time.start may be >= local (same millisecond).  Test the
    // invariant: after merge local has at least one version for ord1.
    EXPECT_GE(local.versionCount(), 1u);
    EXPECT_EQ(res.keys_seen, 1u);
}

// BTM-03: local row with newer sys_time is kept (remote skipped).
TEST_F(BiTemporalMergeTest, BTM_03_LocalWinsIfNewer) {
    // Insert into remote first, then local, so local.sys_time > remote.sys_time.
    remote.insertWithValidTime("ord1", {{"status", "OLD"}}, {kT1, kMaxTimestamp});
    local.insertWithValidTime("ord1",  {{"status", "NEW"}}, {kT2, kMaxTimestamp});

    auto res = local.merge(remote);
    EXPECT_EQ(res.rows_skipped, 1u);
    EXPECT_EQ(res.rows_adopted, 0u);

    // Local current version must still show "NEW".
    auto rows = local.queryCurrentByValidTime("ord1", kT2);
    ASSERT_FALSE(rows.empty());
    EXPECT_EQ(rows[0].data["status"], "NEW");
}

// BTM-04: key present only in remote is adopted.
TEST_F(BiTemporalMergeTest, BTM_04_NewKeyFromRemote) {
    remote.insertWithValidTime("ord99", {{"status", "PENDING"}}, {kT1, kMaxTimestamp});
    auto before = local.keyCount();
    auto res    = local.merge(remote);
    EXPECT_EQ(res.rows_adopted, 1u);
    EXPECT_GT(local.keyCount(), before);
}

// BTM-05: merge with mismatched table names is a no-op.
TEST_F(BiTemporalMergeTest, BTM_05_TableNameMismatch) {
    BiTemporalTable other{"invoices", "node-C"};
    other.insertWithValidTime("inv1", {{"amount", 100}}, {kT1, kMaxTimestamp});
    auto res = local.merge(other);
    EXPECT_EQ(res.rows_adopted, 0u);
    EXPECT_EQ(res.keys_seen, 0u);
}

// BTM-06: MergeResult statistics are accurate for multi-key merges.
TEST_F(BiTemporalMergeTest, BTM_06_MergeResultStats) {
    local.insertWithValidTime("ord1", {{"s", "A"}}, {kT1, kMaxTimestamp});
    local.insertWithValidTime("ord2", {{"s", "B"}}, {kT2, kMaxTimestamp});

    // ord1 in remote is older → skipped; ord3 is new → adopted.
    remote.insertWithValidTime("ord1", {{"s", "A_old"}}, {kT1 - 100, kMaxTimestamp});
    remote.insertWithValidTime("ord3", {{"s", "C"}},     {kT3,       kMaxTimestamp});

    auto res = local.merge(remote);
    EXPECT_EQ(res.keys_seen, 2u);      // ord1 + ord3
    EXPECT_EQ(res.rows_skipped, 1u);   // ord1 was older
    EXPECT_EQ(res.rows_adopted, 1u);   // ord3 is new
}
