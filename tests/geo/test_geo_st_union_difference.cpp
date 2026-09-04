/*
 * Unit tests for ST_UNION and ST_DIFFERENCE geometry operations.
 * Covers the cpu_exact and gpu_spatial (CPU-fallback) backends.
 * Test structure mirrors tests/geo/test_geo_st_buffer.cpp.
 */

#include <gtest/gtest.h>
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GeometryInfo makePoint(double x, double y) {
    GeometryInfo g(GeometryType::Point);
    g.coords.push_back({x, y});
    return g;
}

static GeometryInfo makeClosedPolygon(
        std::initializer_list<std::pair<double, double>> pts) {
    GeometryInfo g(GeometryType::Polygon);
    std::vector<Coordinate> ring;
    for (const auto& p : pts) ring.push_back({p.first, p.second});
    ring.push_back(ring[0]); // close the ring
    g.rings.push_back(ring);
    return g;
}

/// Return the unsigned area of the first ring of a polygon using the Shoelace
/// formula (absolute value of the signed result).
static double ringArea(const GeometryInfo& g) {
    const auto& rings = g.rings;
    if (rings.empty()) {
      return 0.0;
    }
    const auto& ring = rings[0];
    double area = 0.0;
    const std::size_t n = ring.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        area += ring[j].x * ring[i].y - ring[i].x * ring[j].y;
    }
    return std::abs(area) * 0.5;
}

// ---------------------------------------------------------------------------
// Parameterised fixture – run every test against both backends
// ---------------------------------------------------------------------------

class StUnionDiffTest : public ::testing::TestWithParam<ISpatialComputeBackend*> {
protected:
    ISpatialComputeBackend* backend() { return GetParam(); }
};

// ===========================================================================
// ST_UNION tests
// ===========================================================================

TEST_P(StUnionDiffTest, Union_NonOverlapping_ReturnsCollection) {
    // Two squares separated by a gap — union should be a GeometryCollection
    // (or MultiPolygon) containing both.
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}});
    const GeometryInfo b = makeClosedPolygon(
        {{2.0, 0.0}, {3.0, 0.0}, {3.0, 1.0}, {2.0, 1.0}});

    const GeometryInfo result = backend()->stUnion(a, b);

    // The result must be a GeometryCollection (two disjoint polygons).
    EXPECT_EQ(result.type, GeometryType::GeometryCollection)
        << "Union of disjoint polygons should be a GeometryCollection";
    EXPECT_EQ(result.geometries.size(), 2u)
        << "GeometryCollection should contain exactly two sub-geometries";
}

TEST_P(StUnionDiffTest, Union_ContainedPolygon_ReturnsOuter) {
    // B is fully inside A — union should be A.
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}});
    const GeometryInfo b = makeClosedPolygon(
        {{1.0, 1.0}, {3.0, 1.0}, {3.0, 3.0}, {1.0, 3.0}});

    const GeometryInfo result = backend()->stUnion(a, b);

    ASSERT_TRUE(result.isPolygon())
        << "Union when one polygon contains the other should be a Polygon";
    const double area_a  = ringArea(a);
    const double area_res = ringArea(result);
    EXPECT_NEAR(area_res, area_a, area_a * 0.01)
        << "Union area should equal the outer polygon area";
}

TEST_P(StUnionDiffTest, Union_Overlapping_ProducesLargerPolygon) {
    // Two overlapping unit squares: A at (0,0)-(2,2), B at (1,-1)-(3,1).
    // Their union is an L-shaped octagon larger than either polygon.
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}});
    const GeometryInfo b = makeClosedPolygon(
        {{1.0, -1.0}, {3.0, -1.0}, {3.0, 1.0}, {1.0, 1.0}});

    const GeometryInfo result = backend()->stUnion(a, b);

    ASSERT_TRUE(result.isPolygon())
        << "Union of overlapping polygons should be a Polygon";
    ASSERT_FALSE(result.rings.empty());
    const double area_a   = ringArea(a); // 4.0
    const double area_b   = ringArea(b); // 4.0
    const double area_res = ringArea(result);
    EXPECT_GT(area_res, area_a)
        << "Union area must exceed polygon A area";
    EXPECT_GT(area_res, area_b)
        << "Union area must exceed polygon B area";
    // Max possible area is sum of both (no overlap counted twice).
    EXPECT_LT(area_res, area_a + area_b + 1e-6)
        << "Union area must not exceed the sum of both areas";
}

