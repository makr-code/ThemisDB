#include <gtest/gtest.h>
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <string>
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

static GeometryInfo makeBox(double minx, double miny, double maxx, double maxy) {
    GeometryInfo g(GeometryType::Polygon);
    std::vector<Coordinate> ring = {
        {minx, miny}, {maxx, miny}, {maxx, maxy}, {minx, maxy}, {minx, miny}
    };
    g.rings.push_back(ring);
    return g;
}

// ---------------------------------------------------------------------------
// Backend factory / name tests
// ---------------------------------------------------------------------------

TEST(GeoPrecisionMode, ExactBackendHasCorrectName) {
    ISpatialComputeBackend* b = getBackendForPrecision(GeoPrecisionMode::Exact);
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->name(), "cpu_exact");
}

TEST(GeoPrecisionMode, ApproximateBackendHasCorrectName) {
    ISpatialComputeBackend* b = getBackendForPrecision(GeoPrecisionMode::Approximate);
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->name(), "cpu_approximate");
}

TEST(GeoPrecisionMode, ExactBackendIsAvailable) {
    EXPECT_TRUE(getBackendForPrecision(GeoPrecisionMode::Exact)->isAvailable());
}

TEST(GeoPrecisionMode, ApproximateBackendIsAvailable) {
    EXPECT_TRUE(getBackendForPrecision(GeoPrecisionMode::Approximate)->isAvailable());
}

TEST(GeoPrecisionMode, ExactAndApproximateAreDistinctInstances) {
    ISpatialComputeBackend* exact = getBackendForPrecision(GeoPrecisionMode::Exact);
    ISpatialComputeBackend* approx = getBackendForPrecision(GeoPrecisionMode::Approximate);
    EXPECT_NE(exact, approx);
}

TEST(GeoPrecisionMode, CpuApproximateBackendSingletonIsStable) {
    EXPECT_EQ(getCpuApproximateBackend(), getCpuApproximateBackend());
}

// ---------------------------------------------------------------------------
// Intersects: exact and approximate must agree on obvious cases
// ---------------------------------------------------------------------------

class PrecisionModeIntersectsTest
    : public ::testing::TestWithParam<ISpatialComputeBackend*> {
protected:
    ISpatialComputeBackend* backend() { return GetParam(); }
};

TEST_P(PrecisionModeIntersectsTest, PointInsidePolygon_Intersects) {
    const GeometryInfo pt  = makePoint(0.5, 0.5);
    const GeometryInfo box = makeBox(0.0, 0.0, 1.0, 1.0);
    EXPECT_TRUE(backend()->exactIntersects(pt, box));
    EXPECT_TRUE(backend()->exactIntersects(box, pt));
}

TEST_P(PrecisionModeIntersectsTest, PointOutsidePolygon_NoIntersect) {
    const GeometryInfo pt  = makePoint(5.0, 5.0);
    const GeometryInfo box = makeBox(0.0, 0.0, 1.0, 1.0);
    EXPECT_FALSE(backend()->exactIntersects(pt, box));
    EXPECT_FALSE(backend()->exactIntersects(box, pt));
}

TEST_P(PrecisionModeIntersectsTest, OverlappingPolygons_Intersect) {
    const GeometryInfo box1 = makeBox(0.0, 0.0, 2.0, 2.0);
    const GeometryInfo box2 = makeBox(1.0, 1.0, 3.0, 3.0);
    EXPECT_TRUE(backend()->exactIntersects(box1, box2));
    EXPECT_TRUE(backend()->exactIntersects(box2, box1));
}

TEST_P(PrecisionModeIntersectsTest, DisjointPolygons_NoIntersect) {
    const GeometryInfo box1 = makeBox(0.0, 0.0, 1.0, 1.0);
    const GeometryInfo box2 = makeBox(2.0, 2.0, 3.0, 3.0);
    EXPECT_FALSE(backend()->exactIntersects(box1, box2));
    EXPECT_FALSE(backend()->exactIntersects(box2, box1));
}

