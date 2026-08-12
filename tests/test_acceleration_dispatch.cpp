#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <cstdint>
#include <algorithm>

#include "acceleration/cpu_backend.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/kernel_invocation.h"

using namespace themis::acceleration;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Returns the squared Euclidean distance between two float vectors.
float squaredL2(const float* a, const float* b, int dim) {
    float s = 0.f;
    for (int i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

/// Returns cosine distance (1 - cosine_similarity) between two float vectors.
float cosineDistance(const float* a, const float* b, int dim) {
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (int i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    const float denom = std::sqrt(na) * std::sqrt(nb);
    return (denom > 1e-10f) ? 1.f - dot / denom : 1.f;
}

} // namespace

// ============================================================================
// CPU dispatch table fixture
// ============================================================================

class CpuANNDispatchTest : public ::testing::Test {
protected:
    CPUVectorBackend backend_;
    ANNKernelDispatch disp_;

    void SetUp() override {
        ASSERT_TRUE(backend_.initialize());
        disp_ = backend_.populateANNDispatch();
        ASSERT_NE(disp_.launchL2Distance,   nullptr);
        ASSERT_NE(disp_.launchCosine,       nullptr);
        ASSERT_NE(disp_.launchInnerProduct, nullptr);
        ASSERT_NE(disp_.launchTopK,         nullptr);
    }
};

// ============================================================================
// L2 distance dispatch
// ============================================================================

TEST_F(CpuANNDispatchTest, L2Distance_ZeroQuery_NonzeroVectors) {
    // A zero query should have equal distance to all unit-axis vectors
    const int dim = 3, nVec = 3, nQry = 1;
    const std::vector<float> queries = {0.f, 0.f, 0.f};
    const std::vector<float> vectors = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    std::vector<float> out(nQry * nVec, -1.f);

    ASSERT_EQ(disp_.launchL2Distance(queries.data(), vectors.data(),
                                     out.data(), nQry, nVec, dim, nullptr), 0);

    // Each axis vector is at squared distance 1 from origin
    for (int v = 0; v < nVec; ++v) {
        EXPECT_FLOAT_EQ(out[v], 1.f) << "vector index " << v;
    }
}

TEST_F(CpuANNDispatchTest, L2Distance_IdenticalVectors) {
    const int dim = 4, nVec = 2, nQry = 1;
    const std::vector<float> q = {1.f, 2.f, 3.f, 4.f};
    const std::vector<float> db = {1.f, 2.f, 3.f, 4.f,   // identical to q
                                   0.f, 0.f, 0.f, 0.f};
    std::vector<float> out(nQry * nVec);

    ASSERT_EQ(disp_.launchL2Distance(q.data(), db.data(), out.data(),
                                     nQry, nVec, dim, nullptr), 0);

    EXPECT_FLOAT_EQ(out[0], 0.f);               // identical → distance 0
    EXPECT_FLOAT_EQ(out[1], squaredL2(q.data(), db.data() + dim, dim));
}

TEST_F(CpuANNDispatchTest, L2Distance_MatchesScalarReference) {
    const int dim = 8, nVec = 4, nQry = 2;
    std::vector<float> q(nQry * dim), v(nVec * dim);
    for (int i = 0; i < nQry * dim; ++i) q[i] = static_cast<float>(i % 5) - 2.f;
    for (int i = 0; i < nVec * dim; ++i) v[i] = static_cast<float>(i % 7) - 3.f;

    std::vector<float> out(nQry * nVec);
    ASSERT_EQ(disp_.launchL2Distance(q.data(), v.data(), out.data(),
                                     nQry, nVec, dim, nullptr), 0);

    for (int qi = 0; qi < nQry; ++qi) {
        for (int vi = 0; vi < nVec; ++vi) {
            float expected = squaredL2(q.data() + qi * dim,
                                       v.data() + vi * dim, dim);
            EXPECT_NEAR(out[qi * nVec + vi], expected, 1e-4f)
                << "qi=" << qi << " vi=" << vi;
        }
    }
}

// ============================================================================
// Cosine distance dispatch
// ============================================================================

TEST_F(CpuANNDispatchTest, CosineDistance_OrthogonalVectors) {
    const int dim = 2, nVec = 2, nQry = 1;
    const std::vector<float> q  = {1.f, 0.f};
    const std::vector<float> db = {1.f, 0.f,    // same direction → dist 0
                                   0.f, 1.f};   // orthogonal → dist 1
    std::vector<float> out(nQry * nVec);

    ASSERT_EQ(disp_.launchCosine(q.data(), db.data(), out.data(),
                                 nQry, nVec, dim, nullptr), 0);

    EXPECT_NEAR(out[0], 0.f, 1e-6f);
    EXPECT_NEAR(out[1], 1.f, 1e-6f);
}

TEST_F(CpuANNDispatchTest, CosineDistance_ZeroVector_ReturnMaxDist) {
    const int dim = 3, nVec = 1, nQry = 1;
    const std::vector<float> q  = {1.f, 1.f, 1.f};
    const std::vector<float> db = {0.f, 0.f, 0.f};
    std::vector<float> out(1);

    ASSERT_EQ(disp_.launchCosine(q.data(), db.data(), out.data(),
                                 nQry, nVec, dim, nullptr), 0);

    EXPECT_FLOAT_EQ(out[0], 1.f);
}

// ============================================================================
// Inner-product dispatch (negative dot product so smaller = better)
// ============================================================================

TEST_F(CpuANNDispatchTest, InnerProduct_KnownValues) {
    const int dim = 3, nVec = 2, nQry = 1;
    const std::vector<float> q  = {1.f, 2.f, 3.f};
    const std::vector<float> db = {1.f, 0.f, 0.f,   // dot = 1
                                   1.f, 1.f, 1.f};  // dot = 6
    std::vector<float> out(nQry * nVec);

    ASSERT_EQ(disp_.launchInnerProduct(q.data(), db.data(), out.data(),
                                       nQry, nVec, dim, nullptr), 0);

    // Stored as negative IP: smaller = better match
    EXPECT_FLOAT_EQ(out[0], -1.f);
    EXPECT_FLOAT_EQ(out[1], -6.f);
}

// ============================================================================
// TopK dispatch
// ============================================================================

TEST_F(CpuANNDispatchTest, TopK_SelectsSmallest) {
    // 6 pre-computed distances for 1 query; pick top-3 smallest
    const int nQry = 1, nVec = 6, topK = 3;
    const std::vector<float> dists = {5.f, 1.f, 8.f, 2.f, 9.f, 0.f};
    std::vector<uint32_t> outIdx(topK);
    std::vector<float>    outDst(topK);

    ASSERT_EQ(disp_.launchTopK(dists.data(), outIdx.data(), outDst.data(),
                                nQry, nVec, topK, nullptr), 0);

    // Sorted ascending: dists[5]=0, dists[1]=1, dists[3]=2
    EXPECT_FLOAT_EQ(outDst[0], 0.f); EXPECT_EQ(outIdx[0], 5u);
    EXPECT_FLOAT_EQ(outDst[1], 1.f); EXPECT_EQ(outIdx[1], 1u);
    EXPECT_FLOAT_EQ(outDst[2], 2.f); EXPECT_EQ(outIdx[2], 3u);
}

TEST_F(CpuANNDispatchTest, TopK_MultipleQueries) {
    const int nQry = 2, nVec = 4, topK = 2;
    // Row 0: 3,1,4,1 — top-2: index 1 (dist=1), index 3 (dist=1)
    // Row 1: 9,2,6,0 — top-2: index 3 (dist=0), index 1 (dist=2)
    const std::vector<float> dists = {3.f, 1.f, 4.f, 1.f,
                                      9.f, 2.f, 6.f, 0.f};
    std::vector<uint32_t> outIdx(nQry * topK);
    std::vector<float>    outDst(nQry * topK);

    ASSERT_EQ(disp_.launchTopK(dists.data(), outIdx.data(), outDst.data(),
                                nQry, nVec, topK, nullptr), 0);

    // Row 0
    EXPECT_LE(outDst[0], outDst[1]);  // sorted ascending
    // Row 1
    EXPECT_FLOAT_EQ(outDst[2], 0.f); EXPECT_EQ(outIdx[2], 3u);
    EXPECT_LE(outDst[2], outDst[3]);
}

// ============================================================================
// CPU Geo dispatch fixture
// ============================================================================

class CpuGeoDispatchTest : public ::testing::Test {
protected:
    CPUGeoBackend backend_;
    GeoKernelDispatch disp_;

    void SetUp() override {
        ASSERT_TRUE(backend_.initialize());
        disp_ = backend_.populateGeoDispatch();
        ASSERT_NE(disp_.launchDistance,    nullptr);
        ASSERT_NE(disp_.launchContainment, nullptr);
    }
};

// ============================================================================
// Haversine distance dispatch
// ============================================================================

TEST_F(CpuGeoDispatchTest, HaversineDistance_LondonParis) {
    // London (51.5074, -0.1278) — Paris (48.8566, 2.3522)
    // Approximate great-circle distance ≈ 340–345 km
    const std::vector<double> lats1 = {51.5074};
    const std::vector<double> lons1 = {-0.1278};
    const std::vector<double> lats2 = {48.8566};
    const std::vector<double> lons2 = {2.3522};
    std::vector<float> out(1);

    ASSERT_EQ(disp_.launchDistance(lats1.data(), lons1.data(),
                                   lats2.data(), lons2.data(),
                                   out.data(), 1,
                                   GeoDistanceFormula::HAVERSINE, nullptr), 0);

    EXPECT_GT(out[0], 330.f);
    EXPECT_LT(out[0], 360.f);
}

TEST_F(CpuGeoDispatchTest, HaversineDistance_SamePoint_IsZero) {
    const std::vector<double> lats = {48.0};
    const std::vector<double> lons = {11.0};
    std::vector<float> out(1);

    ASSERT_EQ(disp_.launchDistance(lats.data(), lons.data(),
                                   lats.data(), lons.data(),
                                   out.data(), 1,
                                   GeoDistanceFormula::HAVERSINE, nullptr), 0);

    EXPECT_NEAR(out[0], 0.f, 1e-3f);
}

TEST_F(CpuGeoDispatchTest, HaversineDistance_Batch_AllNonNegative) {
    const int count = 100;
    std::vector<double> lats1(count), lons1(count), lats2(count), lons2(count);
    for (int i = 0; i < count; ++i) {
        lats1[i] = -90.0 + 1.8 * i;
        lons1[i] = -180.0 + 3.6 * i;
        lats2[i] = -89.0 + 1.8 * i;
        lons2[i] = -179.0 + 3.6 * i;
    }
    std::vector<float> out(count);

    ASSERT_EQ(disp_.launchDistance(lats1.data(), lons1.data(),
                                   lats2.data(), lons2.data(),
                                   out.data(), count,
                                   GeoDistanceFormula::HAVERSINE, nullptr), 0);

    for (int i = 0; i < count; ++i) {
        EXPECT_GE(out[i], 0.f) << "index " << i;
    }
}

// ============================================================================
// Point-in-polygon containment dispatch
// ============================================================================

TEST_F(CpuGeoDispatchTest, PointInPolygon_InsideAndOutside) {
    // Square polygon: corners at lat/lon (0,0),(10,0),(10,10),(0,10)
    // Stored interleaved: [lat0,lon0, lat1,lon1, ...]
    const std::vector<double> polygon = {
        0.0, 0.0,
        10.0, 0.0,
        10.0, 10.0,
        0.0, 10.0
    };
    const int numVerts = 4;

    const std::vector<double> ptLats = {5.0, 15.0};
    const std::vector<double> ptLons = {5.0,  5.0};
    std::vector<uint8_t> out(2, 255u);

    ASSERT_EQ(disp_.launchContainment(ptLats.data(), ptLons.data(), 2,
                                      polygon.data(), numVerts,
                                      out.data(), nullptr), 0);

    EXPECT_NE(out[0], 0u);  // (5,5) is inside
    EXPECT_EQ(out[1], 0u);  // (15,5) is outside
}

TEST_F(CpuGeoDispatchTest, PointInPolygon_BerlinBoundingBox) {
    // Berlin bounding box (same polygon used in bench_cuda_vs_cpu.cpp)
    const std::vector<double> polygon = {
        52.34, 13.09,  52.34, 13.76,
        52.68, 13.76,  52.68, 13.09
    };
    const int numVerts = 4;

    // Berlin center (inside), Paris (outside)
    const std::vector<double> ptLats = {52.52, 48.86};
    const std::vector<double> ptLons = {13.40,  2.35};
    std::vector<uint8_t> out(2, 255u);

    ASSERT_EQ(disp_.launchContainment(ptLats.data(), ptLons.data(), 2,
                                      polygon.data(), numVerts,
                                      out.data(), nullptr), 0);

    EXPECT_NE(out[0], 0u);  // Berlin center is inside its bounding box
    EXPECT_EQ(out[1], 0u);  // Paris is outside
}

// ============================================================================
// CUDA backend: availability reporting
// ============================================================================

TEST(CudaBackendDispatchTest, CudaVectorBackend_DispatchIsEmptyWhenNotCompiledOrUnavailable) {
    CUDAVectorBackend cuda;
    // Whether CUDA is compiled in or not, we should never crash.
    // If unavailable, populateANNDispatch() returns an empty (all-null) table.
    ANNKernelDispatch d = cuda.populateANNDispatch();

#ifdef THEMIS_ENABLE_CUDA
    if (cuda.isAvailable()) {
        // On a CUDA-capable host all slots should be populated
        EXPECT_NE(d.launchL2Distance,   nullptr);
        EXPECT_NE(d.launchCosine,       nullptr);
        EXPECT_NE(d.launchInnerProduct, nullptr);
        EXPECT_NE(d.launchTopK,         nullptr);
    } else {
        // CUDA compiled-in but no device: table may be populated (function
        // pointers exist) or empty — both are acceptable.  Just verify no-crash.
        (void)d;
    }
#else
    // CUDA not compiled: all slots must be null (CPU fallback applies)
    EXPECT_EQ(d.launchL2Distance,   nullptr);
    EXPECT_EQ(d.launchCosine,       nullptr);
    EXPECT_EQ(d.launchInnerProduct, nullptr);
    EXPECT_EQ(d.launchTopK,         nullptr);
#endif
}

TEST(CudaBackendDispatchTest, CudaGeoBackend_DispatchIsEmptyWhenNotCompiledOrUnavailable) {
    CUDAGeoBackend cuda;
    GeoKernelDispatch d = cuda.populateGeoDispatch();

#ifdef THEMIS_ENABLE_CUDA
    // Just verify we don't crash — slot contents depend on device availability
    (void)d;
#else
    EXPECT_EQ(d.launchDistance,    nullptr);
    EXPECT_EQ(d.launchContainment, nullptr);
#endif
}

TEST(CudaBackendDispatchTest, CudaVectorBackend_InitializeFails_WhenCudaUnavailable) {
    CUDAVectorBackend cuda;
#ifndef THEMIS_ENABLE_CUDA
    // CUDA not compiled: initialize() must return false
    EXPECT_FALSE(cuda.initialize());
    EXPECT_FALSE(cuda.isAvailable());
#else
    // CUDA compiled: result depends on whether a device is present
    bool available = cuda.isAvailable();
    bool initOk    = cuda.initialize();
    EXPECT_EQ(initOk, available);
#endif
}
