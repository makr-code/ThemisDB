/*
 * Test suite: R-tree Cursor API (rtree_cursor.h)
 *
 * Tests: RTC-01 … RTC-10
 *
 * RTC-01  GeoRTreeIndex: empty index has size 0
 * RTC-02  GeoRTreeIndex: insert() increases size, range cursor finds entry
 * RTC-03  GeoRTreeIndex: bulkLoad() populates index, range cursor covers all
 * RTC-04  RangeCursor: non-overlapping bbox yields END immediately
 * RTC-05  RangeCursor: estimatedResultCount() matches actual hits
 * RTC-06  RangeCursor: STALE after index mutation (insert)
 * RTC-07  KNNCursor: k-NN returns k results sorted ascending by distance
 * RTC-08  KNNCursor: k > index size returns all entries
 * RTC-09  KNNCursor: STALE after index mutation (clear)
 * RTC-10  GeoRTreeIndex: clear() resets size; subsequent range cursor yields END
 */

#include <gtest/gtest.h>
#include "geo/rtree_cursor.h"

#include <cmath>
#include <string>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Build a GeometryInfo for a Point geometry at (lon, lat).
GeometryInfo makePoint(double lon, double lat, const std::string& /*key*/ = "") {
    GeometryInfo g;
    g.type = GeometryType::Point;
    g.coords.emplace_back(lon, lat);
    return g;
}

/// Build a GeometryInfo for a simple box polygon centred at (lon, lat).
GeometryInfo makeBox(double lon, double lat, double half = 0.5) {
    GeometryInfo g;
    g.type = GeometryType::Polygon;
    // Exterior ring (5 coords, closed)
    g.rings.push_back({
        Coordinate{lon - half, lat - half},
        Coordinate{lon + half, lat - half},
        Coordinate{lon + half, lat + half},
        Coordinate{lon - half, lat + half},
        Coordinate{lon - half, lat - half},
    });
    return g;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// RTC-01 Empty index has size 0
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC01_EmptyIndex) {
    GeoRTreeIndex idx;
    EXPECT_EQ(idx.size(), 0u);
}

// ---------------------------------------------------------------------------
// RTC-02 insert() — range cursor finds inserted entry
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC02_Insert_RangeCursorFindsEntry) {
    GeoRTreeIndex idx;
    idx.insert("berlin", makePoint(13.4050, 52.5200));
    EXPECT_EQ(idx.size(), 1u);

    // Query bbox that contains Berlin
    MBR bbox{13.0, 52.0, 14.0, 53.0};
    auto cursor = idx.openRangeCursor(bbox);
    ASSERT_NE(cursor, nullptr);

    GeoIndexEntry e;
    EXPECT_EQ(cursor->next(e), CursorStatus::OK);
    EXPECT_EQ(e.key, "berlin");
    EXPECT_EQ(cursor->next(e), CursorStatus::END);
}

// ---------------------------------------------------------------------------
// RTC-03 bulkLoad() — range cursor covers all entries
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC03_BulkLoad_RangeCursorCoversAll) {
    GeoRTreeIndex idx;
    std::vector<std::pair<std::string, GeometryInfo>> entries = {
        {"p1", makePoint( 2.3522, 48.8566)},  // Paris
        {"p2", makePoint(13.4050, 52.5200)},  // Berlin
        {"p3", makePoint(-0.1276, 51.5074)},  // London
    };
    idx.bulkLoad(entries);
    EXPECT_EQ(idx.size(), 3u);

    // World bbox — must catch all
    MBR world{-180.0, -90.0, 180.0, 90.0};
    auto cursor = idx.openRangeCursor(world);
    int count = 0;
    GeoIndexEntry e;
    while (cursor->next(e) == CursorStatus::OK) {
      ++count;
    }
    EXPECT_EQ(count, 3);
}

// ---------------------------------------------------------------------------
// RTC-04 RangeCursor: non-overlapping bbox yields END immediately
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC04_RangeCursor_NoOverlap_End) {
    GeoRTreeIndex idx;
    idx.insert("berlin", makePoint(13.4050, 52.5200));

    // Bbox somewhere in the Pacific
    MBR pacific{-180.0, -10.0, -100.0, 10.0};
    auto cursor = idx.openRangeCursor(pacific);
    GeoIndexEntry e;
    EXPECT_EQ(cursor->next(e), CursorStatus::END);
}

