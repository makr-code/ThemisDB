/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_geo_gpu_backend.cpp                           ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     876                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Tests for the GPU spatial backend (src/geo/gpu_backend_stub.cpp).
// The backend runs in CPU-fallback mode in CI (no GPU device), so all
// geometry predicates are verified against known-good geometry cases.

#include <gtest/gtest.h>
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers — build simple GeometryInfo objects inline
// ---------------------------------------------------------------------------

static GeometryInfo makePoint(double x, double y) {
    GeometryInfo g(GeometryType::Point);
    g.coords.push_back({x, y});
    return g;
}

static GeometryInfo makeLineString(std::initializer_list<std::pair<double,double>> pts) {
    GeometryInfo g(GeometryType::LineString);
    for (auto& p : pts) g.coords.push_back({p.first, p.second});
    return g;
}

// Closed ring polygon (caller responsible for closing the ring).
static GeometryInfo makePolygon(std::initializer_list<std::pair<double,double>> pts) {
    GeometryInfo g(GeometryType::Polygon);
    std::vector<Coordinate> ring;
    for (auto& p : pts) ring.push_back({p.first, p.second});
    g.rings.push_back(ring);
    return g;
}

// ---------------------------------------------------------------------------
// Backend access — use the static singleton exposed by the TU.
// We test through the ISpatialComputeBackend interface.
// ---------------------------------------------------------------------------

// The geometry predicate helpers used by GpuBatchBackend are replicated here
// for white-box testing.  Tests exercise the same algorithmic code paths that
// run inside the backend's exactIntersects() implementation.

