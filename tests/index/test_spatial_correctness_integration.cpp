// Integration tests: Spatial index correctness (Production Readiness Checklist)
//
// Validates that SpatialIndexManager returns correct results for the full set
// of spatial query types:
//   - searchIntersects  (bbox overlap)
//   - searchWithin      (full containment)
//   - searchContains    (index entry contains query bbox)
//   - searchNearby      (radius / haversine)
//   - searchKNN         (k nearest centroids)
//   - searchZRange      (elevation band)
//   - Negative cases    (query outside all entries returns empty)
//   - Precision gate    (no false positives across a large synthetic dataset)
//
// All tests use a real (temporary) RocksDB instance and the built-in CPU exact
// backend to exercise the full SpatialIndexManager path.

#include <gtest/gtest.h>
#include "index/spatial_index.h"
#include "geo/spatial_backend.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/geo/ewkb.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace themis;
using namespace themis::index;
using namespace themis::geo;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class SpatialCorrectnessTest : public ::testing::Test {
protected:
    static constexpr const char* kTable = "correctness_table";

    void SetUp() override {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path_ = (fs::temp_directory_path() /
                    ("themis_spatial_correctness_" + std::to_string(now))).string();

        RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 16;
        cfg.block_cache_size_mb = 16;
        db_  = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        mgr_ = std::make_unique<SpatialIndexManager>(*db_);

        // Wire the CPU exact backend for full exact-check coverage.
        auto* cpu = getCpuExactBackend();
        ASSERT_NE(cpu, nullptr);
        mgr_->setExactBackend(cpu);

        RTreeConfig rtcfg;
        rtcfg.total_bounds = MBR(-180.0, -90.0, 180.0, 90.0);
        ASSERT_TRUE(mgr_->createSpatialIndex(kTable, "geometry", rtcfg));
    }

    void TearDown() override {
        mgr_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    // ----- helpers -----

    static GeoSidecar pointSidecar(double lon, double lat) {
        GeoSidecar sc;
        sc.mbr      = MBR(lon, lat, lon, lat);
        sc.centroid = Coordinate(lon, lat);
        return sc;
    }

    static GeoSidecar boxSidecar(double minx, double miny,
                                  double maxx, double maxy) {
        GeoSidecar sc;
        sc.mbr      = MBR(minx, miny, maxx, maxy);
        sc.centroid = Coordinate((minx + maxx) * 0.5, (miny + maxy) * 0.5);
        return sc;
    }

    static GeoSidecar pointSidecar3D(double lon, double lat, double z) {
        GeoSidecar sc;
        sc.mbr      = MBR(lon, lat, lon, lat);
        sc.centroid = Coordinate(lon, lat);
        sc.z_min    = z;
        sc.z_max    = z;
        return sc;
    }

    static bool hasKey(const std::vector<SpatialResult>& v,
                       const std::string& k) {
        return std::any_of(v.begin(), v.end(),
                           [&](const SpatialResult& r){ return r.primary_key == k; });
    }

    std::string                           db_path_;
    std::unique_ptr<RocksDBWrapper>       db_;
    std::unique_ptr<SpatialIndexManager>  mgr_;
};

// ===========================================================================
// searchIntersects – correctness
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Intersects_SinglePoint_Inside) {
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", pointSidecar(13.4, 52.5)));

    auto res = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].primary_key, "berlin");
}

TEST_F(SpatialCorrectnessTest, Intersects_SinglePoint_Outside) {
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", pointSidecar(13.4, 52.5)));

    auto res = mgr_->searchIntersects(kTable, MBR(0.0, 0.0, 5.0, 5.0));
    EXPECT_TRUE(res.empty());
}

TEST_F(SpatialCorrectnessTest, Intersects_MultiplePoints_CorrectSubset) {
    ASSERT_TRUE(mgr_->insert(kTable, "berlin",  pointSidecar(13.4,   52.5)));
    ASSERT_TRUE(mgr_->insert(kTable, "paris",   pointSidecar( 2.35,  48.85)));
    ASSERT_TRUE(mgr_->insert(kTable, "hamburg", pointSidecar(10.0,   53.6)));
    ASSERT_TRUE(mgr_->insert(kTable, "madrid",  pointSidecar(-3.7,   40.4)));

    // Bbox covering Germany only
    auto res = mgr_->searchIntersects(kTable, MBR(6.0, 47.0, 15.0, 55.0));
    EXPECT_EQ(res.size(), 2u);
    EXPECT_TRUE(hasKey(res, "berlin"));
    EXPECT_TRUE(hasKey(res, "hamburg"));
    EXPECT_FALSE(hasKey(res, "paris"));
    EXPECT_FALSE(hasKey(res, "madrid"));
}

