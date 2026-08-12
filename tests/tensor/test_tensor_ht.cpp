/**
 * @file test_tensor_ht.cpp
 * @brief Phase 5 HT decomposition tests.
 *
 * Test IDs
 * --------
 * HT-01  decompose d=2 tensor; reconstruction error < 5%
 * HT-02  decompose d=4 tensor; reconstruction error < 5%
 * HT-03  innerProduct(A, A) == squared Frobenius norm
 * HT-04  cosineSimilarity(A, A) == 1.0
 * HT-05  cosineSimilarity of near-orthogonal tensors < 0.15
 * HT-06  FlatHTIndex: add + search returns the added id first
 * HT-07  FlatHTIndex: search with k=1 returns exactly 1 result
 * HT-08  FlatHTIndex: remove reduces size by 1
 * HT-09  serialization round-trip preserves shape and can reconstruct
 * HT-10  totalParams() < N (compression check on low-rank tensor)
 * HT-11  compressionRatio() > 1 for compressible tensor
 * HT-12  frobeniusNorm == sqrt(innerProduct(A, A))
 * HT-13  distinct tensors have distinct inner products
 * HT-14  HTTrain::clone() produces structurally equal train
 * HT-15  decompose d=2 rank-1 tensor; compression ratio > 1
 * HT-16  FlatHTIndex: serialize / deserialize round-trip
 * HT-17  FlatHTIndex::get() retrieves stored entry by id
 * HT-18  FlatHTIndex: replacing an entry with same id works
 */

#include "storage/hierarchical_tucker_decomposer.h"
#include "tensor/ht_index.h"
#include "tensor/ht_train.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numeric>
#include <random>
#include <vector>

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a flat row-major tensor from a vector of values.
std::vector<float> makeTensor(const std::vector<std::size_t>& shape,
                               float fill_value = 0.0f)
{
    std::size_t N = 1;
    for (auto s : shape) N *= s;
    return std::vector<float>(N, fill_value);
}

/// Build a random tensor with a given seed.
std::vector<float> randomTensor(const std::vector<std::size_t>& shape,
                                 unsigned seed = 42)
{
    std::size_t N = 1;
    for (auto s : shape) N *= s;
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> t(N);
    for (auto& v : t) v = dist(rng);
    return t;
}

/// Build a rank-1 tensor: T[i,j,...] = u[i] * v[j] * ...
std::vector<float> rank1Tensor(const std::vector<std::size_t>& shape, unsigned seed = 7)
{
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    // Factor vectors for each mode
    std::vector<std::vector<float>> factors;
    for (auto s : shape) {
        std::vector<float> f(s);
        for (auto& v : f) v = dist(rng);
        factors.push_back(std::move(f));
    }

    std::size_t N = 1;
    for (auto s : shape) N *= s;
    std::vector<float> t(N, 0.0f);

    std::size_t d = shape.size();
    for (std::size_t flat = 0; flat < N; ++flat) {
        float val = 1.0f;
        std::size_t tmp = flat;
        for (std::size_t k = d; k-- > 0;) {
            std::size_t idx = tmp % shape[k];
            tmp /= shape[k];
            val *= factors[k][idx];
        }
        t[flat] = val;
    }
    return t;
}

/// Frobenius norm of a flat vector.
double frobNorm(const std::vector<float>& v) {
    double s = 0.0;
    for (float x : v) s += static_cast<double>(x) * x;
    return std::sqrt(s);
}

/// Relative error between two flat vectors.
double relError(const std::vector<float>& a, const std::vector<float>& b) {
    double na = frobNorm(a);
    if (na < 1e-12) return 0.0;
    double err = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        double d = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        err += d * d;
    }
    return std::sqrt(err) / na;
}

themis::storage::HierarchicalTuckerDecomposer makeDecomp(std::size_t max_rank = 8,
                                                          double      eps      = 1e-4)
{
    themis::storage::HTConfig cfg;
    cfg.max_rank = max_rank;
    cfg.eps      = eps;
    return themis::storage::HierarchicalTuckerDecomposer(cfg);
}

} // anonymous namespace