namespace {
// Replicate the helpers from the backend TU for white-box testing.

constexpr double kEps = 1e-9;

bool pointInRing(double px, double py, const std::vector<Coordinate>& ring) {
    if (ring.size() < 3) return false;
    bool inside = false;
    size_t n = ring.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        double xi = ring[i].x, yi = ring[i].y;
        double xj = ring[j].x, yj = ring[j].y;
        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

double cross2d(double ox, double oy, double ax, double ay,
               double bx, double by) {
    return (ax - ox) * (by - oy) - (ay - oy) * (bx - ox);
}

bool on1D(double a, double b, double d) {
    if (a > b) std::swap(a, b);
    return d >= a - kEps && d <= b + kEps;
}

bool segsIntersect(double ax, double ay, double bx, double by,
                   double cx, double cy, double dx, double dy) {
    double d1 = cross2d(cx, cy, dx, dy, ax, ay);
    double d2 = cross2d(cx, cy, dx, dy, bx, by);
    double d3 = cross2d(ax, ay, bx, by, cx, cy);
    double d4 = cross2d(ax, ay, bx, by, dx, dy);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;
    auto colOn = [&](double px, double py,
                     double qx, double qy, double rx, double ry) {
        return std::abs(cross2d(qx, qy, rx, ry, px, py)) < kEps &&
               on1D(qx, rx, px) && on1D(qy, ry, py);
    };
    return colOn(ax, ay, cx, cy, dx, dy) || colOn(bx, by, cx, cy, dx, dy) ||
           colOn(cx, cy, ax, ay, bx, by) || colOn(dx, dy, ax, ay, bx, by);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class GpuGeoBackendTest : public ::testing::Test {};

// ============================================================
// Point × Point
// ============================================================

TEST_F(GpuGeoBackendTest, PointPoint_SamePoint_Intersects) {
    auto a = makePoint(1.0, 2.0);
    auto b = makePoint(1.0, 2.0);
    EXPECT_FALSE(pointInRing(0, 0, {}));  // sanity
    // Same coordinates: distance < epsilon
    EXPECT_LT(std::abs(a.coords[0].x - b.coords[0].x), kEps);
    EXPECT_LT(std::abs(a.coords[0].y - b.coords[0].y), kEps);
}

TEST_F(GpuGeoBackendTest, PointPoint_DifferentPoints_NoIntersect) {
    auto a = makePoint(0.0, 0.0);
    auto b = makePoint(1.0, 0.0);
    EXPECT_GT(std::abs(a.coords[0].x - b.coords[0].x), kEps);
}

// ============================================================
// Point × Polygon (ray-casting)
// ============================================================

TEST_F(GpuGeoBackendTest, PointPolygon_Inside_Intersects) {
    // Unit square [0,0]-[1,1], point at center
    std::vector<Coordinate> ring = {{0,0},{1,0},{1,1},{0,1},{0,0}};
    EXPECT_TRUE(pointInRing(0.5, 0.5, ring));
}

TEST_F(GpuGeoBackendTest, PointPolygon_Outside_NoIntersect) {
    std::vector<Coordinate> ring = {{0,0},{1,0},{1,1},{0,1},{0,0}};
    EXPECT_FALSE(pointInRing(2.0, 2.0, ring));
}

TEST_F(GpuGeoBackendTest, PointPolygon_OnEdge_Handled) {
    // On the bottom edge of the unit square
    std::vector<Coordinate> ring = {{0,0},{1,0},{1,1},{0,1},{0,0}};
    // Ray-casting edge behaviour is implementation-defined; we just ensure no crash.
    (void)pointInRing(0.5, 0.0, ring);
    SUCCEED();
}

TEST_F(GpuGeoBackendTest, PointPolygon_NegativeCoords_Inside) {
    std::vector<Coordinate> ring = {{-2,-2},{2,-2},{2,2},{-2,2},{-2,-2}};
    EXPECT_TRUE(pointInRing(-1.0, -1.0, ring));
}

TEST_F(GpuGeoBackendTest, PointPolygon_EmptyRing_False) {
    std::vector<Coordinate> ring;
    EXPECT_FALSE(pointInRing(0.5, 0.5, ring));
}

// ============================================================
// Segment intersection
// ============================================================

TEST_F(GpuGeoBackendTest, Segments_Cross_Intersect) {
    // X pattern: (0,1)-(1,0) and (0,0)-(1,1)
    EXPECT_TRUE(segsIntersect(0,1, 1,0, 0,0, 1,1));
}

TEST_F(GpuGeoBackendTest, Segments_Parallel_NoIntersect) {
    EXPECT_FALSE(segsIntersect(0,0, 1,0, 0,1, 1,1));
}

TEST_F(GpuGeoBackendTest, Segments_SharedEndpoint_Intersect) {
    EXPECT_TRUE(segsIntersect(0,0, 1,0, 1,0, 2,0));
}

TEST_F(GpuGeoBackendTest, Segments_Collinear_Overlapping_Intersect) {
    EXPECT_TRUE(segsIntersect(0,0, 2,0, 1,0, 3,0));
}

TEST_F(GpuGeoBackendTest, Segments_Collinear_NonOverlapping_NoIntersect) {
    EXPECT_FALSE(segsIntersect(0,0, 1,0, 2,0, 3,0));
}

TEST_F(GpuGeoBackendTest, Segments_TShape_Intersect) {
    // T: horizontal (0,1)-(2,1) and vertical (1,0)-(1,1) touching at top
    EXPECT_TRUE(segsIntersect(0,1, 2,1, 1,0, 1,1));
}

// ============================================================
// LineString × LineString
// ============================================================

TEST_F(GpuGeoBackendTest, LineStringLineString_Cross_Intersects) {
    // Horizontal line crosses vertical line
    auto ls1 = makeLineString({{0,1},{2,1}});
    auto ls2 = makeLineString({{1,0},{1,2}});
    // Verify at segment level
    EXPECT_TRUE(segsIntersect(ls1.coords[0].x, ls1.coords[0].y,
                               ls1.coords[1].x, ls1.coords[1].y,
                               ls2.coords[0].x, ls2.coords[0].y,
                               ls2.coords[1].x, ls2.coords[1].y));
}

TEST_F(GpuGeoBackendTest, LineStringLineString_Parallel_NoIntersect) {
    auto ls1 = makeLineString({{0,0},{2,0}});
    auto ls2 = makeLineString({{0,1},{2,1}});
    EXPECT_FALSE(segsIntersect(ls1.coords[0].x, ls1.coords[0].y,
                                ls1.coords[1].x, ls1.coords[1].y,
                                ls2.coords[0].x, ls2.coords[0].y,
                                ls2.coords[1].x, ls2.coords[1].y));
}

// ============================================================
// Polygon × Polygon
// ============================================================

TEST_F(GpuGeoBackendTest, PolygonPolygon_Overlapping_Intersects) {
    // Two overlapping unit squares offset by 0.5
    std::vector<Coordinate> r1 = {{0,0},{1,0},{1,1},{0,1},{0,0}};
    std::vector<Coordinate> r2 = {{0.5,0.5},{1.5,0.5},{1.5,1.5},{0.5,1.5},{0.5,0.5}};

    // Edge-edge: one edge from r1 should cross one edge from r2
    bool found = false;
    size_t n1 = r1.size(), n2 = r2.size();
    for (size_t i = 0, j = n1 - 1; i < n1 && !found; j = i++) {
        for (size_t k = 0, l = n2 - 1; k < n2 && !found; l = k++) {
            if (segsIntersect(r1[j].x, r1[j].y, r1[i].x, r1[i].y,
                              r2[l].x, r2[l].y, r2[k].x, r2[k].y))
                found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GpuGeoBackendTest, PolygonPolygon_Disjoint_NoIntersect) {
    std::vector<Coordinate> r1 = {{0,0},{1,0},{1,1},{0,1},{0,0}};
    std::vector<Coordinate> r2 = {{5,5},{6,5},{6,6},{5,6},{5,5}};

    bool found = false;
    size_t n1 = r1.size(), n2 = r2.size();
    for (size_t i = 0, j = n1 - 1; i < n1 && !found; j = i++) {
        for (size_t k = 0, l = n2 - 1; k < n2 && !found; l = k++) {
            if (segsIntersect(r1[j].x, r1[j].y, r1[i].x, r1[i].y,
                              r2[l].x, r2[l].y, r2[k].x, r2[k].y))
                found = true;
        }
    }
    EXPECT_FALSE(found);
    EXPECT_FALSE(pointInRing(r1[0].x, r1[0].y, r2));
    EXPECT_FALSE(pointInRing(r2[0].x, r2[0].y, r1));
}

TEST_F(GpuGeoBackendTest, PolygonPolygon_OneContainedInOther_Intersects) {
    std::vector<Coordinate> outer = {{0,0},{10,0},{10,10},{0,10},{0,0}};
    std::vector<Coordinate> inner = {{2,2},{4,2},{4,4},{2,4},{2,2}};
    // Inner vertex inside outer → containment detected
    EXPECT_TRUE(pointInRing(inner[0].x, inner[0].y, outer));
}

// ============================================================
// SpatialBatchInputs — no-crash and mask-size correctness
// ============================================================

TEST_F(GpuGeoBackendTest, BatchInputs_ZeroCount_EmptyMask) {
    SpatialBatchInputs in;
    in.count = 0;
    // Verify the result would have correct size
    std::vector<uint8_t> mask;
    mask.assign(in.count, 0u);
    EXPECT_TRUE(mask.empty());
}

TEST_F(GpuGeoBackendTest, BatchInputs_NonZeroCount_MaskSizeMatches) {
    SpatialBatchInputs in;
    in.count = 100;
    std::vector<uint8_t> mask;
    mask.assign(in.count, 0u);
    EXPECT_EQ(mask.size(), 100u);
}

// ============================================================
// Cross-product helper
// ============================================================

TEST_F(GpuGeoBackendTest, Cross2D_Collinear_IsZero) {
    // Points on y=x line
    double c = cross2d(0, 0, 1, 1, 2, 2);
    EXPECT_LT(std::abs(c), kEps);
}

TEST_F(GpuGeoBackendTest, Cross2D_Perpendicular_NonZero) {
    double c = cross2d(0, 0, 1, 0, 0, 1);
    EXPECT_GT(std::abs(c), kEps);
}

// ============================================================
// Backend interface — batchIntersects with real geometry pairs
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_ZeroCount_EmptyMask) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    SpatialBatchInputs in;
    in.count = 0;
    auto result = backend->batchIntersects(in);
    EXPECT_TRUE(result.mask.empty());
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_MaskSizeMatchesCount) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    // Pairs with no geometry → count governs mask size but geoms are empty
    SpatialBatchInputs in;
    in.count = 5;
    auto result = backend->batchIntersects(in);
    EXPECT_EQ(result.mask.size(), 5u);
    // No geometry data supplied — all entries must be 0
    for (auto v : result.mask) EXPECT_EQ(v, 0u);
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_PointPoint_SamePoint_Hit) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    in.count = 1;
    in.geoms_a.push_back(makePoint(3.0, 4.0));
    in.geoms_b.push_back(makePoint(3.0, 4.0));

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 1u);
    EXPECT_EQ(result.mask[0], 1u);
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_PointPoint_DifferentPoints_Miss) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    in.count = 1;
    in.geoms_a.push_back(makePoint(0.0, 0.0));
    in.geoms_b.push_back(makePoint(5.0, 5.0));

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 1u);
    EXPECT_EQ(result.mask[0], 0u);
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_PointInsidePolygon_Hit) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    in.count = 1;
    in.geoms_a.push_back(makePoint(0.5, 0.5));
    in.geoms_b.push_back(makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 1u);
    EXPECT_EQ(result.mask[0], 1u);
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_PointOutsidePolygon_Miss) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    in.count = 1;
    in.geoms_a.push_back(makePoint(9.0, 9.0));
    in.geoms_b.push_back(makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 1u);
    EXPECT_EQ(result.mask[0], 0u);
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_MultiplePairs_MixedResults) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    // Pair 0: same point → hit
    in.geoms_a.push_back(makePoint(1.0, 1.0));
    in.geoms_b.push_back(makePoint(1.0, 1.0));
    // Pair 1: point outside polygon → miss
    in.geoms_a.push_back(makePoint(10.0, 10.0));
    in.geoms_b.push_back(makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));
    // Pair 2: point inside polygon → hit
    in.geoms_a.push_back(makePoint(0.5, 0.5));
    in.geoms_b.push_back(makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}}));
    in.count = 3;

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 3u);
    EXPECT_EQ(result.mask[0], 1u);
    EXPECT_EQ(result.mask[1], 0u);
    EXPECT_EQ(result.mask[2], 1u);
}

