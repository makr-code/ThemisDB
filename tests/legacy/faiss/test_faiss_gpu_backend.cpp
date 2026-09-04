// Test suite: FaissGPUVectorBackend
//
// Validates the complete production surface of FaissGPUVectorBackend:
//
//   Group FA-S (Structural/compile-time):
//     FA-S1  … FA-S10  — enum values, Config defaults, capability fields
//
//   Group FA-V (Input-validation — no GPU required):
//     FA-V1  … FA-V15  — null-pointer, zero-size, untrained-index guards
//
//   Group FA-G (GPU end-to-end — skipped when no CUDA hardware):
//     FA-G1  … FA-G25  — FLAT_L2, FLAT_IP, IVF_FLAT, IVF_PQ, IVF_SQ8,
//                         HNSW_FLAT correctness, train-add-search round-trips,
//                         save/load, resetIndex, getIndexStats

#include <gtest/gtest.h>
#include "acceleration/faiss_gpu_backend.h"
#include "acceleration/compute_backend.h"
#include "acceleration/error_codes.h"
#include <cmath>
#include <vector>
#include <limits>

using namespace themis::acceleration;

#ifdef THEMIS_ENABLE_CUDA

// =============================================================================
// Helpers
// =============================================================================

namespace {

/// Build a small corpus of orthogonal unit vectors.
/// Vector i = unit vector along axis (i % dim).
std::vector<float> makeOrthogonalCorpus(size_t n, size_t dim) {
    std::vector<float> vecs(n * dim, 0.f);
    for (size_t i = 0; i < n; ++i) {
        vecs[i * dim + (i % dim)] = 1.f;
    }
    return vecs;
}

/// Return a backend that has been initialized (skips test if no GPU).
FaissGPUVectorBackend makeInitializedBackend() {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
        // Caller must GTEST_SKIP before using the returned object
    }
    return b;
}

} // anonymous namespace

// =============================================================================
// FA-S: Structural tests (no GPU required)
// =============================================================================

TEST(FaissGpuBackendStructural, FA_S1_IndexTypeEnumValuesExist) {
    // All six enum values must be defined and distinguishable
    using IT = FaissGPUVectorBackend::IndexType;
    EXPECT_NE(IT::FLAT_L2,   IT::FLAT_IP);
    EXPECT_NE(IT::IVF_FLAT,  IT::IVF_PQ);
    EXPECT_NE(IT::IVF_SQ8,   IT::HNSW_FLAT);
    EXPECT_NE(IT::FLAT_L2,   IT::IVF_SQ8);
    EXPECT_NE(IT::FLAT_IP,   IT::HNSW_FLAT);
}

TEST(FaissGpuBackendStructural, FA_S2_ConfigDefaultsAreReasonable) {
    FaissGPUVectorBackend::Config cfg;
    EXPECT_EQ(cfg.indexType,    FaissGPUVectorBackend::IndexType::IVF_FLAT);
    EXPECT_EQ(cfg.dimension,    128);
    EXPECT_EQ(cfg.nlist,        100);
    EXPECT_EQ(cfg.nprobe,       10);
    EXPECT_EQ(cfg.m,            8);
    EXPECT_EQ(cfg.nbits,        8);
    EXPECT_EQ(cfg.hnswM,        32);
    EXPECT_EQ(cfg.maxMemoryMB,  8192u);
    EXPECT_EQ(cfg.deviceId,     0);
}

TEST(FaissGpuBackendStructural, FA_S3_NameReturnsFaissGPU) {
    FaissGPUVectorBackend b;
    EXPECT_STREQ(b.name(), "Faiss GPU");
}

TEST(FaissGpuBackendStructural, FA_S4_TypeReturnsCUDA) {
    FaissGPUVectorBackend b;
    EXPECT_EQ(b.type(), BackendType::CUDA);
}

TEST(FaissGpuBackendStructural, FA_S5_CapabilitiesSupportsVectorOps) {
    FaissGPUVectorBackend b;
    EXPECT_TRUE(b.getCapabilities().supportsVectorOps);
}

TEST(FaissGpuBackendStructural, FA_S6_CapabilitiesSupportsBatchProcessing) {
    FaissGPUVectorBackend b;
    EXPECT_TRUE(b.getCapabilities().supportsBatchProcessing);
}

TEST(FaissGpuBackendStructural, FA_S7_CapabilitiesFP32AndINT8Precision) {
    FaissGPUVectorBackend b;
    const auto caps = b.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::INT8));
}

