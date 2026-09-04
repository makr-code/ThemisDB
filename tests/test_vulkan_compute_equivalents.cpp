// Test: Vulkan Compute Equivalents
//
// Validates that the Vulkan backend provides compute equivalents matching
// the CUDA backend's capabilities:
//   - All three ANN distance metrics (L2, cosine, inner product) are supported
//   - ANNKernelDispatch table is fully populated under THEMIS_ENABLE_VULKAN
//   - VulkanGeoBackend exposes a fully populated GeoKernelDispatch table
//   - Capabilities correctly advertise supportedMetrics and supportedPrecisions
//   - Dispatch functions produce numerically correct results
//
// Tests gracefully skip when Vulkan hardware is unavailable so they run
// in CPU-only CI environments without false failures.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"
#include "acceleration/graphics_backends.h"

#include <cmath>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// VulkanVectorBackend capability tests (no hardware required)
// =============================================================================

#ifdef THEMIS_ENABLE_VULKAN

TEST(VulkanComputeEquivalents, VulkanVectorBackend_Capabilities_SupportedMetricsSet) {
    VulkanVectorBackend backend;

    auto caps = backend.getCapabilities();

    // All three ANN distance metrics must be advertised
    EXPECT_TRUE((caps.supportedMetrics & metricBit(DistanceMetric::L2)) != 0);
    EXPECT_TRUE((caps.supportedMetrics & metricBit(DistanceMetric::COSINE)) != 0);
    EXPECT_TRUE((caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT)) != 0);
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_Capabilities_SupportedPrecisionsSet) {
    VulkanVectorBackend backend;

    auto caps = backend.getCapabilities();

    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_Capabilities_VectorOpsEnabled) {
    VulkanVectorBackend backend;

    auto caps = backend.getCapabilities();

    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

// =============================================================================
// VulkanVectorBackend dispatch table tests (no hardware required)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanVectorBackend_PopulateANNDispatch_AllSlotsNonNull) {
    VulkanVectorBackend backend;

    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ANNDispatch_L2_CorrectResult) {
    VulkanVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchL2Distance, nullptr);

    // Query [1,0,0] vs vectors [1,0,0] and [0,1,0]
    // Expected squared-L2: 0, 2
    const float queries[] = {1.f, 0.f, 0.f};
    const float vectors[] = {1.f, 0.f, 0.f,  0.f, 1.f, 0.f};
    float distances[2]    = {};

    const int rc = d.launchL2Distance(queries, vectors, distances,
                                      1, 2, 3, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(distances[0], 0.f, 1e-5f);
    EXPECT_NEAR(distances[1], 2.f, 1e-5f);
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ANNDispatch_Cosine_CorrectResult) {
    VulkanVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchCosine, nullptr);

    // Identical unit vectors → cosine distance = 0
    const float v[] = {1.f, 0.f, 0.f};
    float dist = 0.f;

    const int rc = d.launchCosine(v, v, &dist, 1, 1, 3, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, 0.f, 1e-5f);
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ANNDispatch_InnerProduct_CorrectResult) {
    VulkanVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchInnerProduct, nullptr);

    // Query [1,0] vs vector [3,4] → dot = 3 → distance = -3
    const float queries[] = {1.f, 0.f};
    const float vectors[] = {3.f, 4.f};
    float dist = 0.f;

    const int rc = d.launchInnerProduct(queries, vectors, &dist, 1, 1, 2, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, -3.f, 1e-5f);
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ANNDispatch_TopK_SelectsSmallest) {
    VulkanVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchTopK, nullptr);

    // Distances: 3, 1, 4, 0 — top-2 should be idx 3 (0.0) and idx 1 (1.0)
    const float distances[4] = {3.f, 1.f, 4.f, 0.f};
    uint32_t indices[2] = {};
    float    dists[2]   = {};

    const int rc = d.launchTopK(distances, indices, dists, 1, 4, 2, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(indices[0], 3u);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_EQ(indices[1], 1u);
    EXPECT_NEAR(dists[1], 1.f, 1e-5f);
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ANNDispatch_DistanceLauncherFor_Routes) {
    VulkanVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            d.launchL2Distance);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        d.launchCosine);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), d.launchInnerProduct);
}