TEST_P(PrecisionModeIntersectsTest, IdenticalPoints_Intersect) {
    const GeometryInfo pt1 = makePoint(10.0, 20.0);
    const GeometryInfo pt2 = makePoint(10.0, 20.0);
    EXPECT_TRUE(backend()->exactIntersects(pt1, pt2));
}

TEST_P(PrecisionModeIntersectsTest, DifferentPoints_NoIntersect) {
    const GeometryInfo pt1 = makePoint(10.0, 20.0);
    const GeometryInfo pt2 = makePoint(10.5, 20.5);
    EXPECT_FALSE(backend()->exactIntersects(pt1, pt2));
}

INSTANTIATE_TEST_SUITE_P(
    ExactMode, PrecisionModeIntersectsTest,
    ::testing::Values(getBackendForPrecision(GeoPrecisionMode::Exact)),
    [](const testing::TestParamInfo<ISpatialComputeBackend*>&) {
        return "exact";
    });

INSTANTIATE_TEST_SUITE_P(
    ApproximateMode, PrecisionModeIntersectsTest,
    ::testing::Values(getBackendForPrecision(GeoPrecisionMode::Approximate)),
    [](const testing::TestParamInfo<ISpatialComputeBackend*>&) {
        return "approximate";
    });

// ---------------------------------------------------------------------------
// Approximate backend: no false negatives
// If exact says "intersects", approximate must too.
// ---------------------------------------------------------------------------

TEST(GeoPrecisionModeNoFalseNegatives, ExactIntersectImpliesApproximateIntersect) {
    ISpatialComputeBackend* exact  = getBackendForPrecision(GeoPrecisionMode::Exact);
    ISpatialComputeBackend* approx = getBackendForPrecision(GeoPrecisionMode::Approximate);

    // A set of geometry pairs that definitely intersect
    struct Pair { GeometryInfo a; GeometryInfo b; };
    std::vector<Pair> pairs = {
        { makePoint(0.5, 0.5), makeBox(0.0, 0.0, 1.0, 1.0) },
        { makeBox(0.0, 0.0, 2.0, 2.0), makeBox(1.0, 1.0, 3.0, 3.0) },
        { makePoint(1.0, 1.0), makePoint(1.0, 1.0) },
    };

    for (const auto& p : pairs) {
        if (exact->exactIntersects(p.a, p.b)) {
            EXPECT_TRUE(approx->exactIntersects(p.a, p.b))
                << "Approximate backend must not produce false negatives";
        }
    }
}

// ---------------------------------------------------------------------------
// Batch intersects respects precision mode
// ---------------------------------------------------------------------------

TEST(GeoPrecisionModeBatch, ExactMode_BatchIntersects) {
    ISpatialComputeBackend* b = getBackendForPrecision(GeoPrecisionMode::Exact);

    SpatialBatchInputs in;
    in.geoms_a = { makePoint(0.5, 0.5), makePoint(5.0, 5.0) };
    in.geoms_b = { makeBox(0.0, 0.0, 1.0, 1.0), makeBox(0.0, 0.0, 1.0, 1.0) };
    in.count   = 2;

    const auto result = b->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 2u);
    EXPECT_EQ(result.mask[0], 1u) << "Point inside box should intersect";
    EXPECT_EQ(result.mask[1], 0u) << "Point outside box should not intersect";
}

TEST(GeoPrecisionModeBatch, ApproximateMode_BatchIntersects) {
    ISpatialComputeBackend* b = getBackendForPrecision(GeoPrecisionMode::Approximate);

    SpatialBatchInputs in;
    in.geoms_a = { makePoint(0.5, 0.5), makePoint(5.0, 5.0) };
    in.geoms_b = { makeBox(0.0, 0.0, 1.0, 1.0), makeBox(0.0, 0.0, 1.0, 1.0) };
    in.count   = 2;

    const auto result = b->batchIntersects(in);
    ASSERT_EQ(result.mask.size(), 2u);
    // Approximate uses MBR: first pair must intersect, second must not
    EXPECT_EQ(result.mask[0], 1u);
    EXPECT_EQ(result.mask[1], 0u);
}

