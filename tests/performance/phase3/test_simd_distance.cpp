// Unit tests for SIMD distance computations (Phase 3, Issue #1964)
// Tests cover:
//  - L2 distance (Euclidean) correctness vs. scalar reference
//  - L2 squared distance
//  - Batch L2 squared distances
//  - Inner product correctness
//  - Cosine distance correctness
//  - Edge cases: dim=0, dim=1, dim < SIMD width, dim not multiple of SIMD width
//  - Numerical accuracy within floating-point tolerance
//  - AVX-512 feature flag defaults

#include "utils/simd_distance.h"
#include "performance/phase3/feature_flags.h"

#include <gtest/gtest.h>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::simd;
using namespace themis::performance::phase3;

// ============================================================
// Reference scalar implementations
// ============================================================

static float ref_l2_sq(const float* a, const float* b, std::size_t dim) {
    float acc = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        acc += d * d;
    }
    return acc;
}

static float ref_inner_product(const float* a, const float* b, std::size_t dim) {
    float acc = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
      acc += a[i] * b[i];
    }
    return acc;
}

static float ref_cosine_distance(const float* a, const float* b, std::size_t dim) {
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    if (denom < 1e-10f) {
      return 1.0f;
    }
    float sim = dot / denom;
    if (sim > 1.0f) {
      sim = 1.0f;
    }
    if (sim < -1.0f) {
      sim = -1.0f;
    }
    return 1.0f - sim;
}

// ============================================================
// Fixture
// ============================================================

class SimdDistanceTest : public ::testing::Test {
protected:
    // Tolerance for float comparisons (relative to magnitude)
    static constexpr float kTol = 1e-4f;

    static std::vector<float> make_vec(std::size_t dim, float fill) {
        return std::vector<float>(dim, fill);
    }

    static std::vector<float> make_rand(std::size_t dim, unsigned seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) {
          x = dist(rng);
        }
        return v;
    }

    static void expect_near(float actual, float expected, const char* label) {
        float tol = kTol * (std::abs(expected) + 1.0f);
        EXPECT_NEAR(actual, expected, tol) << label;
    }
};

// ============================================================
// L2 distance tests
// ============================================================

TEST_F(SimdDistanceTest, L2DistanceZeroVector) {
    auto a = make_vec(128, 0.0f);
    auto b = make_vec(128, 0.0f);
    EXPECT_FLOAT_EQ(l2_distance(a.data(), b.data(), 128), 0.0f);
    EXPECT_FLOAT_EQ(l2_distance_sq(a.data(), b.data(), 128), 0.0f);
}

TEST_F(SimdDistanceTest, L2DistanceIdenticalVectors) {
    auto a = make_rand(512, 7);
    EXPECT_NEAR(l2_distance(a.data(), a.data(), 512), 0.0f, 1e-6f);
    EXPECT_NEAR(l2_distance_sq(a.data(), a.data(), 512), 0.0f, 1e-6f);
}

TEST_F(SimdDistanceTest, L2DistanceDim1) {
    float a = 3.0f, b = 7.0f;
    EXPECT_NEAR(l2_distance(&a, &b, 1), 4.0f, kTol);
    EXPECT_NEAR(l2_distance_sq(&a, &b, 1), 16.0f, kTol);
}

TEST_F(SimdDistanceTest, L2DistanceDim0) {
    float dummy = 0.0f;
    EXPECT_FLOAT_EQ(l2_distance(&dummy, &dummy, 0), 0.0f);
    EXPECT_FLOAT_EQ(l2_distance_sq(&dummy, &dummy, 0), 0.0f);
}

// Verify against scalar for various dimensions (including non-multiples of SIMD widths)
class L2CorrectnessTest : public SimdDistanceTest,
                           public ::testing::WithParamInterface<std::size_t> {};

TEST_P(L2CorrectnessTest, MatchesScalar) {
    std::size_t dim = GetParam();
    auto a = make_rand(dim, 1);
    auto b = make_rand(dim, 2);

    float expected_sq = ref_l2_sq(a.data(), b.data(), dim);
    float actual_sq   = l2_distance_sq(a.data(), b.data(), dim);
    expect_near(actual_sq, expected_sq, "l2_distance_sq");

    float actual_dist = l2_distance(a.data(), b.data(), dim);
    expect_near(actual_dist, std::sqrt(expected_sq), "l2_distance");
}

INSTANTIATE_TEST_SUITE_P(Dimensions, L2CorrectnessTest,
    ::testing::Values(1u, 3u, 8u, 15u, 16u, 17u, 32u, 63u, 64u,
                      128u, 256u, 511u, 512u, 513u, 768u, 1024u, 1536u));

// ============================================================
// Batch L2 distance tests
// ============================================================

TEST_F(SimdDistanceTest, BatchL2ConsistentWithSingle) {
    const std::size_t n = 20, dim = 128;
    auto query = make_rand(dim, 99);
    std::vector<float> db_flat(n * dim);
    for (std::size_t i = 0; i < n; ++i) {
        auto v = make_rand(dim, static_cast<unsigned>(i + 1));
        std::copy(v.begin(), v.end(), db_flat.begin() + static_cast<std::ptrdiff_t>(i * dim));
    }
    std::vector<float> batch_dists(n);
    batch_l2_distance_sq(query.data(), db_flat.data(), n, dim, batch_dists.data());

    for (std::size_t i = 0; i < n; ++i) {
        float single = l2_distance_sq(query.data(), db_flat.data() + i * dim, dim);
        EXPECT_NEAR(batch_dists[i], single, kTol * (single + 1.0f))
            << "Mismatch at index " << i;
    }
}

