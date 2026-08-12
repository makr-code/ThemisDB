#include <gtest/gtest.h>
#include "geo/geo_clustering.h"
#include "utils/geo/ewkb.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GeometryInfo makePoint(double lon, double lat) {
    GeometryInfo g(GeometryType::Point);
    g.coords.emplace_back(lon, lat);
    return g;
}

static GeometryInfo makePolygon() {
    // A minimal polygon (not a Point) — used to verify non-Point handling.
    GeometryInfo g(GeometryType::Polygon);
    g.coords.emplace_back(0.0, 0.0);
    g.coords.emplace_back(1.0, 0.0);
    g.coords.emplace_back(0.0, 1.0);
    g.coords.emplace_back(0.0, 0.0);
    return g;
}

// ---------------------------------------------------------------------------
// DBSCAN: degenerate inputs
// ---------------------------------------------------------------------------

TEST(DbscanCluster, EmptyInput_ReturnsEmpty) {
    auto result = dbscanCluster({});
    EXPECT_TRUE(result.labels.empty());
    EXPECT_EQ(result.num_clusters, 0);
}

TEST(DbscanCluster, ZeroEpsilon_AllNoise) {
    std::vector<GeometryInfo> pts = {makePoint(0.0, 0.0), makePoint(0.001, 0.0)};
    DbscanConfig cfg;
    cfg.epsilon_m = 0.0;
    auto result = dbscanCluster(pts, cfg);
    ASSERT_EQ(result.labels.size(), 2u);
    EXPECT_EQ(result.labels[0], kDbscanNoise);
    EXPECT_EQ(result.labels[1], kDbscanNoise);
    EXPECT_EQ(result.num_clusters, 0);
}

TEST(DbscanCluster, SinglePoint_BelowMinPoints_IsNoise) {
    DbscanConfig cfg;
    cfg.epsilon_m  = 1000.0;
    cfg.min_points = 2; // need at least 2 in neighbourhood
    auto result = dbscanCluster({makePoint(13.4050, 52.5200)}, cfg);
    ASSERT_EQ(result.labels.size(), 1u);
    EXPECT_EQ(result.labels[0], kDbscanNoise);
    EXPECT_EQ(result.num_clusters, 0);
}

TEST(DbscanCluster, SinglePoint_MeetsMinPoints_IsCluster) {
    DbscanConfig cfg;
    cfg.epsilon_m  = 1000.0;
    cfg.min_points = 1; // the point is its own neighbourhood
    auto result = dbscanCluster({makePoint(13.4050, 52.5200)}, cfg);
    ASSERT_EQ(result.labels.size(), 1u);
    EXPECT_EQ(result.labels[0], 0);
    EXPECT_EQ(result.num_clusters, 1);
}

// ---------------------------------------------------------------------------
// DBSCAN: two well-separated clusters
// ---------------------------------------------------------------------------

TEST(DbscanCluster, TwoSeparatedClusters) {
    // Cluster A: 3 points around Berlin  (~13.4, 52.5) spaced ~100 m apart
    // Cluster B: 3 points around Paris   (~2.35, 48.85) spaced ~100 m apart
    // The two clusters are ~878 km apart — well outside epsilon = 500 m
    std::vector<GeometryInfo> pts = {
        // Cluster A
        makePoint(13.4050, 52.5200),
        makePoint(13.4060, 52.5200),  // ~71 m east
        makePoint(13.4050, 52.5205),  // ~56 m north
        // Cluster B
        makePoint(2.3522, 48.8566),
        makePoint(2.3532, 48.8566),   // ~70 m east
        makePoint(2.3522, 48.8576),   // ~111 m north
    };
    DbscanConfig cfg;
    cfg.epsilon_m  = 500.0;
    cfg.min_points = 2;
    auto result = dbscanCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 6u);
    EXPECT_EQ(result.num_clusters, 2);

    // All points in cluster A have the same label; all points in cluster B
    // have the same label; the two labels are different.
    const int labelA = result.labels[0];
    const int labelB = result.labels[3];
    EXPECT_NE(labelA, kDbscanNoise);
    EXPECT_NE(labelB, kDbscanNoise);
    EXPECT_NE(labelA, labelB);

    EXPECT_EQ(result.labels[1], labelA);
    EXPECT_EQ(result.labels[2], labelA);
    EXPECT_EQ(result.labels[4], labelB);
    EXPECT_EQ(result.labels[5], labelB);
}

// ---------------------------------------------------------------------------
// DBSCAN: noise detection
// ---------------------------------------------------------------------------