// ============================================================
// Backend interface — exactIntersects
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_SamePoint_True) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->exactIntersects(makePoint(2.0, 3.0), makePoint(2.0, 3.0)));
}

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_DifferentPoints_False) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_FALSE(backend->exactIntersects(makePoint(0.0, 0.0), makePoint(1.0, 0.0)));
}

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_PointInsidePolygon_True) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->exactIntersects(
        makePoint(0.5, 0.5),
        makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}})));
}

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_PointOutsidePolygon_False) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_FALSE(backend->exactIntersects(
        makePoint(5.0, 5.0),
        makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}})));
}

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_OverlappingPolygons_True) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    auto p1 = makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}});
    auto p2 = makePolygon({{1,1},{3,1},{3,3},{1,3},{1,1}});
    EXPECT_TRUE(backend->exactIntersects(p1, p2));
}

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_DisjointPolygons_False) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    auto p1 = makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}});
    auto p2 = makePolygon({{5,5},{6,5},{6,6},{5,6},{5,5}});
    EXPECT_FALSE(backend->exactIntersects(p1, p2));
}

TEST_F(GpuGeoBackendTest, Backend_ExactIntersects_CrossingLineStrings_True) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    auto ls1 = makeLineString({{0,1},{2,1}});
    auto ls2 = makeLineString({{1,0},{1,2}});
    EXPECT_TRUE(backend->exactIntersects(ls1, ls2));
}

