// Test: HIP-Accelerated ANN Kernel Dispatch
//
// Validates the HIP/ROCm ANN (Approximate Nearest Neighbor) kernel dispatch
// table implemented in src/acceleration/hip/ann_kernels.hip.
//
// All structural tests run on any platform (no AMD GPU required).
// Hardware-dependent paths are gracefully skipped when HIP is unavailable.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"

#ifdef THEMIS_ENABLE_HIP
#include "acceleration/hip_backend.h"
#endif

#include <cmath>
#include <cstdint>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// Structural / compile-time tests (no GPU required)
// =============================================================================

#ifdef THEMIS_ENABLE_HIP

TEST(HipAnnKernels, DispatchTable_AllSlotsPopulated) {
    HIPVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();

    // Under THEMIS_ENABLE_HIP every slot must be non-null; no GPU required.
    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);
}

TEST(HipAnnKernels, DispatchTable_DistanceLauncherForRoutesAllMetrics) {
    HIPVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            d.launchL2Distance);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        d.launchCosine);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), d.launchInnerProduct);
}

TEST(HipAnnKernels, Capabilities_SupportsVectorOpsAndBatch) {
    HIPVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

TEST(HipAnnKernels, Capabilities_DeclaresFP32Precision) {
    HIPVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(HipAnnKernels, Capabilities_DeclaresANNMetrics) {
    HIPVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(HipAnnKernels, Capabilities_SatisfiesDefaultVectorRequirements) {
    HIPVectorBackend backend;
    EXPECT_TRUE(BackendRegistry::satisfies(
        backend.getCapabilities(),
        BackendRegistry::defaultVectorRequirements()));
}

TEST(HipAnnKernels, Backend_NameIsHIP) {
    HIPVectorBackend backend;
    EXPECT_STREQ(backend.name(), "HIP");
}

TEST(HipAnnKernels, Backend_TypeIsHIP) {
    HIPVectorBackend backend;
    EXPECT_EQ(backend.type(), BackendType::HIP);
}

// =============================================================================
// Input-validation tests (verify guard paths; no GPU required)
// =============================================================================

TEST(HipAnnKernels, ComputeDistances_NullQueryReturnsEmpty) {
    HIPVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }
    const float vectors[] = {1.f, 0.f};
    auto result = backend.computeDistances(nullptr, 1, 2, vectors, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(HipAnnKernels, ComputeDistances_ZeroDimReturnsEmpty) {
    HIPVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }
    const float q[] = {1.f};
    const float v[] = {1.f};
    auto result = backend.computeDistances(q, 1, 0, v, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(HipAnnKernels, BatchKnnSearch_ZeroKReturnsEmptyNeighbours) {
    HIPVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f, 0.f, 1.f};
    auto result = backend.batchKnnSearch(q, 1, 2, v, 2, 0, true);
    // When k=0 the backend should return one (empty) neighbour list per query.
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].empty());
    backend.shutdown();
}

// =============================================================================
// Hardware-dependent correctness tests (skip when no AMD GPU)
// =============================================================================

TEST(HipAnnKernels, L2Distance_CorrectValues_OnDevice) {
    HIPVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }

    // q0 = (1, 0), q1 = (0, 1)
    // v0 = (1, 0), v1 = (0, 1), v2 = (1, 1)
    const float queries[]  = {1.f, 0.f,  0.f, 1.f};
    const float vectors[]  = {1.f, 0.f,  0.f, 1.f,  1.f, 1.f};

    auto dist = backend.computeDistances(queries, 2, 2, vectors, 3, /*useL2=*/true);

    ASSERT_EQ(dist.size(), 6u);  // 2 queries × 3 vectors

    // q0 vs v0: squared L2 = 0
    EXPECT_NEAR(dist[0], 0.f, 1e-5f);
    // q0 vs v1: squared L2 = 1+1 = 2
    EXPECT_NEAR(dist[1], 2.f, 1e-5f);
    // q0 vs v2: squared L2 = 0+1 = 1
    EXPECT_NEAR(dist[2], 1.f, 1e-5f);
    // q1 vs v0: squared L2 = 1+1 = 2
    EXPECT_NEAR(dist[3], 2.f, 1e-5f);
    // q1 vs v1: squared L2 = 0
    EXPECT_NEAR(dist[4], 0.f, 1e-5f);
    // q1 vs v2: squared L2 = 1+0 = 1
    EXPECT_NEAR(dist[5], 1.f, 1e-5f);

    backend.shutdown();
}

TEST(HipAnnKernels, BatchKnnSearch_ReturnsTopKNeighbors) {
    HIPVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }

    // Query: (1, 0, 0, 0)
    // Vectors: v0=(1,0,0,0), v1=(0,1,0,0), v2=(0,0,1,0), v3=(0,0,0,1)
    const float q[] = {1.f, 0.f, 0.f, 0.f};
    const float v[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };

    auto results = backend.batchKnnSearch(q, 1, 4, v, 4, 2, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 2u);

    // Nearest should be v0 with distance 0, second nearest any of v1/v2/v3 with distance 2.
    EXPECT_EQ(results[0][0].first, 0u);
    EXPECT_NEAR(results[0][0].second, 0.f, 1e-5f);
    EXPECT_NEAR(results[0][1].second, 2.f, 1e-5f);

    backend.shutdown();
}

