// test_hip_geo_kernels.cpp — Tests for ROCm/HIP geo kernel dispatch
//
// Validates the HIP/ROCm geospatial kernel dispatch implemented in
// src/acceleration/hip/geo_kernels.hip and the HIPGeoBackend wired in
// src/acceleration/hip_backend.cpp.
//
// All structural tests run on any platform (no AMD GPU required).
// Hardware-dependent paths are gracefully skipped when HIP is unavailable.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"
#include "geo/gpu_kernel_dispatcher.h"

#ifdef THEMIS_ENABLE_HIP
#include "acceleration/hip_backend.h"
#endif

#include <cmath>
#include <cstdint>
#include <vector>

using namespace themis::acceleration;
using namespace themis::geo;

// =============================================================================
// Structural / compile-time tests (no GPU required)
// =============================================================================

#ifdef THEMIS_ENABLE_HIP

TEST(HipGeoKernels, GeoDispatchTable_AllSlotsPopulated) {
    HIPGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();

    // Under THEMIS_ENABLE_HIP every slot must be non-null; no GPU required.
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(HipGeoKernels, GeoDispatchTable_NotAvailableWhenNotInitialized) {
    // populateGeoDispatch() must return valid function pointers even before
    // initialize() is called.  The isAvailable() check governs dispatch,
    // not the presence of function pointers.
    HIPGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(HipGeoKernels, GeoDispatchTableName_IsHIP) {
    HIPGeoBackend backend;
    EXPECT_STREQ(backend.name(), "HIP");
}

TEST(HipGeoKernels, GeoDispatchTableType_IsHIP) {
    HIPGeoBackend backend;
    EXPECT_EQ(backend.type(), BackendType::HIP);
}

TEST(HipGeoKernels, Capabilities_SupportsGeoOpsAndBatch) {
    HIPGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

#else // !THEMIS_ENABLE_HIP

// When HIP is not enabled the dispatch table is all-null.
TEST(HipGeoKernels, GeoDispatchTable_AllNullWhenHipDisabled) {
    GeoKernelDispatch d;  // zero-initialized (all nullptr)
    EXPECT_EQ(d.launchDistance,    nullptr);
    EXPECT_EQ(d.launchContainment, nullptr);
}

#endif // THEMIS_ENABLE_HIP

// =============================================================================
// GpuKernelDispatcher interface tests (THEMIS_GEO_HIP path)
// =============================================================================

// Helper: build a square polygon as interleaved [lat, lon] × 4 vertices.
static std::vector<double> makeSquarePoly(double lat0, double lon0,
                                           double lat1, double lon1) {
    return {lat0, lon0, lat0, lon1, lat1, lon1, lat1, lon0};
}

class HipGeoKernelDispatcherTest : public ::testing::Test {
protected:
    // Empty dispatch table — always uses the CPU stub path.
    GeoKernelDispatch empty_table_{};
    GpuKernelDispatcher dispatcher_{empty_table_};
};

TEST_F(HipGeoKernelDispatcherTest, EmptyTable_IsNotAvailable) {
    EXPECT_FALSE(dispatcher_.isAvailable());
}

TEST_F(HipGeoKernelDispatcherTest, EmptyTable_ContainmentReturnsNotDispatched) {
    const double lat = 1.0, lon = 1.0;
    auto poly = makeSquarePoly(0.0, 0.0, 2.0, 2.0);
    auto res = dispatcher_.dispatchContainment(&lat, &lon, 1, poly.data(), 4);
    EXPECT_FALSE(res.dispatched);
    EXPECT_TRUE(res.mask.empty());
}

TEST_F(HipGeoKernelDispatcherTest, EmptyTable_DistanceReturnsNotDispatched) {
    const double lat1 = 0.0, lon1 = 0.0, lat2 = 1.0, lon2 = 1.0;
    auto res = dispatcher_.dispatchDistance(&lat1, &lon1, &lat2, &lon2, 1,
                                             GeoDistanceFormula::HAVERSINE);
    EXPECT_FALSE(res.dispatched);
    EXPECT_TRUE(res.distances_km.empty());
}

TEST_F(HipGeoKernelDispatcherTest, NullInputs_ContainmentReturnsNotDispatched) {
    auto res = dispatcher_.dispatchContainment(nullptr, nullptr, 1, nullptr, 3);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(HipGeoKernelDispatcherTest, ZeroPoints_ContainmentReturnsNotDispatched) {
    const double poly_data[] = {0, 0, 0, 1, 1, 0};
    auto res = dispatcher_.dispatchContainment(nullptr, nullptr, 0, poly_data, 3);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(HipGeoKernelDispatcherTest, TooFewPolygonVertices_ContainmentReturnsNotDispatched) {
    const double lat = 0.5, lon = 0.5;
    const double poly[] = {0, 0, 1, 0};  // only 2 vertices — invalid polygon
    auto res = dispatcher_.dispatchContainment(&lat, &lon, 1, poly, 2);
    EXPECT_FALSE(res.dispatched);
}

// =============================================================================
// THEMIS_GEO_HIP — populate dispatch table from HIP backend
// =============================================================================

#if defined(THEMIS_GEO_HIP) && defined(THEMIS_ENABLE_HIP)

TEST(HipGeoKernelDispatch, WithHipTable_IsAvailable) {
    HIPGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();

    GpuKernelDispatcher dispatcher(d);
    EXPECT_TRUE(dispatcher.isAvailable());
}

TEST(HipGeoKernelDispatch, WithHipTable_ContainmentReturnsNotDispatchedWithoutDevice) {
    // On a machine without an AMD GPU, dispatch falls back through the circuit-
    // breaker.  On a machine WITH a GPU this test must be skipped or adapted.
    HIPGeoBackend backend;
    if (backend.isAvailable()) {
        GTEST_SKIP() << "capability:no_hip_device_path_exercisable=false;reason=amd_gpu_present";
    }

    GeoKernelDispatch d = backend.populateGeoDispatch();
    GpuKernelDispatcher dispatcher(d);

    const double lat = 1.0, lon = 1.0;
    auto poly = makeSquarePoly(0.0, 0.0, 2.0, 2.0);
    auto res = dispatcher.dispatchContainment(&lat, &lon, 1, poly.data(), 4);
    // Without a real HIP device hipMalloc fails → dispatched=false is expected.
    EXPECT_FALSE(res.dispatched);
}

TEST(HipGeoKernelDispatch, WithHipTable_DistanceReturnsNotDispatchedWithoutDevice) {
    HIPGeoBackend backend;
    if (backend.isAvailable()) {
        GTEST_SKIP() << "capability:no_hip_device_path_exercisable=false;reason=amd_gpu_present";
    }

    GeoKernelDispatch d = backend.populateGeoDispatch();
    GpuKernelDispatcher dispatcher(d);

    const double lat1 = 0.0, lon1 = 0.0, lat2 = 1.0, lon2 = 1.0;
    auto res = dispatcher.dispatchDistance(&lat1, &lon1, &lat2, &lon2, 1,
                                            GeoDistanceFormula::HAVERSINE);
    EXPECT_FALSE(res.dispatched);
}

#endif // THEMIS_GEO_HIP && THEMIS_ENABLE_HIP