// ============================================================================
// HT-01: decompose d=2, reconstruct ≈ original
// ============================================================================
TEST(HierarchicalTucker, HT01_Decompose2D) {
    // max_rank=8 captures full mode rank of [8,8] → near-exact reconstruction
    auto data = randomTensor({8, 8}, 1);
    auto [ht, stats] = makeDecomp(8).decompose(data, {8, 8});
    ASSERT_TRUE(ht.root != nullptr);
    EXPECT_EQ(ht.order(), 2u);

    auto recon = ht.reconstruct();
    ASSERT_EQ(recon.size(), data.size());
    double err = relError(data, recon);
    EXPECT_LT(err, 5e-4) << "Reconstruction error too large: " << err;
}

// ============================================================================
// HT-02: decompose d=4, reconstruct ≈ original
// ============================================================================
TEST(HierarchicalTucker, HT02_Decompose4D) {
    // max_rank=20 > internal SVD ranks of [6,5,4,4] → near-exact reconstruction
    std::vector<std::size_t> shape{6, 5, 4, 4};
    auto data = randomTensor(shape, 2);
    auto [ht, stats] = makeDecomp(20).decompose(data, shape);
    ASSERT_TRUE(ht.root != nullptr);
    EXPECT_EQ(ht.order(), 4u);

    auto recon = ht.reconstruct();
    ASSERT_EQ(recon.size(), data.size());
    double err = relError(data, recon);
    EXPECT_LT(err, 2e-3) << "Reconstruction error too large: " << err;
}

// ============================================================================
// HT-03: innerProduct(A, A) == squared Frobenius norm
// ============================================================================
TEST(HierarchicalTucker, HT03_InnerProductSelf) {
    std::vector<std::size_t> shape{6, 6};
    auto data = randomTensor(shape, 3);
    auto [ht, _] = makeDecomp(6).decompose(data, shape);

    double ip   = themis::tensor::HTContractionEngine::innerProduct(ht, ht);
    double norm = themis::tensor::HTContractionEngine::frobeniusNorm(ht);
    EXPECT_NEAR(ip, norm * norm, 1e-4 * norm * norm + 1e-10);
}

// ============================================================================
// HT-04: cosineSimilarity(A, A) == 1.0
// ============================================================================
TEST(HierarchicalTucker, HT04_CosineSelf) {
    auto data = randomTensor({5, 5}, 4);
    auto [ht, _] = makeDecomp(5).decompose(data, {5, 5});

    double cs = themis::tensor::HTContractionEngine::cosineSimilarity(ht, ht);
    EXPECT_NEAR(cs, 1.0, 1e-6);
}

// ============================================================================
// HT-05: cosine similarity of near-orthogonal tensors < 0.15
// ============================================================================
TEST(HierarchicalTucker, HT05_CosineOrthogonal) {
    // Build two tensors that are approximately orthogonal
    std::size_t n = 8;
    std::vector<float> A(n * n, 0.0f), B(n * n, 0.0f);
    for (std::size_t i = 0; i < n; ++i) A[i * n + i] = 1.0f;  // diagonal
    for (std::size_t i = 0; i < n - 1; ++i) {
        B[i * n + (i + 1)] = 1.0f;
        B[(i + 1) * n + i] = 1.0f;
    }

    auto [htA, _A] = makeDecomp(4).decompose(A, {n, n});
    auto [htB, _B] = makeDecomp(4).decompose(B, {n, n});

    double cs = themis::tensor::HTContractionEngine::cosineSimilarity(htA, htB);
    EXPECT_LT(std::abs(cs), 0.15);
}

// ============================================================================
// HT-06: FlatHTIndex add + search returns the added id first
// ============================================================================
TEST(HierarchicalTucker, HT06_IndexSearchReturnsAdded) {
    auto data = randomTensor({6, 6}, 5);
    auto [ht, _] = makeDecomp(4).decompose(data, {6, 6});

    themis::tensor::FlatHTIndex idx;
    idx.add("vec_A", ht.clone());
    idx.add("vec_B", makeDecomp(4).decompose(randomTensor({6,6}, 99), {6,6}).first);

    auto results = idx.search(ht, 5);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results.front().id, "vec_A");
    EXPECT_NEAR(results.front().similarity, 1.0, 1e-5);
}

