// Tests for the OpenGL Compute Shader vector backend.
//
// When built WITHOUT THEMIS_ENABLE_OPENGL all tests verify the
// "unavailable" path (isAvailable() == false, operations return empty).
//
// When built WITH THEMIS_ENABLE_OPENGL the tests exercise the CPU
// fallback path (which always activates in headless CI environments
// without an OpenGL 4.3+ display / EGL driver) and, when real GL
// hardware is present, exercise the GPU path transparently.

#include <gtest/gtest.h>
#include "acceleration/graphics_backends.h"
#include "acceleration/compute_backend.h"
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <numeric>

using namespace themis::acceleration;

// ============================================================================
// Helper – reference CPU distance calculations
// ============================================================================

namespace {

float refL2(const float* a, const float* b, int dim) {
    float s = 0.f;
    for (int i = 0; i < dim; ++i) { float d = a[i] - b[i]; s += d * d; }
    return s;
}

float refCosine(const float* a, const float* b, int dim) {
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (int i = 0; i < dim; ++i) {
        dot += a[i] * b[i]; na += a[i] * a[i]; nb += b[i] * b[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    return (denom > 1e-10f) ? 1.f - dot / denom : 1.f;
}

} // namespace

// ============================================================================
// Fixture
// ============================================================================

class OpenGLBackendTest : public ::testing::Test {
protected:
    OpenGLVectorBackend backend_;

    void TearDown() override {
        backend_.shutdown();
    }
};

// ============================================================================
// Identity and type tests — always run regardless of OpenGL availability
// ============================================================================

TEST_F(OpenGLBackendTest, NameAndType) {
    EXPECT_STREQ(backend_.name(), "OpenGL");
    EXPECT_EQ(backend_.type(), BackendType::OPENGL);
}

TEST_F(OpenGLBackendTest, CapabilitiesAlwaysPopulated) {
#ifdef THEMIS_ENABLE_OPENGL
    // getCapabilities() must return populated struct when compiled in
    auto caps = backend_.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_FALSE(caps.supportsAsync);   // OpenGL compute is synchronous
    EXPECT_FALSE(caps.deviceName.empty());
#else
    // When OpenGL is not compiled in, capabilities are empty but must not crash
    auto caps = backend_.getCapabilities();
    EXPECT_FALSE(caps.supportsVectorOps);
#endif
}

// ============================================================================
// Initialization tests
// ============================================================================

TEST_F(OpenGLBackendTest, InitializeWithoutOpenGLHeader) {
#ifdef THEMIS_ENABLE_OPENGL
    // initialize() must always return true (GPU or CPU-fallback path)
    EXPECT_TRUE(backend_.initialize());
    // Double-init must be idempotent
    EXPECT_TRUE(backend_.initialize());
#else
    // Without compile-time OpenGL support, initialize() returns false
    EXPECT_FALSE(backend_.initialize());
#endif
}

TEST_F(OpenGLBackendTest, ShutdownBeforeInitIsHarmless) {
    // shutdown() on an uninitialised backend must not crash or assert
    EXPECT_NO_THROW(backend_.shutdown());
}

// ============================================================================
// Computation correctness — only meaningful when THEMIS_ENABLE_OPENGL
// is defined (CPU-fallback path active in headless CI)
// ============================================================================

#ifdef THEMIS_ENABLE_OPENGL

class OpenGLComputeTest : public ::testing::Test {
protected:
    OpenGLVectorBackend backend_;

    void SetUp() override {
        ASSERT_TRUE(backend_.initialize());
    }
    void TearDown() override {
        backend_.shutdown();
    }
};

// ---- computeDistances -------------------------------------------------------

TEST_F(OpenGLComputeTest, L2Distance_SingleQuerySingleVector_ZeroForIdentical) {
    const std::vector<float> v = {1.f, 2.f, 3.f};
    auto dist = backend_.computeDistances(v.data(), 1, 3, v.data(), 1, /*useL2=*/true);
    ASSERT_EQ(dist.size(), 1u);
    EXPECT_NEAR(dist[0], 0.f, 1e-6f);
}

TEST_F(OpenGLComputeTest, L2Distance_KnownValues) {
    // query = (0,0,0),  vectors = axis unit vectors
    const std::vector<float> q = {0.f, 0.f, 0.f};
    const std::vector<float> v = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    auto dist = backend_.computeDistances(q.data(), 1, 3, v.data(), 3, true);
    ASSERT_EQ(dist.size(), 3u);
    // Each distance from origin to a unit-axis vector is 1.0 (squared L2)
    for (float d : dist) EXPECT_NEAR(d, 1.f, 1e-5f);
}

TEST_F(OpenGLComputeTest, L2Distance_MultipleQueries_MatchesReference) {
    const int nq = 4, nv = 5, dim = 3;
    std::vector<float> queries(nq * dim), vectors(nv * dim);
    // Fill with deterministic values
    for (int i = 0; i < nq * dim; ++i) queries[i] = static_cast<float>(i % 7) * 0.3f;
    for (int i = 0; i < nv * dim; ++i) vectors[i] = static_cast<float>(i % 5) * 0.5f;

    auto dist = backend_.computeDistances(
        queries.data(), nq, dim, vectors.data(), nv, true);

    ASSERT_EQ(dist.size(), static_cast<size_t>(nq * nv));
    for (int q = 0; q < nq; ++q) {
        for (int v = 0; v < nv; ++v) {
            float expected = refL2(&queries[q * dim], &vectors[v * dim], dim);
            EXPECT_NEAR(dist[q * nv + v], expected, 1e-4f)
                << "q=" << q << " v=" << v;
        }
    }
}

TEST_F(OpenGLComputeTest, CosineDistance_IdenticalVectors_ZeroDistance) {
    const std::vector<float> v = {3.f, 4.f};
    auto dist = backend_.computeDistances(v.data(), 1, 2, v.data(), 1, /*useL2=*/false);
    ASSERT_EQ(dist.size(), 1u);
    EXPECT_NEAR(dist[0], 0.f, 1e-5f);
}

TEST_F(OpenGLComputeTest, CosineDistance_OrthogonalVectors_IsOne) {
    const std::vector<float> q = {1.f, 0.f};
    const std::vector<float> v = {0.f, 1.f};
    auto dist = backend_.computeDistances(q.data(), 1, 2, v.data(), 1, false);
    ASSERT_EQ(dist.size(), 1u);
    EXPECT_NEAR(dist[0], 1.f, 1e-5f);
}

TEST_F(OpenGLComputeTest, CosineDistance_MultipleQueries_MatchesReference) {
    const int nq = 3, nv = 4, dim = 4;
    std::vector<float> queries(nq * dim), vectors(nv * dim);
    for (int i = 0; i < nq * dim; ++i) queries[i] = static_cast<float>((i * 3 + 1) % 9) + 0.1f;
    for (int i = 0; i < nv * dim; ++i) vectors[i] = static_cast<float>((i * 2 + 3) % 7) + 0.2f;

    auto dist = backend_.computeDistances(
        queries.data(), nq, dim, vectors.data(), nv, false);

    ASSERT_EQ(dist.size(), static_cast<size_t>(nq * nv));
    for (int q = 0; q < nq; ++q) {
        for (int v = 0; v < nv; ++v) {
            float expected = refCosine(&queries[q * dim], &vectors[v * dim], dim);
            EXPECT_NEAR(dist[q * nv + v], expected, 1e-4f)
                << "q=" << q << " v=" << v;
        }
    }
}

// ---- batchKnnSearch ---------------------------------------------------------

TEST_F(OpenGLComputeTest, KnnSearch_ResultsSortedAscendingByDistance) {
    // 5 vectors in 3D; query = (0,0,0) → distances are equal to squared norms
    const std::vector<float> vectors = {
        1.f, 0.f, 0.f,   // d² = 1
        2.f, 0.f, 0.f,   // d² = 4
        3.f, 0.f, 0.f,   // d² = 9
        0.f, 2.f, 0.f,   // d² = 4
        0.f, 0.f, 3.f,   // d² = 9
    };
    const std::vector<float> query = {0.f, 0.f, 0.f};

    auto results = backend_.batchKnnSearch(
        query.data(), 1, 3, vectors.data(), 5, 3, true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 3u);

    // Must be sorted ascending by distance
    for (size_t i = 1; i < results[0].size(); ++i)
        EXPECT_LE(results[0][i - 1].second, results[0][i].second);

    // Nearest neighbour must be vector 0 (d²=1)
    EXPECT_EQ(results[0][0].first, 0u);
    EXPECT_NEAR(results[0][0].second, 1.f, 1e-5f);
}

TEST_F(OpenGLComputeTest, KnnSearch_KClampedToNumVectors) {
    const std::vector<float> vectors = {1.f, 0.f, 0.f, 1.f};  // 2 vectors, dim=2
    const std::vector<float> query   = {0.f, 0.f};
    // k=10 > numVectors=2; must not crash and must return exactly 2 results
    auto results = backend_.batchKnnSearch(
        query.data(), 1, 2, vectors.data(), 2, 10, true);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), 2u);  // clamped to numVectors
}