TEST_F(SpatialCorrectnessTest, Intersects_BoxVsBox_Overlapping) {
    // Insert a box covering [10,50]-[12,52]
    ASSERT_TRUE(mgr_->insert(kTable, "poly_a", boxSidecar(10.0, 50.0, 12.0, 52.0)));

    // Query bbox overlaps the inserted box
    auto res = mgr_->searchIntersects(kTable, MBR(11.0, 51.0, 14.0, 54.0));
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].primary_key, "poly_a");
}

TEST_F(SpatialCorrectnessTest, Intersects_BoxVsBox_Disjoint) {
    ASSERT_TRUE(mgr_->insert(kTable, "poly_a", boxSidecar(10.0, 50.0, 12.0, 52.0)));

    // Query bbox is completely outside the inserted box
    auto res = mgr_->searchIntersects(kTable, MBR(20.0, 60.0, 25.0, 65.0));
    EXPECT_TRUE(res.empty());
}

// ===========================================================================
// Insert / remove correctness
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Remove_DisappearsFromResults) {
    auto sc = pointSidecar(13.4, 52.5);
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", sc));

    EXPECT_EQ(mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0)).size(), 1u);

    ASSERT_TRUE(mgr_->remove(kTable, "berlin", sc));

    EXPECT_TRUE(mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0)).empty());
}

TEST_F(SpatialCorrectnessTest, Remove_OtherEntriesUnaffected) {
    auto sc_b = pointSidecar(13.4, 52.5);
    auto sc_p = pointSidecar(2.35, 48.85);
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", sc_b));
    ASSERT_TRUE(mgr_->insert(kTable, "paris",  sc_p));

    ASSERT_TRUE(mgr_->remove(kTable, "berlin", sc_b));

    auto res = mgr_->searchIntersects(kTable, MBR(-180.0, -90.0, 180.0, 90.0));
    EXPECT_EQ(res.size(), 1u);
    EXPECT_TRUE(hasKey(res, "paris"));
    EXPECT_FALSE(hasKey(res, "berlin"));
}

TEST_F(SpatialCorrectnessTest, Update_MovesEntryToNewLocation) {
    auto sc_old = pointSidecar(13.4, 52.5);   // Berlin
    auto sc_new = pointSidecar(2.35, 48.85);  // Paris

    ASSERT_TRUE(mgr_->insert(kTable, "city", sc_old));
    ASSERT_TRUE(mgr_->update(kTable, "city", sc_old, sc_new));

    // Should no longer be in the Berlin area
    auto in_berlin = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    EXPECT_FALSE(hasKey(in_berlin, "city"));

    // Should now be in the Paris area
    auto in_paris = mgr_->searchIntersects(kTable, MBR(1.0, 47.0, 4.0, 50.0));
    EXPECT_TRUE(hasKey(in_paris, "city"));
}

// ===========================================================================
// searchWithin – point contained in query bbox
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Within_PointStrictlyInside) {
    ASSERT_TRUE(mgr_->insert(kTable, "pt", pointSidecar(5.0, 5.0)));

    auto res = mgr_->searchWithin(kTable, MBR(0.0, 0.0, 10.0, 10.0));
    ASSERT_FALSE(res.empty());
    EXPECT_TRUE(hasKey(res, "pt"));
}

TEST_F(SpatialCorrectnessTest, Within_PointOutsideBbox) {
    ASSERT_TRUE(mgr_->insert(kTable, "pt", pointSidecar(50.0, 50.0)));

    auto res = mgr_->searchWithin(kTable, MBR(0.0, 0.0, 10.0, 10.0));
    EXPECT_FALSE(hasKey(res, "pt"));
}

// ===========================================================================
// searchContains – index entry contains the query bbox
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Contains_LargeBoxContainsPoint) {
    // Insert a large box [0,0]-[20,20]
    ASSERT_TRUE(mgr_->insert(kTable, "big", boxSidecar(0.0, 0.0, 20.0, 20.0)));
    // Insert a small box [30,30]-[40,40] that does NOT contain the query point
    ASSERT_TRUE(mgr_->insert(kTable, "small", boxSidecar(30.0, 30.0, 40.0, 40.0)));

    // Query point (7.0, 7.0) is inside "big" but not inside "small"
    auto res = mgr_->searchContains(kTable, 7.0 /*x=lon*/, 7.0 /*y=lat*/);
    EXPECT_TRUE(hasKey(res, "big"));
    EXPECT_FALSE(hasKey(res, "small"));
}