// =============================================================================
// VulkanGeoBackend capability tests (no hardware required)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanGeoBackend_Capabilities_GeoOpsEnabled) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));

    backend.shutdown();
}

// =============================================================================
// VulkanGeoBackend dispatch table tests (no hardware required)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanGeoBackend_PopulateGeoDispatch_AllSlotsNonNull) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_GeoDispatch_HaversineDistance) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchDistance, nullptr);

    // Paris (48.8566°N, 2.3522°E) → London (51.5074°N, 0.1278°W) ≈ 340 km
    const double lats1[] = {48.8566};
    const double lons1[] = { 2.3522};
    const double lats2[] = {51.5074};
    const double lons2[] = {-0.1278};
    float dist = 0.f;

    const int rc = d.launchDistance(lats1, lons1, lats2, lons2, &dist,
                                    1, GeoDistanceFormula::HAVERSINE, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, 340.f, 10.f);

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_GeoDispatch_PointInPolygon) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchContainment, nullptr);

    // Square polygon (lat,lon interleaved): (0,0),(0,2),(2,2),(2,0)
    const double polygon[] = {0.0, 0.0,  0.0, 2.0,  2.0, 2.0,  2.0, 0.0};
    const double pLats[]   = {1.0, 3.0};
    const double pLons[]   = {1.0, 3.0};
    uint8_t results[2]     = {0, 0};

    const int rc = d.launchContainment(pLats, pLons, 2,
                                       polygon, 4,
                                       results, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(results[0], 1u);
    EXPECT_EQ(results[1], 0u);

    backend.shutdown();
}

// =============================================================================
// VulkanGeoBackend functional tests (batchDistances / batchPointInPolygon)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchDistances_HaversineCorrectness) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double lats1[] = {48.8566, 40.7128};
    const double lons1[] = { 2.3522, -74.0060};
    const double lats2[] = {51.5074, 51.5074};
    const double lons2[] = {-0.1278,  -0.1278};

    auto dists = backend.batchDistances(lats1, lons1, lats2, lons2, 2);
    ASSERT_EQ(dists.size(), 2u);
    EXPECT_NEAR(dists[0], 340.f, 10.f);   // Paris → London ≈ 340 km
    EXPECT_GT(dists[1], 5500.f);           // New York → London > 5500 km

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchPointInPolygon_InsideOutside) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double polygon[] = {0.0, 0.0,  0.0, 2.0,  2.0, 2.0,  2.0, 0.0};
    const double pLats[]   = {1.0, 3.0};
    const double pLons[]   = {1.0, 3.0};

    auto results = backend.batchPointInPolygon(pLats, pLons, 2, polygon, 4);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0]);
    EXPECT_FALSE(results[1]);

    backend.shutdown();
}

// =============================================================================
// Vulkan hardware tests (skipped gracefully when no GPU available)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanVectorBackend_GPU_ComputeDistances_L2) {
    VulkanVectorBackend backend = {};
    if (!backend.initialize() || !backend.isAvailable()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_hardware_not_available";
    }

    // Two identical 3-D unit vectors → L2 squared distance = 0
    const float v[]   = {1.f, 0.f, 0.f};
    auto dists = backend.computeDistances(v, 1, 3, v, 1, /*useL2=*/true);

    ASSERT_EQ(dists.size(), 1u);
    EXPECT_NEAR(dists[0], 0.f, 0.01f);

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_GPU_ComputeDistances_Cosine) {
    VulkanVectorBackend backend = {};
    if (!backend.initialize() || !backend.isAvailable()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_hardware_not_available";
    }

    // Two identical unit vectors → cosine distance = 0
    const float v[]   = {1.f, 0.f, 0.f};
    auto dists = backend.computeDistances(v, 1, 3, v, 1, /*useL2=*/false);

    ASSERT_EQ(dists.size(), 1u);
    EXPECT_NEAR(dists[0], 0.f, 0.01f);

    backend.shutdown();
}