TEST_F(OpenGLComputeTest, KnnSearch_TieBreaking_DeterministicLowerIndexFirst) {
    // Two vectors equidistant from origin; lower index must come first
    const std::vector<float> vectors = {
        1.f, 0.f,
        0.f, 1.f,
    };
    const std::vector<float> query = {0.f, 0.f};
    auto results = backend_.batchKnnSearch(
        query.data(), 1, 2, vectors.data(), 2, 2, true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 2u);
    // Both distances are 1.0; lower index (0) must be first
    EXPECT_EQ(results[0][0].first, 0u);
    EXPECT_EQ(results[0][1].first, 1u);
    EXPECT_NEAR(results[0][0].second, 1.f, 1e-5f);
    EXPECT_NEAR(results[0][1].second, 1.f, 1e-5f);
}

TEST_F(OpenGLComputeTest, KnnSearch_MultipleQueries_EachHasCorrectNearestNeighbour) {
    // Cluster structure: q0 near v0, q1 near v3
    const std::vector<float> vectors = {
        0.1f, 0.1f,   // v0
        5.0f, 5.0f,   // v1
        5.1f, 4.9f,   // v2
        10.f, 10.f,   // v3
    };
    const std::vector<float> queries = {
        0.0f, 0.0f,   // q0 → nearest is v0
        10.f, 10.f,   // q1 → nearest is v3
    };
    auto results = backend_.batchKnnSearch(
        queries.data(), 2, 2, vectors.data(), 4, 1, true);

    ASSERT_EQ(results.size(), 2u);
    ASSERT_EQ(results[0].size(), 1u);
    ASSERT_EQ(results[1].size(), 1u);
    EXPECT_EQ(results[0][0].first, 0u);  // q0 nearest is v0
    EXPECT_EQ(results[1][0].first, 3u);  // q1 nearest is v3
}