// ===========================================================================
// searchNearby – radius / haversine (geographic coordinates)
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Nearby_PointWithinRadius) {
    // Insert a point at the origin and one 5000 km away (lon=45, lat=0)
    // Distance from (0,0) to (45,0) ≈ 5003 km; only the origin should be within 1000 km.
    ASSERT_TRUE(mgr_->insert(kTable, "origin", pointSidecar(0.0, 0.0)));
    ASSERT_TRUE(mgr_->insert(kTable, "distant", pointSidecar(45.0, 0.0)));

    // searchNearby(table, x=lon, y=lat, radius_m)
    auto res = mgr_->searchNearby(kTable,
                                   0.0 /*x=lon*/, 0.0 /*y=lat*/,
                                   1'000'000.0 /*1000 km*/);
    EXPECT_TRUE(hasKey(res, "origin"));
    EXPECT_FALSE(hasKey(res, "distant"));
}

TEST_F(SpatialCorrectnessTest, Nearby_NoPointWithinRadius) {
    // Only point at (45, 0); query from origin with 100 km radius
    ASSERT_TRUE(mgr_->insert(kTable, "distant", pointSidecar(45.0, 0.0)));

    auto res = mgr_->searchNearby(kTable,
                                   0.0 /*x=lon*/, 0.0 /*y=lat*/,
                                   100'000.0 /*100 km*/);
    EXPECT_TRUE(res.empty());
}

// ===========================================================================
// searchKNN – k nearest neighbours
// ===========================================================================

TEST_F(SpatialCorrectnessTest, KNN_ReturnExactlyK) {
    // Insert 10 European cities
    const std::vector<std::pair<std::string, std::pair<double,double>>> cities = {
        {"berlin",    {13.4,   52.5}},
        {"paris",     { 2.35,  48.85}},
        {"madrid",    {-3.7,   40.4}},
        {"rome",      {12.5,   41.9}},
        {"vienna",    {16.37,  48.2}},
        {"amsterdam", { 4.9,   52.37}},
        {"warsaw",    {21.0,   52.23}},
        {"prague",    {14.42,  50.08}},
        {"budapest",  {19.04,  47.5}},
        {"athens",    {23.73,  37.98}},
    };
    for (const auto& c : cities)
        ASSERT_TRUE(mgr_->insert(kTable, c.first,
                                  pointSidecar(c.second.first, c.second.second)));

    int k = 5;
    auto res = mgr_->searchKNN(kTable, 13.4 /*lon=x*/, 52.5 /*lat=y*/, k);  // centred on Berlin
    EXPECT_EQ((int)res.size(), k);
}

TEST_F(SpatialCorrectnessTest, KNN_ClosestIsNearest) {
    ASSERT_TRUE(mgr_->insert(kTable, "close", pointSidecar(1.0, 1.0)));
    ASSERT_TRUE(mgr_->insert(kTable, "far",   pointSidecar(50.0, 50.0)));

    auto res = mgr_->searchKNN(kTable, 1.0 /*lon=x*/, 1.0 /*lat=y*/, 2);
    ASSERT_GE(res.size(), 1u);
    EXPECT_EQ(res[0].primary_key, "close");
}

// ===========================================================================
// searchZRange – 3-D elevation band
// ===========================================================================

TEST_F(SpatialCorrectnessTest, ZRange_FindsPointInBand) {
    // Insert points at different altitudes
    ASSERT_TRUE(mgr_->insert(kTable, "sea",     pointSidecar3D(10.0, 50.0,   0.0)));
    ASSERT_TRUE(mgr_->insert(kTable, "hill",    pointSidecar3D(10.0, 50.0, 300.0)));
    ASSERT_TRUE(mgr_->insert(kTable, "summit",  pointSidecar3D(10.0, 50.0, 800.0)));

    // Band 200–500 m → only "hill"
    auto res = mgr_->searchZRange(kTable, 200.0, 500.0);
    EXPECT_TRUE(hasKey(res, "hill"));
    EXPECT_FALSE(hasKey(res, "sea"));
    EXPECT_FALSE(hasKey(res, "summit"));
}

// ===========================================================================
// Precision gate: no false positives from a grid of 100 labelled points
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Precision_NoFalsePositives) {
    // Lay out a 10×10 grid of points: lon ∈ [0,9], lat ∈ [0,9]
    for (int lon = 0; lon < 10; ++lon) {
        for (int lat = 0; lat < 10; ++lat) {
            std::string pk = "p_" + std::to_string(lon) + "_" + std::to_string(lat);
            ASSERT_TRUE(mgr_->insert(kTable, pk, pointSidecar(lon, lat)));
        }
    }

    // Query bbox covering only lon [2,4], lat [3,5]
    auto res = mgr_->searchIntersects(kTable, MBR(2.0, 3.0, 4.0, 5.0));

    // Every returned entry MUST be inside (or on the border of) the query bbox.
    for (const auto& r : res) {
        EXPECT_GE(r.mbr.minx, 2.0) << "False positive: " << r.primary_key;
        EXPECT_LE(r.mbr.maxx, 4.0) << "False positive: " << r.primary_key;
        EXPECT_GE(r.mbr.miny, 3.0) << "False positive: " << r.primary_key;
        EXPECT_LE(r.mbr.maxy, 5.0) << "False positive: " << r.primary_key;
    }

    // Also verify expected points are present (completeness check)
    // Points with lon∈{2,3,4} and lat∈{3,4,5} = 9 points
    EXPECT_EQ(res.size(), 9u);
}

