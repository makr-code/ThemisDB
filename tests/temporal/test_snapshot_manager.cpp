/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_snapshot_manager.cpp                          ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     189                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
