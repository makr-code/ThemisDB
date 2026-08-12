// Unit tests for include/utils/geometric_distances.h
//
// Covers:
//   - simd::l2_distance / l2_distance_sq (via geometric_distances.h)
//   - simd::cosine_distance / cosine_similarity
//   - simd::inner_product
//   - geo::haversine_km / haversine_m
//   - themis::manhattan_distance

#include <gtest/gtest.h>
#include "utils/geometric_distances.h"

#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr float kEps = 1e-4f;
static constexpr double kGeoEpsPct = 0.005; // 0.5 % tolerance for haversine

// ---------------------------------------------------------------------------
// L2 distance
// ---------------------------------------------------------------------------

TEST(GeometricDistancesL2, IdenticalVectorsHaveZeroDistance) {
    const float a[] = {1.0f, 2.0f, 3.0f};
    EXPECT_NEAR(themis::simd::l2_distance(a, a, 3), 0.0f, kEps);
    EXPECT_NEAR(themis::simd::l2_distance_sq(a, a, 3), 0.0f, kEps);
}

TEST(GeometricDistancesL2, KnownDistance3D) {
    const float a[] = {0.0f, 0.0f, 0.0f};
    const float b[] = {1.0f, 2.0f, 2.0f};
    // |b| = sqrt(1+4+4) = 3
    EXPECT_NEAR(themis::simd::l2_distance(a, b, 3), 3.0f, kEps);
    EXPECT_NEAR(themis::simd::l2_distance_sq(a, b, 3), 9.0f, kEps);
}

TEST(GeometricDistancesL2, SquaredAndSquareRootConsistent) {
    const float a[] = {1.5f, -2.3f, 0.7f, 4.1f};
    const float b[] = {-0.5f, 1.1f, 3.0f, 0.0f};
    float sq = themis::simd::l2_distance_sq(a, b, 4);
    float d  = themis::simd::l2_distance(a, b, 4);
    EXPECT_NEAR(d, std::sqrt(sq), kEps);
}

TEST(GeometricDistancesL2, Dim1) {
    const float a[] = {5.0f};
    const float b[] = {2.0f};
    EXPECT_NEAR(themis::simd::l2_distance(a, b, 1), 3.0f, kEps);
}

// ---------------------------------------------------------------------------
// Cosine distance / similarity
// ---------------------------------------------------------------------------

TEST(GeometricDistancesCosine, SameDirectionZeroDistance) {
    const float a[] = {1.0f, 0.0f, 0.0f};
    const float b[] = {2.0f, 0.0f, 0.0f};
    EXPECT_NEAR(themis::simd::cosine_distance(a, b, 3), 0.0f, kEps);
    EXPECT_NEAR(themis::simd::cosine_similarity(a, b, 3), 1.0f, kEps);
}

TEST(GeometricDistancesCosine, OrthogonalVectors) {
    const float a[] = {1.0f, 0.0f};
    const float b[] = {0.0f, 1.0f};
    EXPECT_NEAR(themis::simd::cosine_distance(a, b, 2), 1.0f, kEps);
    EXPECT_NEAR(themis::simd::cosine_similarity(a, b, 2), 0.0f, kEps);
}

TEST(GeometricDistancesCosine, OppositeDirectionMaxDistance) {
    const float a[] = {1.0f, 0.0f};
    const float b[] = {-1.0f, 0.0f};
    EXPECT_NEAR(themis::simd::cosine_distance(a, b, 2), 2.0f, kEps);
    EXPECT_NEAR(themis::simd::cosine_similarity(a, b, 2), -1.0f, kEps);
}

TEST(GeometricDistancesCosine, ZeroVectorReturnsMaxDistance) {
    const float a[] = {1.0f, 2.0f, 3.0f};
    const float z[] = {0.0f, 0.0f, 0.0f};
    EXPECT_NEAR(themis::simd::cosine_distance(a, z, 3), 1.0f, kEps);
}

TEST(GeometricDistancesCosine, KnownAngle45Degrees) {
    const float a[] = {1.0f, 0.0f};
    const float b[] = {1.0f, 1.0f}; // 45° from a
    float sim = themis::simd::cosine_similarity(a, b, 2);
    EXPECT_NEAR(sim, std::cos(M_PI / 4.0f), kEps);
}

// ---------------------------------------------------------------------------
// Inner product
// ---------------------------------------------------------------------------

TEST(GeometricDistancesInnerProduct, KnownDotProduct) {
    const float a[] = {1.0f, 2.0f, 3.0f};
    const float b[] = {4.0f, 5.0f, 6.0f};
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    EXPECT_NEAR(themis::simd::inner_product(a, b, 3), 32.0f, kEps);
}

TEST(GeometricDistancesInnerProduct, OrthogonalVectorsZero) {
    const float a[] = {1.0f, 0.0f};
    const float b[] = {0.0f, 1.0f};
    EXPECT_NEAR(themis::simd::inner_product(a, b, 2), 0.0f, kEps);
}

// ---------------------------------------------------------------------------
// Batch functions
// ---------------------------------------------------------------------------