TEST_P(StUnionDiffTest, Union_IdenticalPolygons_ReturnsSameArea) {
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}});

    const GeometryInfo result = backend()->stUnion(a, a);

    // The union of a polygon with itself is itself.
    const double area_a   = ringArea(a);
    const double area_res = ringArea(result);
    EXPECT_NEAR(area_res, area_a, area_a * 0.05)
        << "Union of polygon with itself should have the same area";
}

TEST_P(StUnionDiffTest, Union_Points_EqualReturnsPoint) {
    const GeometryInfo p1 = makePoint(1.0, 2.0);
    const GeometryInfo p2 = makePoint(1.0, 2.0);
    const GeometryInfo result = backend()->stUnion(p1, p2);
    ASSERT_TRUE(result.isPoint())
        << "Union of two identical points should be a Point";
    ASSERT_FALSE(result.coords.empty());
    EXPECT_NEAR(result.coords[0].x, 1.0, 1e-9);
    EXPECT_NEAR(result.coords[0].y, 2.0, 1e-9);
}

TEST_P(StUnionDiffTest, Union_Points_DistinctReturnsCollection) {
    const GeometryInfo p1 = makePoint(0.0, 0.0);
    const GeometryInfo p2 = makePoint(1.0, 1.0);
    const GeometryInfo result = backend()->stUnion(p1, p2);
    EXPECT_EQ(result.type, GeometryType::GeometryCollection)
        << "Union of two distinct points should be a GeometryCollection";
    EXPECT_EQ(result.geometries.size(), 2u);
}

TEST_P(StUnionDiffTest, Union_PointInsidePolygon_ReturnsPolygon) {
    const GeometryInfo poly = makeClosedPolygon(
        {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}});
    const GeometryInfo pt = makePoint(2.0, 2.0); // inside
    const GeometryInfo result = backend()->stUnion(pt, poly);
    EXPECT_TRUE(result.isPolygon())
        << "Union of a point inside a polygon should be the polygon";
}

// ===========================================================================
// ST_DIFFERENCE tests
// ===========================================================================

TEST_P(StUnionDiffTest, Diff_NonOverlapping_ReturnsA) {
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}});
    const GeometryInfo b = makeClosedPolygon(
        {{2.0, 0.0}, {3.0, 0.0}, {3.0, 1.0}, {2.0, 1.0}});

    const GeometryInfo result = backend()->stDifference(a, b);

    ASSERT_TRUE(result.isPolygon())
        << "Difference of disjoint polygons should return A";
    const double area_a   = ringArea(a);
    const double area_res = ringArea(result);
    EXPECT_NEAR(area_res, area_a, area_a * 0.01)
        << "Result area should equal A when A and B are disjoint";
}

TEST_P(StUnionDiffTest, Diff_AContainedInB_ReturnsEmpty) {
    // A is fully inside B → A \ B = empty.
    const GeometryInfo b = makeClosedPolygon(
        {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}});
    const GeometryInfo a = makeClosedPolygon(
        {{1.0, 1.0}, {3.0, 1.0}, {3.0, 3.0}, {1.0, 3.0}});

    const GeometryInfo result = backend()->stDifference(a, b);

    EXPECT_FALSE(result.isPolygon())
        << "Difference when A is inside B should be empty";
    EXPECT_TRUE(result.rings.empty())
        << "Empty difference should have no rings";
}