// =============================================================================
// Input validation tests — VulkanVectorBackend (no hardware required)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ComputeDistances_NullQuery_ReturnsEmpty) {
    VulkanVectorBackend backend;
    // Validation must work even without initialization
    const float v[] = {1.f, 0.f};
    auto result = backend.computeDistances(nullptr, 1, 2, v, 1);
    EXPECT_TRUE(result.empty());
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ComputeDistances_NullVectors_ReturnsEmpty) {
    VulkanVectorBackend backend;
    const float q[] = {1.f, 0.f};
    auto result = backend.computeDistances(q, 1, 2, nullptr, 1);
    EXPECT_TRUE(result.empty());
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ComputeDistances_ZeroDim_ReturnsEmpty) {
    VulkanVectorBackend backend;
    const float q[] = {1.f};
    auto result = backend.computeDistances(q, 1, 0, q, 1);
    EXPECT_TRUE(result.empty());
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_ComputeDistances_ZeroQueries_ReturnsEmpty) {
    VulkanVectorBackend backend;
    const float q[] = {1.f};
    auto result = backend.computeDistances(q, 0, 1, q, 1);
    EXPECT_TRUE(result.empty());
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_BatchKnnSearch_NullQuery_ReturnsEmpty) {
    VulkanVectorBackend backend;
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(nullptr, 1, 2, v, 1, 1);
    EXPECT_TRUE(result.empty());
}

TEST(VulkanComputeEquivalents, VulkanVectorBackend_BatchKnnSearch_ZeroK_ReturnsEmpty) {
    VulkanVectorBackend backend;
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(v, 1, 2, v, 1, 0);
    EXPECT_TRUE(result.empty());
}

// =============================================================================
// Input validation tests — VulkanGeoBackend (no hardware required)
// =============================================================================

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchDistances_NullPointer_ReturnsEmpty) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double lat[] = {48.8566};
    const double lon[] = {2.3522};
    auto result = backend.batchDistances(nullptr, lon, lat, lon, 1);
    EXPECT_TRUE(result.empty());

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchDistances_ZeroCount_ReturnsEmpty) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double lat[] = {48.8566};
    const double lon[] = {2.3522};
    auto result = backend.batchDistances(lat, lon, lat, lon, 0);
    EXPECT_TRUE(result.empty());

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchPointInPolygon_NullPointer_ReturnsEmpty) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double polygon[] = {0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 2.0, 0.0};
    const double lats[] = {1.0};
    auto result = backend.batchPointInPolygon(nullptr, lats, 1, polygon, 4);
    EXPECT_TRUE(result.empty());

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchPointInPolygon_TooFewVertices_ReturnsEmpty) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double polygon[] = {0.0, 0.0, 1.0, 1.0};  // only 2 vertices — invalid
    const double lats[] = {0.5};
    const double lons[] = {0.5};
    auto result = backend.batchPointInPolygon(lats, lons, 1, polygon, 2);
    EXPECT_TRUE(result.empty());

    backend.shutdown();
}

TEST(VulkanComputeEquivalents, VulkanGeoBackend_BatchPointInPolygon_ZeroPoints_ReturnsEmpty) {
    VulkanGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    const double polygon[] = {0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 2.0, 0.0};
    const double lats[] = {1.0};
    const double lons[] = {1.0};
    auto result = backend.batchPointInPolygon(lats, lons, 0, polygon, 4);
    EXPECT_TRUE(result.empty());

    backend.shutdown();
}

#else // !THEMIS_ENABLE_VULKAN

TEST(VulkanComputeEquivalents, VulkanNotCompiled_ANNDispatchAllNull) {
    VulkanVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();

    // When Vulkan is not compiled, all slots must be null so the
    // BackendRegistry falls back to the CPU dispatch table.
    EXPECT_EQ(d.launchL2Distance,   nullptr);
    EXPECT_EQ(d.launchCosine,       nullptr);
    EXPECT_EQ(d.launchInnerProduct, nullptr);
    EXPECT_EQ(d.launchTopK,         nullptr);
}

TEST(VulkanComputeEquivalents, VulkanNotCompiled_GeoDispatchAllNull) {
    VulkanGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();

    EXPECT_EQ(d.launchDistance,    nullptr);
    EXPECT_EQ(d.launchContainment, nullptr);
}

#endif // THEMIS_ENABLE_VULKAN
