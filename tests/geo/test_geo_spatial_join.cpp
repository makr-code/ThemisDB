#include <gtest/gtest.h>
#include "geo/spatial_join.h"
#include "utils/geo/ewkb.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
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

/// Make a small axis-aligned square Polygon centred at (lon, lat).
/// The centroid of the resulting ring should be very close to (lon, lat).
static GeometryInfo makeSquarePolygon(double lon, double lat, double half_deg = 0.0001) {
    GeometryInfo g(GeometryType::Polygon);
    // Closed ring: 5 vertices (last == first)
    g.rings.push_back({
        {lon - half_deg, lat - half_deg},
        {lon + half_deg, lat - half_deg},
        {lon + half_deg, lat + half_deg},
        {lon - half_deg, lat + half_deg},
        {lon - half_deg, lat - half_deg},
    });
    return g;
}

static bool hasPair(const std::vector<SpatialJoinPair>& pairs,
                    const std::string& ka, const std::string& kb) {
    for (const auto& p : pairs) {
        if (p.key_a == ka && p.key_b == kb) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// haversineDistanceM unit tests
// ---------------------------------------------------------------------------

TEST(HaversineDistanceM, SamePoint_IsZero) {
    EXPECT_NEAR(haversineDistanceM(13.4050, 52.5200, 13.4050, 52.5200), 0.0, 1e-6);
}

TEST(HaversineDistanceM, BerlinToParis_Approx) {
    // Berlin (13.4050, 52.5200) → Paris (2.3522, 48.8566) ≈ 878 km
    const double dist = haversineDistanceM(13.4050, 52.5200, 2.3522, 48.8566);
    EXPECT_NEAR(dist, 878'000.0, 5'000.0) // ±5 km tolerance
        << "Berlin-Paris distance should be approximately 878 km";
}

TEST(HaversineDistanceM, ShortDistance_500m) {
    // Two points ~500 m apart at equator (approximately 0.0045 degrees longitude)
    const double dist = haversineDistanceM(0.0, 0.0, 0.0045, 0.0);
    EXPECT_NEAR(dist, 500.0, 20.0);
}

TEST(HaversineDistanceM, Symmetric) {
    const double d1 = haversineDistanceM(10.0, 50.0, 11.0, 51.0);
    const double d2 = haversineDistanceM(11.0, 51.0, 10.0, 50.0);
    EXPECT_NEAR(d1, d2, 1e-6);
}

// ---------------------------------------------------------------------------
// spatialJoin unit tests
// ---------------------------------------------------------------------------

TEST(SpatialJoin, EmptyOuter_ReturnsEmpty) {
    std::vector<std::pair<std::string, GeometryInfo>> outer;
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4, 52.5)}
    };
    auto result = spatialJoin(outer, inner, 1000.0);
    EXPECT_TRUE(result.empty());
}

TEST(SpatialJoin, EmptyInner_ReturnsEmpty) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4, 52.5)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner;
    auto result = spatialJoin(outer, inner, 1000.0);
    EXPECT_TRUE(result.empty());
}

TEST(SpatialJoin, ZeroThreshold_ReturnsEmpty) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4, 52.5)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4, 52.5)}
    };
    auto result = spatialJoin(outer, inner, 0.0);
    EXPECT_TRUE(result.empty());
}

TEST(SpatialJoin, NegativeThreshold_ReturnsEmpty) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(0.0, 0.0)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(0.0, 0.0)}
    };
    auto result = spatialJoin(outer, inner, -500.0);
    EXPECT_TRUE(result.empty());
}

TEST(SpatialJoin, IdenticalPoints_WithinThreshold) {
    // Same coordinate — distance is 0, always within any positive threshold.
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4050, 52.5200)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4050, 52.5200)}
    };
    auto result = spatialJoin(outer, inner, 1.0); // 1 m threshold
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].key_a, "a1");
    EXPECT_EQ(result[0].key_b, "b1");
    EXPECT_NEAR(result[0].distance_m, 0.0, 1e-3);
}

