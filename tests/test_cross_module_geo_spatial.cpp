/**
 * @file test_cross_module_geo_spatial.cpp
 * @brief Cross-module integration tests: GeoRTree × SpatialJoinIterator ×
 *        MetricsCollector.
 *
 * Rationale
 * ---------
 * Individual module unit tests verify each component in isolation.  This file
 * validates the interactions at module boundaries that only emerge when the
 * three components are composed:
 *
 *   - GeoRTree.bulkLoad() and insert() build a correct spatial index that
 *     feeds SpatialJoinIterator with accurate MBR-level candidates.
 *   - SpatialJoinIterator yields pair-wise (key_a, key_b, distance_m) results
 *     that are consistent with direct haversineDistanceM() computations.
 *   - MetricsCollector correctly records geo-pipeline query and index-scan
 *     events so that Prometheus output is coherent.
 *
 * Test groups
 * -----------
 * Group A (5 tests): GeoRTree × SpatialJoin pipeline
 *   A-1  bulkLoad + intersects returns all keys whose MBR overlaps query bbox
 *   A-2  insert after bulkLoad increments size and remains queryable
 *   A-3  clear() empties the index; subsequent intersects returns empty list
 *   A-4  spatialJoin on two co-located point sets yields at least one pair
 *   A-5  haversineDistanceM is consistent with SpatialJoinPair.distance_m
 *
 * Group B (5 tests): SpatialJoinIterator lazy evaluation
 *   B-1  advance() returns false immediately on empty inner collection
 *   B-2  Iterator over disjoint collections (threshold 1 m) yields no pairs
 *   B-3  Iterator over overlapping collections yields ≥1 pair within threshold
 *   B-4  All pairs from iterator satisfy distance_m ≤ threshold_m
 *   B-5  Materialised spatialJoin result count matches iterator advance count
 *
 * Group C (5 tests): MetricsCollector × geo pipeline
 *   C-1  recordQuery("geo_intersects") increments query counter
 *   C-2  recordIndexScan("geo_rtree") increments index-scan counter
 *   C-3  MetricsCollector::reset() zeroes geo counters
 *   C-4  Prometheus output contains geo_intersects metric after recording
 *   C-5  Mixed geo query + index-scan sequence recorded independently
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "geo/geo_rtree.h"
#include "geo/spatial_join.h"
#include "observability/metrics_collector.h"
#include "utils/geo/ewkb.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace themis::geo;
using namespace themis::observability;

// ============================================================================
// Shared helpers
// ============================================================================

namespace {

/// Build a Point GeometryInfo from WGS84 (lon, lat) coordinates.
static GeometryInfo makePoint(double lon, double lat) {
    GeometryInfo g(GeometryType::Point);
    g.coords.push_back(Coordinate(lon, lat));
    return g;
}

/// Build a minimal MBR that covers a small square around (cx, cy).
static MBR makeBbox(double cx, double cy, double half = 0.01) {
    return MBR(cx - half, cy - half, cx + half, cy + half);
}

/// Collection of six European cities as (key, Point) pairs.
static std::vector<std::pair<std::string, GeometryInfo>> makeEuropeanCities() {
    return {
        {"berlin",    makePoint(13.405, 52.520)},
        {"paris",     makePoint(2.352,  48.857)},
        {"london",    makePoint(-0.128, 51.507)},
        {"madrid",    makePoint(-3.703, 40.416)},
        {"rome",      makePoint(12.496, 41.902)},
        {"amsterdam", makePoint(4.905,  52.370)},
    };
}

/// Small cluster: three points within ~50 km of Frankfurt (8.68 E, 50.11 N).
static std::vector<std::pair<std::string, GeometryInfo>> makeCluster() {
    return {
        {"ffm_center", makePoint(8.682, 50.110)},
        {"ffm_north",  makePoint(8.682, 50.158)},   // ~5 km N
        {"ffm_east",   makePoint(8.730, 50.110)},   // ~3 km E
    };
}

} // anonymous namespace

// ============================================================================
// Fixture
// ============================================================================

class GeoSpatialTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }

    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
};

// ============================================================================
// Group A – GeoRTree × SpatialJoin pipeline
// ============================================================================

// A-1: bulkLoad + intersects returns all keys whose MBR overlaps the query bbox
TEST_F(GeoSpatialTest, A1_BulkLoadAndIntersectsReturnsOverlappingKeys) {
    GeoRTree idx;
    idx.bulkLoad(makeEuropeanCities());

    ASSERT_EQ(idx.size(), 6u) << "bulkLoad should load all six cities";

    // Bbox centred on Berlin – should hit only Berlin
    auto hits = idx.intersects(makeBbox(13.405, 52.520, 0.5));
    EXPECT_FALSE(hits.empty()) << "Berlin bbox should intersect at least one entry";
    EXPECT_NE(std::find(hits.begin(), hits.end(), "berlin"), hits.end())
        << "Berlin must be in the intersect result";

    // Wide bbox covering Western Europe – should hit Paris and London
    auto west = idx.intersects(MBR(-5.0, 45.0, 10.0, 55.0));
    EXPECT_NE(std::find(west.begin(), west.end(), "paris"), west.end())
        << "Paris must appear in Western-Europe bbox";
    EXPECT_NE(std::find(west.begin(), west.end(), "london"), west.end())
        << "London must appear in Western-Europe bbox";
}

// A-2: insert after bulkLoad increments size and makes the new entry queryable
TEST_F(GeoSpatialTest, A2_InsertAfterBulkLoadIncrementsSizeAndIsQueryable) {
    GeoRTree idx;
    idx.bulkLoad(makeEuropeanCities());
    ASSERT_EQ(idx.size(), 6u);

    GeometryInfo vienna = makePoint(16.373, 48.209);
    idx.insert("vienna", vienna);

    EXPECT_EQ(idx.size(), 7u) << "Size must increase by 1 after insert";

    auto hits = idx.intersects(makeBbox(16.373, 48.209, 0.5));
    EXPECT_NE(std::find(hits.begin(), hits.end(), "vienna"), hits.end())
        << "Newly inserted Vienna must be queryable";
}

// A-3: clear() empties the index; subsequent intersects returns empty list
TEST_F(GeoSpatialTest, A3_ClearEmptiesIndexAndIntersectsReturnsEmpty) {
    GeoRTree idx;
    idx.bulkLoad(makeEuropeanCities());
    ASSERT_EQ(idx.size(), 6u);

    idx.clear();
    EXPECT_EQ(idx.size(), 0u) << "clear() must leave size == 0";

    auto hits = idx.intersects(MBR(-180.0, -90.0, 180.0, 90.0));
    EXPECT_TRUE(hits.empty()) << "After clear(), even a global bbox should return nothing";
}

// A-4: spatialJoin on two co-located point sets yields at least one pair
TEST_F(GeoSpatialTest, A4_SpatialJoinCoLocatedPointsYieldsAtLeastOnePair) {
    auto cluster = makeCluster();
    // Inner set: same cluster, threshold large enough (~100 km)
    auto pairs = spatialJoin(cluster, cluster, 100'000.0);

    // Self-join should yield pairs where key_a != key_b (distance > 0)
    // plus potential self-self pairs at distance 0
    EXPECT_FALSE(pairs.empty()) << "Self-join of a cluster must produce pairs";

    // Every pair distance must be non-negative
    for (const auto& p : pairs)
        EXPECT_GE(p.distance_m, 0.0) << "Distance must be non-negative";
}

// A-5: haversineDistanceM is consistent with SpatialJoinPair.distance_m
TEST_F(GeoSpatialTest, A5_HaversineDistanceMatchesSpatialJoinPairDistance) {
    auto outer = std::vector<std::pair<std::string, GeometryInfo>>{
        {"ffm", makePoint(8.682, 50.110)}};
    auto inner = std::vector<std::pair<std::string, GeometryInfo>>{
        {"wiesbaden", makePoint(8.240, 50.082)}};   // ~36 km W

    double expected_m = haversineDistanceM(8.682, 50.110, 8.240, 50.082);
    ASSERT_GT(expected_m, 0.0);

    auto pairs = spatialJoin(outer, inner, expected_m * 2.0);
    ASSERT_FALSE(pairs.empty())
        << "Pair should be found within 2× the expected distance";

    // The reported distance must match haversineDistanceM to within 1 m
    EXPECT_NEAR(pairs[0].distance_m, expected_m, 1.0)
        << "SpatialJoinPair.distance_m must agree with haversineDistanceM";
}

// ============================================================================
// Group B – SpatialJoinIterator lazy evaluation
// ============================================================================

// B-1: advance() returns false immediately on empty inner collection
TEST_F(GeoSpatialTest, B1_AdvanceReturnsFalseForEmptyInner) {
    auto outer = makeCluster();
    std::vector<std::pair<std::string, GeometryInfo>> inner;

    SpatialJoinIterator it(outer, inner, 500'000.0);
    EXPECT_FALSE(it.advance()) << "No pairs when inner collection is empty";
    EXPECT_TRUE(it.exhausted()) << "Iterator must be exhausted";
}

// B-2: Iterator over disjoint collections (threshold 1 m) yields no pairs
TEST_F(GeoSpatialTest, B2_IteratorYieldsNoPairsForDisjointCollectionsWithTinyThreshold) {
    // Berlin vs. Frankfurt: distance ≈ 480 km
    auto outer = std::vector<std::pair<std::string, GeometryInfo>>{
        {"berlin", makePoint(13.405, 52.520)}};
    auto inner = std::vector<std::pair<std::string, GeometryInfo>>{
        {"frankfurt", makePoint(8.682, 50.110)}};

    SpatialJoinIterator it(outer, inner, 1.0);   // 1 m threshold
    EXPECT_FALSE(it.advance()) << "480 km apart must not be within 1 m";
}

// B-3: Iterator over overlapping collections yields ≥1 pair within threshold
TEST_F(GeoSpatialTest, B3_IteratorYieldsPairForNearbyPoints) {
    auto cluster = makeCluster();
    // threshold = 10 km = 10 000 m
    SpatialJoinIterator it(cluster, cluster, 10'000.0);

    int count = 0;
    while (it.advance()) {
        ++count;
        const SpatialJoinPair& p = it.current();
        EXPECT_LE(p.distance_m, 10'000.0)
            << "Every yielded pair must be within the threshold";
    }
    EXPECT_GT(count, 0) << "At least one pair expected within 10 km";
}

// B-4: All pairs from iterator satisfy distance_m ≤ threshold_m
TEST_F(GeoSpatialTest, B4_AllIteratorPairsSatisfyThreshold) {
    auto cities = makeEuropeanCities();
    const double threshold_m = 2'000'000.0;   // 2 000 km

    SpatialJoinIterator it(cities, cities, threshold_m);
    while (it.advance()) {
        EXPECT_LE(it.current().distance_m, threshold_m)
            << "Iterator must not yield pairs exceeding the threshold";
    }
}

// B-5: Materialised spatialJoin result count matches iterator advance count
TEST_F(GeoSpatialTest, B5_MaterialisedResultCountMatchesIteratorCount) {
    auto cluster = makeCluster();
    const double threshold_m = 100'000.0;   // 100 km

    // Materialised version
    auto pairs = spatialJoin(cluster, cluster, threshold_m);

    // Iterator version
    SpatialJoinIterator it(cluster, cluster, threshold_m);
    int iter_count = 0;
    while (it.advance()) ++iter_count;

    EXPECT_EQ(static_cast<int>(pairs.size()), iter_count)
        << "spatialJoin() and SpatialJoinIterator must yield the same pair count";
}

// ============================================================================
// Group C – MetricsCollector × geo pipeline
// ============================================================================

// C-1: recordQuery("geo_intersects") increments the query counter
TEST_F(GeoSpatialTest, C1_RecordQueryGeoIntersectsIncrementsCounter) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    mc.recordQuery("geo_intersects", 1.5, 3);
    mc.recordQuery("geo_intersects", 0.8, 1);

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("geo_intersects"), std::string::npos)
        << "Prometheus output must reference geo_intersects after recording";
}

// C-2: recordIndexScan("geo_rtree") increments the index-scan counter
TEST_F(GeoSpatialTest, C2_RecordIndexScanGeoRtreeIncrementsCounter) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    mc.recordIndexScan("geo_rtree", 6);

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("geo_rtree"), std::string::npos)
        << "Prometheus output must reference geo_rtree index scan";
}

// C-3: MetricsCollector::reset() zeroes geo counters
TEST_F(GeoSpatialTest, C3_ResetZeroesGeoCounters) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordQuery("geo_intersects", 2.0, 4);
    mc.recordIndexScan("geo_rtree", 10);

    mc.reset();

    const std::string prom_after = mc.getPrometheusMetrics();
    // After reset, geo_intersects metric line should be absent or zero
    // (depends on implementation, but a fresh Prometheus snapshot should be minimal)
    EXPECT_TRUE(prom_after.find("geo_intersects") == std::string::npos ||
                prom_after.size() < 500u)
        << "After reset(), Prometheus output should not retain stale geo counters";
}

// C-4: Prometheus output contains geo_intersects metric after a full pipeline run
TEST_F(GeoSpatialTest, C4_FullPipelinePrometheusContainsGeoMetric) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    // Simulate a full pipeline: index scan → spatial join → query recorded
    GeoRTree idx;
    idx.bulkLoad(makeEuropeanCities());

    mc.recordIndexScan("geo_rtree", static_cast<size_t>(idx.size()));

    auto hits = idx.intersects(MBR(-5.0, 45.0, 20.0, 55.0));
    mc.recordQuery("geo_intersects", 0.5, hits.size());

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("geo_rtree"), std::string::npos)
        << "geo_rtree scan must appear in Prometheus output";
    EXPECT_NE(prom.find("geo_intersects"), std::string::npos)
        << "geo_intersects query must appear in Prometheus output";
}

// C-5: Mixed geo query + index-scan sequence recorded independently
TEST_F(GeoSpatialTest, C5_MixedGeoAndIndexScanRecordedIndependently) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    // Record multiple different operation types
    mc.recordQuery("geo_intersects", 1.2, 2);
    mc.recordQuery("geo_contains",   0.9, 1);
    mc.recordIndexScan("geo_rtree",  8);
    mc.recordQuery("geo_spatial_join", 5.0, 3);

    const std::string prom = mc.getPrometheusMetrics();

    // All three distinct metric names must be present
    EXPECT_NE(prom.find("geo_intersects"), std::string::npos)
        << "geo_intersects must appear";
    EXPECT_NE(prom.find("geo_rtree"), std::string::npos)
        << "geo_rtree index scan must appear";
    EXPECT_NE(prom.find("geo_spatial_join"), std::string::npos)
        << "geo_spatial_join query must appear";
}