// ---------------------------------------------------------------------------
// RTC-05 RangeCursor: estimatedResultCount() == actual hit count
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC05_RangeCursor_EstimatedResultCount) {
    GeoRTreeIndex idx;
    idx.insert("a", makePoint(1.0, 1.0));
    idx.insert("b", makePoint(2.0, 2.0));
    idx.insert("c", makePoint(10.0, 10.0));

    // Bbox that catches a and b but not c
    MBR bbox{0.0, 0.0, 3.0, 3.0};
    auto cursor = idx.openRangeCursor(bbox);
    const auto estimate = cursor->estimatedResultCount();

    // Drain the cursor and count actual hits
    GeoIndexEntry e;
    std::size_t actual = 0;
    while (cursor->next(e) == CursorStatus::OK) {
      ++actual;
    }

    EXPECT_EQ(estimate, actual);
    EXPECT_EQ(actual, 2u);
}

// ---------------------------------------------------------------------------
// RTC-06 RangeCursor: STALE after index mutation
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC06_RangeCursor_Stale_AfterInsert) {
    GeoRTreeIndex idx;
    idx.insert("a", makePoint(1.0, 1.0));

    MBR world{-180.0, -90.0, 180.0, 90.0};
    auto cursor = idx.openRangeCursor(world);

    // Drain one entry normally
    GeoIndexEntry e;
    EXPECT_EQ(cursor->next(e), CursorStatus::OK);

    // Mutate the index
    idx.insert("b", makePoint(2.0, 2.0));

    // Now cursor must be STALE
    EXPECT_EQ(cursor->next(e), CursorStatus::STALE);
}

// ---------------------------------------------------------------------------
// RTC-07 KNNCursor: returns k results sorted ascending by distance
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC07_KNNCursor_Sorted) {
    GeoRTreeIndex idx;
    // Insert 4 points at increasing distances from (0, 0)
    idx.insert("d1", makePoint(0.0,  1.0));  // closest
    idx.insert("d2", makePoint(0.0,  2.0));
    idx.insert("d3", makePoint(0.0,  3.0));
    idx.insert("d4", makePoint(0.0, 10.0));  // farthest

    Coordinate origin{0.0, 0.0};
    auto cursor = idx.openKNNCursor(origin, 3);
    ASSERT_NE(cursor, nullptr);
    EXPECT_EQ(cursor->estimatedResultCount(), 3u);

    GeoIndexEntry prev;
    GeoIndexEntry cur;
    ASSERT_EQ(cursor->next(prev), CursorStatus::OK);
    ASSERT_EQ(cursor->next(cur),  CursorStatus::OK);
    EXPECT_LE(prev.distance_m, cur.distance_m) << "Results must be sorted ascending";
    ASSERT_EQ(cursor->next(prev), CursorStatus::OK);
    EXPECT_LE(cur.distance_m, prev.distance_m);
    EXPECT_EQ(cursor->next(cur), CursorStatus::END);
}

// ---------------------------------------------------------------------------
// RTC-08 KNNCursor: k > index size returns all entries
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC08_KNNCursor_KLargerThanIndex) {
    GeoRTreeIndex idx;
    idx.insert("a", makePoint(1.0, 1.0));
    idx.insert("b", makePoint(2.0, 2.0));

    Coordinate origin{0.0, 0.0};
    auto cursor = idx.openKNNCursor(origin, 100);
    EXPECT_EQ(cursor->estimatedResultCount(), 2u);

    GeoIndexEntry e;
    int count = 0;
    while (cursor->next(e) == CursorStatus::OK) {
      ++count;
    }
    EXPECT_EQ(count, 2);
}

// ---------------------------------------------------------------------------
// RTC-09 KNNCursor: STALE after index mutation (clear)
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC09_KNNCursor_Stale_AfterClear) {
    GeoRTreeIndex idx;
    idx.insert("a", makePoint(1.0, 1.0));

    Coordinate origin{0.0, 0.0};
    auto cursor = idx.openKNNCursor(origin, 1);

    // Read first entry OK
    GeoIndexEntry e;
    EXPECT_EQ(cursor->next(e), CursorStatus::OK);

    // Mutate
    idx.clear();

    // Now STALE
    EXPECT_EQ(cursor->next(e), CursorStatus::STALE);
}

// ---------------------------------------------------------------------------
// RTC-10 clear() resets size; subsequent range cursor yields END
// ---------------------------------------------------------------------------

TEST(RTreeCursor, RTC10_Clear_ResetsIndex) {
    GeoRTreeIndex idx;
    idx.insert("a", makePoint(1.0, 1.0));
    EXPECT_EQ(idx.size(), 1u);
    idx.clear();
    EXPECT_EQ(idx.size(), 0u);

    MBR world{-180.0, -90.0, 180.0, 90.0};
    auto cursor = idx.openRangeCursor(world);
    GeoIndexEntry e;
    EXPECT_EQ(cursor->next(e), CursorStatus::END);
}