TEST(DbscanCluster, IsolatedPoint_IsNoise) {
    // 3 dense points (cluster) + 1 isolated point far away
    std::vector<GeometryInfo> pts = {
        makePoint(0.0, 0.0),
        makePoint(0.001, 0.0),   // ~111 m from first
        makePoint(0.0,  0.001),  // ~111 m from first
        makePoint(10.0, 10.0),   // isolated — >1000 km away
    };
    DbscanConfig cfg;
    cfg.epsilon_m  = 500.0;
    cfg.min_points = 2;
    auto result = dbscanCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 4u);
    EXPECT_EQ(result.num_clusters, 1);
    EXPECT_EQ(result.labels[3], kDbscanNoise);

    // The first 3 points should be in cluster 0
    EXPECT_EQ(result.labels[0], 0);
    EXPECT_EQ(result.labels[1], 0);
    EXPECT_EQ(result.labels[2], 0);
}

// ---------------------------------------------------------------------------
// DBSCAN: non-Point geometry is treated as noise
// ---------------------------------------------------------------------------

TEST(DbscanCluster, NonPointGeometry_TreatedAsNoise) {
    std::vector<GeometryInfo> pts = {
        makePoint(0.0, 0.0),
        makePoint(0.001, 0.0),
        makePolygon(), // non-Point: should get noise label
    };
    DbscanConfig cfg;
    cfg.epsilon_m  = 1000.0;
    cfg.min_points = 2;
    auto result = dbscanCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 3u);
    EXPECT_EQ(result.labels[2], kDbscanNoise);
    // The two valid points form a cluster
    EXPECT_EQ(result.num_clusters, 1);
}

// ---------------------------------------------------------------------------
// DBSCAN: label count consistency
// ---------------------------------------------------------------------------

TEST(DbscanCluster, LabelCount_MatchesNumClusters) {
    std::vector<GeometryInfo> pts;
    for (int i = 0; i < 5; ++i)
        pts.push_back(makePoint(static_cast<double>(i) * 0.001, 0.0)); // tight group
    for (int i = 0; i < 3; ++i)
        pts.push_back(makePoint(static_cast<double>(i) * 0.001 + 10.0, 0.0)); // another group

    DbscanConfig cfg;
    cfg.epsilon_m  = 200.0;
    cfg.min_points = 2;
    auto result = dbscanCluster(pts, cfg);

    // Every non-noise label must be in [0, num_clusters)
    for (int lbl : result.labels) {
        if (lbl != kDbscanNoise) {
            EXPECT_GE(lbl, 0);
            EXPECT_LT(lbl, result.num_clusters);
        }
    }
}

// ---------------------------------------------------------------------------
// k-means: degenerate inputs
// ---------------------------------------------------------------------------

TEST(KMeansCluster, EmptyInput_ReturnsEmpty) {
    auto result = kmeansCluster({});
    EXPECT_TRUE(result.labels.empty());
    EXPECT_EQ(result.num_clusters, 0);
}

TEST(KMeansCluster, KZero_ThrowsInvalidArgument) {
    KMeansConfig cfg;
    cfg.k = 0;
    EXPECT_THROW(kmeansCluster({makePoint(0.0, 0.0)}, cfg), std::invalid_argument);
}

TEST(KMeansCluster, KGreaterThanPoints_ThrowsInvalidArgument) {
    KMeansConfig cfg;
    cfg.k = 5;
    std::vector<GeometryInfo> pts = {makePoint(0.0, 0.0), makePoint(1.0, 0.0)};
    EXPECT_THROW(kmeansCluster(pts, cfg), std::invalid_argument);
}

TEST(KMeansCluster, SinglePoint_K1_GetsCluster0) {
    KMeansConfig cfg;
    cfg.k = 1;
    auto result = kmeansCluster({makePoint(13.4050, 52.5200)}, cfg);
    ASSERT_EQ(result.labels.size(), 1u);
    EXPECT_EQ(result.labels[0], 0);
    EXPECT_EQ(result.num_clusters, 1);
}

// ---------------------------------------------------------------------------
// k-means: basic clustering
// ---------------------------------------------------------------------------