// ---- Input validation -------------------------------------------------------

TEST_F(OpenGLComputeTest, NullQueryPointerReturnsEmpty) {
    const std::vector<float> v = {1.f, 0.f};
    auto dist = backend_.computeDistances(nullptr, 1, 2, v.data(), 1, true);
    EXPECT_TRUE(dist.empty());
}

TEST_F(OpenGLComputeTest, NullVectorPointerReturnsEmpty) {
    const std::vector<float> q = {1.f, 0.f};
    auto dist = backend_.computeDistances(q.data(), 1, 2, nullptr, 1, true);
    EXPECT_TRUE(dist.empty());
}

TEST_F(OpenGLComputeTest, ZeroDimReturnsEmpty) {
    const std::vector<float> q = {1.f};
    const std::vector<float> v = {1.f};
    auto dist = backend_.computeDistances(q.data(), 1, 0, v.data(), 1, true);
    EXPECT_TRUE(dist.empty());
}

TEST_F(OpenGLComputeTest, ZeroKReturnsEmpty) {
    const std::vector<float> v = {1.f, 0.f};
    const std::vector<float> q = {0.f, 0.f};
    auto res = backend_.batchKnnSearch(q.data(), 1, 2, v.data(), 1, 0, true);
    EXPECT_TRUE(res.empty());
}