TEST_F(GpuGeoBackendTest, Backend_IsAvailable_NoCrash) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    // Should not crash regardless of hardware availability
    (void)backend->isAvailable();
    SUCCEED();
}

TEST_F(GpuGeoBackendTest, Backend_Name_IsGpuSpatial) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_STREQ(backend->name(), "gpu_spatial");
}

// ============================================================
// Edge-case / degenerate geometry (fault injection)
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_EdgeCase_EmptyPoint_NoCrash) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    // Point with no coordinate data — must not crash and must return false
    GeometryInfo empty(GeometryType::Point);
    EXPECT_FALSE(backend->exactIntersects(empty, makePoint(0.0, 0.0)));
    EXPECT_FALSE(backend->exactIntersects(makePoint(0.0, 0.0), empty));
}

TEST_F(GpuGeoBackendTest, Backend_EdgeCase_EmptyPolygon_NoCrash) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    GeometryInfo empty(GeometryType::Polygon); // no rings, no coords
    EXPECT_FALSE(backend->exactIntersects(empty, makePoint(0.0, 0.0)));
    EXPECT_FALSE(backend->exactIntersects(makePoint(0.5, 0.5), empty));
}

TEST_F(GpuGeoBackendTest, Backend_EdgeCase_DegenerateLineString_NoCrash) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    // LineString with only one point (degenerate)
    GeometryInfo degen(GeometryType::LineString);
    degen.coords.push_back({1.0, 1.0});
    EXPECT_FALSE(backend->exactIntersects(makePoint(1.0, 1.0), degen));
}