TEST(KMeansCluster, TwoWellSeparatedGroups_K2) {
    // Group A: 4 points around origin (within ~200 m of each other)
    // Group B: 4 points around (10.0, 10.0) (within ~200 m of each other)
    std::vector<GeometryInfo> pts = {
        makePoint(0.0,   0.0),
        makePoint(0.001, 0.0),
        makePoint(0.0,   0.001),
        makePoint(0.001, 0.001),
        makePoint(10.0,   10.0),
        makePoint(10.001, 10.0),
        makePoint(10.0,   10.001),
        makePoint(10.001, 10.001),
    };
    KMeansConfig cfg;
    cfg.k              = 2;
    cfg.max_iterations = 100;
    cfg.seed           = 0; // deterministic init

    auto result = kmeansCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 8u);
    EXPECT_EQ(result.num_clusters, 2);

    // All points 0-3 should share one label; all points 4-7 should share another.
    const int labelA = result.labels[0];
    const int labelB = result.labels[4];
    EXPECT_NE(labelA, labelB);
    EXPECT_GE(labelA, 0);
    EXPECT_GE(labelB, 0);

    EXPECT_EQ(result.labels[1], labelA);
    EXPECT_EQ(result.labels[2], labelA);
    EXPECT_EQ(result.labels[3], labelA);
    EXPECT_EQ(result.labels[5], labelB);
    EXPECT_EQ(result.labels[6], labelB);
    EXPECT_EQ(result.labels[7], labelB);
}

TEST(KMeansCluster, AllLabelsInValidRange) {
    std::vector<GeometryInfo> pts;
    for (int i = 0; i < 20; ++i)
        pts.push_back(makePoint(static_cast<double>(i) * 0.5, 0.0));

    KMeansConfig cfg;
    cfg.k    = 4;
    cfg.seed = 0;
    auto result = kmeansCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 20u);
    EXPECT_EQ(result.num_clusters, 4);
    for (int lbl : result.labels) {
        EXPECT_GE(lbl, 0);
        EXPECT_LT(lbl, 4);
    }
}

TEST(KMeansCluster, KEqualsN_EachPointOwnCluster) {
    // When k == number of points the algorithm may assign each point to its
    // own cluster or merge some; the only hard guarantee is that every label
    // is in [0, k) and num_clusters == k.
    std::vector<GeometryInfo> pts = {
        makePoint(0.0, 0.0),
        makePoint(5.0, 5.0),
        makePoint(-5.0, -5.0),
    };
    KMeansConfig cfg;
    cfg.k = 3;
    auto result = kmeansCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 3u);
    EXPECT_EQ(result.num_clusters, 3);
    for (int lbl : result.labels) {
        EXPECT_GE(lbl, 0);
        EXPECT_LT(lbl, 3);
    }
}

// ---------------------------------------------------------------------------
// k-means: k-means++ seeding (seed != 0)
// ---------------------------------------------------------------------------

TEST(KMeansCluster, KMeansPlusPlusSeed_ProducesValidResult) {
    std::vector<GeometryInfo> pts;
    for (int i = 0; i < 10; ++i)
        pts.push_back(makePoint(static_cast<double>(i), 0.0));

    KMeansConfig cfg;
    cfg.k    = 3;
    cfg.seed = 42; // enable k-means++ path
    auto result = kmeansCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 10u);
    EXPECT_EQ(result.num_clusters, 3);
    for (int lbl : result.labels) {
        EXPECT_GE(lbl, 0);
        EXPECT_LT(lbl, 3);
    }
}

// ---------------------------------------------------------------------------
// k-means: non-Point geometry receives label -1
// ---------------------------------------------------------------------------

TEST(KMeansCluster, NonPointGeometry_GetsMinusOne) {
    std::vector<GeometryInfo> pts = {
        makePoint(0.0, 0.0),
        makePolygon(), // non-Point
        makePoint(1.0, 0.0),
    };
    KMeansConfig cfg;
    cfg.k = 1;
    auto result = kmeansCluster(pts, cfg);

    ASSERT_EQ(result.labels.size(), 3u);
    EXPECT_EQ(result.labels[1], -1); // polygon → -1
    EXPECT_EQ(result.labels[0], 0);
    EXPECT_EQ(result.labels[2], 0);
    EXPECT_EQ(result.num_clusters, 1);
}

// ---------------------------------------------------------------------------
// k-means: convergence — same result on repeated calls with same seed
// ---------------------------------------------------------------------------

TEST(KMeansCluster, DeterministicResult_SameSeed) {
    std::vector<GeometryInfo> pts;
    for (int i = 0; i < 12; ++i)
        pts.push_back(makePoint(static_cast<double>(i % 4) * 2.0,
                                static_cast<double>(i / 4) * 2.0));

    KMeansConfig cfg;
    cfg.k    = 3;
    cfg.seed = 0; // deterministic initialisation

    auto r1 = kmeansCluster(pts, cfg);
    auto r2 = kmeansCluster(pts, cfg);

    ASSERT_EQ(r1.labels.size(), r2.labels.size());
    for (std::size_t i = 0; i < r1.labels.size(); ++i) {
        EXPECT_EQ(r1.labels[i], r2.labels[i]);
    }
}

