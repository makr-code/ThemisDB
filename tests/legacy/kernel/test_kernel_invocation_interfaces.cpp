// Test: Frozen Kernel Invocation Interfaces
//
// Validates that the kernel invocation interface types declared in
// include/acceleration/kernel_invocation.h compile correctly, carry the
// expected default values, and satisfy the interface stability contract.
//
// These tests run on any platform (no GPU required) because they exercise
// only C++ types and the CPU backend; they contain no hardware-specific calls.

#include <gtest/gtest.h>
#include "acceleration/kernel_invocation.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"

#include <cstdint>
#include <type_traits>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// Interface version
// =============================================================================

TEST(KernelInvocationInterfaces, InterfaceVersionIsOneHundred) {
    EXPECT_EQ(KERNEL_INVOCATION_INTERFACE_VERSION, 100u);
}

// =============================================================================
// DistanceMetric enum
// =============================================================================

TEST(KernelInvocationInterfaces, DistanceMetricValues) {
    EXPECT_EQ(static_cast<uint32_t>(DistanceMetric::L2),            0u);
    EXPECT_EQ(static_cast<uint32_t>(DistanceMetric::COSINE),        1u);
    EXPECT_EQ(static_cast<uint32_t>(DistanceMetric::INNER_PRODUCT), 2u);
}

TEST(KernelInvocationInterfaces, DistanceMetricUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<DistanceMetric>::type, uint32_t>::value,
        "DistanceMetric must use uint32_t as its underlying type"
    );
}

// =============================================================================
// GeoDistanceFormula enum
// =============================================================================

TEST(KernelInvocationInterfaces, GeoDistanceFormulaValues) {
    EXPECT_EQ(static_cast<uint32_t>(GeoDistanceFormula::HAVERSINE), 0u);
    EXPECT_EQ(static_cast<uint32_t>(GeoDistanceFormula::VINCENTY),  1u);
}

TEST(KernelInvocationInterfaces, GeoDistanceFormulaUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<GeoDistanceFormula>::type, uint32_t>::value,
        "GeoDistanceFormula must use uint32_t as its underlying type"
    );
}

// =============================================================================
// ANNKernelParams defaults
// =============================================================================

TEST(KernelInvocationInterfaces, ANNKernelParamsDefaultValues) {
    ANNKernelParams p;
    EXPECT_EQ(p.queries,    nullptr);
    EXPECT_EQ(p.numQueries, 0u);
    EXPECT_EQ(p.dim,        0u);
    EXPECT_EQ(p.vectors,    nullptr);
    EXPECT_EQ(p.numVectors, 0u);
    EXPECT_EQ(p.topK,       1u);
    EXPECT_EQ(p.metric,     DistanceMetric::L2);
}

TEST(KernelInvocationInterfaces, ANNKernelParamsCanBePopulated) {
    std::vector<float> q = {1.f, 0.f};
    std::vector<float> v = {0.f, 1.f, 1.f, 0.f};

    ANNKernelParams p;
    p.queries    = q.data();
    p.numQueries = 1;
    p.dim        = 2;
    p.vectors    = v.data();
    p.numVectors = 2;
    p.topK       = 2;
    p.metric     = DistanceMetric::COSINE;

    EXPECT_EQ(p.numQueries, 1u);
    EXPECT_EQ(p.dim,        2u);
    EXPECT_EQ(p.numVectors, 2u);
    EXPECT_EQ(p.topK,       2u);
    EXPECT_EQ(p.metric,     DistanceMetric::COSINE);
}

// =============================================================================
// ANNKernelResult defaults
// =============================================================================

TEST(KernelInvocationInterfaces, ANNKernelResultDefaultValues) {
    ANNKernelResult r;
    EXPECT_EQ(r.indices,   nullptr);
    EXPECT_EQ(r.distances, nullptr);
}

// =============================================================================
// GeoKernelParams defaults
// =============================================================================

TEST(KernelInvocationInterfaces, GeoKernelParamsDefaultValues) {
    GeoKernelParams p;
    EXPECT_EQ(p.latitudes1,  nullptr);
    EXPECT_EQ(p.longitudes1, nullptr);
    EXPECT_EQ(p.latitudes2,  nullptr);
    EXPECT_EQ(p.longitudes2, nullptr);
    EXPECT_EQ(p.count,       0u);
    EXPECT_EQ(p.formula,     GeoDistanceFormula::HAVERSINE);
}

// =============================================================================
// GeoContainmentParams defaults
// =============================================================================

TEST(KernelInvocationInterfaces, GeoContainmentParamsDefaultValues) {
    GeoContainmentParams p;
    EXPECT_EQ(p.pointLats,          nullptr);
    EXPECT_EQ(p.pointLons,          nullptr);
    EXPECT_EQ(p.numPoints,          0u);
    EXPECT_EQ(p.polygonCoords,      nullptr);
    EXPECT_EQ(p.numPolygonVertices, 0u);
}