TEST_F(OpenGLComputeTest, UninitializedComputeDistancesReturnsEmpty) {
    OpenGLVectorBackend uninit;
    const std::vector<float> q = {1.f, 0.f};
    const std::vector<float> v = {1.f, 0.f};
    auto dist = uninit.computeDistances(q.data(), 1, 2, v.data(), 1, true);
    EXPECT_TRUE(dist.empty());
}

TEST_F(OpenGLComputeTest, UninitializedBatchKnnReturnsEmpty) {
    OpenGLVectorBackend uninit;
    const std::vector<float> q = {1.f, 0.f};
    const std::vector<float> v = {1.f, 0.f};
    auto res = uninit.batchKnnSearch(q.data(), 1, 2, v.data(), 1, 1, true);
    EXPECT_TRUE(res.empty());
}

// ---- Large batch smoke test -------------------------------------------------

TEST_F(OpenGLComputeTest, LargeBatch_DoesNotCrash) {
    const size_t nq = 64, nv = 128, dim = 32;
    std::vector<float> queries(nq * dim), vectors(nv * dim);
    for (size_t i = 0; i < queries.size(); ++i)
        queries[i] = static_cast<float>(i % 17) * 0.1f;
    for (size_t i = 0; i < vectors.size(); ++i)
        vectors[i] = static_cast<float>(i % 13) * 0.2f;

    auto dist = backend_.computeDistances(
        queries.data(), nq, dim, vectors.data(), nv, true);
    EXPECT_EQ(dist.size(), nq * nv);

    auto knn = backend_.batchKnnSearch(
        queries.data(), nq, dim, vectors.data(), nv, 10, true);
    ASSERT_EQ(knn.size(), nq);
    for (const auto& row : knn) EXPECT_EQ(row.size(), 10u);
}

#endif  // THEMIS_ENABLE_OPENGL

// ============================================================================
// isAvailable() — portable probe test
// ============================================================================

TEST(OpenGLAvailabilityTest, IsAvailableReturnsBoolWithoutCrash) {
    OpenGLVectorBackend backend;
    // Must not crash; may return true or false depending on the platform
    bool avail = backend.isAvailable();
    (void)avail;
    // If available, name and type must be correct regardless
    EXPECT_STREQ(backend.name(), "OpenGL");
    EXPECT_EQ(backend.type(), BackendType::OPENGL);
}

// ============================================================================
// BackendRegistry integration — verify OpenGL backend is registered and
// retrievable via BackendType::OPENGL when THEMIS_ENABLE_OPENGL is active
// ============================================================================

#ifdef THEMIS_ENABLE_OPENGL

TEST(OpenGLRegistryTest, BackendRegistrableAndRetrievable) {
    auto& registry = BackendRegistry::instance();

    // The backend should already be registered from the BackendRegistry
    // constructor (wired in this PR). If not (e.g., in a test that cleared
    // the registry), register it explicitly.
    auto* retrieved = registry.getBackend(BackendType::OPENGL);
    if (!retrieved) {
        registry.registerBackend(std::make_unique<OpenGLVectorBackend>());
        retrieved = registry.getBackend(BackendType::OPENGL);
    }

    ASSERT_NE(retrieved, nullptr);
    EXPECT_STREQ(retrieved->name(), "OpenGL");
    EXPECT_EQ(retrieved->type(), BackendType::OPENGL);
}

