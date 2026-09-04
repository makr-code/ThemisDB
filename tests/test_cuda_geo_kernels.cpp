// Tests for CUDA geospatial distance and containment kernels.
//
// These tests validate the CUDAGeoBackend implementation introduced in
// src/acceleration/cuda_backend.cpp.  When CUDA hardware is unavailable
// the CUDA-specific cases are skipped via GTEST_SKIP; the CPU-reference
// cases run unconditionally and verify the algorithmic correctness of the
// haversine and ray-casting formulas that mirror the CUDA kernels.

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

// ============================================================================
// Helper constants
// ============================================================================

// Paris (48.8566 N, 2.3522 E) to London (51.5074 N, 0.1278 W) ≈ 340 km
static constexpr double kPariLat  = 48.8566;
static constexpr double kPariLon  = 2.3522;
static constexpr double kLondLat  = 51.5074;
static constexpr double kLondLon  = -0.1278;
static constexpr float  kPariLondKm = 340.f;
static constexpr float  kDistTol    = 10.f;   // ±10 km tolerance

// Simple square polygon corners (interleaved lat,lon):
//   (0,0) → (0,2) → (2,2) → (2,0)
static const double kSquarePoly[] = {0.0, 0.0,  0.0, 2.0,  2.0, 2.0,  2.0, 0.0};
static constexpr int kSquareVerts = 4;

// ============================================================================
// CPU reference correctness tests (run on every platform)
// ============================================================================

TEST(CudaGeoKernels, CPUReference_HaversineParisLondon) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double lats1[] = {kPariLat};
    const double lons1[] = {kPariLon};
    const double lats2[] = {kLondLat};
    const double lons2[] = {kLondLon};

    auto distances = backend.batchDistances(lats1, lons1, lats2, lons2, 1, true);
    ASSERT_EQ(distances.size(), 1u);
    EXPECT_NEAR(distances[0], kPariLondKm, kDistTol);

    backend.shutdown();
}

TEST(CudaGeoKernels, CPUReference_PointInPolygon_InsideAndOutside) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double pLats[] = {1.0, 3.0};
    const double pLons[] = {1.0, 3.0};

    auto results = backend.batchPointInPolygon(pLats, pLons, 2, kSquarePoly, kSquareVerts);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0]);   // (1,1) is inside the square
    EXPECT_FALSE(results[1]);  // (3,3) is outside the square

    backend.shutdown();
}

TEST(CudaGeoKernels, CPUReference_PointInPolygon_EdgeCases) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Corner point — ray-cast may classify as inside or outside depending on
    // implementation; we only verify no crash and a bool is returned.
    const double pLats[] = {0.0};
    const double pLons[] = {0.0};

    auto results = backend.batchPointInPolygon(pLats, pLons, 1, kSquarePoly, kSquareVerts);
    ASSERT_EQ(results.size(), 1u);

    backend.shutdown();
}

TEST(CudaGeoKernels, CPUReference_BatchDistances_ZeroDistance) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Same point should give ~0 distance
    const double lats1[] = {48.8566};
    const double lons1[] = {2.3522};
    const double lats2[] = {48.8566};
    const double lons2[] = {2.3522};

    auto distances = backend.batchDistances(lats1, lons1, lats2, lons2, 1, true);
    ASSERT_EQ(distances.size(), 1u);
    EXPECT_NEAR(distances[0], 0.f, 1e-3f);

    backend.shutdown();
}

TEST(CudaGeoKernels, CPUReference_BatchDistances_MultiplePairs) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Two pairs: Paris→London and same-point (0 distance)
    const double lats1[] = {kPariLat, 0.0};
    const double lons1[] = {kPariLon, 0.0};
    const double lats2[] = {kLondLat, 0.0};
    const double lons2[] = {kLondLon, 0.0};

    auto distances = backend.batchDistances(lats1, lons1, lats2, lons2, 2, true);
    ASSERT_EQ(distances.size(), 2u);
    EXPECT_NEAR(distances[0], kPariLondKm, kDistTol);
    EXPECT_NEAR(distances[1], 0.f, 1e-3f);

    backend.shutdown();
}

// ============================================================================
// CPU GeoDispatch — frozen-interface correctness tests
// ============================================================================

TEST(CudaGeoKernels, CPUDispatch_HaversineDistance) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchDistance, nullptr);

    const double lats1[] = {kPariLat};
    const double lons1[] = {kPariLon};
    const double lats2[] = {kLondLat};
    const double lons2[] = {kLondLon};
    float dist = 0.f;

    int rc = d.launchDistance(lats1, lons1, lats2, lons2, &dist, 1,
                              GeoDistanceFormula::HAVERSINE, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, kPariLondKm, kDistTol);

    backend.shutdown();
}

TEST(CudaGeoKernels, CPUDispatch_PointInPolygon) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchContainment, nullptr);

    const double pLats[] = {1.0, 3.0};
    const double pLons[] = {1.0, 3.0};
    uint8_t results[2]   = {0, 0};

    int rc = d.launchContainment(pLats, pLons, 2,
                                 kSquarePoly, kSquareVerts,
                                 results, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(results[0], 1u);  // inside
    EXPECT_EQ(results[1], 0u);  // outside

    backend.shutdown();
}