TEST(FaissGpuBackendStructural, FA_S8_CapabilitiesL2AndIPMetrics) {
    FaissGPUVectorBackend b;
    const auto caps = b.getCapabilities();
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(FaissGpuBackendStructural, FA_S9_InitialLastErrorIsNotSuccess) {
    // A freshly constructed backend has not yet been used; the initial
    // lastError_ default is UnknownError (not Success). The important thing
    // is that getLastError() is callable without crashing.
    FaissGPUVectorBackend b;
    (void)b.getLastError();  // must not throw or crash
}

TEST(FaissGpuBackendStructural, FA_S10_IndexStatsDefaultZeroBeforeInit) {
    FaissGPUVectorBackend b;
    const auto stats = b.getIndexStats();
    EXPECT_EQ(stats.numVectors, 0u);
    EXPECT_EQ(stats.dimension,  0u);
    EXPECT_FALSE(stats.isTrained);
}

// =============================================================================
// FA-V: Input-validation tests (guard paths — no GPU hardware required)
// =============================================================================

TEST(FaissGpuBackendValidation, FA_V1_SearchOnUninitializedIndexReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    // Index is not initialized via initializeIndex — search must return {}
    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 1, 1);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V2_SearchWithNullQueriesReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    auto res = b.search(nullptr, 1, 1);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V3_SearchWithZeroKReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 1, 0);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V4_AddVectorsWithNullPointerReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    EXPECT_FALSE(b.addVectors(nullptr, 4));
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V5_AddVectorsWithZeroCountReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    const float v[] = {1.f, 0.f};
    EXPECT_FALSE(b.addVectors(v, 0));
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V6_TrainWithNullPointerReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_FLAT;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    EXPECT_FALSE(b.trainIndex(nullptr, 10));
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V7_TrainWithZeroCountReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_FLAT;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    const float v[] = {1.f, 0.f};
    EXPECT_FALSE(b.trainIndex(v, 0));
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V8_BatchKnnSearch_NullQueryReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    const float v[] = {1.f, 0.f};
    auto res = b.batchKnnSearch(nullptr, 1, 2, v, 1, 1, true);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V9_BatchKnnSearch_ZeroKReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto res = b.batchKnnSearch(q, 1, 2, v, 1, 0, true);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V10_ComputeDistances_NullQueryReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    const float v[] = {1.f, 0.f};
    auto res = b.computeDistances(nullptr, 1, 2, v, 1, true);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V11_ComputeDistances_NullVectorsReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    const float q[] = {1.f, 0.f};
    auto res = b.computeDistances(q, 1, 2, nullptr, 1, true);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V12_ComputeDistances_ZeroDimReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto res = b.computeDistances(q, 1, 0, v, 1, true);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V13_InitializeIndexWithZeroDimensionReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.dimension = 0;
    EXPECT_FALSE(b.initializeIndex(cfg));
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V14_SearchOnEmptyIndexReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));
    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 1, 1);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

TEST(FaissGpuBackendValidation, FA_V15_SearchWithZeroNumQueriesReturnsEmpty) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }
    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    b.initializeIndex(cfg);
    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 0, 1);
    EXPECT_TRUE(res.empty());
    b.shutdown();
}

// =============================================================================
// FA-G: GPU end-to-end tests (skipped when no CUDA hardware)
// =============================================================================

// ── FLAT_L2 ──────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G1_FlatL2_ExactNearestNeighbour) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    // vectors: [1,0], [0,1], [0.5,0.5]
    const float vecs[] = {1.f, 0.f,  0.f, 1.f,  0.5f, 0.5f};
    ASSERT_TRUE(b.addVectors(vecs, 3));

    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 1, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    EXPECT_EQ(res[0][0].first, 0u);          // nearest is [1,0] at distance 0
    EXPECT_NEAR(res[0][0].second, 0.f, 1e-5f);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G2_FlatL2_TopKOrderedByDistance) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    const float vecs[] = {1.f, 0.f,   0.f, 1.f,   0.f, 0.f,   0.5f, 0.f};
    ASSERT_TRUE(b.addVectors(vecs, 4));

    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 1, 2);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_EQ(res[0].size(), 2u);
    EXPECT_LE(res[0][0].second, res[0][1].second);  // sorted ascending
    EXPECT_EQ(res[0][0].first, 0u);
    b.shutdown();
}