TEST(OpenGLRegistryTest, BackendInitializesViaRegistry) {
    auto& registry = BackendRegistry::instance();
    auto* b = registry.getBackend(BackendType::OPENGL);
    if (!b) {
        // Register if not yet present
        registry.registerBackend(std::make_unique<OpenGLVectorBackend>());
        b = registry.getBackend(BackendType::OPENGL);
    }
    ASSERT_NE(b, nullptr);
    // initialize() must succeed (GPU path or CPU-fallback path)
    EXPECT_TRUE(b->initialize());
    b->shutdown();
}

#endif  // THEMIS_ENABLE_OPENGL

// ============================================================================
// OpenGLGeoBackend tests
// ============================================================================

TEST(OpenGLGeoTest, NameAndType) {
    OpenGLGeoBackend backend;
    EXPECT_STREQ(backend.name(), "OpenGLGeo");
    EXPECT_EQ(backend.type(), BackendType::OPENGL);
}

TEST(OpenGLGeoTest, IsAvailableDoesNotCrash) {
    OpenGLGeoBackend backend;
    (void)backend.isAvailable();
}

TEST(OpenGLGeoTest, CapabilitiesGeoOpsSet) {
#ifdef THEMIS_ENABLE_OPENGL
    OpenGLGeoBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGeoOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_FALSE(caps.supportsAsync);  // synchronous compute
    backend.shutdown();
#endif
}

TEST(OpenGLGeoTest, InitializeSucceeds) {
#ifdef THEMIS_ENABLE_OPENGL
    OpenGLGeoBackend backend;
    EXPECT_TRUE(backend.initialize());
    EXPECT_TRUE(backend.initialize());  // idempotent
    backend.shutdown();
#else
    OpenGLGeoBackend backend;
    EXPECT_FALSE(backend.initialize());
#endif
}

TEST(OpenGLGeoTest, ShutdownBeforeInitIsHarmless) {
    OpenGLGeoBackend backend;
    EXPECT_NO_THROW(backend.shutdown());
}

TEST(OpenGLGeoTest, UninitializedBatchDistancesReturnsEmpty) {
#ifdef THEMIS_ENABLE_OPENGL
    OpenGLGeoBackend backend;
    const double lat1[] = {0.0}, lon1[] = {0.0}, lat2[] = {1.0}, lon2[] = {1.0};
    auto res = backend.batchDistances(lat1, lon1, lat2, lon2, 1);
    EXPECT_TRUE(res.empty());
#endif
}

TEST(OpenGLGeoTest, UninitializedBatchPIPReturnsEmpty) {
#ifdef THEMIS_ENABLE_OPENGL
    OpenGLGeoBackend backend;
    const double pLat[] = {0.5}, pLon[] = {0.5};
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    auto res = backend.batchPointInPolygon(pLat, pLon, 1, poly, 4);
    EXPECT_TRUE(res.empty());
#endif
}

#ifdef THEMIS_ENABLE_OPENGL

class OpenGLGeoComputeTest : public ::testing::Test {
protected:
    OpenGLGeoBackend backend_;
    void SetUp()    override { ASSERT_TRUE(backend_.initialize()); }
    void TearDown() override { backend_.shutdown(); }
};

TEST_F(OpenGLGeoComputeTest, HaversineDistance_SamePoint_IsZero) {
    const double lat[] = {48.8566}, lon[] = {2.3522};  // Paris
    auto res = backend_.batchDistances(lat, lon, lat, lon, 1);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_NEAR(res[0], 0.f, 1e-3f);
}

TEST_F(OpenGLGeoComputeTest, HaversineDistance_KnownPair_Approximate) {
    // Paris (48.8566°N, 2.3522°E) to London (51.5074°N, -0.1278°E) ≈ 340 km
    const double lat1[] = {48.8566}, lon1[] = {2.3522};
    const double lat2[] = {51.5074}, lon2[] = {-0.1278};
    auto res = backend_.batchDistances(lat1, lon1, lat2, lon2, 1);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_GT(res[0], 300.f);
    EXPECT_LT(res[0], 400.f);
}