// ============================================================
// Inner product tests
// ============================================================

class InnerProductTest : public SimdDistanceTest,
                          public ::testing::WithParamInterface<std::size_t> {};

TEST_P(InnerProductTest, MatchesScalar) {
    std::size_t dim = GetParam();
    auto a = make_rand(dim, 10);
    auto b = make_rand(dim, 20);

    float expected = ref_inner_product(a.data(), b.data(), dim);
    float actual   = inner_product(a.data(), b.data(), dim);
    expect_near(actual, expected, "inner_product");
}

INSTANTIATE_TEST_SUITE_P(Dimensions, InnerProductTest,
    ::testing::Values(1u, 4u, 8u, 15u, 16u, 17u, 32u, 64u,
                      128u, 256u, 512u, 768u, 1024u, 1536u));

TEST_F(SimdDistanceTest, InnerProductOrthogonal) {
    // e1 and e2 are orthogonal: dot = 0
    std::vector<float> e1(16, 0.0f), e2(16, 0.0f);
    e1[0] = 1.0f;
    e2[1] = 1.0f;
    EXPECT_NEAR(inner_product(e1.data(), e2.data(), 16), 0.0f, 1e-6f);
}

TEST_F(SimdDistanceTest, InnerProductSelfEqualsNormSq) {
    auto a = make_rand(256, 55);
    float ip   = inner_product(a.data(), a.data(), 256);
    float l2sq = l2_distance_sq(a.data(), std::vector<float>(256, 0.0f).data(), 256);
    EXPECT_NEAR(ip, l2sq, kTol * (ip + 1.0f));
}

// ============================================================
// Cosine distance tests
// ============================================================

class CosineDistanceTest : public SimdDistanceTest,
                            public ::testing::WithParamInterface<std::size_t> {};

TEST_P(CosineDistanceTest, MatchesScalar) {
    std::size_t dim = GetParam();
    auto a = make_rand(dim, 30);
    auto b = make_rand(dim, 40);

    float expected = ref_cosine_distance(a.data(), b.data(), dim);
    float actual   = cosine_distance(a.data(), b.data(), dim);
    expect_near(actual, expected, "cosine_distance");
}

INSTANTIATE_TEST_SUITE_P(Dimensions, CosineDistanceTest,
    ::testing::Values(1u, 4u, 8u, 16u, 32u, 64u,
                      128u, 256u, 512u, 768u, 1024u, 1536u));

TEST_F(SimdDistanceTest, CosineDistanceIdenticalVectors) {
    auto a = make_rand(128, 77);
    EXPECT_NEAR(cosine_distance(a.data(), a.data(), 128), 0.0f, kTol);
}

TEST_F(SimdDistanceTest, CosineDistanceOppositeVectors) {
    auto a = make_rand(128, 88);
    std::vector<float> neg(128);
    for (std::size_t i = 0; i < 128; ++i) {
      neg[i] = -a[i];
    }
    EXPECT_NEAR(cosine_distance(a.data(), neg.data(), 128), 2.0f, kTol);
}

TEST_F(SimdDistanceTest, CosineDistanceOrthogonal) {
    std::vector<float> e1(16, 0.0f), e2(16, 0.0f);
    e1[0] = 1.0f;
    e2[1] = 1.0f;
    EXPECT_NEAR(cosine_distance(e1.data(), e2.data(), 16), 1.0f, kTol);
}

TEST_F(SimdDistanceTest, CosineDistanceZeroNorm) {
    auto zeros = make_vec(32, 0.0f);
    auto a     = make_rand(32, 9);
    // Zero-norm vector: function should return 1.0 without dividing by zero
    EXPECT_FLOAT_EQ(cosine_distance(zeros.data(), a.data(), 32), 1.0f);
    EXPECT_FLOAT_EQ(cosine_distance(a.data(), zeros.data(), 32), 1.0f);
}

// ============================================================
// Feature flag tests
// ============================================================

TEST(Avx512FeatureFlagTest, DefaultStateReflectsHardware) {
    auto& flags = Phase3FeatureFlags::instance();
    // The flag should be accessible without throwing
    EXPECT_NO_THROW(flags.avx512_distance_enabled());
#if defined(__AVX512F__)
    // On AVX-512-capable hardware, the flag defaults to true
    EXPECT_TRUE(flags.avx512_distance_enabled());
#else
    // On other hardware, the flag defaults to false
    EXPECT_FALSE(flags.avx512_distance_enabled());
#endif
}

TEST(Avx512FeatureFlagTest, RuntimeToggle) {
    auto& flags = Phase3FeatureFlags::instance();
    bool initial = flags.avx512_distance_enabled();

    flags.set_avx512_distance_enabled(true);
    EXPECT_TRUE(flags.avx512_distance_enabled());

    flags.set_avx512_distance_enabled(false);
    EXPECT_FALSE(flags.avx512_distance_enabled());

    // Restore
    flags.set_avx512_distance_enabled(initial);
}

TEST(Avx512FeatureFlagTest, MacroAccessible) {
    EXPECT_NO_THROW({ (void)THEMIS_PHASE3_AVX512_DISTANCE_ENABLED(); });
}
