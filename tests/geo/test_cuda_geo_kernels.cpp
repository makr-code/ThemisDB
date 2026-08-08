/*
 * ThemisDB | File: test_cuda_geo_kernels.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// test_cuda_geo_kernels.cpp — Tests for CUDA geo kernel dispatch
//
// Validates the CUDA geospatial kernel dispatch implemented in
// src/acceleration/cuda/geo_kernels.cu and the CUDAGeoBackend wired in
// src/acceleration/cuda_backend.cpp.
//
// All structural tests run on any platform (no NVIDIA GPU required).
// Hardware-dependent paths are gracefully skipped when CUDA is unavailable.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"
#include "geo/gpu_kernel_dispatcher.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#include <cmath>
#include <cstdint>
#include <vector>

using namespace themis::acceleration;
using namespace themis::geo;

// =============================================================================
// Structural / compile-time tests (no GPU required)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CudaGeoKernelDispatch, GeoDispatchTable_AllSlotsPopulated) {
    CUDAGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();

    // Under THEMIS_ENABLE_CUDA every slot must be non-null; no GPU required.
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(CudaGeoKernelDispatch, GeoDispatchTable_NotAvailableWhenNotInitialized) {
    // populateGeoDispatch() must return valid function pointers even before
    // initialize() is called.  The isAvailable() check governs dispatch,
    // not the presence of function pointers.
    CUDAGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(CudaGeoKernelDispatch, GeoDispatchTableName_IsCUDA) {
    CUDAGeoBackend backend;
    EXPECT_STREQ(backend.name(), "CUDA");
}

TEST(CudaGeoKernelDispatch, GeoDispatchTableType_IsCUDA) {
    CUDAGeoBackend backend;
    EXPECT_EQ(backend.type(), BackendType::CUDA);
}

TEST(CudaGeoKernelDispatch, Capabilities_SupportsGeoOpsAndBatch) {
    CUDAGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

#else // !THEMIS_ENABLE_CUDA

// When CUDA is not enabled the dispatch table is all-null.
TEST(CudaGeoKernelDispatch, GeoDispatchTable_AllNullWhenCudaDisabled) {
    GeoKernelDispatch d;  // zero-initialized (all nullptr)
    EXPECT_EQ(d.launchDistance,    nullptr);
    EXPECT_EQ(d.launchContainment, nullptr);
}

#endif // THEMIS_ENABLE_CUDA

// =============================================================================
// GpuKernelDispatcher interface tests (THEMIS_GEO_CUDA path)
// =============================================================================

// Helper: build a square polygon as interleaved [lat, lon] × 4 vertices.
static std::vector<double> makeCudaSquarePoly(double lat0, double lon0,
                                               double lat1, double lon1) {
    return {lat0, lon0, lat0, lon1, lat1, lon1, lat1, lon0};
}

class CudaGeoKernelDispatcherTest : public ::testing::Test {
protected:
    // Empty dispatch table — always uses the CPU stub path.
    GeoKernelDispatch empty_table_{};
    GpuKernelDispatcher dispatcher_{empty_table_};
};

TEST_F(CudaGeoKernelDispatcherTest, EmptyTable_IsNotAvailable) {
    EXPECT_FALSE(dispatcher_.isAvailable());
}

TEST_F(CudaGeoKernelDispatcherTest, EmptyTable_ContainmentReturnsNotDispatched) {
    const double lat = 1.0, lon = 1.0;
    auto poly = makeCudaSquarePoly(0.0, 0.0, 2.0, 2.0);
    auto res = dispatcher_.dispatchContainment(&lat, &lon, 1, poly.data(), 4);
    EXPECT_FALSE(res.dispatched);
    EXPECT_TRUE(res.mask.empty());
}

TEST_F(CudaGeoKernelDispatcherTest, EmptyTable_DistanceReturnsNotDispatched) {
    const double lat1 = 0.0, lon1 = 0.0, lat2 = 1.0, lon2 = 1.0;
    auto res = dispatcher_.dispatchDistance(&lat1, &lon1, &lat2, &lon2, 1,
                                             GeoDistanceFormula::HAVERSINE);
    EXPECT_FALSE(res.dispatched);
    EXPECT_TRUE(res.distances_km.empty());
}

TEST_F(CudaGeoKernelDispatcherTest, NullInputs_ContainmentReturnsNotDispatched) {
    auto res = dispatcher_.dispatchContainment(nullptr, nullptr, 1, nullptr, 3);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(CudaGeoKernelDispatcherTest, ZeroPoints_ContainmentReturnsNotDispatched) {
    const double poly_data[] = {0, 0, 0, 1, 1, 0};
    auto res = dispatcher_.dispatchContainment(nullptr, nullptr, 0, poly_data, 3);
    EXPECT_FALSE(res.dispatched);
}

TEST_F(CudaGeoKernelDispatcherTest, TooFewPolygonVertices_ContainmentReturnsNotDispatched) {
    const double lat = 0.5, lon = 0.5;
    const double poly[] = {0, 0, 1, 0};  // only 2 vertices — invalid polygon
    auto res = dispatcher_.dispatchContainment(&lat, &lon, 1, poly, 2);
    EXPECT_FALSE(res.dispatched);
}

// =============================================================================
// THEMIS_GEO_CUDA — populate dispatch table from CUDA backend
// =============================================================================

#if defined(THEMIS_GEO_CUDA) && defined(THEMIS_ENABLE_CUDA)

TEST(CudaGeoKernelDispatch, WithCudaTable_IsAvailable) {
    CUDAGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();

    GpuKernelDispatcher dispatcher(d);
    EXPECT_TRUE(dispatcher.isAvailable());
}

TEST(CudaGeoKernelDispatch, WithCudaTable_ContainmentReturnsNotDispatchedWithoutDevice) {
    // On a machine without an NVIDIA GPU, dispatch fails through the
    // cuda*Malloc path.  On a machine WITH a GPU this test must be skipped.
    CUDAGeoBackend backend;
    if (backend.isAvailable()) {
        GTEST_SKIP() << "capability:no_cuda_device_path_exercisable=false;reason=cuda_device_present";
    }

    GeoKernelDispatch d = backend.populateGeoDispatch();
    GpuKernelDispatcher dispatcher(d);

    const double lat = 1.0, lon = 1.0;
    auto poly = makeCudaSquarePoly(0.0, 0.0, 2.0, 2.0);
    auto res = dispatcher.dispatchContainment(&lat, &lon, 1, poly.data(), 4);
    // Without a real CUDA device cudaMalloc fails → dispatched=false is expected.
    EXPECT_FALSE(res.dispatched);
}

TEST(CudaGeoKernelDispatch, WithCudaTable_DistanceReturnsNotDispatchedWithoutDevice) {
    CUDAGeoBackend backend;
    if (backend.isAvailable()) {
        GTEST_SKIP() << "capability:no_cuda_device_path_exercisable=false;reason=cuda_device_present";
    }

    GeoKernelDispatch d = backend.populateGeoDispatch();
    GpuKernelDispatcher dispatcher(d);

    const double lat1 = 0.0, lon1 = 0.0, lat2 = 1.0, lon2 = 1.0;
    auto res = dispatcher.dispatchDistance(&lat1, &lon1, &lat2, &lon2, 1,
                                            GeoDistanceFormula::HAVERSINE);
    EXPECT_FALSE(res.dispatched);
}

TEST(CudaGeoKernelDispatch, WithCudaTable_VincentyDistanceReturnsNotDispatchedWithoutDevice) {
    CUDAGeoBackend backend;
    if (backend.isAvailable()) {
        GTEST_SKIP() << "capability:no_cuda_device_path_exercisable=false;reason=cuda_device_present";
    }

    GeoKernelDispatch d = backend.populateGeoDispatch();
    GpuKernelDispatcher dispatcher(d);

    const double lat1 = 0.0, lon1 = 0.0, lat2 = 1.0, lon2 = 1.0;
    auto res = dispatcher.dispatchDistance(&lat1, &lon1, &lat2, &lon2, 1,
                                            GeoDistanceFormula::VINCENTY);
    EXPECT_FALSE(res.dispatched);
}

#endif // THEMIS_GEO_CUDA && THEMIS_ENABLE_CUDA