// ---------------------------------------------------------------------------
// Performance: DBSCAN — 10 000 points at 500 m epsilon in ≤ 5 s (AC-9)
// Reference: Ester et al. (1996), KDD-96, pp. 226–231.
// ---------------------------------------------------------------------------

TEST(DbscanCluster, Performance_10kPoints_Under5Seconds) {
    // Guarded: only runs when THEMIS_RUN_PERF_TESTS=1 is set to avoid
    // spurious failures on shared / throttled CI runners.
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping DBSCAN perf benchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-9: DBSCAN ≤ 5 s on 10 000 points.";
    }
    // Generate 10 000 points arranged in a tight 100×100 grid around Berlin
    // (~13.4 °E, 52.5 °N).  Adjacent cells are ~111 m apart, so virtually
    // every point has many neighbours within epsilon = 500 m.
    constexpr int kSide = 100; // 100 × 100 = 10 000 points
    std::vector<GeometryInfo> pts;
    pts.reserve(static_cast<std::size_t>(kSide * kSide));
    for (int row = 0; row < kSide; ++row) {
        for (int col = 0; col < kSide; ++col) {
            // 0.001° spacing ≈ 71 m (lon) / 111 m (lat)
            pts.push_back(makePoint(13.4 + col * 0.001,
                                    52.5 + row * 0.001));
        }
    }

    DbscanConfig cfg;
    cfg.epsilon_m  = 500.0;
    cfg.min_points = 3;

    const auto t0     = std::chrono::steady_clock::now();
    auto       result = dbscanCluster(pts, cfg);
    const auto t1     = std::chrono::steady_clock::now();

    const double elapsed_s =
        std::chrono::duration<double>(t1 - t0).count();

    EXPECT_GE(result.num_clusters, 1);
    EXPECT_EQ(result.labels.size(), static_cast<std::size_t>(kSide * kSide));
    // Performance target: ≤ 5 s single-threaded (design spec, not a hard CI
    // gate, but flagged as a test failure to surface regressions).
    EXPECT_LE(elapsed_s, 5.0)
        << "DBSCAN on 10 000 points exceeded 5 s target (took "
        << elapsed_s << " s)";
}

// ---------------------------------------------------------------------------
// Performance: k-means — k=10, 100 000 points, 100 iterations in ≤ 2 s (AC-10)
// Reference: Lloyd (1982), IEEE Trans. Inf. Theory 28(2):129-137.
// ---------------------------------------------------------------------------

TEST(KMeansCluster, Performance_100kPoints_K10_Under2Seconds) {
    // Guarded: only runs when THEMIS_RUN_PERF_TESTS=1 is set to avoid
    // spurious failures on shared / throttled CI runners.
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping k-means perf benchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-10: k-means k=10, 100 000 points, 100 iter ≤ 2 s.";
    }
    // Generate 100 000 points spread along 10 well-separated bands (one per
    // cluster), each 10 000 points wide, offset by 2° in longitude.
    // Points are generated in interleaved order (j outer, c inner) so the
    // first kK points span all clusters — this ensures the seed=0
    // deterministic init (first k distinct points) picks one centroid per
    // cluster and EXPECT_EQ(num_clusters, kK) is reliable.
    constexpr int kPointsPerCluster = 10000;
    constexpr int kK                = 10;
    std::vector<GeometryInfo> pts;
    pts.reserve(static_cast<std::size_t>(kPointsPerCluster * kK));
    for (int j = 0; j < kPointsPerCluster; ++j) {
        for (int c = 0; c < kK; ++c) {
            const double base_lon = static_cast<double>(c) * 2.0; // 2° separation
            // Jitter: 100 discrete steps of 0.0001° (0.0000..0.0099°, ~0..1.1 km)
            // to avoid identical points within a cluster.
            const double jitter = (j % 100) * 0.0001;
            pts.push_back(makePoint(base_lon + jitter, 48.0 + jitter));
        }
    }

    KMeansConfig cfg;
    cfg.k              = kK;
    cfg.max_iterations = 100;
    cfg.seed           = 0; // deterministic init

    const auto t0     = std::chrono::steady_clock::now();
    auto       result = kmeansCluster(pts, cfg);
    const auto t1     = std::chrono::steady_clock::now();

    const double elapsed_s =
        std::chrono::duration<double>(t1 - t0).count();

    EXPECT_EQ(result.num_clusters, kK);
    EXPECT_EQ(result.labels.size(),
              static_cast<std::size_t>(kPointsPerCluster * kK));
    // Performance target: ≤ 2 s single-threaded (design spec).
    EXPECT_LE(elapsed_s, 2.0)
        << "k-means on 100 000 points (k=10, 100 iter) exceeded 2 s target "
           "(took " << elapsed_s << " s)";
}
