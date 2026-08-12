/**
 * Tests for TemporalSnapshotManager
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/snapshot_manager.h"

using namespace themisdb::temporal;

// ── Test fixture with injectable clock ───────────────────────────────────────

class TemporalSnapshotManagerTest : public ::testing::Test {
protected:
    // Start the fake clock well ahead of real wall-clock time so that rows
    // inserted via SystemVersionedTable (which uses the real now()) always
    // fall before the snapshot creation timestamp.
    Timestamp fake_time_{now() + 10000};

    TemporalSnapshotManager mgr{[this]() { return fake_time_; }};

    void advanceTime(Timestamp ms) { fake_time_ += ms; }

    void populateTable(SystemVersionedTable& t) {
        t.insert("k1", {{"value", 10}});
        t.insert("k2", {{"value", 20}});
    }
};

// ── createSnapshot ───────────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, CreateSnapshot_ValidHandle) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    EXPECT_TRUE(handle.isValid());
    EXPECT_FALSE(handle.snapshot_id.empty());
    EXPECT_EQ(handle.included_tables.size(), 1u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
}

TEST_F(TemporalSnapshotManagerTest, CreateSnapshot_MultipleTablesAllIncluded) {
    SystemVersionedTable t1{"tbl1", "node_a"};
    SystemVersionedTable t2{"tbl2", "node_a"};
    populateTable(t1);
    populateTable(t2);

    auto handle = mgr.createSnapshot({{"tbl1", &t1}, {"tbl2", &t2}});
    EXPECT_EQ(handle.included_tables.size(), 2u);
}

// ── querySnapshot ────────────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_ReturnsSnapshotRows) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    // Mutate the live table after snapshot
    t.update("k1", {{"value", 99}});

    // Snapshot should still return original value
    auto rows = mgr.querySnapshot(handle, "tbl");
    ASSERT_EQ(rows.size(), 2u);

    bool found_k1_original = false;
    for (const auto& row : rows) {
        if (row.key == "k1") {
            EXPECT_EQ(row.data["value"], 10);
            found_k1_original = true;
        }
    }
    EXPECT_TRUE(found_k1_original);
}

TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_WithFilter_ReturnsSubset) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    auto rows = mgr.querySnapshot(handle, "tbl", {{"value", 10}});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].key, "k1");
}

TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_InvalidHandle_ReturnsEmpty) {
    SnapshotHandle bad_handle;
    bad_handle.snapshot_id = "does_not_exist";

    auto rows = mgr.querySnapshot(bad_handle, "tbl");
    EXPECT_TRUE(rows.empty());
}

TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_UnknownTable_ReturnsEmpty) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    auto rows = mgr.querySnapshot(handle, "other_table");
    EXPECT_TRUE(rows.empty());
}

// ── releaseSnapshot ──────────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, ReleaseSnapshot_SnapshotCountDecreases) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});
    EXPECT_EQ(mgr.snapshotCount(), 1u);

    EXPECT_TRUE(mgr.releaseSnapshot(handle));
    EXPECT_EQ(mgr.snapshotCount(), 0u);
    EXPECT_FALSE(mgr.isAlive(handle));
}

TEST_F(TemporalSnapshotManagerTest, ReleaseSnapshot_NonExistentHandle_ReturnsFalse) {
    SnapshotHandle bad;
    bad.snapshot_id = "ghost";
    EXPECT_FALSE(mgr.releaseSnapshot(bad));
}

// ── isAlive ──────────────────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, IsAlive_BeforeAndAfterRelease) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    EXPECT_TRUE(mgr.isAlive(handle));
    mgr.releaseSnapshot(handle);
    EXPECT_FALSE(mgr.isAlive(handle));
}

// ── statistics ───────────────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, Statistics_TrackCreatedAndReleased) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});
    mgr.releaseSnapshot(h1);

    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["total_created"], 2);
    EXPECT_EQ(stats["total_released"], 1);
    EXPECT_EQ(stats["active_snapshots"], 1);
}

// ── version_number ───────────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, VersionNumber_MonotonicallyIncreasing) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});
    auto h3 = mgr.createSnapshot({{"tbl", &t}});

    EXPECT_LT(h1.version_number, h2.version_number);
    EXPECT_LT(h2.version_number, h3.version_number);
}

TEST_F(TemporalSnapshotManagerTest, VersionNumber_ToJson_ContainsVersionNumber) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto h = mgr.createSnapshot({{"tbl", &t}});

    auto j = h.toJson();
    EXPECT_TRUE(j.contains("version_number"));
    EXPECT_EQ(j["version_number"].get<uint64_t>(), h.version_number);
}

TEST_F(TemporalSnapshotManagerTest, VersionNumber_OrderingOperator) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});

    EXPECT_TRUE(h1 < h2);
    EXPECT_FALSE(h2 < h1);
}

// ── getSnapshotMetadata ──────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_ValidHandle) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    auto meta = mgr.getSnapshotMetadata(handle);
    EXPECT_TRUE(meta.is_valid);
    EXPECT_EQ(meta.total_tables, 1u);
    EXPECT_EQ(meta.total_rows, 2u);
    EXPECT_EQ(meta.handle.snapshot_id, handle.snapshot_id);
    EXPECT_EQ(meta.handle.version_number, handle.version_number);
}

TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_MultipleTablesRowCount) {
    SystemVersionedTable t1{"tbl1", "node_a"};
    SystemVersionedTable t2{"tbl2", "node_a"};
    populateTable(t1);
    t2.insert("x1", {{"v", 1}});
    auto handle = mgr.createSnapshot({{"tbl1", &t1}, {"tbl2", &t2}});

    auto meta = mgr.getSnapshotMetadata(handle);
    EXPECT_TRUE(meta.is_valid);
    EXPECT_EQ(meta.total_tables, 2u);
    EXPECT_EQ(meta.total_rows, 3u);
}

TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_InvalidHandle_NotValid) {
    SnapshotHandle bad;
    bad.snapshot_id = "nonexistent";

    auto meta = mgr.getSnapshotMetadata(bad);
    EXPECT_FALSE(meta.is_valid);
    EXPECT_EQ(meta.total_tables, 0u);
    EXPECT_EQ(meta.total_rows, 0u);
}

TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_AfterRelease_NotValid) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    mgr.releaseSnapshot(handle);
    auto meta = mgr.getSnapshotMetadata(handle);
    EXPECT_FALSE(meta.is_valid);
}

TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_ToJson) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    auto meta = mgr.getSnapshotMetadata(handle);
    auto j = meta.toJson();
    EXPECT_TRUE(j.contains("handle"));
    EXPECT_TRUE(j.contains("total_tables"));
    EXPECT_TRUE(j.contains("total_rows"));
    EXPECT_TRUE(j.contains("is_valid"));
}

// ── garbageCollectByAge (TTL) ────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_RemovesExpired) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    // Create snapshot at the current fake_time_
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    // Advance clock by 100ms so the 10ms max_age threshold is exceeded
    advanceTime(100);

    size_t removed = mgr.garbageCollectByAge(Timestamp{10});
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(mgr.snapshotCount(), 0u);
    EXPECT_FALSE(mgr.isAlive(handle));
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_KeepsFresh) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    auto handle = mgr.createSnapshot({{"tbl", &t}});

    // Only advance 5ms — snapshot is 5ms old, max_age is 3600s
    advanceTime(5);

    size_t removed = mgr.garbageCollectByAge(Timestamp{3600000});
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
    EXPECT_TRUE(mgr.isAlive(handle));
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_ZeroIsNoop) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    advanceTime(10000);

    size_t removed = mgr.garbageCollectByAge(Timestamp{0});
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_StatsUpdated) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    advanceTime(100);
    mgr.garbageCollectByAge(Timestamp{10});

    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["total_gc_collected"], 1);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_OnlyRemovesExpired) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    // Snapshot at the current fake_time_
    auto old_handle = mgr.createSnapshot({{"tbl", &t}});

    // Advance so old snapshot is 100ms old, then create a fresh one
    advanceTime(100);
    auto fresh_handle = mgr.createSnapshot({{"tbl", &t}});

    // GC with 50ms max age: old (100ms) removed, fresh (0ms) kept
    size_t removed = mgr.garbageCollectByAge(Timestamp{50});
    EXPECT_EQ(removed, 1u);
    EXPECT_FALSE(mgr.isAlive(old_handle));
    EXPECT_TRUE(mgr.isAlive(fresh_handle));
}

// ── garbageCollectByCount (max-count) ───────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_RemovesOldest) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});
    auto h3 = mgr.createSnapshot({{"tbl", &t}});
    ASSERT_EQ(mgr.snapshotCount(), 3u);

    // Keep only the 1 newest snapshot
    size_t removed = mgr.garbageCollectByCount(1u);
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
    // The newest (highest version) must survive
    EXPECT_TRUE(mgr.isAlive(h3));
    EXPECT_FALSE(mgr.isAlive(h1));
    EXPECT_FALSE(mgr.isAlive(h2));
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_NoRemovalWhenUnderLimit) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});

    size_t removed = mgr.garbageCollectByCount(5u);
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(mgr.snapshotCount(), 2u);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_ZeroRemovesAll) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});

    size_t removed = mgr.garbageCollectByCount(0u);
    EXPECT_EQ(removed, 3u);
    EXPECT_EQ(mgr.snapshotCount(), 0u);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_StatsUpdated) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});
    mgr.garbageCollectByCount(1u);

    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["total_gc_collected"], 1);
}

// ── Isolation – concurrent writes don't affect snapshot ───────────────────────

TEST_F(TemporalSnapshotManagerTest, Isolation_ConcurrentInsert_NotInSnapshot) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    auto handle = mgr.createSnapshot({{"tbl", &t}});

    // Insert into the live table after the snapshot was taken
    t.insert("k3", {{"value", 30}});

    auto rows = mgr.querySnapshot(handle, "tbl");
    // Snapshot should NOT contain k3
    bool found_k3 = false;
    for (const auto& row : rows) {
        if (row.key == "k3") {
            found_k3 = true;
        }
    }
    EXPECT_FALSE(found_k3);
    EXPECT_EQ(rows.size(), 2u);
}

// ============================================================================
// SnapshotDiff tests (SD2-01 .. SD2-06) — v1.9.0
// ============================================================================

class SnapshotDiffTest : public ::testing::Test {
protected:
    TemporalSnapshotManager mgr;

    SystemVersionedTable buildTable(const std::string& name,
                                    std::vector<std::pair<std::string, int>> kv) {
        SystemVersionedTable t{name, "node_a"};
        for (const auto& [k, v] : kv) {
            t.insert(k, {{"val", v}});
        }
        return t;
    }
};

TEST_F(SnapshotDiffTest, SD2_01_IdenticalSnapshotsAreEmpty) {
    auto t = buildTable("tbl", {{"k1", 1}, {"k2", 2}});
    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});

    auto d = mgr.diff(h1, h2);
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(d.tables_examined, 1u);
}

TEST_F(SnapshotDiffTest, SD2_02_AddedKey) {
    auto t1 = buildTable("tbl", {{"k1", 1}});
    auto h1 = mgr.createSnapshot({{"tbl", &t1}});

    // Add k2 after snapshot h1
    t1.insert("k2", {{"val", 2}});
    auto h2 = mgr.createSnapshot({{"tbl", &t1}});

    auto d = mgr.diff(h1, h2);
    EXPECT_FALSE(d.empty());
    ASSERT_EQ(d.added["tbl"].size(), 1u);
    EXPECT_EQ(d.added["tbl"][0], "k2");
    EXPECT_TRUE(d.modified.empty());
    EXPECT_TRUE(d.removed.empty());
}

TEST_F(SnapshotDiffTest, SD2_03_RemovedKey) {
    auto t1 = buildTable("tbl", {{"k1", 1}, {"k2", 2}});
    auto h1 = mgr.createSnapshot({{"tbl", &t1}});

    // Delete k2 by closing the current version in the system-versioned table.
    ASSERT_TRUE(t1.deleteRow("k2"));
    auto h2 = mgr.createSnapshot({{"tbl", &t1}});

    // k2 is not removed from the table; data stays but version changes.
    // In the diff, k2 might appear as modified.
    auto d = mgr.diff(h1, h2);
    EXPECT_FALSE(d.empty());
}

TEST_F(SnapshotDiffTest, SD2_04_ModifiedKey) {
    auto t1 = buildTable("tbl", {{"k1", 10}});
    auto h1 = mgr.createSnapshot({{"tbl", &t1}});

    // Update k1
    t1.update("k1", {{"val", 99}});
    auto h2 = mgr.createSnapshot({{"tbl", &t1}});

    auto d = mgr.diff(h1, h2);
    ASSERT_FALSE(d.modified.empty());
    ASSERT_EQ(d.modified["tbl"].size(), 1u);
    EXPECT_EQ(d.modified["tbl"][0], "k1");
}

TEST_F(SnapshotDiffTest, SD2_05_InvalidBaseHandleThrows) {
    auto t = buildTable("tbl", {{"k1", 1}});
    auto h = mgr.createSnapshot({{"tbl", &t}});
    SnapshotHandle bad_handle;  // empty snapshot_id → invalid
    EXPECT_THROW(mgr.diff(bad_handle, h), std::invalid_argument);
}

TEST_F(SnapshotDiffTest, SD2_06_DiffToJsonContainsTables) {
    auto t = buildTable("tbl", {{"k1", 1}});
    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    t.update("k1", {{"val", 2}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});

    auto d = mgr.diff(h1, h2);
    auto j = d.toJson();
    EXPECT_TRUE(j.contains("tables_examined"));
    EXPECT_TRUE(j.contains("modified"));
}