// ---------------------------------------------------------------------------
// Approximate backend: demonstrates false-positive behavior
// Two disjoint polygons whose MBRs overlap (classic L-shape + corner box).
// The approximate backend must return TRUE (no false negatives), but it is
// allowed to return TRUE even when the shapes don't touch (false positive).
// The exact backend must return FALSE for the same pair.
// ---------------------------------------------------------------------------

TEST(GeoPrecisionModeFalsePositive, ApproximateCanReturnTrueWhenExactReturnsFalse) {
    // L-shape: 2×2 square with the top-right 1×1 corner removed.
    // Vertices (CCW outer ring): (0,0)→(2,0)→(2,1)→(1,1)→(1,2)→(0,2)→close
    GeometryInfo l_shape(GeometryType::Polygon);
    l_shape.rings.push_back({
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {1.0, 1.0},
        {1.0, 2.0}, {0.0, 2.0}, {0.0, 0.0}  // closed ring
    });

    // Small box sitting entirely in the removed corner: x∈[1.5,2], y∈[1.5,2].
    // This region is outside the L-shape polygon.
    const GeometryInfo corner_box = makeBox(1.5, 1.5, 2.0, 2.0);

    ISpatialComputeBackend* exact  = getBackendForPrecision(GeoPrecisionMode::Exact);
    ISpatialComputeBackend* approx = getBackendForPrecision(GeoPrecisionMode::Approximate);

    // Exact check: shapes are disjoint — must return false.
    EXPECT_FALSE(exact->exactIntersects(l_shape, corner_box))
        << "Exact mode must correctly identify that the corner box is outside the L-shape";

    // Approximate check (MBR-based): MBRs do overlap ([0,0,2,2] vs [1.5,1.5,2,2]).
    // The approximate backend is allowed to return true here (false positive).
    // This test documents that the approximate backend uses MBR and intentionally
    // does not distinguish between "MBR overlap" and "actual shape overlap".
    EXPECT_TRUE(approx->exactIntersects(l_shape, corner_box))
        << "Approximate mode uses MBR; the MBRs overlap so it should return true "
           "(false positive by design, no false negatives)";
}

TEST(GeoPrecisionModeBuffer, ApproximateBuffer_SameAsExact) {
    ISpatialComputeBackend* exact  = getBackendForPrecision(GeoPrecisionMode::Exact);
    ISpatialComputeBackend* approx = getBackendForPrecision(GeoPrecisionMode::Approximate);

    const GeometryInfo pt = makePoint(13.4050, 52.5200); // Berlin
    const GeometryInfo r_exact  = exact->stBuffer(pt, 500.0, 36);
    const GeometryInfo r_approx = approx->stBuffer(pt, 500.0, 36);

    ASSERT_TRUE(r_exact.isPolygon());
    ASSERT_TRUE(r_approx.isPolygon());
    ASSERT_EQ(r_exact.rings.size(), r_approx.rings.size());
    ASSERT_EQ(r_exact.rings[0].size(), r_approx.rings[0].size());

    for (std::size_t i = 0; i < r_exact.rings[0].size(); ++i) {
        EXPECT_NEAR(r_exact.rings[0][i].x, r_approx.rings[0][i].x, 1e-12);
        EXPECT_NEAR(r_exact.rings[0][i].y, r_approx.rings[0][i].y, 1e-12);
    }
}

// ---------------------------------------------------------------------------
// GeoContainmentFn injection tests (GEO-CF-01 … GEO-CF-04)
// ---------------------------------------------------------------------------