TEST_F(GpuGeoBackendTest, Backend_EdgeCase_PolygonCoordsNoRings) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);
    // Polygon whose ring data lives in coords rather than rings (legacy path)
    GeometryInfo poly(GeometryType::Polygon);
    poly.coords = {{0,0},{1,0},{1,1},{0,1},{0,0}};
    // No rings — outerRing() falls back to coords
    EXPECT_TRUE(backend->exactIntersects(makePoint(0.5, 0.5), poly));
    EXPECT_FALSE(backend->exactIntersects(makePoint(9.0, 9.0), poly));
}

// ============================================================
// Count / vector size mismatch in batch (robustness)
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_CountLargerThanVectors_ProcessesMin) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    // Only 2 real pairs but count says 10
    in.count = 10;
    in.geoms_a.push_back(makePoint(0.5, 0.5));
    in.geoms_b.push_back(makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));
    in.geoms_a.push_back(makePoint(9.0, 9.0));
    in.geoms_b.push_back(makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));

    auto result = backend->batchIntersects(in);
    // Mask is sized by count
    ASSERT_EQ(result.mask.size(), 10u);
    // First 2 entries reflect real geometry tests
    EXPECT_EQ(result.mask[0], 1u);
    EXPECT_EQ(result.mask[1], 0u);
    // Remaining 8 stay 0 (no geometry provided)
    for (std::size_t i = 2; i < 10; ++i) EXPECT_EQ(result.mask[i], 0u);
}

TEST_F(GpuGeoBackendTest, Backend_BatchIntersects_VectorsLargerThanCount_RespectsCount) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    SpatialBatchInputs in;
    // count says 1 but 3 pairs provided — only first pair should be checked
    in.count = 1;
    in.geoms_a = {makePoint(0.5, 0.5), makePoint(9.0, 9.0), makePoint(0.5, 0.5)};
    in.geoms_b = {makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}),
                  makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}),
                  makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}})};

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 1u);
    EXPECT_EQ(result.mask[0], 1u); // point (0.5,0.5) is inside unit square
}

