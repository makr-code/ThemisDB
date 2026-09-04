// Test: CUDA-Accelerated Geospatial Backend
//
// Validates the CUDA-accelerated geospatial implementation in CUDAGeoBackend.
// All structural tests run on any platform (no GPU required); hardware-dependent
// paths are skipped gracefully when CUDA is unavailable.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/kernel_invocation.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#include <cmath>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// Structural / compile-time tests (no GPU required)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CudaGeoBackend, DispatchTableStructure_BothSlotsNonNull) {
    // Both slots must be populated from extern "C" function pointers that exist
    // at link time — no GPU hardware is needed to verify this.
    CUDAGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(CudaGeoBackend, Capabilities_SupportsGeoOps) {
    CUDAGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
}

TEST(CudaGeoBackend, Capabilities_SupportedPrecisionsFP32Set) {
    CUDAGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(CudaGeoBackend, Capabilities_SupportsBatchAndAsync) {
    CUDAGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_TRUE(caps.supportsAsync);
}

// =============================================================================
// Input-validation tests (no GPU required — validate guard paths)
// =============================================================================

TEST(CudaGeoBackend, BatchDistances_NullInputReturnsEmpty) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    auto result = backend.batchDistances(nullptr, nullptr, nullptr, nullptr, 5, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaGeoBackend, BatchDistances_ZeroCountReturnsEmpty) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const double lats[] = {51.5074};
    const double lons[] = {-0.1278};
    auto result = backend.batchDistances(lats, lons, lats, lons, 0, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaGeoBackend, BatchPointInPolygon_NullInputReturnsEmpty) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const double poly[] = {0, 0, 0, 1, 1, 1, 1, 0};  // 4 vertices
    auto result = backend.batchPointInPolygon(nullptr, nullptr, 3, poly, 4);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaGeoBackend, BatchPointInPolygon_TooFewVerticesReturnsEmpty) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const double pLats[] = {0.5};
    const double pLons[] = {0.5};
    const double poly[]  = {0, 0, 0, 1};  // only 2 vertices — polygon requires >= 3
    auto result = backend.batchPointInPolygon(pLats, pLons, 1, poly, 2);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

// =============================================================================
// GPU-hardware-dependent end-to-end tests
// =============================================================================

TEST(CudaGeoBackend, BatchDistances_Haversine_SamePointIsZero) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // Distance from a point to itself must be 0 km
    const double lats[] = {51.5074};
    const double lons[] = {-0.1278};
    auto result = backend.batchDistances(lats, lons, lats, lons, 1, true);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_NEAR(result[0], 0.f, 1e-3f);

    backend.shutdown();
}

TEST(CudaGeoBackend, BatchDistances_Haversine_LondonParis) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // London (51.5074°N, 0.1278°W) to Paris (48.8566°N, 2.3522°E)
    // Haversine distance ≈ 340 km (±2 km tolerance)
    const double lats1[] = {51.5074};
    const double lons1[] = {-0.1278};
    const double lats2[] = {48.8566};
    const double lons2[] = {2.3522};

    auto result = backend.batchDistances(lats1, lons1, lats2, lons2, 1, true);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_NEAR(result[0], 340.f, 2.f);

    backend.shutdown();
}

TEST(CudaGeoBackend, BatchDistances_Haversine_BatchOf2) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // Pair 0: same point → 0 km; Pair 1: London→Paris ≈ 340 km
    const double lats1[] = {51.5074, 51.5074};
    const double lons1[] = {-0.1278, -0.1278};
    const double lats2[] = {51.5074, 48.8566};
    const double lons2[] = {-0.1278,  2.3522};

    auto result = backend.batchDistances(lats1, lons1, lats2, lons2, 2, true);

    ASSERT_EQ(result.size(), 2u);
    EXPECT_NEAR(result[0],   0.f, 1e-3f);
    EXPECT_NEAR(result[1], 340.f, 2.f);

    backend.shutdown();
}