TEST_F(OpenGLGeoComputeTest, HaversineDistance_NullPointerReturnsEmpty) {
    const double lat[] = {0.0}, lon[] = {0.0};
    EXPECT_TRUE(backend_.batchDistances(nullptr, lon, lat, lon, 1).empty());
    EXPECT_TRUE(backend_.batchDistances(lat, nullptr, lat, lon, 1).empty());
    EXPECT_TRUE(backend_.batchDistances(lat, lon, nullptr, lon, 1).empty());
    EXPECT_TRUE(backend_.batchDistances(lat, lon, lat, nullptr, 1).empty());
}

TEST_F(OpenGLGeoComputeTest, HaversineDistance_ZeroCountReturnsEmpty) {
    const double lat[] = {0.0}, lon[] = {0.0};
    EXPECT_TRUE(backend_.batchDistances(lat, lon, lat, lon, 0).empty());
}

TEST_F(OpenGLGeoComputeTest, HaversineDistance_MultiplePoints) {
    // Three known pairs
    const double lat1[] = {0.0, 48.8566, 35.6762};
    const double lon1[] = {0.0,  2.3522, 139.6503};
    const double lat2[] = {0.0, 51.5074, 37.6173};
    const double lon2[] = {0.0, -0.1278,  55.7558};
    auto res = backend_.batchDistances(lat1, lon1, lat2, lon2, 3);
    ASSERT_EQ(res.size(), 3u);
    // Same point → ~0
    EXPECT_NEAR(res[0], 0.f, 1e-3f);
    // Paris→London ~340 km
    EXPECT_GT(res[1], 300.f); EXPECT_LT(res[1], 400.f);
    // Tokyo→Moscow ~7500 km
    EXPECT_GT(res[2], 7000.f); EXPECT_LT(res[2], 8000.f);
}

TEST_F(OpenGLGeoComputeTest, PointInPolygon_InsideSquare) {
    // Unit square: (0,0)-(1,0)-(1,1)-(0,1)
    const double pLat[] = {0.5}, pLon[] = {0.5};
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    auto res = backend_.batchPointInPolygon(pLat, pLon, 1, poly, 4);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_TRUE(res[0]);
}

TEST_F(OpenGLGeoComputeTest, PointInPolygon_OutsideSquare) {
    const double pLat[] = {2.0}, pLon[] = {2.0};
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    auto res = backend_.batchPointInPolygon(pLat, pLon, 1, poly, 4);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_FALSE(res[0]);
}

TEST_F(OpenGLGeoComputeTest, PointInPolygon_NullPointerReturnsEmpty) {
    const double pLat[] = {0.5}, pLon[] = {0.5};
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    EXPECT_TRUE(backend_.batchPointInPolygon(nullptr, pLon, 1, poly, 4).empty());
    EXPECT_TRUE(backend_.batchPointInPolygon(pLat, nullptr, 1, poly, 4).empty());
    EXPECT_TRUE(backend_.batchPointInPolygon(pLat, pLon,   1, nullptr, 4).empty());
}

TEST_F(OpenGLGeoComputeTest, PointInPolygon_TooFewVerticesReturnsEmpty) {
    const double pLat[] = {0.5}, pLon[] = {0.5};
    const double poly[] = {0.0,0.0, 1.0,0.0};
    EXPECT_TRUE(backend_.batchPointInPolygon(pLat, pLon, 1, poly, 2).empty());
}

TEST_F(OpenGLGeoComputeTest, PointInPolygon_ZeroPointsReturnsEmpty) {
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    EXPECT_TRUE(backend_.batchPointInPolygon(nullptr, nullptr, 0, poly, 4).empty());
}

