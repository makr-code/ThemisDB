#include <gtest/gtest.h>
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr double kPi = 3.14159265358979323846;

static GeometryInfo makePoint(double x, double y) {
    GeometryInfo g(GeometryType::Point);
    g.coords.push_back({x, y});
    return g;
}

static GeometryInfo makeClosedPolygon(
        std::initializer_list<std::pair<double, double>> pts) {
    GeometryInfo g(GeometryType::Polygon);
    std::vector<Coordinate> ring = {};

    for (const auto& p : pts) ring.push_back({p.first, p.second});
    ring.push_back(ring[0]); // close the ring
    g.rings.push_back(ring);
    return g;
}

/// Approximate geodesic radius of a buffered point polygon (average vertex
/// distance from centre converted to metres).
static double approxRadiusM(const GeometryInfo& buffered,
                             double cx, double cy) {
    if (buffered.rings.empty()) {
      return 0.0;
    }
    const auto& ring = buffered.rings[0];
    if (ring.size() < 2) {
      return 0.0;
    }
    double sum = 0.0;
    std::size_t n = ring.size() - 1; // last vertex == first (closed ring)
    if (n == 0) {
      return 0.0;
    }
    const double lat_rad = cy * kPi / 180.0;
    const double m_per_lat_deg = 111320.0;
    const double m_per_lon_deg = 111320.0 * std::cos(lat_rad);
    for (std::size_t i = 0; i < n; ++i) {
        double dlat_m = (ring[i].y - cy) * m_per_lat_deg;
        double dlon_m = (ring[i].x - cx) * m_per_lon_deg;
        sum += std::sqrt(dlat_m * dlat_m + dlon_m * dlon_m);
    }
    return sum / static_cast<double>(n);
}

/// Compute the approximate 2-D area of the polygon ring in geographic degrees²
/// using the Shoelace formula.
static double ringAreaDeg2(const std::vector<Coordinate>& ring) {
    if (ring.size() < 3) {
      return 0.0;
    }
    double area = 0.0;
    std::size_t n = ring.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        area += (ring[j].x + ring[i].x) * (ring[j].y - ring[i].y);
    }
    return std::abs(area) * 0.5;
}

// ---------------------------------------------------------------------------
// Parameterised fixture: run all tests against both available backends
// ---------------------------------------------------------------------------

