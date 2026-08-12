#include <gtest/gtest.h>
#include "geo/geo_rtree.h"
#include "utils/geo/ewkb.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace themis::geo;

class GeoRTreeTest : public ::testing::Test {
protected:
    GeoRTree tree_;

    // Helper: create a Point geometry
    static GeometryInfo makePoint(double x, double y) {
        GeometryInfo g(GeometryType::Point);
        g.coords.emplace_back(x, y);
        return g;
    }

    // Helper: create a Polygon geometry from a bounding box
    static GeometryInfo makeBox(double minx, double miny, double maxx, double maxy) {
        GeometryInfo g(GeometryType::Polygon);
        std::vector<Coordinate> ring = {
            {minx, miny}, {maxx, miny}, {maxx, maxy}, {minx, maxy}, {minx, miny}
        };
        g.rings.push_back(ring);
        return g;
    }

    static bool contains_key(const std::vector<std::string>& v, const std::string& key) {
        return std::find(v.begin(), v.end(), key) != v.end();
    }
};

// ── Construction ──────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, DefaultConstructor_EmptyIndex) {
    EXPECT_EQ(tree_.size(), 0u);
    // memoryBytes() includes Impl struct overhead even when empty;
    // require it is non-zero (struct overhead) but below an unreasonable ceiling.
    EXPECT_GT(tree_.memoryBytes(), 0u);
    EXPECT_LT(tree_.memoryBytes(), 1024u);
}

// ── Insert ────────────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, Insert_SinglePoint) {
    tree_.insert("p1", makePoint(13.4, 52.5));
    EXPECT_EQ(tree_.size(), 1u);
}

TEST_F(GeoRTreeTest, Insert_MultipleGeometries) {
    tree_.insert("p1", makePoint(13.4, 52.5));
    tree_.insert("p2", makePoint(2.35, 48.85));
    tree_.insert("b1", makeBox(10.0, 50.0, 15.0, 55.0));
    EXPECT_EQ(tree_.size(), 3u);
}

// ── BulkLoad ──────────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, BulkLoad_Empty) {
    tree_.bulkLoad({});
    EXPECT_EQ(tree_.size(), 0u);
}

TEST_F(GeoRTreeTest, BulkLoad_ReplacesPreviousContent) {
    tree_.insert("old", makePoint(0.0, 0.0));
    EXPECT_EQ(tree_.size(), 1u);

    tree_.bulkLoad({{"new1", makePoint(1.0, 1.0)}, {"new2", makePoint(2.0, 2.0)}});
    EXPECT_EQ(tree_.size(), 2u);

    // The old entry must no longer be reachable
    auto res = tree_.intersects(MBR(-0.5, -0.5, 0.5, 0.5));
    EXPECT_TRUE(res.empty());
}

TEST_F(GeoRTreeTest, BulkLoad_ManyPoints) {
    std::vector<std::pair<std::string, GeometryInfo>> entries;
    for (int i = 0; i < 200; ++i) {
        entries.emplace_back("pt_" + std::to_string(i),
                             makePoint(static_cast<double>(i) * 0.5,
                                       static_cast<double>(i) * 0.25));
    }
    tree_.bulkLoad(entries);
    EXPECT_EQ(tree_.size(), 200u);
}

// ── Remove ────────────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, Remove_ExistingEntry) {
    auto geom = makePoint(13.4, 52.5);
    tree_.insert("berlin", geom);
    EXPECT_EQ(tree_.size(), 1u);

    bool removed = tree_.remove("berlin", geom);
    EXPECT_TRUE(removed);
    EXPECT_EQ(tree_.size(), 0u);
}

TEST_F(GeoRTreeTest, Remove_NonExistentKeyReturnsFalse) {
    auto geom = makePoint(1.0, 1.0);
    bool removed = tree_.remove("ghost", geom);
    EXPECT_FALSE(removed);
}

TEST_F(GeoRTreeTest, Remove_LeavesOtherEntriesIntact) {
    auto g1 = makePoint(1.0, 1.0);
    auto g2 = makePoint(5.0, 5.0);
    tree_.insert("a", g1);
    tree_.insert("b", g2);

    tree_.remove("a", g1);
    EXPECT_EQ(tree_.size(), 1u);

    auto res = tree_.intersects(MBR(4.0, 4.0, 6.0, 6.0));
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], "b");
}

// ── Clear ─────────────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, Clear_ResetsIndex) {
    tree_.insert("a", makePoint(1.0, 1.0));
    tree_.insert("b", makePoint(2.0, 2.0));
    tree_.clear();
    EXPECT_EQ(tree_.size(), 0u);
    EXPECT_TRUE(tree_.intersects(MBR(-180, -90, 180, 90)).empty());
}

// ── Intersects ────────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, Intersects_PointInsideBBox) {
    tree_.insert("berlin", makePoint(13.4, 52.5));
    tree_.insert("paris",  makePoint(2.35, 48.85));

    auto res = tree_.intersects(MBR(12.0, 51.0, 15.0, 54.0));  // covers Berlin
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], "berlin");
}

TEST_F(GeoRTreeTest, Intersects_NothingInBBox) {
    tree_.insert("berlin", makePoint(13.4, 52.5));
    auto res = tree_.intersects(MBR(0.0, 0.0, 1.0, 1.0));
    EXPECT_TRUE(res.empty());
}