TEST(SpatialJoin, PointsTooFarApart_ReturnsEmpty) {
    // Berlin and Paris are ~878 km apart — well outside a 10 km threshold.
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"berlin", makePoint(13.4050, 52.5200)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"paris", makePoint(2.3522, 48.8566)}
    };
    auto result = spatialJoin(outer, inner, 10'000.0); // 10 km threshold
    EXPECT_TRUE(result.empty());
}

TEST(SpatialJoin, TwoClose_OneDistant) {
    // a1 is close to b1 and b2 but far from b3.
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4050, 52.5200)} // Berlin
    };
    // b1: ~63 m east of a1 (0.0009° longitude ≈ 63 m at lat 52.5°)
    // b2: ~500 m north of a1 (0.0045° latitude ≈ 500 m)
    // b3: ~878 km from a1 (Paris)
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4050 + 0.0009, 52.5200)},  // ~63 m east
        {"b2", makePoint(13.4050,           52.5245)},  // ~500 m north
        {"b3", makePoint(2.3522,            48.8566)}   // Paris
    };
    auto result = spatialJoin(outer, inner, 1000.0); // 1 km threshold
    ASSERT_EQ(result.size(), 2u);
    EXPECT_TRUE(hasPair(result, "a1", "b1"));
    EXPECT_TRUE(hasPair(result, "a1", "b2"));
    EXPECT_FALSE(hasPair(result, "a1", "b3"));
}

TEST(SpatialJoin, MultipleOuterPoints) {
    // Two outer points at different latitudes (0° and 10°), two inner points
    // close to each respective outer point (~111 m at 0° lat, ~109 m at 10° lat).
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(0.0, 0.0)},
        {"a2", makePoint(10.0, 10.0)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(0.001, 0.0)},   // ~111 m from a1
        {"b2", makePoint(10.001, 10.0)}  // ~111 m from a2
    };
    auto result = spatialJoin(outer, inner, 500.0); // 500 m threshold
    ASSERT_EQ(result.size(), 2u);
    EXPECT_TRUE(hasPair(result, "a1", "b1"));
    EXPECT_TRUE(hasPair(result, "a2", "b2"));
    EXPECT_FALSE(hasPair(result, "a1", "b2"));
    EXPECT_FALSE(hasPair(result, "a2", "b1"));
}