TEST_F(OpenGLGeoComputeTest, PointInPolygon_MixedInsideOutside) {
    const double pLat[] = {0.5, 2.0, 0.1};
    const double pLon[] = {0.5, 2.0, 0.1};
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    auto res = backend_.batchPointInPolygon(pLat, pLon, 3, poly, 4);
    ASSERT_EQ(res.size(), 3u);
    EXPECT_TRUE(res[0]);   // (0.5, 0.5) inside
    EXPECT_FALSE(res[1]);  // (2.0, 2.0) outside
    EXPECT_TRUE(res[2]);   // (0.1, 0.1) inside
}

#endif  // THEMIS_ENABLE_OPENGL

// ============================================================================
// OpenGLGraphBackend tests
// ============================================================================

TEST(OpenGLGraphTest, NameAndType) {
    OpenGLGraphBackend backend;
    EXPECT_STREQ(backend.name(), "OpenGLGraph");
    EXPECT_EQ(backend.type(), BackendType::OPENGL);
}

TEST(OpenGLGraphTest, IsAvailableDoesNotCrash) {
    OpenGLGraphBackend backend;
    (void)backend.isAvailable();
}

TEST(OpenGLGraphTest, CapabilitiesGraphOpsSet) {
#ifdef THEMIS_ENABLE_OPENGL
    OpenGLGraphBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGraphOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_FALSE(caps.supportsAsync);
    backend.shutdown();
#endif
}

TEST(OpenGLGraphTest, InitializeSucceeds) {
#ifdef THEMIS_ENABLE_OPENGL
    OpenGLGraphBackend backend;
    EXPECT_TRUE(backend.initialize());
    EXPECT_TRUE(backend.initialize());  // idempotent
    backend.shutdown();
#else
    OpenGLGraphBackend backend;
    EXPECT_FALSE(backend.initialize());
#endif
}

TEST(OpenGLGraphTest, ShutdownBeforeInitIsHarmless) {
    OpenGLGraphBackend backend;
    EXPECT_NO_THROW(backend.shutdown());
}

#ifdef THEMIS_ENABLE_OPENGL

class OpenGLGraphComputeTest : public ::testing::Test {
protected:
    OpenGLGraphBackend backend_;
    void SetUp()    override { ASSERT_TRUE(backend_.initialize()); }
    void TearDown() override { backend_.shutdown(); }
};

// Adjacency matrix for a simple chain: 0→1→2→3
static constexpr uint32_t kChainAdj4[] = {
    0,1,0,0,
    0,0,1,0,
    0,0,0,1,
    0,0,0,0,
};

// Weights for the chain (unit weights)
static constexpr float kChainWgt4[] = {
    0.f,1.f,0.f,0.f,
    0.f,0.f,1.f,0.f,
    0.f,0.f,0.f,1.f,
    0.f,0.f,0.f,0.f,
};

TEST_F(OpenGLGraphComputeTest, BFS_ChainGraph_Depth0_StartsOnly) {
    const uint32_t starts[] = {0u};
    auto res = backend_.batchBFS(kChainAdj4, 4, starts, 1, 0);
    ASSERT_EQ(res.size(), 1u);
    // depth 0: only the start vertex
    ASSERT_EQ(res[0].size(), 1u);
    EXPECT_EQ(res[0][0], 0u);
}

TEST_F(OpenGLGraphComputeTest, BFS_ChainGraph_Depth2_Reachable) {
    const uint32_t starts[] = {0u};
    auto res = backend_.batchBFS(kChainAdj4, 4, starts, 1, 2);
    ASSERT_EQ(res.size(), 1u);
    // From 0 with depth 2: can reach 0,1,2
    EXPECT_GE(res[0].size(), 1u);
    // 0 must be in result
    bool found0 = false;
    for (auto v : res[0]) if (v == 0u) found0 = true;
    EXPECT_TRUE(found0);
}

TEST_F(OpenGLGraphComputeTest, BFS_ChainGraph_FullDepth_AllReachable) {
    const uint32_t starts[] = {0u};
    auto res = backend_.batchBFS(kChainAdj4, 4, starts, 1, 10);
    ASSERT_EQ(res.size(), 1u);
    // All 4 vertices reachable
    EXPECT_EQ(res[0].size(), 4u);
}

