#include <gtest/gtest.h>

#include "geo/gpu_kernel_dispatcher.h"
#include "geo/spatial_backend.h"
#include "acceleration/kernel_invocation.h"

#include <cmath>
#include <vector>

using namespace themis::geo;
using themis::acceleration::GeoKernelDispatch;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr double kPi = 3.14159265358979323846;

/// Build a square polygon: four corners of a [minLat, maxLat] × [minLon, maxLon] box.
/// Returns interleaved [lat, lon] array, length = 4.
static std::vector<double> makeSquarePolygon(double lat0, double lon0,
                                              double lat1, double lon1) {
    return {lat0, lon0, lat0, lon1, lat1, lon1, lat1, lon0};
}

// ---------------------------------------------------------------------------
// GpuKernelDispatcher interface tests (always run regardless of CUDA)
// ---------------------------------------------------------------------------

class GpuKernelDispatcherTest : public ::testing::Test {
protected:
    // Empty dispatch table → CPU stub behaviour.
    GeoKernelDispatch empty_table_{};
    GpuKernelDispatcher dispatcher_{empty_table_};
};

TEST_F(GpuKernelDispatcherTest, EmptyTable_IsNotAvailable) {
    // With a null dispatch table the dispatcher must report unavailable.
    EXPECT_FALSE(dispatcher_.isAvailable());
}

TEST_F(GpuKernelDispatcherTest, ContainmentDispatch_EmptyTable_ReturnsFalse) {
    std::vector<double> lats  = {0.5, 1.5};
    std::vector<double> lons  = {0.5, 0.5};
    auto poly = makeSquarePolygon(0.0, 0.0, 1.0, 1.0);

    auto res = dispatcher_.dispatchContainment(
        lats.data(), lons.data(), static_cast<int>(lats.size()),
        poly.data(), static_cast<int>(poly.size() / 2));

    EXPECT_FALSE(res.dispatched);
    EXPECT_TRUE(res.mask.empty());
}

TEST_F(GpuKernelDispatcherTest, DistanceDispatch_EmptyTable_ReturnsFalse) {
    std::vector<double> lats1 = {0.0}, lons1 = {0.0};
    std::vector<double> lats2 = {1.0}, lons2 = {0.0};

    auto res = dispatcher_.dispatchDistance(
        lats1.data(), lons1.data(),
        lats2.data(), lons2.data(),
        static_cast<int>(lats1.size()));

    EXPECT_FALSE(res.dispatched);
    EXPECT_TRUE(res.distances_km.empty());
}

