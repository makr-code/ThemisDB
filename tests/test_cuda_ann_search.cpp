// Test: CUDA-Accelerated ANN Search
//
// Validates the CUDA-accelerated Approximate Nearest Neighbor (ANN) search
// implementation in CUDAVectorBackend.  All structural tests run on any
// platform (no GPU required); hardware-dependent paths are skipped gracefully
// when CUDA is unavailable.

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

TEST(CudaAnnSearch, DispatchTableStructure_L2AndTopKSlotsDefined) {
    CUDAVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    // Under THEMIS_ENABLE_CUDA the L2, cosine, inner-product, and top-K slots
    // must all be populated; GPU hardware is not required to verify this.
    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);
}

TEST(CudaAnnSearch, DispatchTableStructure_DistanceLauncherForRoutesAllMetrics) {
    CUDAVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            d.launchL2Distance);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        d.launchCosine);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), d.launchInnerProduct);
}

TEST(CudaAnnSearch, Capabilities_SupportedMetricsFlagsSet) {
    CUDAVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    // After fix the backend must advertise all three metrics so that
    // BackendRegistry::selectVectorBackendFor() can select it.
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(CudaAnnSearch, Capabilities_SupportedPrecisionsFP32Set) {
    CUDAVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(CudaAnnSearch, Capabilities_SupportsVectorAndBatch) {
    CUDAVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

// =============================================================================
// Input-validation tests (no GPU required — validate guard paths)
// =============================================================================

TEST(CudaAnnSearch, ComputeDistances_NullQueryReturnsEmpty) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }
    const float vectors[] = {1.f, 0.f};
    auto result = backend.computeDistances(nullptr, 1, 2, vectors, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaAnnSearch, ComputeDistances_ZeroDimReturnsEmpty) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }
    const float q[] = {1.f};
    const float v[] = {1.f};
    auto result = backend.computeDistances(q, 1, 0, v, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_NullQueryReturnsEmpty) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }
    const float vectors[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(nullptr, 1, 2, vectors, 1, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_ZeroKReturnsEmpty) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(q, 1, 2, v, 1, 0, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

// =============================================================================
// GPU-hardware-dependent end-to-end tests
// =============================================================================

TEST(CudaAnnSearch, ComputeDistances_L2_CorrectResults) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }

    // Query [1,0] vs vectors [1,0] and [0,1]
    // Expected squared-L2: 0 and 2
    const float queries[]  = {1.f, 0.f};
    const float vectors[]  = {1.f, 0.f,  0.f, 1.f};

    auto dists = backend.computeDistances(queries, 1, 2, vectors, 2, /*useL2=*/true);

    ASSERT_EQ(dists.size(), 2u);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_NEAR(dists[1], 2.f, 1e-5f);

    backend.shutdown();
}

TEST(CudaAnnSearch, ComputeDistances_Cosine_SameVectorIsZeroDistance) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }

    const float queries[]  = {1.f, 0.f};
    const float vectors[]  = {1.f, 0.f};

    auto dists = backend.computeDistances(queries, 1, 2, vectors, 1, /*useL2=*/false);

    ASSERT_EQ(dists.size(), 1u);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);  // cosine distance of identical vectors = 0

    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_L2_ReturnsCorrectTopK) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }

    // 4 vectors; query = [1,0]; nearest should be idx 0 (dist 0), then idx 3 (dist 0.25)
    const float queries[] = {1.f, 0.f};
    const float vectors[] = {
        1.f, 0.f,   // idx 0: dist² = 0
        0.f, 1.f,   // idx 1: dist² = 2
        0.f, 0.f,   // idx 2: dist² = 1
        0.5f, 0.f,  // idx 3: dist² = 0.25
    };

    auto results = backend.batchKnnSearch(queries, 1, 2, vectors, 4, 2, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 2u);
    EXPECT_EQ(results[0][0].first, 0u);   // closest: idx 0
    EXPECT_NEAR(results[0][0].second, 0.f, 1e-5f);
    EXPECT_EQ(results[0][1].first, 3u);   // 2nd closest: idx 3
    EXPECT_NEAR(results[0][1].second, 0.25f, 1e-5f);

    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_KLargerThanVectors_ClampsK) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,  0.f, 1.f};  // 2 vectors

    // k=5 but only 2 vectors — should return 2 results, not crash
    auto results = backend.batchKnnSearch(queries, 1, 2, vectors, 2, 5, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_LE(results[0].size(), 2u);

    backend.shutdown();
}

// =============================================================================
// Dispatch-table ANN search: inner-product kernel round-trip
// =============================================================================

TEST(CudaAnnSearch, ANNDispatch_InnerProduct_SlotIsNonNullOnGPU) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "CUDA hardware not available";
    }

    ANNKernelDispatch d = backend.populateANNDispatch();
    // Inner-product slot must be wired when CUDA hardware is present
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), nullptr);

    backend.shutdown();
}

#endif // THEMIS_ENABLE_CUDA

// =============================================================================
// CPU-backend parity checks (always run, no GPU required)
// =============================================================================

TEST(CudaAnnSearch, CpuParity_L2DistancesMatch) {
    // Verify CPU backend dispatch produces expected squared-L2 distances.
    // This serves as a reference baseline against which the CUDA results are compared.
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchL2Distance, nullptr);

    const float queries[] = {1.f, 0.f, 0.f};
    const float vectors[] = {1.f, 0.f, 0.f,  0.f, 1.f, 0.f};
    float dists[2] = {};

    const int rc = d.launchL2Distance(queries, vectors, dists,
                                      1, 2, 3, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_NEAR(dists[1], 2.f, 1e-5f);

    backend.shutdown();
}

TEST(CudaAnnSearch, CpuParity_InnerProductDispatch_NegativeDot) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchInnerProduct, nullptr);

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,  0.f, 1.f};
    float dists[2] = {};

    const int rc = d.launchInnerProduct(queries, vectors, dists,
                                        1, 2, 2, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dists[0], -1.f, 1e-5f);  // -dot([1,0],[1,0]) = -1
    EXPECT_NEAR(dists[1],  0.f, 1e-5f);  // -dot([1,0],[0,1]) =  0

    backend.shutdown();
}
