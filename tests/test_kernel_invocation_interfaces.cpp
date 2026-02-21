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
    static int stub_l2(const float*, const float*, float*, int, int, int, void*) { return 0; }
    static int stub_cos(const float*, const float*, float*, int, int, int, void*) { return 1; }
    static int stub_ip(const float*, const float*, float*, int, int, int, void*) { return 2; }

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
