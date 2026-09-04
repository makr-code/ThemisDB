/**
 * Tests for IntervalTreeIndex
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
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

// ── AVL balancing ─────────────────────────────────────────────────────────────

// ITI-AVL-01: Inserting N entries in ascending order must not degenerate into
// a linear chain.  For a balanced BST height <= ceil(1.44 * log2(N+2)).
TEST_F(IntervalTreeIndexTest, AVL_AscendingInsert_HeightIsLogarithmic) {
    constexpr int N = 1024;
    for (int i = 0; i < N; ++i) {
        tree.insert(makeEntry("k", static_cast<Timestamp>(i * 10),
                              static_cast<Timestamp>(i * 10 + 5)));
    }
    EXPECT_EQ(tree.size(), static_cast<size_t>(N));
    const auto s = tree.stats();
    // AVL guarantee: height <= 1.44 * log2(N+2) ≈ 15 for N=1024
    EXPECT_LE(s.height, static_cast<size_t>(22)); // generous upper bound
}

// ITI-AVL-02: Inserting N entries in descending order must also stay balanced.
TEST_F(IntervalTreeIndexTest, AVL_DescendingInsert_HeightIsLogarithmic) {
    constexpr int N = 1024;
    for (int i = N - 1; i >= 0; --i) {
        tree.insert(makeEntry("k", static_cast<Timestamp>(i * 10),
                              static_cast<Timestamp>(i * 10 + 5)));
    }
    EXPECT_EQ(tree.size(), static_cast<size_t>(N));
    const auto s = tree.stats();
    EXPECT_LE(s.height, static_cast<size_t>(22));
}

// ITI-AVL-03: After removing half the entries the tree must remain balanced.
TEST_F(IntervalTreeIndexTest, AVL_AfterRemove_HeightStaysLogarithmic) {
    constexpr int N = 512;
    for (int i = 0; i < N; ++i) {
        tree.insert(makeEntry("k" + std::to_string(i),
                              static_cast<Timestamp>(i * 10),
                              static_cast<Timestamp>(i * 10 + 5)));
    }
    // Remove every second key
    for (int i = 0; i < N; i += 2) {
        tree.remove("k" + std::to_string(i),
                    {static_cast<Timestamp>(i * 10),
                     static_cast<Timestamp>(i * 10 + 5)});
    }
    EXPECT_EQ(tree.size(), static_cast<size_t>(N / 2));
    const auto s = tree.stats();
    EXPECT_LE(s.height, static_cast<size_t>(22));
}

// ── Secondary key index ───────────────────────────────────────────────────────

// ITI-KEY-01: queryKey returns all versions for a key across a large tree
// without iterating through unrelated keys.
TEST_F(IntervalTreeIndexTest, KeyIndex_QueryKey_ReturnsCorrectCount) {
    constexpr int NUM_KEYS     = 100;
    constexpr int VERSIONS     = 50;
    for (int k = 0; k < NUM_KEYS; ++k) {
        for (int v = 0; v < VERSIONS; ++v) {
            tree.insert(makeEntry("key" + std::to_string(k),
                                  static_cast<Timestamp>((k * VERSIONS + v) * 10),
                                  static_cast<Timestamp>((k * VERSIONS + v) * 10 + 9)));
        }
    }
    // Each key should have exactly VERSIONS entries
    for (int k = 0; k < NUM_KEYS; ++k) {
        auto results = tree.queryKey("key" + std::to_string(k));
        EXPECT_EQ(results.size(), static_cast<size_t>(VERSIONS))
            << "key" << k << " has wrong count";
    }
    // Unknown key returns empty
    EXPECT_TRUE(tree.queryKey("nonexistent").empty());
}

// ITI-KEY-02: queryKey with range filter uses the secondary index.
TEST_F(IntervalTreeIndexTest, KeyIndex_QueryKey_WithRange_Filtered) {
    tree.insert(makeEntry("x", 100, 200));
    tree.insert(makeEntry("x", 300, 400));
    tree.insert(makeEntry("x", 500, 600));

    // Only [100,200) and [300,400) should overlap [50,350)
    auto r = tree.queryKey("x", TimeRange{50, 350});
    EXPECT_EQ(r.size(), 2u);
}

// ITI-KEY-03: After remove() the secondary index no longer contains the entry.
TEST_F(IntervalTreeIndexTest, KeyIndex_AfterRemove_EntryGone) {
    tree.insert(makeEntry("r", 100, 200));
    tree.insert(makeEntry("r", 200, 300));
    tree.remove("r", {100, 200});

    auto results = tree.queryKey("r");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].range.start, 200);
}

// ITI-KEY-04: After removeKey() the secondary index is fully cleared.
TEST_F(IntervalTreeIndexTest, KeyIndex_AfterRemoveKey_Empty) {
    tree.insert(makeEntry("del", 100, 200));
    tree.insert(makeEntry("del", 200, 300));
    tree.insert(makeEntry("other", 50, 150));

    EXPECT_EQ(tree.removeKey("del"), 2u);
    EXPECT_TRUE(tree.queryKey("del").empty());
    EXPECT_EQ(tree.queryKey("other").size(), 1u);
}

// ITI-KEY-05: After clear() the secondary index is empty.
TEST_F(IntervalTreeIndexTest, KeyIndex_AfterClear_Empty) {
    tree.insert(makeEntry("a", 100, 200));
    tree.insert(makeEntry("b", 200, 300));
    tree.clear();

    EXPECT_TRUE(tree.queryKey("a").empty());
    EXPECT_TRUE(tree.queryKey("b").empty());
    EXPECT_EQ(tree.size(), 0u);
}

// ── Concurrent reads ──────────────────────────────────────────────────────────

// ITI-CONC-01: Multiple threads can queryPoint simultaneously without
// deadlocking or corrupting results (shared_mutex allows concurrent readers).
TEST_F(IntervalTreeIndexTest, ConcurrentReads_SharedMutex_NoDeadlock) {
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        tree.insert(makeEntry("k" + std::to_string(i),
                              static_cast<Timestamp>(i * 10),
                              static_cast<Timestamp>(i * 10 + 9)));
    }

    constexpr int NUM_THREADS = 8;
    std::vector<std::thread> threads;
    std::atomic<int> total_hits{0};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int q = 0; q < 50; ++q) {
                // Query a point inside interval (t*25 + q), always valid
                Timestamp pt = static_cast<Timestamp>(((t * 25 + q) % N) * 10 + 4);
                auto res = tree.queryPoint(pt);
                total_hits.fetch_add(static_cast<int>(res.size()),
                                     std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Each query targets exactly one interval → 8*50 = 400 hits expected
    EXPECT_EQ(total_hits.load(), NUM_THREADS * 50);
}


// ── erase() STL-alias tests (ITX-ERASE-01..04) ───────────────────────────────

TEST(IntervalTreeEraseTest, ITX_ERASE_01_EraseExistingKey_ReturnsCount) {
    IntervalTreeIndex tree{"test"};
    tree.insert({"k1", {0, 10}});
    tree.insert({"k1", {20, 30}});
    tree.insert({"k2", {5, 15}});

    size_t removed = tree.erase("k1");
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(tree.size(), 1u);
}

TEST(IntervalTreeEraseTest, ITX_ERASE_02_EraseAbsentKey_ReturnsZero) {
    IntervalTreeIndex tree{"test"};
    tree.insert({"k1", {0, 10}});

    size_t removed = tree.erase("no_such_key");
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(tree.size(), 1u);
}

TEST(IntervalTreeEraseTest, ITX_ERASE_03_TreeRemainsQueryable_AfterErase) {
    IntervalTreeIndex tree{"test"};
    tree.insert({"k1", {0, 20}});
    tree.insert({"k2", {5, 25}});

    tree.erase("k1");

    auto hits = tree.queryPoint(10);
    EXPECT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0].key, "k2");
}

TEST(IntervalTreeEraseTest, ITX_ERASE_04_EraseAll_TreeEmpty) {
    IntervalTreeIndex tree{"test"};
    tree.insert({"a", {0, 5}});
    tree.insert({"b", {10, 15}});

    tree.erase("a");
    tree.erase("b");

    EXPECT_EQ(tree.size(), 0u);
    EXPECT_TRUE(tree.queryPoint(2).empty());
}