// ============================================================================
// CUDA backend structural tests (no GPU required)
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CudaGeoKernels, CUDAGeoBackend_GeoDispatchSlotsNonNull) {
    // The dispatch table must be populated (slots non-null) regardless of
    // whether a physical GPU is present, because the function pointers are
    // assigned unconditionally at compile time.
    CUDAGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(CudaGeoKernels, CUDAGeoBackend_CapabilitiesReportGeoOps) {
    CUDAGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

// ============================================================================
// CUDA backend functional tests (require physical GPU — skipped when absent)
// ============================================================================

TEST(CudaGeoKernels, CUDAGeoBackend_InitializeAndShutdown) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    EXPECT_TRUE(backend.initialize());
    backend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_HaversineParisLondon) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(backend.initialize());

    const double lats1[] = {kPariLat};
    const double lons1[] = {kPariLon};
    const double lats2[] = {kLondLat};
    const double lons2[] = {kLondLon};

    auto distances = backend.batchDistances(lats1, lons1, lats2, lons2, 1, true);
    ASSERT_EQ(distances.size(), 1u);
    EXPECT_NEAR(distances[0], kPariLondKm, kDistTol);

    backend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_BatchDistances_MultiplePairs) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(backend.initialize());

    const double lats1[] = {kPariLat, 0.0};
    const double lons1[] = {kPariLon, 0.0};
    const double lats2[] = {kLondLat, 0.0};
    const double lons2[] = {kLondLon, 0.0};

    auto distances = backend.batchDistances(lats1, lons1, lats2, lons2, 2, true);
    ASSERT_EQ(distances.size(), 2u);
    EXPECT_NEAR(distances[0], kPariLondKm, kDistTol);
    EXPECT_NEAR(distances[1], 0.f, 1e-3f);

    backend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_PointInPolygon_InsideAndOutside) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(backend.initialize());

    const double pLats[] = {1.0, 3.0};
    const double pLons[] = {1.0, 3.0};

    auto results = backend.batchPointInPolygon(pLats, pLons, 2, kSquarePoly, kSquareVerts);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0]);   // (1,1) inside square
    EXPECT_FALSE(results[1]);  // (3,3) outside square

    backend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_BatchDistances_EmptyInput) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(backend.initialize());

    auto distances = backend.batchDistances(nullptr, nullptr, nullptr, nullptr, 0, true);
    EXPECT_TRUE(distances.empty());

    backend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_PointInPolygon_EmptyInput) {
    CUDAGeoBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(backend.initialize());

    auto results = backend.batchPointInPolygon(nullptr, nullptr, 0, kSquarePoly, kSquareVerts);
    EXPECT_TRUE(results.empty());

    backend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_ConsistencyWithCPU_Distances) {
    CUDAGeoBackend cudaBackend = {};
    if (!cudaBackend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(cudaBackend.initialize());

    CPUGeoBackend cpuBackend;
    ASSERT_TRUE(cpuBackend.initialize());

    // Three coordinate pairs
    const double lats1[] = {kPariLat, 40.7128, 35.6762};
    const double lons1[] = {kPariLon, -74.0060, 139.6503};
    const double lats2[] = {kLondLat, 51.5074, -33.8688};
    const double lons2[] = {kLondLon, -0.1278, 151.2093};

    auto cudaDist = cudaBackend.batchDistances(lats1, lons1, lats2, lons2, 3, true);
    auto cpuDist  = cpuBackend.batchDistances(lats1, lons1, lats2, lons2, 3, true);

    ASSERT_EQ(cudaDist.size(), 3u);
    ASSERT_EQ(cpuDist.size(), 3u);

    for (size_t i = 0; i < 3; ++i) {
        // Allow 0.1% relative error between CUDA and CPU results
        EXPECT_NEAR(cudaDist[i], cpuDist[i], cpuDist[i] * 0.001f + 0.1f)
            << "Pair " << i << ": CUDA=" << cudaDist[i] << " CPU=" << cpuDist[i];
    }

    cudaBackend.shutdown();
    cpuBackend.shutdown();
}

TEST(CudaGeoKernels, CUDAGeoBackend_ConsistencyWithCPU_Containment) {
    CUDAGeoBackend cudaBackend = {};
    if (!cudaBackend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    ASSERT_TRUE(cudaBackend.initialize());

    CPUGeoBackend cpuBackend;
    ASSERT_TRUE(cpuBackend.initialize());

    const double pLats[] = {1.0, 3.0, 0.5, 2.5};
    const double pLons[] = {1.0, 3.0, 1.8, 0.1};

    auto cudaRes = cudaBackend.batchPointInPolygon(pLats, pLons, 4, kSquarePoly, kSquareVerts);
    auto cpuRes  = cpuBackend.batchPointInPolygon(pLats, pLons, 4, kSquarePoly, kSquareVerts);

    ASSERT_EQ(cudaRes.size(), 4u);
    ASSERT_EQ(cpuRes.size(), 4u);

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(cudaRes[i], cpuRes[i]) << "Point " << i << " mismatch";
    }

    cudaBackend.shutdown();
    cpuBackend.shutdown();
}

#endif // THEMIS_ENABLE_CUDA
