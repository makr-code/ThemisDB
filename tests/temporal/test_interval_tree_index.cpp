/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_interval_tree_index.cpp                       ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:52:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     208                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ab6254146d  2026-03-20  docs(temporal): document Phase 4 components and enrich tests ║
    • f8f5de7b2b  2026-03-20  feat(temporal): add Phase 4 tests, update CMake/CI/ROADMAP ║
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

// ── Additional edge-case tests ────────────────────────────────────────────────

TEST_F(IntervalTreeIndexTest, QueryOverlap_TouchingBoundary_ReturnsEmpty) {
    // [100,200) and [200,300): half-open intervals — they touch but do not overlap
    tree.insert(makeEntry("k1", 100, 200));
    auto results = tree.queryOverlap(200, 300);
    EXPECT_TRUE(results.empty());
}

TEST_F(IntervalTreeIndexTest, Insert_DuplicateKey_DifferentRange_SizeIncreasesByTwo) {
    tree.insert(makeEntry("dup", 100, 200));
    tree.insert(makeEntry("dup", 300, 400));
    EXPECT_EQ(tree.size(), 2u);
}

TEST_F(IntervalTreeIndexTest, QueryPoint_ManyIntervals_ReturnsAllContaining) {
    // 10 non-overlapping intervals [i*100, (i+1)*100) for i in 0..9
    for (int i = 0; i < 10; ++i) {
        tree.insert(makeEntry("k" + std::to_string(i),
                              static_cast<Timestamp>(i * 100),
                              static_cast<Timestamp>((i + 1) * 100)));
    }
    // Query exactly in the middle of interval #5 → [500,600)
    auto results = tree.queryPoint(550);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].key, "k5");
}

// ============================================================================
// erase() tests (ITX-ERASE-01 .. ITX-ERASE-04)
// ============================================================================

// ITX-ERASE-01: erase() removes all entries for the given key
TEST_F(IntervalTreeIndexTest, Erase_RemovesAllEntriesForKey) {
    tree.insert(makeEntry("alpha", 100, 200));
    tree.insert(makeEntry("alpha", 300, 400));
    tree.insert(makeEntry("beta",  100, 200));
    ASSERT_EQ(tree.size(), 3u);

    size_t removed = tree.erase("alpha");
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(tree.size(), 1u);
    EXPECT_TRUE(tree.queryKey("alpha").empty());
}

// ITX-ERASE-02: erase() on a non-existent key returns 0 and leaves tree intact
TEST_F(IntervalTreeIndexTest, Erase_NonExistentKey_ReturnsZero) {
    tree.insert(makeEntry("x", 10, 20));
    size_t removed = tree.erase("does_not_exist");
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(tree.size(), 1u);
}

// ITX-ERASE-03: erase() is equivalent to removeKey()
TEST_F(IntervalTreeIndexTest, Erase_EquivalentToRemoveKey) {
    IntervalTreeIndex t1{"t1"}, t2{"t2"};
    t1.insert(makeEntry("k", 0, 100));
    t1.insert(makeEntry("k", 200, 300));
    t2.insert(makeEntry("k", 0, 100));
    t2.insert(makeEntry("k", 200, 300));

    size_t by_remove = t1.removeKey("k");
    size_t by_erase  = t2.erase("k");

    EXPECT_EQ(by_remove, by_erase);
    EXPECT_EQ(t1.size(), t2.size());
}

// ITX-ERASE-04: erase() on an empty tree does not crash
TEST_F(IntervalTreeIndexTest, Erase_EmptyTree_NoCrash) {
    EXPECT_NO_THROW(tree.erase("any_key"));
    EXPECT_EQ(tree.size(), 0u);
}