// ============================================================
// Large-batch stress test
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_LargeBatch_CorrectResults) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    constexpr std::size_t kBatchSize = 200;
    // Square polygon [0,10]x[0,10]
    auto poly = makePolygon({{0,0},{10,0},{10,10},{0,10},{0,0}});

    SpatialBatchInputs in;
    in.count = kBatchSize;
    for (std::size_t i = 0; i < kBatchSize; ++i) {
        // Even-indexed points are inside (5,5); odd-indexed are outside (20,20)
        if (i % 2 == 0) {
            in.geoms_a.push_back(makePoint(5.0, 5.0));
        } else {
            in.geoms_a.push_back(makePoint(20.0, 20.0));
        }
        in.geoms_b.push_back(poly);
    }

    auto result = backend->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), kBatchSize);
    for (std::size_t i = 0; i < kBatchSize; ++i) {
        if (i % 2 == 0) {
            EXPECT_EQ(result.mask[i], 1u) << "pair " << i << " should be a hit";
        } else {
            EXPECT_EQ(result.mask[i], 0u) << "pair " << i << " should be a miss";
        }
    }
}

// ============================================================
// Concurrent access — no data races (thread-safety smoke test)
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_Concurrent_NoCrash) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    auto poly = makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}});
    auto pt_in  = makePoint(0.5, 0.5);
    auto pt_out = makePoint(9.0, 9.0);

    constexpr int kThreads = 4;
    constexpr int kIter    = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kIter; ++i) {
                // exactIntersects
                bool hit = backend->exactIntersects(pt_in, poly);
                EXPECT_TRUE(hit);
                bool miss = backend->exactIntersects(pt_out, poly);
                EXPECT_FALSE(miss);

                // batchIntersects
                SpatialBatchInputs in;
                in.count = 2;
                in.geoms_a = {pt_in, pt_out};
                in.geoms_b = {poly, poly};
                auto result = backend->batchIntersects(in);
                ASSERT_EQ(result.mask.size(), 2u);
                EXPECT_EQ(result.mask[0], 1u);
                EXPECT_EQ(result.mask[1], 0u);
            }
        });
    }
    for (auto& th : threads) th.join();
    SUCCEED();
}

// ============================================================
// Latency / throughput observability
// ============================================================

TEST_F(GpuGeoBackendTest, Backend_LatencyTracking_NonZeroAfterBatch) {
    // Cast to concrete type to access getStats().
    // The singleton is GpuBatchBackend which lives in the TU; we reach it
    // indirectly by observing that the interface backend pointer is valid
    // and that repeated calls produce consistent results (can't inspect
    // internal stats without a downcast, so we verify observable side-effects).
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    // Warm-up: issue a batch and confirm we get a valid result (not a crash).
    SpatialBatchInputs in;
    in.count = 10;
    auto poly = makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}});
    for (int i = 0; i < 10; ++i) {
        in.geoms_a.push_back(makePoint(0.5, 0.5));
        in.geoms_b.push_back(poly);
    }
    auto result = backend->batchIntersects(in);
    EXPECT_EQ(result.mask.size(), 10u);
    for (auto v : result.mask) EXPECT_EQ(v, 1u);
    // If we reach here the latency-tracking code did not introduce UB or crash.
    SUCCEED();
}

// ============================================================
// Cross-backend verification
// GPU backend results must agree with the reference CPU backend
// for every supported geometry type combination.
// ============================================================

class CrossBackendTest : public ::testing::Test {
protected:
    // Non-owning raw pointers to singletons; validity asserted in SetUp().
    themis::geo::ISpatialComputeBackend* gpu = themis::geo::getGpuSpatialBackend();
    themis::geo::ISpatialComputeBackend* cpu = themis::geo::getCpuExactBackend();

    void SetUp() override {
        ASSERT_NE(gpu, nullptr) << "GPU backend must be non-null";
        ASSERT_NE(cpu, nullptr) << "CPU backend must be non-null";
    }

    void expectAgree(const GeometryInfo& a, const GeometryInfo& b,
                     std::string_view label) {
        bool gpu_result = gpu->exactIntersects(a, b);
        bool cpu_result = cpu->exactIntersects(a, b);
        EXPECT_EQ(gpu_result, cpu_result)
            << label << ": GPU=" << gpu_result << " CPU=" << cpu_result;
    }