// GEO-CF-01 — Injected containment fn returning true → point always inside polygon
TEST(GeoCpuContainmentFn, GEO_CF_01_InjectedFnAlwaysTrue) {
    setCpuExactContainmentFn([](double, double, const std::vector<Coordinate>&) {
        return true;
    });

    ISpatialComputeBackend* backend = getCpuExactBackend();
    // Point clearly outside the unit box — injected fn returns true anyway
    const GeometryInfo pt  = makePoint(100.0, 100.0);
    const GeometryInfo box = makeBox(0.0, 0.0, 1.0, 1.0);

    EXPECT_TRUE(backend->exactIntersects(pt, box))
        << "GEO-CF-01: injected always-true fn must override ray-casting result";

    // Restore default
    setCpuExactContainmentFn(nullptr);
}

// GEO-CF-02 — Injected containment fn returning false → point always outside polygon
TEST(GeoCpuContainmentFn, GEO_CF_02_InjectedFnAlwaysFalse) {
    setCpuExactContainmentFn([](double, double, const std::vector<Coordinate>&) {
        return false;
    });

    ISpatialComputeBackend* backend = getCpuExactBackend();
    // Point clearly inside the unit box — injected fn returns false
    const GeometryInfo pt  = makePoint(0.5, 0.5);
    const GeometryInfo box = makeBox(0.0, 0.0, 1.0, 1.0);

    EXPECT_FALSE(backend->exactIntersects(pt, box))
        << "GEO-CF-02: injected always-false fn must override ray-casting result";

    setCpuExactContainmentFn(nullptr);
}

// GEO-CF-03 — Clearing the injection (nullptr) restores built-in ray-casting
TEST(GeoCpuContainmentFn, GEO_CF_03_ClearInjection_RestoresBuiltin) {
    // Inject an always-true fn, then clear it.
    setCpuExactContainmentFn([](double, double, const std::vector<Coordinate>&) {
        return true;
    });
    setCpuExactContainmentFn(nullptr); // clear

    ISpatialComputeBackend* backend = getCpuExactBackend();
    const GeometryInfo pt_inside  = makePoint(0.5, 0.5);
    const GeometryInfo pt_outside = makePoint(5.0, 5.0);
    const GeometryInfo box        = makeBox(0.0, 0.0, 1.0, 1.0);

    // Built-in ray-casting should now be in effect
    EXPECT_TRUE(backend->exactIntersects(pt_inside, box))
        << "GEO-CF-03: after clearing fn, built-in ray-casting must correctly report containment";
    EXPECT_FALSE(backend->exactIntersects(pt_outside, box))
        << "GEO-CF-03: after clearing fn, built-in ray-casting must correctly reject exterior point";
}

// GEO-CF-04 — Injected fn receives correct (px, py, ring) arguments
TEST(GeoCpuContainmentFn, GEO_CF_04_InjectedFnReceivesCorrectArgs) {
    double last_px = -1.0, last_py = -1.0;
    bool fn_called = false;

    setCpuExactContainmentFn([&](double px, double py,
                                  const std::vector<Coordinate>& /*ring*/) {
        last_px    = px;
        last_py    = py;
        fn_called  = true;
        // Use the expected coordinates to decide the result
        return (std::abs(px - 0.5) < 1.0 && std::abs(py - 0.5) < 1.0);
    });

    ISpatialComputeBackend* backend = getCpuExactBackend();
    const GeometryInfo pt  = makePoint(0.5, 0.5);
    const GeometryInfo box = makeBox(0.0, 0.0, 1.0, 1.0);
    static_cast<void>(backend->exactIntersects(pt, box));

    EXPECT_TRUE(fn_called) << "GEO-CF-04: injected fn must be called";
    EXPECT_NEAR(last_px, 0.5, 1e-12) << "GEO-CF-04: px must match point x coordinate";
    EXPECT_NEAR(last_py, 0.5, 1e-12) << "GEO-CF-04: py must match point y coordinate";

    setCpuExactContainmentFn(nullptr);
}
