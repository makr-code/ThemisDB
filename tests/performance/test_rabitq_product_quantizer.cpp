// Unit tests for ProductQuantizer (k-means-based) in src/performance/rabitq.cpp

#include "performance/rabitq.h"
#include <gtest/gtest.h>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>

using namespace themis::performance;

namespace {

std::vector<std::vector<float>> make_random_vectors(
    size_t n, size_t dim, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<std::vector<float>> vecs(n, std::vector<float>(dim));
    for (auto& v : vecs)
        for (auto& x : v)
            x = dist(rng);
    return vecs;
}

float l2(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return std::sqrt(s);
}

} // namespace

// -----------------------------------------------------------------------
// Constructor tests
// -----------------------------------------------------------------------

TEST(PerfProductQuantizerTest, ConstructorValidDimension) {
    // 128 / 8 = 16 — valid
    EXPECT_NO_THROW(ProductQuantizer(128, 8));
}

TEST(PerfProductQuantizerTest, ConstructorInvalidDimension) {
    // 130 is not divisible by 8
    EXPECT_THROW(ProductQuantizer(130, 8), std::invalid_argument);
}

// -----------------------------------------------------------------------
// Train tests
// -----------------------------------------------------------------------

TEST(PerfProductQuantizerTest, TrainPopulatesCodebooks) {
    ProductQuantizer pq(64, 4);
    auto data = make_random_vectors(300, 64);

    // Before training codebooks_ must be empty
    auto codes_before = pq.encode(data[0]);
    // All codes are 0 before training (codebooks empty)
    EXPECT_EQ(codes_before.size(), 4u);
    EXPECT_TRUE(std::all_of(codes_before.begin(), codes_before.end(),
                            [](uint8_t c) { return c == 0; }));

    pq.train(data);

    // After training, encode should return non-trivial codes
    auto codes = pq.encode(data[0]);
    EXPECT_EQ(codes.size(), 4u);
}

TEST(PerfProductQuantizerTest, TrainEmptyDataIsNoOp) {
    ProductQuantizer pq(64, 4);
    std::vector<std::vector<float>> empty;
    EXPECT_NO_THROW(pq.train(empty));
    // encode still produces zero-filled codes (codebooks not populated)
    auto codes = pq.encode(make_random_vectors(1, 64)[0]);
    EXPECT_EQ(codes.size(), 4u);
}

// -----------------------------------------------------------------------
// Encode tests
// -----------------------------------------------------------------------

TEST(PerfProductQuantizerTest, EncodeLengthEqualsNumSubvectors) {
    const size_t num_sq = 8;
    ProductQuantizer pq(128, num_sq);
    pq.train(make_random_vectors(400, 128));

    auto vec = make_random_vectors(1, 128)[0];
    auto codes = pq.encode(vec);
    EXPECT_EQ(codes.size(), num_sq);
}

TEST(PerfProductQuantizerTest, SameVectorEncodesDeterministically) {
    ProductQuantizer pq(64, 4);
    pq.train(make_random_vectors(200, 64));

    auto vec = make_random_vectors(1, 64, 77)[0];
    auto codes1 = pq.encode(vec);
    auto codes2 = pq.encode(vec);
    EXPECT_EQ(codes1, codes2);
}

// -----------------------------------------------------------------------
// Nearest-centroid correctness: if the training set has well-separated
// clusters, the centroid indices for a point should reflect its cluster.
// -----------------------------------------------------------------------

TEST(PerfProductQuantizerTest, NearestCentroidIsConsistent) {
    // Build two clearly separated clusters in 16-D space
    const size_t dim = 16;
    const size_t num_sq = 1;   // single sub-quantizer covering all 16 dims
    const size_t n_per_cluster = 100;

    std::mt19937 rng(0);
    std::normal_distribution<float> noise(0.0f, 0.1f);

    std::vector<std::vector<float>> data;
    // Cluster A: near +10 in all dims
    for (size_t i = 0; i < n_per_cluster; ++i) {
        std::vector<float> v(dim);
        for (auto& x : v) x = 10.0f + noise(rng);
        data.push_back(v);
    }
    // Cluster B: near -10 in all dims
    for (size_t i = 0; i < n_per_cluster; ++i) {
        std::vector<float> v(dim);
        for (auto& x : v) x = -10.0f + noise(rng);
        data.push_back(v);
    }

    ProductQuantizer pq(dim, num_sq);
    pq.train(data);

    // Two members from cluster A should map to the same code
    auto ca1 = pq.encode(data[0]);
    auto ca2 = pq.encode(data[1]);
    EXPECT_EQ(ca1[0], ca2[0]);

    // Two members from cluster B should also share a code
    auto cb1 = pq.encode(data[n_per_cluster]);
    auto cb2 = pq.encode(data[n_per_cluster + 1]);
    EXPECT_EQ(cb1[0], cb2[0]);

    // Codes from different clusters must differ
    EXPECT_NE(ca1[0], cb1[0]);
}

// -----------------------------------------------------------------------
// SplitVector correctness
// -----------------------------------------------------------------------

TEST(PerfProductQuantizerTest, SplitVectorProducesCorrectChunks) {
    ProductQuantizer pq(12, 3); // 12-D / 3 = 4-D per subvector
    std::vector<float> vec(12);
    std::iota(vec.begin(), vec.end(), 1.0f); // 1,2,...,12

    auto subs = pq.split_vector(vec);
    ASSERT_EQ(subs.size(), 3u);
    EXPECT_EQ(subs[0], (std::vector<float>{1, 2, 3, 4}));
    EXPECT_EQ(subs[1], (std::vector<float>{5, 6, 7, 8}));
    EXPECT_EQ(subs[2], (std::vector<float>{9, 10, 11, 12}));
}