TEST_F(GpuKernelDispatcherTest, ContainmentDispatch_NullPointers_ReturnsFalse) {
    auto poly = makeSquarePolygon(0.0, 0.0, 1.0, 1.0);
    auto res  = dispatcher_.dispatchContainment(
        nullptr, nullptr, 2, poly.data(), 4);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(GpuKernelDispatcherTest, ContainmentDispatch_ZeroPoints_ReturnsFalse) {
    std::vector<double> lats, lons;
    auto poly = makeSquarePolygon(0.0, 0.0, 1.0, 1.0);
    auto res  = dispatcher_.dispatchContainment(
        lats.data(), lons.data(), 0, poly.data(), 4);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(GpuKernelDispatcherTest, ContainmentDispatch_TooFewPolygonVertices_ReturnsFalse) {
    std::vector<double> lats = {0.5}, lons = {0.5};
    std::vector<double> poly = {0.0, 0.0, 1.0, 1.0}; // only 2 vertices
    auto res = dispatcher_.dispatchContainment(
        lats.data(), lons.data(), 1, poly.data(), 2);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(GpuKernelDispatcherTest, DistanceDispatch_NullPointers_ReturnsFalse) {
    auto res = dispatcher_.dispatchDistance(
        nullptr, nullptr, nullptr, nullptr, 1);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(GpuKernelDispatcherTest, DistanceDispatch_ZeroCount_ReturnsFalse) {
    std::vector<double> lats1, lons1, lats2, lons2;
    auto res = dispatcher_.dispatchDistance(
        lats1.data(), lons1.data(), lats2.data(), lons2.data(), 0);
    EXPECT_FALSE(res.dispatched);
}

#ifndef THEMIS_GEO_CUDA
TEST_F(GpuKernelDispatcherTest, BridgeCallbacksEnableCpuDispatcher) {
    GpuKernelDispatcher::setContainmentDispatchFn(
        [](const double*, const double*, int num_points, const double*, int) {
            GpuKernelDispatcher::ContainmentResult result;
            result.dispatched = true;
            result.mask.assign(static_cast<size_t>(num_points), 1);
            return result;
        });
    GpuKernelDispatcher::setDistanceDispatchFn(
        [](const double*, const double*, const double*, const double*, int count,
           themis::acceleration::GeoDistanceFormula) {
            GpuKernelDispatcher::DistanceResult result;
            result.dispatched = true;
            result.distances_km.assign(static_cast<size_t>(count), 42.0f);
            return result;
        });

    EXPECT_TRUE(dispatcher_.isAvailable());

    std::vector<double> lats  = {0.5, 1.5};
    std::vector<double> lons  = {0.5, 0.5};
    auto poly = makeSquarePolygon(0.0, 0.0, 1.0, 1.0);
    auto containment = dispatcher_.dispatchContainment(
        lats.data(), lons.data(), static_cast<int>(lats.size()),
        poly.data(), static_cast<int>(poly.size() / 2));
    ASSERT_TRUE(containment.dispatched);
    ASSERT_EQ(containment.mask.size(), 2u);
    EXPECT_EQ(containment.mask[0], 1u);

    auto distance = dispatcher_.dispatchDistance(
        lats.data(), lons.data(), lats.data(), lons.data(), static_cast<int>(lats.size()));
    ASSERT_TRUE(distance.dispatched);
    ASSERT_EQ(distance.distances_km.size(), 2u);
    EXPECT_FLOAT_EQ(distance.distances_km[0], 42.0f);

    GpuKernelDispatcher::setContainmentDispatchFn(nullptr);
    GpuKernelDispatcher::setDistanceDispatchFn(nullptr);
}
#endif

// ---------------------------------------------------------------------------
// GpuBatchBackend contract — same results via both backends
// ---------------------------------------------------------------------------

/// These tests validate that GpuBatchBackend (gpu_spatial) produces the same
/// intersection results as getCpuExactBackend() for typical point-in-polygon
/// queries regardless of whether a GPU is present.  When THEMIS_GEO_CUDA is
/// defined and a GPU is available, the results come from the CUDA kernel;
/// otherwise they come from the CPU fallback path.

class GpuBatchContainmentTest : public ::testing::Test {
protected:
    ISpatialComputeBackend* cpu_backend_ = getCpuExactBackend();
    ISpatialComputeBackend* gpu_backend_ = getGpuSpatialBackend();

    static GeometryInfo makePoint(double x, double y) {
        GeometryInfo g(GeometryType::Point);
        g.coords.push_back({x, y});
        return g;
    }

    static GeometryInfo makeSquare(double lat0, double lon0,
                                    double lat1, double lon1) {
        GeometryInfo g(GeometryType::Polygon);
        std::vector<Coordinate> ring = {
            {lat0, lon0}, {lat0, lon1}, {lat1, lon1}, {lat1, lon0}, {lat0, lon0}
        };
        g.rings.push_back(ring);
        return g;
    }
};

TEST_F(GpuBatchContainmentTest, BatchIntersects_PointsVsPolygon_AgreesWithCpu) {
    // Square [0,0]–[1,1].
    const GeometryInfo square = makeSquare(0.0, 0.0, 1.0, 1.0);

    // Mixed: some inside, some outside.
    const std::vector<std::pair<double,double>> test_pts = {
        {0.5, 0.5},   // inside
        {0.1, 0.1},   // inside
        {1.5, 0.5},   // outside
        {-0.1, 0.5},  // outside
        {0.9, 0.9},   // inside
    };

    SpatialBatchInputs batch;
    batch.count = test_pts.size();
    for (const auto& p : test_pts) {
        batch.geoms_a.push_back(makePoint(p.first, p.second));
        batch.geoms_b.push_back(square);
    }

    const auto cpu_res = cpu_backend_->batchIntersects(batch);
    const auto gpu_res = gpu_backend_->batchIntersects(batch);

    ASSERT_EQ(cpu_res.mask.size(), test_pts.size());
    ASSERT_EQ(gpu_res.mask.size(), test_pts.size());
    for (std::size_t i = 0; i < test_pts.size(); ++i) {
        EXPECT_EQ(cpu_res.mask[i], gpu_res.mask[i])
            << "Mismatch at index " << i
            << " (" << test_pts[i].first << ", " << test_pts[i].second << ")";
    }
}

TEST_F(GpuBatchContainmentTest, BatchIntersects_EmptyBatch_ReturnsEmptyMask) {
    SpatialBatchInputs empty;
    empty.count = 0;

    auto cpu_res = cpu_backend_->batchIntersects(empty);
    auto gpu_res = gpu_backend_->batchIntersects(empty);

    EXPECT_TRUE(cpu_res.mask.empty());
    EXPECT_TRUE(gpu_res.mask.empty());
}

TEST_F(GpuBatchContainmentTest, GpuStatsJson_ContainsExpectedFields) {
    const std::string json = getGpuSpatialBackendStatsJson();
    EXPECT_NE(json.find("\"backend_name\":\"gpu_spatial\""), std::string::npos);
    EXPECT_NE(json.find("\"gpu_present\":"),                std::string::npos);
    EXPECT_NE(json.find("\"circuit_open\":"),               std::string::npos);
    EXPECT_NE(json.find("\"gpu_kernel_available\":"),       std::string::npos);
}