TEST_F(GeoRTreeTest, Intersects_MultipleHits) {
    tree_.insert("a", makePoint(1.0, 1.0));
    tree_.insert("b", makePoint(2.0, 2.0));
    tree_.insert("c", makePoint(50.0, 50.0));

    auto res = tree_.intersects(MBR(0.0, 0.0, 3.0, 3.0));
    ASSERT_EQ(res.size(), 2u);
    EXPECT_TRUE(contains_key(res, "a"));
    EXPECT_TRUE(contains_key(res, "b"));
    EXPECT_FALSE(contains_key(res, "c"));
}

TEST_F(GeoRTreeTest, Intersects_OverlappingPolygons) {
    // Box A: [0,0]-[4,4]
    tree_.insert("boxA", makeBox(0.0, 0.0, 4.0, 4.0));
    // Box B: [3,3]-[7,7]  overlaps A
    tree_.insert("boxB", makeBox(3.0, 3.0, 7.0, 7.0));
    // Box C: [10,10]-[15,15]  does not overlap query
    tree_.insert("boxC", makeBox(10.0, 10.0, 15.0, 15.0));

    // Query MBR intersects A and B
    auto res = tree_.intersects(MBR(2.0, 2.0, 5.0, 5.0));
    EXPECT_TRUE(contains_key(res, "boxA"));
    EXPECT_TRUE(contains_key(res, "boxB"));
    EXPECT_FALSE(contains_key(res, "boxC"));
}

TEST_F(GeoRTreeTest, Intersects_WorldQuery_ReturnsAll) {
    tree_.insert("p1", makePoint(13.4, 52.5));
    tree_.insert("p2", makePoint(-74.0, 40.7));
    tree_.insert("p3", makePoint(139.7, 35.7));

    auto res = tree_.intersects(MBR(-180.0, -90.0, 180.0, 90.0));
    EXPECT_EQ(res.size(), 3u);
}

// ── Contains ──────────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, Contains_PointInsidePolygon) {
    // Box covering Germany roughly: [6,47]-[15,55]
    tree_.insert("germany", makeBox(6.0, 47.0, 15.0, 55.0));

    // Berlin (13.4, 52.5) is inside
    auto res = tree_.contains(13.4, 52.5);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], "germany");
}

TEST_F(GeoRTreeTest, Contains_PointOutsideAllGeometries) {
    tree_.insert("germany", makeBox(6.0, 47.0, 15.0, 55.0));

    // Tokyo (139.7, 35.7) is outside Germany's MBR
    auto res = tree_.contains(139.7, 35.7);
    EXPECT_TRUE(res.empty());
}

TEST_F(GeoRTreeTest, Contains_MultipleContainingBoxes) {
    tree_.insert("large",  makeBox(0.0, 0.0, 10.0, 10.0));
    tree_.insert("small",  makeBox(3.0, 3.0, 7.0, 7.0));
    tree_.insert("other",  makeBox(20.0, 20.0, 30.0, 30.0));

    // Point (5, 5) is inside both "large" and "small"
    auto res = tree_.contains(5.0, 5.0);
    EXPECT_EQ(res.size(), 2u);
    EXPECT_TRUE(contains_key(res, "large"));
    EXPECT_TRUE(contains_key(res, "small"));
    EXPECT_FALSE(contains_key(res, "other"));
}

TEST_F(GeoRTreeTest, Contains_PointOnMBRBoundary) {
    tree_.insert("box", makeBox(0.0, 0.0, 5.0, 5.0));

    // Corners and edges should be inside (MBR::contains is inclusive)
    EXPECT_FALSE(tree_.contains(0.0, 0.0).empty());
    EXPECT_FALSE(tree_.contains(5.0, 5.0).empty());
    EXPECT_FALSE(tree_.contains(2.5, 0.0).empty());  // bottom edge
}

// ── Memory reporting ──────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, MemoryBytes_GrowsWithInserts) {
    std::size_t before = tree_.memoryBytes();
    tree_.insert("p1", makePoint(1.0, 1.0));
    tree_.insert("p2", makePoint(2.0, 2.0));
    std::size_t after = tree_.memoryBytes();
    EXPECT_GE(after, before);  // must not shrink
    EXPECT_GT(after, 0u);
}

TEST_F(GeoRTreeTest, MemoryBytes_BulkLoad) {
    std::vector<std::pair<std::string, GeometryInfo>> entries;
    for (int i = 0; i < 100; ++i) {
        entries.emplace_back("k" + std::to_string(i), makePoint(i * 0.1, i * 0.1));
    }
    tree_.bulkLoad(entries);
    EXPECT_GT(tree_.memoryBytes(), 0u);
}

// ── Move semantics ────────────────────────────────────────────────────────

TEST_F(GeoRTreeTest, MoveConstructor) {
    tree_.insert("p1", makePoint(1.0, 1.0));
    GeoRTree moved(std::move(tree_));
    EXPECT_EQ(moved.size(), 1u);
    auto res = moved.intersects(MBR(0.5, 0.5, 1.5, 1.5));
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], "p1");
}

TEST_F(GeoRTreeTest, MoveAssignment) {
    tree_.insert("p1", makePoint(1.0, 1.0));
    GeoRTree other;
    other = std::move(tree_);
    EXPECT_EQ(other.size(), 1u);
}
