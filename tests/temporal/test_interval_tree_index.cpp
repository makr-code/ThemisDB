/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_interval_tree_index.cpp                       ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:22:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     200                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for IntervalTreeIndex
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/interval_tree_index.h"

using namespace themisdb::temporal;

class IntervalTreeIndexTest : public ::testing::Test {
protected:
    IntervalTreeIndex tree{"test_tree"};

    IntervalEntry makeEntry(const std::string& key, Timestamp start,
                            Timestamp end,
                            nlohmann::json payload = {}) {
        return {key, {start, end}, std::move(payload)};
    }
};

// ── Insert / Size ─────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, Insert_Single_SizeBecomesOne) {
    tree.insert(makeEntry("k1", 100, 200));
    EXPECT_EQ(tree.size(), 1u);
}

TEST_F(IntervalTreeIndexTest, Insert_Multiple_SizeIncreases) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k2", 150, 300));
    tree.insert(makeEntry("k3", 250, 400));
    EXPECT_EQ(tree.size(), 3u);
}

// ── queryPoint ───────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, QueryPoint_MatchingEntries) {
    tree.insert(makeEntry("k1", 100, 300));
    tree.insert(makeEntry("k2", 150, 250));
    tree.insert(makeEntry("k3", 400, 500));

    auto results = tree.queryPoint(200);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(IntervalTreeIndexTest, QueryPoint_NoMatch_ReturnsEmpty) {
    tree.insert(makeEntry("k1", 100, 200));
    EXPECT_TRUE(tree.queryPoint(50).empty());
    EXPECT_TRUE(tree.queryPoint(200).empty()); // half-open: [100,200)
}

TEST_F(IntervalTreeIndexTest, QueryPoint_AtStart_ReturnsEntry) {
    tree.insert(makeEntry("k1", 100, 200));
    auto results = tree.queryPoint(100);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].key, "k1");
}

// ── queryOverlap ─────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, QueryOverlap_ReturnsAllOverlapping) {
    tree.insert(makeEntry("k1", 100, 300));
    tree.insert(makeEntry("k2", 200, 400));
    tree.insert(makeEntry("k3", 500, 600));

    auto results = tree.queryOverlap(150, 350);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(IntervalTreeIndexTest, QueryOverlap_NoOverlap_ReturnsEmpty) {
    tree.insert(makeEntry("k1", 100, 200));
    EXPECT_TRUE(tree.queryOverlap(200, 300).empty());
}

TEST_F(IntervalTreeIndexTest, QueryOverlap_PartialOverlap) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k2", 180, 300));
    tree.insert(makeEntry("k3", 400, 500));

    auto results = tree.queryOverlap(150, 250);
    EXPECT_EQ(results.size(), 2u);
}

// ── queryKey ─────────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, QueryKey_ReturnsAllVersions) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k1", 200, 300));
    tree.insert(makeEntry("k2", 100, 300));

    auto results = tree.queryKey("k1");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(IntervalTreeIndexTest, QueryKey_WithRange_FiltersVersions) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k1", 300, 400));

    auto results = tree.queryKey("k1", TimeRange{50, 250});
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].range.start, 100);
}

// ── remove ───────────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, Remove_ExistingEntry_SizeDecreases) {
    tree.insert(makeEntry("k1", 100, 200));
    EXPECT_EQ(tree.remove("k1", {100, 200}), 1u);
    EXPECT_EQ(tree.size(), 0u);
}

TEST_F(IntervalTreeIndexTest, RemoveKey_RemovesAllVersions) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k1", 200, 300));
    tree.insert(makeEntry("k2", 100, 300));

    EXPECT_EQ(tree.removeKey("k1"), 2u);
    EXPECT_EQ(tree.size(), 1u);
}

// ── clear ─────────────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, Clear_EmptiesTree) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k2", 300, 400));
    EXPECT_EQ(tree.size(), 2u);
    tree.clear();
    EXPECT_EQ(tree.size(), 0u);
    EXPECT_TRUE(tree.queryPoint(150).empty());
}

// ── stats ─────────────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, Stats_TracksCounts) {
    tree.insert(makeEntry("k1", 100, 200));
    tree.insert(makeEntry("k2", 150, 300));

    auto s = tree.stats();
    EXPECT_EQ(s.total_entries, 2u);

    tree.queryPoint(150);
    auto s2 = tree.stats();
    EXPECT_EQ(s2.point_queries, 1u);

    tree.queryOverlap(100, 250);
    auto s3 = tree.stats();
    EXPECT_EQ(s3.overlap_queries, 1u);
}

// ── name ─────────────────────────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, Name_ReturnsGivenName) {
    EXPECT_EQ(tree.name(), "test_tree");
}
