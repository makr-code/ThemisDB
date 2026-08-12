/**
 * Tests for TemporalIndex
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_index.h"

using namespace themisdb::temporal;

class TemporalIndexTest : public ::testing::Test {
protected:
    TemporalIndex idx{"test_index"};

    TemporalIndexEntry makeEntry(const std::string& key, Timestamp start,
                                  Timestamp end,
                                  nlohmann::json payload = {}) {
        return {key, {start, end}, std::move(payload)};
    }
};

// ── Insert / Size ─────────────────────────────────────────────────────────────

TEST_F(TemporalIndexTest, Insert_Single_SizeBecomesOne) {
    idx.insert(makeEntry("k1", 100, 200));
    EXPECT_EQ(idx.size(), 1u);
}

TEST_F(TemporalIndexTest, Insert_Multiple_SizeIncreases) {
    idx.insert(makeEntry("k1", 100, 200));
    idx.insert(makeEntry("k2", 150, 300));
    idx.insert(makeEntry("k3", 250, 400));
    EXPECT_EQ(idx.size(), 3u);
}

// ── queryPoint ───────────────────────────────────────────────────────────────

TEST_F(TemporalIndexTest, QueryPoint_MatchingEntries) {
    idx.insert(makeEntry("k1", 100, 300));
    idx.insert(makeEntry("k2", 150, 250));
    idx.insert(makeEntry("k3", 400, 500));

    // t=200 is contained by k1 and k2
    auto results = idx.queryPoint(200);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(TemporalIndexTest, QueryPoint_NoMatch_ReturnsEmpty) {
    idx.insert(makeEntry("k1", 100, 200));
    EXPECT_TRUE(idx.queryPoint(50).empty());
    EXPECT_TRUE(idx.queryPoint(200).empty()); // half-open: [100,200)
}

TEST_F(TemporalIndexTest, QueryPoint_AtStart_ReturnsEntry) {
    idx.insert(makeEntry("k1", 100, 200));
    auto results = idx.queryPoint(100);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].key, "k1");
}

// ── queryRange ───────────────────────────────────────────────────────────────

TEST_F(TemporalIndexTest, QueryRange_OverlappingEntries) {
    idx.insert(makeEntry("k1", 100, 300));
    idx.insert(makeEntry("k2", 200, 400));
    idx.insert(makeEntry("k3", 500, 600));

    auto results = idx.queryRange(150, 350);
    EXPECT_EQ(results.size(), 2u); // k1 and k2 overlap [150,350)
}

TEST_F(TemporalIndexTest, QueryRange_NoOverlap_ReturnsEmpty) {
    idx.insert(makeEntry("k1", 100, 200));
    EXPECT_TRUE(idx.queryRange(200, 300).empty());
}

// ── queryKey ─────────────────────────────────────────────────────────────────

TEST_F(TemporalIndexTest, QueryKey_ReturnsAllVersionsForKey) {
    idx.insert(makeEntry("k1", 100, 200));
    idx.insert(makeEntry("k1", 200, 300));
    idx.insert(makeEntry("k2", 100, 300));

    auto results = idx.queryKey("k1");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(TemporalIndexTest, QueryKey_WithRange_FiltersVersions) {
    idx.insert(makeEntry("k1", 100, 200));
    idx.insert(makeEntry("k1", 300, 400));

    auto results = idx.queryKey("k1", TimeRange{50, 250});
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].range.start, 100);
}

// ── remove ───────────────────────────────────────────────────────────────────

TEST_F(TemporalIndexTest, Remove_ExistingEntry_SizeDecreases) {
    idx.insert(makeEntry("k1", 100, 200));
    EXPECT_EQ(idx.remove("k1", {100, 200}), 1u);
    EXPECT_EQ(idx.size(), 0u);
}

TEST_F(TemporalIndexTest, RemoveKey_RemovesAllVersions) {
    idx.insert(makeEntry("k1", 100, 200));
    idx.insert(makeEntry("k1", 200, 300));
    idx.insert(makeEntry("k2", 100, 300));

    EXPECT_EQ(idx.removeKey("k1"), 2u);
    EXPECT_EQ(idx.size(), 1u);
}

// ── stats ─────────────────────────────────────────────────────────────────────

TEST_F(TemporalIndexTest, Stats_TotalEntries) {
    idx.insert(makeEntry("k1", 100, 200));
    idx.insert(makeEntry("k2", 150, 300));

    auto s = idx.stats();
    EXPECT_EQ(s.total_entries, 2u);
    EXPECT_EQ(s.min_timestamp, 100);
    // After point query the counter increments
    idx.queryPoint(150);
    auto s2 = idx.stats();
    EXPECT_EQ(s2.point_queries, 1u);
}