// ============================================================================
// HT-07: search with k=1 returns exactly 1 result
// ============================================================================
TEST(HierarchicalTucker, HT07_SearchK1) {
    auto data = randomTensor({4, 4}, 6);
    auto [htQ, _Q] = makeDecomp(4).decompose(data, {4, 4});
    auto [htA, _A] = makeDecomp(4).decompose(randomTensor({4,4},10), {4,4});
    auto [htB, _B] = makeDecomp(4).decompose(randomTensor({4,4},20), {4,4});

    themis::tensor::FlatHTIndex idx;
    idx.add("A", std::move(htA));
    idx.add("B", std::move(htB));

    auto results = idx.search(htQ, 1);
    EXPECT_EQ(results.size(), 1u);
}

// ============================================================================
// HT-08: remove reduces size by 1
// ============================================================================
TEST(HierarchicalTucker, HT08_Remove) {
    auto [ht, _] = makeDecomp(4).decompose(randomTensor({4,4},7), {4,4});

    themis::tensor::FlatHTIndex idx;
    idx.add("A", ht.clone());
    idx.add("B", ht.clone());
    EXPECT_EQ(idx.size(), 2u);

    bool removed = idx.remove("A");
    EXPECT_TRUE(removed);
    EXPECT_EQ(idx.size(), 1u);

    bool removed2 = idx.remove("nonexistent");
    EXPECT_FALSE(removed2);
}

// ============================================================================
// HT-09: serialization round-trip preserves shape and reconstruction
// ============================================================================
TEST(HierarchicalTucker, HT09_SerializeRoundTrip) {
    std::vector<std::size_t> shape{5, 4};
    auto data = randomTensor(shape, 8);
    auto [ht, _] = makeDecomp(4).decompose(data, shape);

    auto bytes = ht.serialize();
    ASSERT_FALSE(bytes.empty());

    auto ht2_opt = themis::tensor::HTTrain::deserialize(bytes);
    ASSERT_TRUE(ht2_opt.has_value());
    auto& ht2 = *ht2_opt;

    EXPECT_EQ(ht2.shape, shape);
    EXPECT_EQ(ht2.max_rank, ht.max_rank);

    auto r1 = ht.reconstruct();
    auto r2 = ht2.reconstruct();
    ASSERT_EQ(r1.size(), r2.size());
    double err = relError(r1, r2);
    EXPECT_LT(err, 1e-5);
}

// ============================================================================
// HT-10: totalParams() < N (compression check on low-rank tensor)
// ============================================================================
TEST(HierarchicalTucker, HT10_Compression) {
    // Rank-1 tensor should compress well
    std::vector<std::size_t> shape{12, 10};
    auto data = rank1Tensor(shape, 9);
    auto [ht, stats] = makeDecomp(4).decompose(data, shape);

    std::size_t N = 12 * 10;
    EXPECT_LT(ht.totalParams(), N) << "No compression achieved";
}

// ============================================================================
// HT-11: compressionRatio() > 1 for compressible tensor
// ============================================================================
TEST(HierarchicalTucker, HT11_CompressionRatio) {
    std::vector<std::size_t> shape{10, 8};
    auto data = rank1Tensor(shape, 11);
    auto [ht, _] = makeDecomp(3).decompose(data, shape);
    EXPECT_GT(ht.compressionRatio(), 1.0);
}

// ============================================================================
// HT-12: frobeniusNorm == sqrt(innerProduct(A, A))
// ============================================================================
TEST(HierarchicalTucker, HT12_NormConsistency) {
    auto data = randomTensor({6, 5}, 12);
    auto [ht, _] = makeDecomp(5).decompose(data, {6, 5});

    double norm = themis::tensor::HTContractionEngine::frobeniusNorm(ht);
    double ip   = themis::tensor::HTContractionEngine::innerProduct(ht, ht);
    EXPECT_NEAR(norm * norm, ip, 1e-6 * std::abs(ip) + 1e-10);
}

// ============================================================================
// HT-13: distinct tensors have distinct inner products
// ============================================================================
TEST(HierarchicalTucker, HT13_DistinctInnerProducts) {
    auto dataA = randomTensor({5, 5}, 13);
    auto dataB = randomTensor({5, 5}, 14);
    auto [htA, _A] = makeDecomp(5).decompose(dataA, {5, 5});
    auto [htB, _B] = makeDecomp(5).decompose(dataB, {5, 5});

    double ipAA = themis::tensor::HTContractionEngine::innerProduct(htA, htA);
    double ipBB = themis::tensor::HTContractionEngine::innerProduct(htB, htB);
    double ipAB = themis::tensor::HTContractionEngine::innerProduct(htA, htB);

    EXPECT_NE(ipAA, ipAB);
    EXPECT_NE(ipBB, ipAB);
}