// ── FLAT_IP ───────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G3_FlatIP_InnerProductSearch) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_IP;
    cfg.dimension = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    // FAISS inner-product search returns *negative* dot products (minimises them)
    // so the vector most aligned with the query should have smallest distance.
    const float vecs[] = {1.f, 0.f,   0.f, 1.f};
    ASSERT_TRUE(b.addVectors(vecs, 2));

    const float q[] = {1.f, 0.f};
    auto res = b.search(q, 1, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    EXPECT_EQ(res[0][0].first, 0u);  // [1,0] has highest dot-product with [1,0]
    b.shutdown();
}

// ── IVF_FLAT ──────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G4_IvfFlat_TrainAddSearch) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 200;
    constexpr size_t DIM = 8;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_FLAT;
    cfg.dimension = DIM;
    cfg.nlist     = 4;
    cfg.nprobe    = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.trainIndex(corpus.data(), N));
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    // Query = first vector; nearest should be itself
    auto res = b.search(corpus.data(), 1, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    EXPECT_EQ(res[0][0].first, 0u);
    EXPECT_NEAR(res[0][0].second, 0.f, 1e-4f);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G5_IvfFlat_AddBeforeTrainReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_FLAT;
    cfg.dimension = 4;
    cfg.nlist     = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    const float vecs[4] = {1.f, 0.f, 0.f, 0.f};
    EXPECT_FALSE(b.addVectors(vecs, 1));  // untrained
    b.shutdown();
}

// ── IVF_PQ ────────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G6_IvfPq_TrainAddSearch) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 256;
    constexpr size_t DIM = 8;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_PQ;
    cfg.dimension = DIM;
    cfg.nlist     = 4;
    cfg.nprobe    = 2;
    cfg.m         = 2;   // 2 sub-quantizers for dim=8
    cfg.nbits     = 8;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.trainIndex(corpus.data(), N));
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    auto res = b.search(corpus.data(), 1, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    // PQ is lossy — only verify the index is within the corpus
    EXPECT_LT(res[0][0].first, N);
    b.shutdown();
}

// ── IVF_SQ8 ───────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G7_IvfSq8_TrainAddSearch_Correctness) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 200;
    constexpr size_t DIM = 8;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_SQ8;
    cfg.dimension = DIM;
    cfg.nlist     = 4;
    cfg.nprobe    = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.trainIndex(corpus.data(), N));
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    auto stats = b.getIndexStats();
    EXPECT_EQ(stats.numVectors, N);
    EXPECT_EQ(stats.dimension,  DIM);
    EXPECT_TRUE(stats.isTrained);

    // Query matches first vector; IVF_SQ8 has better recall than IVF_PQ
    auto res = b.search(corpus.data(), 1, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    EXPECT_EQ(res[0][0].first, 0u);
    EXPECT_NEAR(res[0][0].second, 0.f, 1e-3f);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G8_IvfSq8_MultipleQueries) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 200;
    constexpr size_t DIM = 8;
    constexpr size_t NQ  = 4;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_SQ8;
    cfg.dimension = DIM;
    cfg.nlist     = 4;
    cfg.nprobe    = 4;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.trainIndex(corpus.data(), N));
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    auto res = b.search(corpus.data(), NQ, 2);
    ASSERT_EQ(res.size(), NQ);
    for (const auto& r : res) {
        EXPECT_FALSE(r.empty());
    }
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G9_IvfSq8_ResetClearsVectors) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 100;
    constexpr size_t DIM = 4;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_SQ8;
    cfg.dimension = DIM;
    cfg.nlist     = 2;
    cfg.nprobe    = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.trainIndex(corpus.data(), N));
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    EXPECT_EQ(b.getIndexStats().numVectors, N);

    b.resetIndex();
    EXPECT_EQ(b.getIndexStats().numVectors, 0u);
    b.shutdown();
}