// =============================================================================
// ANNKernelDispatch defaults and helper
// =============================================================================

TEST(KernelInvocationInterfaces, ANNKernelDispatchDefaultsToNullptr) {
    ANNKernelDispatch d;
    EXPECT_EQ(d.launchL2Distance,   nullptr);
    EXPECT_EQ(d.launchCosine,       nullptr);
    EXPECT_EQ(d.launchInnerProduct, nullptr);
    EXPECT_EQ(d.launchTopK,         nullptr);
}

TEST(KernelInvocationInterfaces, ANNKernelDispatchDistanceLauncherForReturnsNullWhenEmpty) {
    ANNKernelDispatch d;
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            nullptr);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        nullptr);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), nullptr);
}

TEST(KernelInvocationInterfaces, ANNKernelDispatchDistanceLauncherForReturnsCorrectEntry) {
    // Use lambdas converted to plain function pointers via a trampoline type.
    auto stub_l2 = +[](const float*, const float*, float*, int, int, int, void*) { return 0; };
    auto stub_cos = +[](const float*, const float*, float*, int, int, int, void*) { return 1; };
    auto stub_ip = +[](const float*, const float*, float*, int, int, int, void*) { return 2; };

    ANNKernelDispatch d;
    d.launchL2Distance   = stub_l2;
    d.launchCosine       = stub_cos;
    d.launchInnerProduct = stub_ip;

    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            stub_l2);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        stub_cos);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), stub_ip);
}

// =============================================================================
// GeoKernelDispatch defaults
// =============================================================================

TEST(KernelInvocationInterfaces, GeoKernelDispatchDefaultsToNullptr) {
    GeoKernelDispatch d;
    EXPECT_EQ(d.launchDistance,    nullptr);
    EXPECT_EQ(d.launchContainment, nullptr);
}

// =============================================================================
// Function-pointer typedef arity / signature checks via static assertions
// =============================================================================

TEST(KernelInvocationInterfaces, FunctionPointerTypesAreCallable) {
    // Verify the typedefs name function pointer types (not class types).
    static_assert(std::is_pointer<ANNDistanceFn>::value,    "ANNDistanceFn must be a pointer type");
    static_assert(std::is_pointer<ANNTopKFn>::value,        "ANNTopKFn must be a pointer type");
    static_assert(std::is_pointer<GeoDistanceFn>::value,    "GeoDistanceFn must be a pointer type");
    static_assert(std::is_pointer<GeoContainmentFn>::value, "GeoContainmentFn must be a pointer type");
}

// =============================================================================
// Integration: CPU backend satisfies the high-level IVectorBackend and
// IGeoBackend interfaces that the frozen dispatch contract is built on top of.
// =============================================================================

TEST(KernelInvocationInterfaces, CPUVectorBackendSatisfiesIVectorBackend) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Two queries, dim=3, three database vectors.
    const float queries[] = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
    };
    const float vectors[] = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };

    auto distances = backend.computeDistances(queries, 2, 3, vectors, 3, /*useL2=*/true);
    ASSERT_EQ(distances.size(), 6u);

    // Query 0 vs vector 0: squared-L2 = 0
    EXPECT_NEAR(distances[0], 0.f, 1e-5f);
    // Query 0 vs vector 1: squared-L2 = 1+1 = 2
    EXPECT_NEAR(distances[1], 2.f, 1e-5f);
    // Query 1 vs vector 1: squared-L2 = 0
    EXPECT_NEAR(distances[4], 0.f, 1e-5f);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUGeoBackendSatisfiesIGeoBackend) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Paris (48.8566, 2.3522) ↔ London (51.5074, -0.1278)
    const double lats1[] = {48.8566};
    const double lons1[] = {2.3522};
    const double lats2[] = {51.5074};
    const double lons2[] = {-0.1278};

    auto distances = backend.batchDistances(lats1, lons1, lats2, lons2, 1, /*useHaversine=*/true);
    ASSERT_EQ(distances.size(), 1u);
    // Paris–London is approximately 340 km; accept ±10 km tolerance.
    EXPECT_NEAR(distances[0], 340.f, 10.f);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUGeoBackendPointInPolygon) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Simple square polygon: corners at (0,0),(0,2),(2,2),(2,0)
    // Interleaved as [lat, lon, lat, lon, ...]
    const double polygon[] = {0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 2.0, 0.0};

    // Inside point (1,1) and outside point (3,3)
    const double pLats[] = {1.0, 3.0};
    const double pLons[] = {1.0, 3.0};

    auto results = backend.batchPointInPolygon(pLats, pLons, 2, polygon, 4);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0]);   // (1,1) is inside
    EXPECT_FALSE(results[1]);  // (3,3) is outside

    backend.shutdown();
}

// =============================================================================
// Dispatch table population — CPU backend
// =============================================================================