TEST(HipAnnKernels, DeviceInfo_WhenInitialized_IsPopulated) {
    HIPVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }

    auto info = backend.getDeviceInfo();
    EXPECT_FALSE(info.name.empty());
    EXPECT_GT(info.computeUnits, 0);
    EXPECT_GT(info.totalMemory,  0u);

    backend.shutdown();
}

// =============================================================================
// HIPGeoBackend structural tests (no GPU required)
// =============================================================================

TEST(HipGeoBackend, NameIsHIP) {
    HIPGeoBackend backend;
    EXPECT_STREQ(backend.name(), "HIP");
}

TEST(HipGeoBackend, TypeIsHIP) {
    HIPGeoBackend backend;
    EXPECT_EQ(backend.type(), BackendType::HIP);
}

TEST(HipGeoBackend, Capabilities_SupportsGeoOps) {
    HIPGeoBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

TEST(HipGeoBackend, GeoDispatch_AllSlotsPopulated) {
    HIPGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();

    // Under THEMIS_ENABLE_HIP both slots must be non-null; no GPU required.
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

// =============================================================================
// HIPGeoBackend hardware-dependent correctness tests (skip when no AMD GPU)
// =============================================================================

TEST(HipGeoBackend, BatchDistances_ZeroCountReturnsEmpty) {
    HIPGeoBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }
    auto result = backend.batchDistances(nullptr, nullptr, nullptr, nullptr, 0, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(HipGeoBackend, BatchPointInPolygon_ZeroPointsReturnsEmpty) {
    HIPGeoBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }
    auto result = backend.batchPointInPolygon(nullptr, nullptr, 0, nullptr, 0);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(HipGeoBackend, BatchDistances_HaversineKnownPair) {
    HIPGeoBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }

    // New York (40.7128 N, 74.0060 W) → Los Angeles (34.0522 N, 118.2437 W)
    // Approximate great-circle distance: ~3940 km
    const double lats1[] = {40.7128};
    const double lons1[] = {-74.0060};
    const double lats2[] = {34.0522};
    const double lons2[] = {-118.2437};

    auto dist = backend.batchDistances(lats1, lons1, lats2, lons2, 1, /*haversine=*/true);

    ASSERT_EQ(dist.size(), 1u);
    EXPECT_NEAR(dist[0], 3940.f, 100.f);  // ±100 km tolerance

    backend.shutdown();
}

TEST(HipGeoBackend, BatchPointInPolygon_InsideAndOutside) {
    HIPGeoBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_hardware_not_available";
    }

    // Unit square with vertices at (lat=0,lon=0)→(1,0)→(1,1)→(0,1)
    // Coordinates interleaved as [lat0, lon0, lat1, lon1, …] per GeoContainmentParams.
    const double polygon[] = {0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0};

    // Inside: (0.5, 0.5); Outside: (2.0, 2.0)
    const double pLats[] = {0.5, 2.0};
    const double pLons[] = {0.5, 2.0};

    auto result = backend.batchPointInPolygon(pLats, pLons, 2, polygon, 4);

    ASSERT_EQ(result.size(), 2u);
    EXPECT_TRUE(result[0]);   // inside
    EXPECT_FALSE(result[1]);  // outside

    backend.shutdown();
}

#endif // THEMIS_ENABLE_HIP

// =============================================================================
// No-HIP compile-time sanity test
// =============================================================================

TEST(HipAnnKernels, KernelInvocationHeader_InterfaceVersionIsStable) {
    // KERNEL_INVOCATION_INTERFACE_VERSION must remain 100 (v1.0) until a
    // breaking change is made.  This test protects against accidental bumps.
    EXPECT_EQ(themis::acceleration::KERNEL_INVOCATION_INTERFACE_VERSION, 100u);
}