TEST_F(OpenGLGraphComputeTest, BFS_MultipleStarts) {
    const uint32_t starts[] = {0u, 2u};
    auto res = backend_.batchBFS(kChainAdj4, 4, starts, 2, 10);
    ASSERT_EQ(res.size(), 2u);
    // From 0: can reach all 4
    EXPECT_EQ(res[0].size(), 4u);
    // From 2: can reach 2 and 3
    EXPECT_EQ(res[1].size(), 2u);
}

TEST_F(OpenGLGraphComputeTest, BFS_NullPointerReturnsEmpty) {
    const uint32_t starts[] = {0u};
    EXPECT_TRUE(backend_.batchBFS(nullptr, 4, starts, 1, 1).empty());
    EXPECT_TRUE(backend_.batchBFS(kChainAdj4, 4, nullptr, 1, 1).empty());
}

TEST_F(OpenGLGraphComputeTest, BFS_ZeroVerticesReturnsEmpty) {
    const uint32_t starts[] = {0u};
    EXPECT_TRUE(backend_.batchBFS(kChainAdj4, 0, starts, 1, 1).empty());
}

TEST_F(OpenGLGraphComputeTest, ShortestPath_ChainGraph_DirectPath) {
    const uint32_t starts[] = {0u};
    const uint32_t ends[]   = {3u};
    auto res = backend_.batchShortestPath(kChainAdj4, kChainWgt4, 4, starts, ends, 1);
    ASSERT_EQ(res.size(), 1u);
    // Path: 0→1→2→3
    ASSERT_EQ(res[0].size(), 4u);
    EXPECT_EQ(res[0][0], 0u);
    EXPECT_EQ(res[0][3], 3u);
}

TEST_F(OpenGLGraphComputeTest, ShortestPath_SameStartEnd) {
    const uint32_t starts[] = {2u};
    const uint32_t ends[]   = {2u};
    auto res = backend_.batchShortestPath(kChainAdj4, kChainWgt4, 4, starts, ends, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_GE(res[0].size(), 1u);
    EXPECT_EQ(res[0][0], 2u);
}

TEST_F(OpenGLGraphComputeTest, ShortestPath_Unreachable_EmptyPath) {
    // Chain 0→1→2→3: vertex 0 is NOT reachable from vertex 3 (directed)
    const uint32_t starts[] = {3u};
    const uint32_t ends[]   = {0u};
    auto res = backend_.batchShortestPath(kChainAdj4, kChainWgt4, 4, starts, ends, 1);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_TRUE(res[0].empty());  // unreachable
}

TEST_F(OpenGLGraphComputeTest, ShortestPath_NullPointerReturnsEmpty) {
    const uint32_t starts[] = {0u}, ends[] = {3u};
    EXPECT_TRUE(backend_.batchShortestPath(nullptr,      kChainWgt4, 4, starts, ends, 1).empty());
    EXPECT_TRUE(backend_.batchShortestPath(kChainAdj4,   nullptr,    4, starts, ends, 1).empty());
    EXPECT_TRUE(backend_.batchShortestPath(kChainAdj4,   kChainWgt4, 4, nullptr, ends, 1).empty());
    EXPECT_TRUE(backend_.batchShortestPath(kChainAdj4,   kChainWgt4, 4, starts, nullptr, 1).empty());
}

TEST_F(OpenGLGraphComputeTest, ShortestPath_MultiplePairs) {
    // Pairs: (0→3) and (1→3)
    const uint32_t starts[] = {0u, 1u};
    const uint32_t ends[]   = {3u, 3u};
    auto res = backend_.batchShortestPath(kChainAdj4, kChainWgt4, 4, starts, ends, 2);
    ASSERT_EQ(res.size(), 2u);
    // 0→3: path length 4
    EXPECT_EQ(res[0].size(), 4u);
    // 1→3: path length 3
    EXPECT_EQ(res[1].size(), 3u);
}

#endif  // THEMIS_ENABLE_OPENGL