TEST(CudaGeoBackend, BatchPointInPolygon_InsideAndOutside) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // Unit square polygon (lat from 0..1, lon from 0..1):
    //   vertices: [lat=0,lon=0], [lat=0,lon=1], [lat=1,lon=1], [lat=1,lon=0]
    // interleaved as [lat0,lon0, lat1,lon1, lat2,lon2, lat3,lon3]
    const double polygon[] = {0.0, 0.0,  0.0, 1.0,  1.0, 1.0,  1.0, 0.0};

    // Point (0.5, 0.5) is inside; point (2.0, 2.0) is outside
    const double pLats[] = {0.5, 2.0};
    const double pLons[] = {0.5, 2.0};

    auto result = backend.batchPointInPolygon(pLats, pLons, 2, polygon, 4);

    ASSERT_EQ(result.size(), 2u);
    EXPECT_TRUE(result[0]);   // (0.5, 0.5) is inside
    EXPECT_FALSE(result[1]);  // (2.0, 2.0) is outside

    backend.shutdown();
}

TEST(CudaGeoBackend, BatchPointInPolygon_AllInside) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // Unit square polygon
    const double polygon[] = {0.0, 0.0,  0.0, 1.0,  1.0, 1.0,  1.0, 0.0};

    const double pLats[] = {0.1, 0.5, 0.9};
    const double pLons[] = {0.1, 0.5, 0.9};

    auto result = backend.batchPointInPolygon(pLats, pLons, 3, polygon, 4);

    ASSERT_EQ(result.size(), 3u);
    EXPECT_TRUE(result[0]);
    EXPECT_TRUE(result[1]);
    EXPECT_TRUE(result[2]);

    backend.shutdown();
}

// =============================================================================
// Dispatch-table round-trip: geo dispatch launchers
// =============================================================================

TEST(CudaGeoBackend, GeoDispatch_DistanceSlotIsNonNullOnGPU) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);

    backend.shutdown();
}

#endif // THEMIS_ENABLE_CUDA

// =============================================================================
// CPU-backend parity checks (always run, no GPU required)
// =============================================================================

TEST(CudaGeoBackend, CpuParity_HaversineDistancesMatch) {
    // Verify CPU geo dispatch produces expected Haversine distances.
    // This serves as a reference baseline against which CUDA results are compared.
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchDistance, nullptr);

    // London (51.5074, -0.1278) to Paris (48.8566, 2.3522) ≈ 343.5 km
    const double lats1[] = {51.5074, 51.5074};
    const double lons1[] = {-0.1278, -0.1278};
    const double lats2[] = {51.5074, 48.8566};
    const double lons2[] = {-0.1278,  2.3522};
    float dists[2] = {};

    const int rc = d.launchDistance(lats1, lons1, lats2, lons2, dists,
                                    2, GeoDistanceFormula::HAVERSINE, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dists[0],   0.f, 1e-3f);   // same point → 0 km
    EXPECT_NEAR(dists[1], 343.5f, 1.0f);   // London → Paris great-circle distance

    backend.shutdown();
}

TEST(CudaGeoBackend, CpuParity_PointInPolygonDispatch_InsideAndOutside) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchContainment, nullptr);

    // Unit square polygon: vertices [0,0], [0,1], [1,1], [1,0]
    const double polygon[] = {0.0, 0.0,  0.0, 1.0,  1.0, 1.0,  1.0, 0.0};
    const double pLats[]   = {0.5, 2.0};
    const double pLons[]   = {0.5, 2.0};
    uint8_t results[2]     = {};

    const int rc = d.launchContainment(pLats, pLons, 2,
                                       polygon, 4,
                                       results, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(results[0], 0u);  // (0.5, 0.5) inside
    EXPECT_EQ(results[1], 0u);  // (2.0, 2.0) outside

    backend.shutdown();
}