// ===========================================================================
// Recall gate: a wider query must find all expected entries
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Recall_AllPointsFoundInWorldQuery) {
    const std::vector<std::pair<std::string, std::pair<double,double>>> pts = {
        {"a", {-90.0, -45.0}},
        {"b", { 90.0,  45.0}},
        {"c", {  0.0,   0.0}},
        {"d", {179.0,  89.0}},
        {"e", {-179.0, -89.0}},
    };
    for (const auto& p : pts)
        ASSERT_TRUE(mgr_->insert(kTable, p.first,
                                  pointSidecar(p.second.first, p.second.second)));

    auto res = mgr_->searchIntersects(kTable, MBR(-180.0, -90.0, 180.0, 90.0));
    EXPECT_EQ(res.size(), pts.size());
    for (const auto& p : pts)
        EXPECT_TRUE(hasKey(res, p.first)) << "Missing: " << p.first;
}

// ===========================================================================
// Index management: hasSpatialIndex / createSpatialIndex / dropSpatialIndex
// ===========================================================================

TEST_F(SpatialCorrectnessTest, IndexManagement_CreateAndDropCycle) {
    const char* kTbl2 = "tmp_table";
    ASSERT_FALSE(mgr_->hasSpatialIndex(kTbl2));

    ASSERT_TRUE(mgr_->createSpatialIndex(kTbl2));
    EXPECT_TRUE(mgr_->hasSpatialIndex(kTbl2));

    ASSERT_TRUE(mgr_->dropSpatialIndex(kTbl2));
    EXPECT_FALSE(mgr_->hasSpatialIndex(kTbl2));
}

TEST_F(SpatialCorrectnessTest, IndexManagement_SearchOnDroppedIndexReturnsEmpty) {
    const char* kTbl3 = "drop_test";
    ASSERT_TRUE(mgr_->createSpatialIndex(kTbl3));
    ASSERT_TRUE(mgr_->insert(kTbl3, "p1", pointSidecar(5.0, 5.0)));

    ASSERT_TRUE(mgr_->dropSpatialIndex(kTbl3));

    // After drop the index is gone; search should return empty, not crash.
    auto res = mgr_->searchIntersects(kTbl3, MBR(-180.0, -90.0, 180.0, 90.0));
    EXPECT_TRUE(res.empty());
}

// ===========================================================================
// Metrics correctness
// ===========================================================================

TEST_F(SpatialCorrectnessTest, Metrics_InsertAndQueryCountsIncrement) {
    mgr_->resetMetrics();
    const auto& m = mgr_->getMetrics();

    ASSERT_TRUE(mgr_->insert(kTable, "p", pointSidecar(1.0, 1.0)));
    EXPECT_EQ(m.insert_count.load(), 1u);

    mgr_->searchIntersects(kTable, MBR(-10.0, -10.0, 10.0, 10.0));
    EXPECT_EQ(m.query_count.load(), 1u);
}