TEST(KernelInvocationInterfaces, CPUVectorBackend_PopulateANNDispatch_AllSlotsNonNull) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUVectorBackend_ANNDispatch_L2_CorrectResult) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchL2Distance, nullptr);

    // Query [1,0,0] vs vectors [1,0,0], [0,1,0]: expected squared-L2 = 0, 2
    const float queries[] = {1.f, 0.f, 0.f};
    const float vectors[] = {1.f, 0.f, 0.f,  0.f, 1.f, 0.f};
    float distances[2] = {};

    const int rc = d.launchL2Distance(queries, vectors, distances,
                                      /*numQueries=*/1, /*numVectors=*/2, /*dim=*/3,
                                      /*stream=*/nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(distances[0], 0.f, 1e-5f);
    EXPECT_NEAR(distances[1], 2.f, 1e-5f);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUVectorBackend_ANNDispatch_TopK_SelectsSmallest) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchTopK, nullptr);

    // One query row with 4 distances: 3.f, 1.f, 4.f, 0.f  → top-2 are idx 3 (0.f) and idx 1 (1.f)
    const float distances[4] = {3.f, 1.f, 4.f, 0.f};
    uint32_t indices[2] = {};
    float    dists[2]   = {};

    const int rc = d.launchTopK(distances, indices, dists,
                                /*numQueries=*/1, /*numVectors=*/4, /*topK=*/2,
                                /*stream=*/nullptr);
    EXPECT_EQ(rc, 0);
    // Results should be sorted ascending by distance
    EXPECT_EQ(indices[0], 3u);   // distance 0.f
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_EQ(indices[1], 1u);   // distance 1.f
    EXPECT_NEAR(dists[1], 1.f, 1e-5f);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUGeoBackend_PopulateGeoDispatch_AllSlotsNonNull) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();

    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUGeoBackend_GeoDispatch_HaversineDistance) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchDistance, nullptr);

    // Paris (48.8566, 2.3522) to London (51.5074, -0.1278) ≈ 340 km
    const double lats1[] = {48.8566};
    const double lons1[] = {2.3522};
    const double lats2[] = {51.5074};
    const double lons2[] = {-0.1278};
    float dist = 0.f;

    const int rc = d.launchDistance(lats1, lons1, lats2, lons2, &dist,
                                    /*count=*/1,
                                    GeoDistanceFormula::HAVERSINE,
                                    /*stream=*/nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, 340.f, 10.f);

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, CPUGeoBackend_GeoDispatch_PointInPolygon) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());

    GeoKernelDispatch d = backend.populateGeoDispatch();
    ASSERT_NE(d.launchContainment, nullptr);

    // Square polygon corners: (0,0),(0,2),(2,2),(2,0) — interleaved [lat,lon]
    const double polygon[] = {0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 2.0, 0.0};
    const double pLats[]   = {1.0, 3.0};
    const double pLons[]   = {1.0, 3.0};
    uint8_t results[2]     = {0, 0};

    const int rc = d.launchContainment(pLats, pLons, /*numPoints=*/2,
                                       polygon, /*numVertices=*/4,
                                       results, /*stream=*/nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(results[0], 1u);  // (1,1) inside
    EXPECT_EQ(results[1], 0u);  // (3,3) outside

    backend.shutdown();
}

TEST(KernelInvocationInterfaces, ANNDispatch_DistanceLauncherFor_RoutesCorrectly) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            d.launchL2Distance);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        d.launchCosine);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), d.launchInnerProduct);

    backend.shutdown();
}

// =============================================================================
// CUDA backend dispatch table structure (GPU not required to test structure)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"

TEST(KernelInvocationInterfaces, CUDAVectorBackend_PopulateANNDispatch_AllSlotsNonNull) {
    CUDAVectorBackend backend;
    // Skip if CUDA hardware is unavailable; only test dispatch table population
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_backend_not_available";
    }
    ANNKernelDispatch d = backend.populateANNDispatch();
    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);
}

TEST(KernelInvocationInterfaces, CUDAGeoBackend_PopulateGeoDispatch_AllSlotsNonNull) {
    CUDAGeoBackend backend;
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_backend_not_available";
    }
    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

TEST(KernelInvocationInterfaces, CUDAVectorBackend_DispatchReturnsValidStructure) {
    // Verifies the method exists and returns a well-formed struct (even without GPU).
    // All 4 ANN slots must be non-null when compiled with THEMIS_ENABLE_CUDA because
    // the adapters are static functions assigned unconditionally at compile time.
    CUDAVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);
}

TEST(KernelInvocationInterfaces, CUDAGeoBackend_DispatchReturnsValidStructure) {
    // Verifies the method exists and both kernel slots are non-null when compiled
    // with THEMIS_ENABLE_CUDA, even without GPU hardware present.  The slots are
    // populated from extern "C" function pointers that always exist at link time.
    CUDAGeoBackend backend;
    GeoKernelDispatch d = backend.populateGeoDispatch();
    EXPECT_NE(d.launchDistance,    nullptr);
    EXPECT_NE(d.launchContainment, nullptr);
}

#endif // THEMIS_ENABLE_CUDA