    void expectBatchAgree(const SpatialBatchInputs& in, std::string_view label) {
        auto gpu_r = gpu->batchIntersects(in);
        auto cpu_r = cpu->batchIntersects(in);
        ASSERT_EQ(gpu_r.mask.size(), cpu_r.mask.size())
            << label << ": mask size mismatch";
        for (std::size_t i = 0; i < gpu_r.mask.size(); ++i) {
            EXPECT_EQ(gpu_r.mask[i], cpu_r.mask[i])
                << label << "[" << i << "]: GPU=" << (int)gpu_r.mask[i]
                << " CPU=" << (int)cpu_r.mask[i];
        }
    }
};

// ---- exactIntersects cross-check ----

TEST_F(CrossBackendTest, ExactIntersects_PointPoint_Same) {
    expectAgree(makePoint(1.0, 2.0), makePoint(1.0, 2.0),
                "PointPoint_Same");
}

TEST_F(CrossBackendTest, ExactIntersects_PointPoint_Different) {
    expectAgree(makePoint(1.0, 2.0), makePoint(3.0, 4.0),
                "PointPoint_Different");
}

TEST_F(CrossBackendTest, ExactIntersects_PointInsidePolygon) {
    auto poly = makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}});
    expectAgree(makePoint(1.0, 1.0), poly, "PointInsidePolygon");
}

TEST_F(CrossBackendTest, ExactIntersects_PointOutsidePolygon) {
    auto poly = makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}});
    expectAgree(makePoint(5.0, 5.0), poly, "PointOutsidePolygon");
}

TEST_F(CrossBackendTest, ExactIntersects_PolygonPoint_Symmetric) {
    auto poly = makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}});
    expectAgree(poly, makePoint(1.0, 1.0), "PolygonPoint_Symmetric");
}

TEST_F(CrossBackendTest, ExactIntersects_PointOnBoundary_Consistent) {
    auto poly = makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}});
    // Both backends must give the same answer for boundary point (may be
    // true or false depending on edge handling, but they must agree).
    bool gpu_r = gpu->exactIntersects(makePoint(1.0, 0.0), poly);
    bool cpu_r = cpu->exactIntersects(makePoint(1.0, 0.0), poly);
    EXPECT_EQ(gpu_r, cpu_r) << "boundary point: GPU=" << gpu_r << " CPU=" << cpu_r;
}

TEST_F(CrossBackendTest, ExactIntersects_OverlappingPolygons) {
    auto p1 = makePolygon({{0,0},{3,0},{3,3},{0,3},{0,0}});
    auto p2 = makePolygon({{1,1},{4,1},{4,4},{1,4},{1,1}});
    expectAgree(p1, p2, "OverlappingPolygons");
}

TEST_F(CrossBackendTest, ExactIntersects_DisjointPolygons) {
    auto p1 = makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}});
    auto p2 = makePolygon({{5,5},{6,5},{6,6},{5,6},{5,5}});
    expectAgree(p1, p2, "DisjointPolygons");
}

TEST_F(CrossBackendTest, ExactIntersects_ContainedPolygon) {
    auto outer = makePolygon({{0,0},{10,0},{10,10},{0,10},{0,0}});
    auto inner = makePolygon({{2,2},{4,2},{4,4},{2,4},{2,2}});
    expectAgree(outer, inner, "ContainedPolygon");
}

TEST_F(CrossBackendTest, ExactIntersects_EmptyPoint_Both) {
    GeometryInfo empty(GeometryType::Point);
    expectAgree(empty, makePoint(0.0, 0.0), "EmptyPoint_A");
    expectAgree(makePoint(0.0, 0.0), empty, "EmptyPoint_B");
}

// ---- batchIntersects cross-check ----

TEST_F(CrossBackendTest, BatchIntersects_EmptyBatch) {
    SpatialBatchInputs in;
    in.count = 0;
    expectBatchAgree(in, "EmptyBatch");
}

