/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_snapshot_manager.cpp                          ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:02:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     182                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for TemporalSnapshotManager
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/snapshot_manager.h"
#include <thread>

using namespace themisdb::temporal;

class TemporalSnapshotManagerTest : public ::testing::Test {
protected:
    TemporalSnapshotManager mgr;

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
    EXPECT_TRUE(j.contains("ttl_ms"));
}

// ── garbageCollect (TTL) ─────────────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_TTL_RemovesExpired) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    // Create a snapshot, then wait so it ages well past the 10ms max_age threshold
    auto handle = mgr.createSnapshot({{"tbl", &t}});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // GC with 10ms max age: the snapshot should be removed
    size_t removed = mgr.garbageCollect(Timestamp{10});
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(mgr.snapshotCount(), 0u);
    EXPECT_FALSE(mgr.isAlive(handle));
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_TTL_KeepsFresh) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    auto handle = mgr.createSnapshot({{"tbl", &t}});

    // GC with 1-hour max age: the fresh snapshot must survive
    size_t removed = mgr.garbageCollect(Timestamp{3600000});
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
    EXPECT_TRUE(mgr.isAlive(handle));
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_TTL_ZeroIsNoop) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});

    size_t removed = mgr.garbageCollect(Timestamp{0});
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_TTL_StatsUpdated) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    mgr.garbageCollect(Timestamp{10});

    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["total_gc_collected"], 1);
}

// ── garbageCollect (max-count) ───────────────────────────────────────────────

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_MaxCount_RemovesOldest) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);

    auto h1 = mgr.createSnapshot({{"tbl", &t}});
    auto h2 = mgr.createSnapshot({{"tbl", &t}});
    auto h3 = mgr.createSnapshot({{"tbl", &t}});
    ASSERT_EQ(mgr.snapshotCount(), 3u);

    // Keep only the 1 newest snapshot
    size_t removed = mgr.garbageCollect(size_t{1});
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(mgr.snapshotCount(), 1u);
    // The newest (highest version) must survive
    EXPECT_TRUE(mgr.isAlive(h3));
    EXPECT_FALSE(mgr.isAlive(h1));
    EXPECT_FALSE(mgr.isAlive(h2));
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_MaxCount_NoRemovalWhenUnderLimit) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});

    size_t removed = mgr.garbageCollect(size_t{5});
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(mgr.snapshotCount(), 2u);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_MaxCount_ZeroRemovesAll) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});

    size_t removed = mgr.garbageCollect(size_t{0});
    EXPECT_EQ(removed, 3u);
    EXPECT_EQ(mgr.snapshotCount(), 0u);
}

TEST_F(TemporalSnapshotManagerTest, GarbageCollect_MaxCount_StatsUpdated) {
    SystemVersionedTable t{"tbl", "node_a"};
    populateTable(t);
    mgr.createSnapshot({{"tbl", &t}});
    mgr.createSnapshot({{"tbl", &t}});
    mgr.garbageCollect(size_t{1});

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