class StBufferTest : public ::testing::TestWithParam<ISpatialComputeBackend*> {
protected:
    ISpatialComputeBackend* backend() { return GetParam(); }
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

TEST_P(StBufferTest, PointBuffer_ProducesPolygon) {
    const GeometryInfo pt = makePoint(13.4050, 52.5200); // Berlin
    const GeometryInfo result = backend()->stBuffer(pt, 500.0, 36);

    ASSERT_TRUE(result.isPolygon())
        << "stBuffer on a Point should return a Polygon";
    ASSERT_FALSE(result.rings.empty())
        << "Buffered polygon must have at least one ring";
    // Closed ring: first == last vertex
    const auto& ring = result.rings[0];
    ASSERT_GE(ring.size(), 4u) << "Ring must have at least 3 unique vertices + closing vertex";
    EXPECT_NEAR(ring.front().x, ring.back().x, 1e-10);
    EXPECT_NEAR(ring.front().y, ring.back().y, 1e-10);
}

TEST_P(StBufferTest, PointBuffer_ApproximateRadius) {
    const double cx = 13.4050, cy = 52.5200;
    const double radius_m = 500.0;
    const GeometryInfo pt = makePoint(cx, cy);
    const GeometryInfo result = backend()->stBuffer(pt, radius_m, 64);

    ASSERT_TRUE(result.isPolygon());
    const double approx_r = approxRadiusM(result, cx, cy);
    // Allow ±10 % tolerance for the circular approximation and degree conversion.
    EXPECT_NEAR(approx_r, radius_m, radius_m * 0.10)
        << "Approximate radius of buffered circle should be within 10 % of "
        << radius_m << " m (got " << approx_r << " m)";
}

TEST_P(StBufferTest, PointBuffer_ArcPointsControlsVertexCount) {
    const GeometryInfo pt = makePoint(0.0, 0.0);
    for (int ap : {8, 16, 36, 72}) {
        const GeometryInfo result = backend()->stBuffer(pt, 100.0, ap);
        ASSERT_TRUE(result.isPolygon());
        ASSERT_FALSE(result.rings.empty());
        // Ring has ap vertices plus a closing duplicate.
        EXPECT_EQ(result.rings[0].size(), static_cast<std::size_t>(ap + 1))
            << "arc_points=" << ap;
    }
}

TEST_P(StBufferTest, PolygonBuffer_ProducesLargerPolygon) {
    // Unit square [0,0]–[0.01,0.01] (≈ 1.1 km side at equator)
    const GeometryInfo square = makeClosedPolygon(
        {{0.0, 0.0}, {0.01, 0.0}, {0.01, 0.01}, {0.0, 0.01}});
    const GeometryInfo result = backend()->stBuffer(square, 500.0, 36);

    ASSERT_TRUE(result.isPolygon());
    ASSERT_FALSE(result.rings.empty());
    // The buffered polygon should have a larger bounding box.
    const auto mbr_in  = square.computeMBR();
    const auto mbr_out = result.computeMBR();
    EXPECT_GT(mbr_out.maxx, mbr_in.maxx) << "Buffered polygon must extend east";
    EXPECT_GT(mbr_out.maxy, mbr_in.maxy) << "Buffered polygon must extend north";
    EXPECT_LT(mbr_out.minx, mbr_in.minx) << "Buffered polygon must extend west";
    EXPECT_LT(mbr_out.miny, mbr_in.miny) << "Buffered polygon must extend south";
}

TEST_P(StBufferTest, PolygonBuffer_LargerArea) {
    const GeometryInfo square = makeClosedPolygon(
        {{0.0, 0.0}, {0.01, 0.0}, {0.01, 0.01}, {0.0, 0.01}});
    const GeometryInfo result = backend()->stBuffer(square, 500.0, 36);

    ASSERT_TRUE(result.isPolygon());
    ASSERT_FALSE(result.rings.empty());
    const double area_in  = ringAreaDeg2(square.rings[0]);
    const double area_out = ringAreaDeg2(result.rings[0]);
    EXPECT_GT(area_out, area_in)
        << "Buffered polygon area must exceed input polygon area";
}

TEST_P(StBufferTest, NegativeDistance_ReturnsEmpty) {
    const GeometryInfo pt = makePoint(0.0, 0.0);
    const GeometryInfo result = backend()->stBuffer(pt, -100.0, 36);
    // Non-positive distance should return an empty/invalid geometry.
    EXPECT_FALSE(result.isPolygon())
        << "Negative distance should produce an empty result";
}

TEST_P(StBufferTest, ZeroDistance_ReturnsEmpty) {
    const GeometryInfo pt = makePoint(0.0, 0.0);
    const GeometryInfo result = backend()->stBuffer(pt, 0.0, 36);
    EXPECT_FALSE(result.isPolygon())
        << "Zero distance should produce an empty result";
}

TEST_P(StBufferTest, EmptyPoint_ReturnsEmpty) {
    GeometryInfo empty_pt(GeometryType::Point);
    // coords intentionally left empty
    const GeometryInfo result = backend()->stBuffer(empty_pt, 500.0, 36);
    EXPECT_FALSE(result.isPolygon())
        << "Buffering an empty point geometry should return an empty result";
}

TEST_P(StBufferTest, ArcPointsClampedToMinimum) {
    const GeometryInfo pt = makePoint(0.0, 0.0);
    const GeometryInfo result = backend()->stBuffer(pt, 100.0, 1); // below minimum
    ASSERT_TRUE(result.isPolygon());
    ASSERT_FALSE(result.rings.empty());
    // After clamping to 3, the ring has 3 unique + 1 closing vertex.
    EXPECT_EQ(result.rings[0].size(), 4u);
}

// ---------------------------------------------------------------------------
// Instantiation: run the suite against both backends
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    CpuExact, StBufferTest,
    ::testing::Values(getCpuExactBackend()),
    [](const testing::TestParamInfo<ISpatialComputeBackend*>&) {
        return "cpu_exact";
    });

INSTANTIATE_TEST_SUITE_P(
    GpuSpatial, StBufferTest,
    ::testing::Values(getGpuSpatialBackend()),
    [](const testing::TestParamInfo<ISpatialComputeBackend*>&) {
        return "gpu_spatial";
    });