TEST(SpatialJoin, DistanceInResult_IsAccurate) {
    // a1 at origin, b1 one degree north (~111 km).
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(0.0, 0.0)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(0.0, 1.0)}
    };
    auto result = spatialJoin(outer, inner, 120'000.0); // 120 km threshold
    ASSERT_EQ(result.size(), 1u);
    // 1 degree of latitude ≈ 111 320 m
    EXPECT_NEAR(result[0].distance_m, 111'320.0, 500.0);
}

TEST(SpatialJoin, MaxPairsLimit_IsEnforced) {
    // 10 outer × 10 inner = 100 potential pairs; set limit to 5.
    std::vector<std::pair<std::string, GeometryInfo>> outer, inner;
    for (int i = 0; i < 10; ++i) {
        outer.push_back({"a" + std::to_string(i), makePoint(static_cast<double>(i) * 0.001, 0.0)});
        inner.push_back({"b" + std::to_string(i), makePoint(static_cast<double>(i) * 0.001, 0.0)});
    }
    SpatialJoinConfig cfg;
    cfg.max_pairs = 5;
    auto result = spatialJoin(outer, inner, 10'000.0, cfg);
    EXPECT_LE(result.size(), 5u);
}

TEST(SpatialJoin, AllResultsWithinThreshold) {
    // Verify that every returned pair satisfies distance_m <= threshold_m.
    std::vector<std::pair<std::string, GeometryInfo>> outer, inner;
    for (int i = 0; i < 20; ++i) {
        outer.push_back({"a" + std::to_string(i),
                         makePoint(static_cast<double>(i) * 0.1, 0.0)});
    }
    for (int j = 0; j < 20; ++j) {
        inner.push_back({"b" + std::to_string(j),
                         makePoint(static_cast<double>(j) * 0.1, 0.0)});
    }
    const double threshold = 50'000.0; // 50 km
    auto result = spatialJoin(outer, inner, threshold);
    for (const auto& p : result) {
        EXPECT_LE(p.distance_m, threshold + 1e-6) // +epsilon for floating-point
            << "Pair (" << p.key_a << ", " << p.key_b
            << ") distance " << p.distance_m << " m exceeds threshold";
    }
}

TEST(SpatialJoin, SelfJoin_IdenticalCollections) {
    // Spatial join of a collection with itself should include all same-key pairs.
    std::vector<std::pair<std::string, GeometryInfo>> pts{
        {"p0", makePoint(0.0, 0.0)},
        {"p1", makePoint(0.001, 0.0)}, // ~111 m from p0
        {"p2", makePoint(5.0, 5.0)}    // far away
    };
    auto result = spatialJoin(pts, pts, 500.0);
    // p0↔p0, p0↔p1, p1↔p0, p1↔p1 should all be within 500 m;
    // p2 pairs with p2 only (p0/p1 are far from p2).
    EXPECT_TRUE(hasPair(result, "p0", "p0"));
    EXPECT_TRUE(hasPair(result, "p0", "p1"));
    EXPECT_TRUE(hasPair(result, "p1", "p0"));
    EXPECT_TRUE(hasPair(result, "p1", "p1"));
    EXPECT_TRUE(hasPair(result, "p2", "p2"));
    EXPECT_FALSE(hasPair(result, "p0", "p2"));
}

TEST(SpatialJoin, NonPointGeometry_UsesCentroid) {
    // For non-Point geometries the centroid is used for distance computation.
    // A tiny square polygon centred at Berlin should behave like a point there.
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"poly_berlin", makeSquarePolygon(13.4050, 52.5200)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        // pt_near: 0.0045° lat north of Berlin.  At ~111 320 m/degree, that is
        // ≈ 501 m (verified via haversineDistanceM(13.4050,52.5200,13.4050,52.5245)).
        {"pt_near",    makePoint(13.4050, 52.5245)},
        // pt_distant: Paris at (2.3522, 48.8566) — ≈ 877 km from Berlin.
        {"pt_distant", makePoint(2.3522,  48.8566)}
    };
    auto result = spatialJoin(outer, inner, 1000.0);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_TRUE(hasPair(result, "poly_berlin", "pt_near"));
    EXPECT_FALSE(hasPair(result, "poly_berlin", "pt_distant"));
}

// ---------------------------------------------------------------------------
// SpatialJoinIterator unit tests
// ---------------------------------------------------------------------------

TEST(SpatialJoinIterator, EmptyOuter_IteratorDoneImmediately) {
    std::vector<std::pair<std::string, GeometryInfo>> outer;
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4, 52.5)}
    };
    SpatialJoinIterator it(outer, inner, 1000.0);
    EXPECT_FALSE(it.advance());
    EXPECT_TRUE(it.done());
}

TEST(SpatialJoinIterator, EmptyInner_IteratorDoneImmediately) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4, 52.5)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner;
    SpatialJoinIterator it(outer, inner, 1000.0);
    EXPECT_FALSE(it.advance());
    EXPECT_TRUE(it.done());
}

TEST(SpatialJoinIterator, ZeroThreshold_IteratorDoneImmediately) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4, 52.5)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4, 52.5)}
    };
    SpatialJoinIterator it(outer, inner, 0.0);
    EXPECT_FALSE(it.advance());
    EXPECT_TRUE(it.done());
}

TEST(SpatialJoinIterator, IdenticalPoints_YieldsOnePair) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4050, 52.5200)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4050, 52.5200)}
    };
    SpatialJoinIterator it(outer, inner, 1.0); // 1 m threshold
    ASSERT_TRUE(it.advance());
    EXPECT_EQ(it.current().key_a, "a1");
    EXPECT_EQ(it.current().key_b, "b1");
    EXPECT_NEAR(it.current().distance_m, 0.0, 1e-3);
    EXPECT_FALSE(it.advance()); // No more pairs
    EXPECT_TRUE(it.done());
}