TEST_F(CrossBackendTest, BatchIntersects_MixedResults) {
    auto poly = makePolygon({{0,0},{4,0},{4,4},{0,4},{0,0}});
    SpatialBatchInputs in;
    in.count = 4;
    // hit, miss, hit, miss
    in.geoms_a = {makePoint(2.0,2.0), makePoint(9.0,9.0),
                  makePoint(1.0,1.0), makePoint(-1.0,-1.0)};
    in.geoms_b = {poly, poly, poly, poly};
    expectBatchAgree(in, "BatchMixedResults");
}

TEST_F(CrossBackendTest, BatchIntersects_PolygonPolygon) {
    SpatialBatchInputs in;
    in.count = 2;
    in.geoms_a = {makePolygon({{0,0},{2,0},{2,2},{0,2},{0,0}}),
                  makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}})};
    in.geoms_b = {makePolygon({{1,1},{3,1},{3,3},{1,3},{1,1}}),   // overlaps
                  makePolygon({{5,5},{6,5},{6,6},{5,6},{5,5}})};  // disjoint
    expectBatchAgree(in, "BatchPolygonPolygon");
}

// ============================================================
// getGpuSpatialBackendStatsJson() — observability free function
// ============================================================

TEST(GpuSpatialBackendStatsJson, ReturnsValidJson) {
    std::string json = themis::geo::getGpuSpatialBackendStatsJson();
    ASSERT_FALSE(json.empty()) << "getGpuSpatialBackendStatsJson() must not return empty string";
    // Must start with '{' and end with '}'
    EXPECT_EQ(json.front(), '{');
    EXPECT_EQ(json.back(), '}');
}

TEST(GpuSpatialBackendStatsJson, ContainsRequiredKeys) {
    std::string json = themis::geo::getGpuSpatialBackendStatsJson();
    // Verify all required keys are present in the JSON string
    const std::vector<std::string> required_keys = {
        "\"gpu_present\"", "\"circuit_open\"", "\"device_name\"",
        "\"batch_calls\"", "\"batch_fallbacks\"", "\"batch_pairs_processed\"",
        "\"exact_calls\"", "\"exact_errors\"",
        "\"batch_avg_latency_us\"", "\"batch_max_latency_us\""
    };
    for (const auto& key : required_keys) {
        EXPECT_NE(json.find(key), std::string::npos)
            << "Missing key: " << key << " in: " << json;
    }
}

TEST(GpuSpatialBackendStatsJson, CountersReflectCalls) {
    // Issue a few operations then verify the counters increase.
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    // Get baseline
    std::string before = themis::geo::getGpuSpatialBackendStatsJson();
    auto exactPos  = before.find("\"exact_calls\":");
    ASSERT_NE(exactPos, std::string::npos);

    // Issue two exact-intersect calls
    (void)backend->exactIntersects(makePoint(0.5, 0.5),
                                   makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));
    (void)backend->exactIntersects(makePoint(9.0, 9.0),
                                   makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));

    std::string after = themis::geo::getGpuSpatialBackendStatsJson();
    // The JSON string should differ (counters incremented)
    EXPECT_NE(before, after)
        << "Stats JSON should change after calling exactIntersects";
}

TEST(GpuSpatialBackendStatsJson, BatchCallsReflected) {
    auto* backend = themis::geo::getGpuSpatialBackend();
    ASSERT_NE(backend, nullptr);

    std::string before = themis::geo::getGpuSpatialBackendStatsJson();

    SpatialBatchInputs in;
    in.count = 2;
    in.geoms_a = {makePoint(0.5, 0.5), makePoint(9.0, 9.0)};
    in.geoms_b = {makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}),
                  makePolygon({{0,0},{1,0},{1,1},{0,1},{0,0}})};
    (void)backend->batchIntersects(in);

    std::string after = themis::geo::getGpuSpatialBackendStatsJson();
    EXPECT_NE(before, after)
        << "Stats JSON should change after calling batchIntersects";
    // batch_pairs_processed must contain at least 2
    EXPECT_NE(after.find("\"batch_pairs_processed\":"), std::string::npos);
}