TEST_P(StUnionDiffTest, Diff_BContainedInA_ProducesHole) {
    // B is fully inside A → A \ B is A with a hole.
    // The result polygon has 2 rings: outer ring (A) and an inner ring (B as hole).
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}});
    const GeometryInfo b = makeClosedPolygon(
        {{1.0, 1.0}, {3.0, 1.0}, {3.0, 3.0}, {1.0, 3.0}});

    const GeometryInfo result = backend()->stDifference(a, b);

    ASSERT_TRUE(result.isPolygon())
        << "Difference when B is inside A should be a Polygon";
    ASSERT_GE(result.rings.size(), 1u)
        << "Result must have at least one ring";
    // The outer ring should match A's area.
    const double area_a   = ringArea(a); // 16.0
    const double area_outer = ringArea(result);
    EXPECT_NEAR(area_outer, area_a, area_a * 0.01)
        << "Outer ring area should equal A";
    // When the result has a hole ring, the polygon has 2 rings.
    if (result.rings.size() >= 2) {
        const auto& hole = result.rings[1];
        double hole_area = 0.0;
        for (std::size_t i = 0, j = hole.size() - 1; i < hole.size(); j = i++) {
            hole_area += hole[j].x * hole[i].y - hole[i].x * hole[j].y;
        }
        hole_area = std::abs(hole_area) * 0.5;
        const double area_b = ringArea(b); // 4.0
        EXPECT_NEAR(hole_area, area_b, area_b * 0.01)
            << "Hole ring area should equal B";
    }
}

TEST_P(StUnionDiffTest, Diff_Overlapping_AreaDecreases) {
    // A = square (0,0)-(2,2), B = square (1,-1)-(3,1).
    // A \ B removes the overlap region; result area < A's area.
    const GeometryInfo a = makeClosedPolygon(
        {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}});
    const GeometryInfo b = makeClosedPolygon(
        {{1.0, -1.0}, {3.0, -1.0}, {3.0, 1.0}, {1.0, 1.0}});

    const GeometryInfo result = backend()->stDifference(a, b);

    ASSERT_TRUE(result.isPolygon())
        << "Difference of overlapping polygons should be a Polygon";
    ASSERT_FALSE(result.rings.empty());
    const double area_a   = ringArea(a); // 4.0
    const double area_res = ringArea(result);
    EXPECT_LT(area_res, area_a)
        << "Difference area must be less than A area when B overlaps A";
    EXPECT_GT(area_res, 0.0)
        << "Difference area must be positive when B does not contain A";
}

TEST_P(StUnionDiffTest, Diff_Points_EqualReturnsEmpty) {
    const GeometryInfo p1 = makePoint(1.0, 2.0);
    const GeometryInfo p2 = makePoint(1.0, 2.0);
    const GeometryInfo result = backend()->stDifference(p1, p2);
    EXPECT_FALSE(result.isPoint())
        << "Difference of equal points should be empty";
}

TEST_P(StUnionDiffTest, Diff_Points_DistinctReturnsFirst) {
    const GeometryInfo p1 = makePoint(0.0, 0.0);
    const GeometryInfo p2 = makePoint(1.0, 1.0);
    const GeometryInfo result = backend()->stDifference(p1, p2);
    ASSERT_TRUE(result.isPoint())
        << "Difference of distinct points should return the first point";
    EXPECT_NEAR(result.coords[0].x, 0.0, 1e-9);
    EXPECT_NEAR(result.coords[0].y, 0.0, 1e-9);
}

TEST_P(StUnionDiffTest, Diff_PointInsidePolygon_ReturnsEmpty) {
    const GeometryInfo poly = makeClosedPolygon(
        {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}});
    const GeometryInfo pt = makePoint(2.0, 2.0); // inside
    const GeometryInfo result = backend()->stDifference(pt, poly);
    EXPECT_FALSE(result.isPoint())
        << "Point inside polygon: difference should be empty";
}

TEST_P(StUnionDiffTest, Diff_PointOutsidePolygon_ReturnsPoint) {
    const GeometryInfo poly = makeClosedPolygon(
        {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}});
    const GeometryInfo pt = makePoint(5.0, 5.0); // outside
    const GeometryInfo result = backend()->stDifference(pt, poly);
    ASSERT_TRUE(result.isPoint())
        << "Point outside polygon: difference should be the point";
    EXPECT_NEAR(result.coords[0].x, 5.0, 1e-9);
    EXPECT_NEAR(result.coords[0].y, 5.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    CpuExact, StUnionDiffTest,
    ::testing::Values(getCpuExactBackend()),
    [](const testing::TestParamInfo<ISpatialComputeBackend*>&) {
        return "cpu_exact";
    });

INSTANTIATE_TEST_SUITE_P(
    GpuSpatial, StUnionDiffTest,
    ::testing::Values(getGpuSpatialBackend()),
    [](const testing::TestParamInfo<ISpatialComputeBackend*>&) {
        return "gpu_spatial";
    });