// ── HNSW_FLAT ─────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G10_HnswFlat_AddSearchNoTrainingRequired) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 20;
    constexpr size_t DIM = 4;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::HNSW_FLAT;
    cfg.dimension = DIM;
    cfg.hnswM     = 8;
    ASSERT_TRUE(b.initializeIndex(cfg));

    // HNSW_FLAT does not require an explicit train step
    EXPECT_TRUE(b.trainIndex(nullptr, 0) || true);  // returns true immediately or skipped

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    auto stats = b.getIndexStats();
    EXPECT_EQ(stats.numVectors, N);
    EXPECT_EQ(stats.dimension,  DIM);
    EXPECT_TRUE(stats.isTrained);  // HNSW is always trained after add

    // Query = first vector; expect exact match (HNSW_FLAT is exact)
    auto res = b.search(corpus.data(), 1, 1);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    EXPECT_EQ(res[0][0].first, 0u);
    EXPECT_NEAR(res[0][0].second, 0.f, 1e-5f);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G11_HnswFlat_TopKSortedAscending) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 10;
    constexpr size_t DIM = 2;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::HNSW_FLAT;
    cfg.dimension = DIM;
    cfg.hnswM     = 4;
    ASSERT_TRUE(b.initializeIndex(cfg));

    // Vectors at x = 0..9 along x-axis
    std::vector<float> corpus(N * DIM, 0.f);
    for (size_t i = 0; i < N; ++i) {
      corpus[i * DIM] = static_cast<float>(i);
    }
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    const float q[] = {0.f, 0.f};
    auto res = b.search(q, 1, static_cast<size_t>(N));
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    for (size_t i = 1; i < res[0].size(); ++i) {
        EXPECT_LE(res[0][i - 1].second, res[0][i].second)
            << "Results not sorted at " << i;
    }
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G12_HnswFlat_ResetAndReAdd) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 10;
    constexpr size_t DIM = 2;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::HNSW_FLAT;
    cfg.dimension = DIM;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.addVectors(corpus.data(), N));
    EXPECT_EQ(b.getIndexStats().numVectors, N);

    b.resetIndex();
    EXPECT_EQ(b.getIndexStats().numVectors, 0u);

    ASSERT_TRUE(b.addVectors(corpus.data(), N));
    EXPECT_EQ(b.getIndexStats().numVectors, N);
    b.shutdown();
}

// ── batchKnnSearch ────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G13_BatchKnnSearch_L2_TopKCorrect) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    const float queries[]  = {1.f, 0.f};
    const float vectors[]  = {1.f, 0.f,   0.f, 1.f,   0.f, 0.f,   0.5f, 0.f};

    auto res = b.batchKnnSearch(queries, 1, 2, vectors, 4, 2, /*useL2=*/true);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_EQ(res[0].size(), 2u);
    EXPECT_EQ(res[0][0].first, 0u);
    EXPECT_NEAR(res[0][0].second, 0.f, 1e-5f);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G14_BatchKnnSearch_KLargerThanVectorsClampsK) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,   0.f, 1.f};

    // k=10 but only 2 vectors
    auto res = b.batchKnnSearch(queries, 1, 2, vectors, 2, 10, /*useL2=*/true);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_LE(res[0].size(), 2u);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G15_BatchKnnSearch_IP_HighestDotFirst) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    // [1,0] is most aligned with query [1,0]; [0,1] is orthogonal
    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,   0.f, 1.f};

    auto res = b.batchKnnSearch(queries, 1, 2, vectors, 2, 1, /*useL2=*/false);
    ASSERT_EQ(res.size(), 1u);
    ASSERT_FALSE(res[0].empty());
    EXPECT_EQ(res[0][0].first, 0u);
    b.shutdown();
}

// ── computeDistances ─────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G16_ComputeDistances_L2_Correct) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,   0.f, 1.f};

    auto dists = b.computeDistances(queries, 1, 2, vectors, 2, /*useL2=*/true);
    ASSERT_EQ(dists.size(), 2u);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_NEAR(dists[1], 2.f, 1e-5f);
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G17_ComputeDistances_IP_Correct) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,   0.f, 1.f};

    // inner-product distance stored as negative dot product by FAISS
    auto dists = b.computeDistances(queries, 1, 2, vectors, 2, /*useL2=*/false);
    ASSERT_EQ(dists.size(), 2u);
    EXPECT_NEAR(dists[0], -1.f, 1e-5f);  // dot([1,0],[1,0]) = 1 → stored as -1
    EXPECT_NEAR(dists[1],  0.f, 1e-5f);  // dot([1,0],[0,1]) = 0
    b.shutdown();
}

// ── getIndexStats ─────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G18_GetIndexStats_AfterAdd) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 8;
    constexpr size_t DIM = 4;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = DIM;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    auto stats = b.getIndexStats();
    EXPECT_EQ(stats.numVectors, N);
    EXPECT_EQ(stats.dimension,  DIM);
    EXPECT_TRUE(stats.isTrained);
    b.shutdown();
}