TEST(SpatialJoinIterator, TwoPairsYieldedInOrder) {
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(0.0, 0.0)},
        {"a2", makePoint(10.0, 10.0)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(0.001, 0.0)},   // ~111 m from a1
        {"b2", makePoint(10.001, 10.0)}  // ~111 m from a2
    };
    SpatialJoinIterator it(outer, inner, 500.0);

    int count = 0;
    bool saw_a1_b1 = false;
    bool saw_a2_b2 = false;
    while (it.advance()) {
        ++count;
        const auto& p = it.current();
        if (p.key_a == "a1" && p.key_b == "b1") saw_a1_b1 = true;
        if (p.key_a == "a2" && p.key_b == "b2") saw_a2_b2 = true;
    }
    EXPECT_EQ(count, 2);
    EXPECT_TRUE(saw_a1_b1);
    EXPECT_TRUE(saw_a2_b2);
    EXPECT_TRUE(it.done());
}

TEST(SpatialJoinIterator, MaxPairsLimit_StopsEarly) {
    std::vector<std::pair<std::string, GeometryInfo>> outer, inner;
    for (int i = 0; i < 10; ++i) {
        outer.push_back({"a" + std::to_string(i), makePoint(static_cast<double>(i) * 0.001, 0.0)});
        inner.push_back({"b" + std::to_string(i), makePoint(static_cast<double>(i) * 0.001, 0.0)});
    }
    SpatialJoinConfig cfg;
    cfg.max_pairs = 3;
    SpatialJoinIterator it(outer, inner, 10'000.0, cfg);

    std::size_t count = 0;
    while (it.advance()) ++count;
    EXPECT_LE(count, 3u);
    EXPECT_TRUE(it.done());
}

TEST(SpatialJoinIterator, AllResultsWithinThreshold) {
    std::vector<std::pair<std::string, GeometryInfo>> outer, inner;
    for (int i = 0; i < 15; ++i) {
        outer.push_back({"a" + std::to_string(i),
                         makePoint(static_cast<double>(i) * 0.1, 0.0)});
        inner.push_back({"b" + std::to_string(i),
                         makePoint(static_cast<double>(i) * 0.1, 0.0)});
    }
    const double threshold = 50'000.0; // 50 km
    SpatialJoinIterator it(outer, inner, threshold);
    while (it.advance()) {
        EXPECT_LE(it.current().distance_m, threshold + 1e-6)
            << "Pair (" << it.current().key_a << ", " << it.current().key_b
            << ") distance " << it.current().distance_m << " m exceeds threshold";
    }
}

TEST(SpatialJoinIterator, MatchesMaterializedSpatialJoin) {
    // The lazy iterator must produce the same pairs as the batch spatialJoin().
    std::vector<std::pair<std::string, GeometryInfo>> outer{
        {"a1", makePoint(13.4050, 52.5200)}, // Berlin
        {"a2", makePoint(0.0, 0.0)}
    };
    std::vector<std::pair<std::string, GeometryInfo>> inner{
        {"b1", makePoint(13.4050 + 0.0009, 52.5200)}, // ~63 m
        {"b2", makePoint(13.4050,           52.5245)}, // ~500 m
        {"b3", makePoint(2.3522,            48.8566)}, // Paris (far)
        {"b4", makePoint(0.001, 0.0)}                  // ~111 m from a2
    };
    const double threshold = 1000.0;

    auto batch = spatialJoin(outer, inner, threshold);
    std::set<std::pair<std::string,std::string>> batch_set;
    for (const auto& p : batch) batch_set.emplace(p.key_a, p.key_b);

    SpatialJoinIterator it(outer, inner, threshold);
    std::set<std::pair<std::string,std::string>> iter_set;
    while (it.advance()) iter_set.emplace(it.current().key_a, it.current().key_b);

    EXPECT_EQ(batch_set, iter_set);
}