TEST(GeometricDistancesBatch, BatchL2MatchesSingle) {
    const float q[] = {1.0f, 0.0f};
    const float db[] = {
        0.0f, 0.0f,  // dist_sq = 1
        1.0f, 0.0f,  // dist_sq = 0
        0.0f, 1.0f,  // dist_sq = 2
    };
    float out[3] = {};
    themis::simd::batch_l2_distance_sq(q, db, 3, 2, out);
    EXPECT_NEAR(out[0], 1.0f, kEps);
    EXPECT_NEAR(out[1], 0.0f, kEps);
    EXPECT_NEAR(out[2], 2.0f, kEps);
}

TEST(GeometricDistancesBatch, BatchCosineSimilarityMatchesSingle) {
    const float q[] = {1.0f, 0.0f};
    const float db[] = {
        1.0f, 0.0f,  // sim = 1
        0.0f, 1.0f,  // sim = 0
    };
    float out[2] = {};
    themis::simd::batch_cosine_similarity(q, db, 2, 2, out);
    EXPECT_NEAR(out[0], 1.0f, kEps);
    EXPECT_NEAR(out[1], 0.0f, kEps);
}

// ---------------------------------------------------------------------------
// Haversine
// ---------------------------------------------------------------------------

// Known reference: London (51.5074, -0.1278) → Paris (48.8566, 2.3522)
// Great-circle ≈ 340 km
TEST(GeometricDistancesHaversine, LondonToParis) {
    double dist_km = themis::geo::haversine_km(51.5074, -0.1278, 48.8566, 2.3522);
    EXPECT_NEAR(dist_km, 340.0, 340.0 * kGeoEpsPct);
}

// Metres variant is km * 1000
TEST(GeometricDistancesHaversine, MetresVariantConsistent) {
    double km = themis::geo::haversine_km(51.5074, -0.1278, 48.8566, 2.3522);
    double m  = themis::geo::haversine_m(51.5074, -0.1278, 48.8566, 2.3522);
    EXPECT_NEAR(m, km * 1000.0, 1.0); // within 1 m rounding
}

TEST(GeometricDistancesHaversine, SamePointZeroDistance) {
    double d = themis::geo::haversine_km(48.8566, 2.3522, 48.8566, 2.3522);
    EXPECT_NEAR(d, 0.0, 1e-9);
}

TEST(GeometricDistancesHaversine, AntipodalPoints) {
    // Antipodal points are half the circumference ≈ 20 015 km
    double d = themis::geo::haversine_km(0.0, 0.0, 0.0, 180.0);
    EXPECT_NEAR(d, 20015.0, 20015.0 * kGeoEpsPct);
}

TEST(GeometricDistancesHaversine, Symmetry) {
    double d1 = themis::geo::haversine_km(51.5074, -0.1278, 40.7128, -74.0060);
    double d2 = themis::geo::haversine_km(40.7128, -74.0060, 51.5074, -0.1278);
    EXPECT_NEAR(d1, d2, 1e-6);
}

// ---------------------------------------------------------------------------
// Manhattan distance
// ---------------------------------------------------------------------------

TEST(GeometricDistancesManhattan, KnownL1) {
    const float a[] = {1.0f, 2.0f, 3.0f};
    const float b[] = {4.0f, 0.0f, 5.0f};
    // |1-4| + |2-0| + |3-5| = 3 + 2 + 2 = 7
    EXPECT_NEAR(themis::manhattan_distance(a, b, 3), 7.0f, kEps);
}

TEST(GeometricDistancesManhattan, SameVectorZero) {
    const float a[] = {-1.5f, 2.0f, 0.0f};
    EXPECT_NEAR(themis::manhattan_distance(a, a, 3), 0.0f, kEps);
}

TEST(GeometricDistancesManhattan, Dim1) {
    const float a[] = {-3.0f};
    const float b[] = {2.0f};
    EXPECT_NEAR(themis::manhattan_distance(a, b, 1), 5.0f, kEps);
}

TEST(GeometricDistancesManhattan, NonNegative) {
    const float a[] = {-1.0f, -2.0f};
    const float b[] = {3.0f, 4.0f};
    EXPECT_GE(themis::manhattan_distance(a, b, 2), 0.0f);
}

// ---------------------------------------------------------------------------
// Large dimension smoke test (exercises SIMD paths)
// ---------------------------------------------------------------------------

TEST(GeometricDistancesLarge, Dim1536SmokeTest) {
    constexpr size_t DIM = 1536;
    std::vector<float> a(DIM, 0.0f), b(DIM, 0.0f);
    // Set a few non-zero elements
    for (size_t i = 0; i < DIM; i += 64) {
        a[i] = 1.0f;
        b[i] = -1.0f;
    }
    float dist = themis::simd::l2_distance(a.data(), b.data(), DIM);
    EXPECT_GT(dist, 0.0f);

    float cos_d = themis::simd::cosine_distance(a.data(), b.data(), DIM);
    EXPECT_GE(cos_d, 0.0f);
    EXPECT_LE(cos_d, 2.0f);
}