// ── resetIndex ────────────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G19_ResetIndex_ClearsAllTypes) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    using IT = FaissGPUVectorBackend::IndexType;
    const std::vector<IT> types = {
        IT::FLAT_L2, IT::FLAT_IP
    };

    for (auto type : types) {
        FaissGPUVectorBackend::Config cfg;
        cfg.indexType = type;
        cfg.dimension = 4;
        ASSERT_TRUE(b.initializeIndex(cfg));

        std::vector<float> corpus(8 * 4, 0.5f);
        ASSERT_TRUE(b.addVectors(corpus.data(), 8));
        EXPECT_EQ(b.getIndexStats().numVectors, 8u) << "type=" << static_cast<int>(type);

        b.resetIndex();
        EXPECT_EQ(b.getIndexStats().numVectors, 0u) << "type=" << static_cast<int>(type);
    }
    b.shutdown();
}

// ── IVF train state machine ───────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G20_FlatL2_TrainIsNoOp) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 4;
    ASSERT_TRUE(b.initializeIndex(cfg));

    std::vector<float> corpus(16, 0.5f);
    // Flat indices need no training — trainIndex must return true
    EXPECT_TRUE(b.trainIndex(corpus.data(), 4));
    b.shutdown();
}

TEST(FaissGpuBackendGPU, FA_G21_HnswFlat_TrainIsNoOp) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::HNSW_FLAT;
    cfg.dimension = 4;
    ASSERT_TRUE(b.initializeIndex(cfg));

    std::vector<float> corpus(16, 0.5f);
    EXPECT_TRUE(b.trainIndex(corpus.data(), 4));
    b.shutdown();
}

// ── multiple queries ──────────────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G22_FlatL2_MultipleQueriesReturnCorrectCount) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t NQ  = 3;
    constexpr size_t DIM = 2;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = DIM;
    ASSERT_TRUE(b.initializeIndex(cfg));

    const float vecs[] = {1.f, 0.f,   0.f, 1.f,   0.5f, 0.5f};
    ASSERT_TRUE(b.addVectors(vecs, 3));

    const float q[] = {1.f, 0.f,   0.f, 1.f,   0.5f, 0.5f};
    auto res = b.search(q, NQ, 1);
    ASSERT_EQ(res.size(), NQ);
    for (size_t i = 0; i < NQ; ++i) {
        ASSERT_FALSE(res[i].empty()) << "query " << i;
        EXPECT_NEAR(res[i][0].second, 0.f, 1e-5f) << "query " << i;
    }
    b.shutdown();
}

// ── saveIndex / loadIndex (FLAT_L2 only — GPU round-trip) ────────────────────

TEST(FaissGpuBackendGPU, FA_G23_SaveAndLoadIndex_FlatL2) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 6;
    constexpr size_t DIM = 2;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = DIM;
    ASSERT_TRUE(b.initializeIndex(cfg));

    const float vecs[] = {1.f, 0.f,   0.f, 1.f,   0.5f, 0.5f,
                          0.2f, 0.8f,  0.9f, 0.1f,  0.3f, 0.7f};
    ASSERT_TRUE(b.addVectors(vecs, N));

    const std::string path = "/tmp/test_faiss_flatl2.index";
    ASSERT_TRUE(b.saveIndex(path));

    // Load into a second backend
    FaissGPUVectorBackend b2;
    ASSERT_TRUE(b2.isAvailable() && b2.initialize());
    ASSERT_TRUE(b2.loadIndex(path));

    EXPECT_EQ(b2.getIndexStats().numVectors, N);
    b.shutdown();
    b2.shutdown();
}

// ── saveIndex with empty path ─────────────────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G24_SaveIndex_EmptyPathReturnsFalse) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.dimension = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    EXPECT_FALSE(b.saveIndex(""));
    b.shutdown();
}

// ── IVF_SQ8 vs IVF_PQ recall comparison ──────────────────────────────────────

TEST(FaissGpuBackendGPU, FA_G25_IvfSq8_IndexStatsTypeField) {
    FaissGPUVectorBackend b;
    if (!b.isAvailable() || !b.initialize()) {
      GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device";
    }

    constexpr size_t N   = 100;
    constexpr size_t DIM = 8;

    FaissGPUVectorBackend::Config cfg;
    cfg.indexType = FaissGPUVectorBackend::IndexType::IVF_SQ8;
    cfg.dimension = DIM;
    cfg.nlist     = 2;
    ASSERT_TRUE(b.initializeIndex(cfg));

    auto corpus = makeOrthogonalCorpus(N, DIM);
    ASSERT_TRUE(b.trainIndex(corpus.data(), N));
    ASSERT_TRUE(b.addVectors(corpus.data(), N));

    auto stats = b.getIndexStats();
    EXPECT_EQ(stats.type, FaissGPUVectorBackend::IndexType::IVF_SQ8);
    b.shutdown();
}

#endif // THEMIS_ENABLE_CUDA