// ============================================================================
// HT-14: HTTrain::clone() produces structurally equal train
// ============================================================================
TEST(HierarchicalTucker, HT14_Clone) {
    auto data = randomTensor({5, 5}, 15);
    auto [ht, _] = makeDecomp(4).decompose(data, {5, 5});

    auto ht2 = ht.clone();
    EXPECT_EQ(ht2.shape, ht.shape);
    EXPECT_EQ(ht2.max_rank, ht.max_rank);
    EXPECT_EQ(ht2.totalParams(), ht.totalParams());

    // Same inner product
    double ip1 = themis::tensor::HTContractionEngine::innerProduct(ht, ht);
    double ip2 = themis::tensor::HTContractionEngine::innerProduct(ht2, ht2);
    EXPECT_NEAR(ip1, ip2, 1e-8 * std::abs(ip1) + 1e-12);
}

// ============================================================================
// HT-15: rank-1 tensor d=2; compression ratio > 1
// ============================================================================
TEST(HierarchicalTucker, HT15_Rank1Compress) {
    std::vector<std::size_t> shape{16, 12};
    auto data = rank1Tensor(shape, 16);
    auto [ht, stats] = makeDecomp(2).decompose(data, shape);
    EXPECT_GT(ht.compressionRatio(), 1.0);
    // Reconstruction should be close for rank-1 with max_rank >= 1
    auto recon = ht.reconstruct();
    double err = relError(data, recon);
    EXPECT_LT(err, 0.05);
}

// ============================================================================
// HT-16: FlatHTIndex serialize / deserialize round-trip
// ============================================================================
TEST(HierarchicalTucker, HT16_IndexSerializeDeserialize) {
    auto [ht1, _1] = makeDecomp(4).decompose(randomTensor({4,4}, 17), {4,4});
    auto [ht2, _2] = makeDecomp(4).decompose(randomTensor({4,4}, 18), {4,4});

    themis::tensor::FlatHTIndex idx;
    idx.add("one", std::move(ht1));
    idx.add("two", std::move(ht2));

    auto bytes = idx.serialize();
    ASSERT_FALSE(bytes.empty());

    themis::tensor::FlatHTIndex idx2;
    bool ok = idx2.deserialize(bytes);
    EXPECT_TRUE(ok);
    EXPECT_EQ(idx2.size(), 2u);
}

// ============================================================================
// HT-17: FlatHTIndex::get() retrieves stored entry by id
// ============================================================================
TEST(HierarchicalTucker, HT17_IndexGet) {
    auto [ht, _] = makeDecomp(4).decompose(randomTensor({4,4}, 19), {4,4});

    themis::tensor::FlatHTIndex idx;
    idx.add("mykey", ht.clone());

    auto found = idx.get("mykey");
    EXPECT_TRUE(found.has_value());

    auto notfound = idx.get("missing");
    EXPECT_FALSE(notfound.has_value());
}

// ============================================================================
// HT-18: FlatHTIndex: replacing an entry with same id works
// ============================================================================
TEST(HierarchicalTucker, HT18_IndexReplace) {
    auto [ht1, _1] = makeDecomp(4).decompose(randomTensor({4,4}, 20), {4,4});
    auto [ht2, _2] = makeDecomp(4).decompose(randomTensor({4,4}, 21), {4,4});

    double norm1 = themis::tensor::HTContractionEngine::frobeniusNorm(ht1);
    double norm2 = themis::tensor::HTContractionEngine::frobeniusNorm(ht2);

    themis::tensor::FlatHTIndex idx;
    idx.add("key", ht1.clone());
    EXPECT_EQ(idx.size(), 1u);

    idx.add("key", ht2.clone());  // should replace
    EXPECT_EQ(idx.size(), 1u);

    auto stored = idx.get("key");
    ASSERT_TRUE(stored.has_value());
    double stored_norm = themis::tensor::HTContractionEngine::frobeniusNorm(**stored);
    EXPECT_NEAR(stored_norm, norm2, 1e-5 * norm2 + 1e-10);
}
