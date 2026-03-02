/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rtree_cpu_integration.cpp                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:01:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     254                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6e8398a0c  2026-02-25  fix(geo): clear R-tree state in dropSpatialIndex and crea... ║
    • 4e4639844  2026-02-24  feat(geo): integrate R-tree index with CPU backend (Spati... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "index/spatial_index.h"
#include "geo/spatial_backend.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

using namespace themis;
using namespace themis::index;
using namespace themis::geo;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: creates a temporary RocksDB, sets up a SpatialIndexManager and
// wires the built-in CPU exact backend so that the R-tree integration path is
// exercised end-to-end.
// ─────────────────────────────────────────────────────────────────────────────
class RTreeCpuIntegrationTest : public ::testing::Test {
protected:
    static constexpr const char* kTable = "test_rtree_table";
    static constexpr const char* kDbPath = "test_rtree_cpu_integration_db";

    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path  = kDbPath;
        cfg.memtable_size_mb   = 16;
        cfg.block_cache_size_mb = 16;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        mgr_ = std::make_unique<SpatialIndexManager>(*db_);

        // Wire the CPU exact backend so Phase-2 exact checks can run.
        auto* cpu = getCpuExactBackend();
        ASSERT_NE(cpu, nullptr);
        mgr_->setExactBackend(cpu);

        // Create a spatial index with world bounds.
        RTreeConfig rtcfg;
        rtcfg.total_bounds = MBR(-180.0, -90.0, 180.0, 90.0);
        ASSERT_TRUE(mgr_->createSpatialIndex(kTable, "geometry", rtcfg));
    }

    void TearDown() override {
        mgr_.reset();
        db_.reset();
        std::filesystem::remove_all(kDbPath);
    }

    // Helper: build a point-sidecar.
    static GeoSidecar pointSidecar(double lon, double lat) {
        GeoSidecar sc;
        sc.mbr      = MBR(lon, lat, lon, lat);
        sc.centroid = Coordinate(lon, lat);
        return sc;
    }

    // Helper: build a box-sidecar.
    static GeoSidecar boxSidecar(double minx, double miny, double maxx, double maxy) {
        GeoSidecar sc;
        sc.mbr      = MBR(minx, miny, maxx, maxy);
        sc.centroid = Coordinate((minx + maxx) * 0.5, (miny + maxy) * 0.5);
        return sc;
    }

    static bool hasKey(const std::vector<SpatialResult>& v, const std::string& k) {
        return std::any_of(v.begin(), v.end(),
                           [&](const SpatialResult& r){ return r.primary_key == k; });
    }

    std::unique_ptr<RocksDBWrapper>       db_;
    std::unique_ptr<SpatialIndexManager>  mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic insert + search
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, InsertAndSearch_SinglePoint) {
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", pointSidecar(13.4, 52.5)));

    auto res = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].primary_key, "berlin");
}

TEST_F(RTreeCpuIntegrationTest, InsertAndSearch_MissesOutsideBBox) {
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", pointSidecar(13.4, 52.5)));

    // Query box does not cover Berlin.
    auto res = mgr_->searchIntersects(kTable, MBR(0.0, 0.0, 5.0, 5.0));
    EXPECT_TRUE(res.empty());
}

TEST_F(RTreeCpuIntegrationTest, InsertAndSearch_MultiplePoints) {
    ASSERT_TRUE(mgr_->insert(kTable, "berlin",  pointSidecar(13.4,  52.5)));
    ASSERT_TRUE(mgr_->insert(kTable, "paris",   pointSidecar( 2.35, 48.85)));
    ASSERT_TRUE(mgr_->insert(kTable, "hamburg", pointSidecar(10.0,  53.6)));

    // Germany-ish bbox
    auto res = mgr_->searchIntersects(kTable, MBR(6.0, 47.0, 15.0, 55.0));
    EXPECT_EQ(res.size(), 2u);   // Berlin + Hamburg
    EXPECT_TRUE(hasKey(res, "berlin"));
    EXPECT_TRUE(hasKey(res, "hamburg"));
    EXPECT_FALSE(hasKey(res, "paris"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Remove
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, Remove_EntryDisappearsFromResults) {
    auto sc = pointSidecar(13.4, 52.5);
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", sc));

    // Verify it's there.
    EXPECT_EQ(mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0)).size(), 1u);

    // Remove it.
    ASSERT_TRUE(mgr_->remove(kTable, "berlin", sc));

    // Should no longer appear.
    EXPECT_TRUE(mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0)).empty());
}

TEST_F(RTreeCpuIntegrationTest, Remove_OtherEntriesUnaffected) {
    auto sc_b = pointSidecar(13.4, 52.5);
    auto sc_p = pointSidecar(2.35, 48.85);
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", sc_b));
    ASSERT_TRUE(mgr_->insert(kTable, "paris",  sc_p));

    ASSERT_TRUE(mgr_->remove(kTable, "berlin", sc_b));

    // World query: only Paris remains.
    auto res = mgr_->searchIntersects(kTable, MBR(-180.0, -90.0, 180.0, 90.0));
    EXPECT_EQ(res.size(), 1u);
    EXPECT_TRUE(hasKey(res, "paris"));
}

// ─────────────────────────────────────────────────────────────────────────────
// searchContains (uses searchIntersects under the hood)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, SearchContains_PointInsideBox) {
    // Insert a box that covers central Germany.
    ASSERT_TRUE(mgr_->insert(kTable, "germany", boxSidecar(6.0, 47.0, 15.0, 55.0)));

    // Berlin (13.4, 52.5) is inside.
    auto res = mgr_->searchContains(kTable, 13.4, 52.5);
    EXPECT_EQ(res.size(), 1u);
    EXPECT_TRUE(hasKey(res, "germany"));
}

TEST_F(RTreeCpuIntegrationTest, SearchContains_PointOutside) {
    ASSERT_TRUE(mgr_->insert(kTable, "germany", boxSidecar(6.0, 47.0, 15.0, 55.0)));

    // Tokyo (139.7, 35.7) is outside.
    auto res = mgr_->searchContains(kTable, 139.7, 35.7);
    EXPECT_TRUE(res.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Lazy rebuild from RocksDB per-PK keys (simulates data surviving a restart)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, LazyRebuild_AfterReopenWithExistingData) {
    // Insert data via the first manager instance.
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", pointSidecar(13.4, 52.5)));
    ASSERT_TRUE(mgr_->insert(kTable, "paris",  pointSidecar(2.35, 48.85)));

    // Discard the manager (simulates process restart / R-tree lost from memory).
    mgr_.reset();

    // Re-open with a fresh manager — R-tree is initially empty.
    mgr_ = std::make_unique<SpatialIndexManager>(*db_);
    mgr_->setExactBackend(getCpuExactBackend());

    // First query triggers lazy rebuild from per-PK RocksDB keys.
    auto res = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    EXPECT_EQ(res.size(), 1u);
    EXPECT_TRUE(hasKey(res, "berlin"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Large-dataset: verify R-tree path handles many inserts correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, LargeDataset_HundredPoints) {
    // Insert 100 points along a longitude grid; all at lat 0.
    for (int i = 0; i < 100; ++i) {
        double lon = static_cast<double>(i) - 50.0;   // -50 .. 49
        mgr_->insert(kTable, "pt_" + std::to_string(i), pointSidecar(lon, 0.0));
    }

    // Query a small slice: lon ∈ [-10, 10].
    auto res = mgr_->searchIntersects(kTable, MBR(-10.0, -1.0, 10.0, 1.0));

    // Expect 21 points: -10, -9, ..., 0, ..., 10.
    EXPECT_EQ(res.size(), 21u);
}

// ─────────────────────────────────────────────────────────────────────────────
// insertBatch / removeBatch path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, InsertBatch_RTreeUpdated) {
    auto wb = db_->createWriteBatch();
    auto sc = pointSidecar(13.4, 52.5);
    ASSERT_TRUE(mgr_->insertBatch(*wb, kTable, "berlin_batch", sc));
    ASSERT_TRUE(wb->commit());

    auto res = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    EXPECT_EQ(res.size(), 1u);
    EXPECT_TRUE(hasKey(res, "berlin_batch"));
}

TEST_F(RTreeCpuIntegrationTest, RemoveBatch_RTreeUpdated) {
    auto sc = pointSidecar(13.4, 52.5);
    // First insert.
    ASSERT_TRUE(mgr_->insert(kTable, "berlin_rb", sc));
    EXPECT_EQ(mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0)).size(), 1u);

    // Remove via batch.
    auto wb = db_->createWriteBatch();
    ASSERT_TRUE(mgr_->removeBatch(*wb, kTable, "berlin_rb", sc));
    ASSERT_TRUE(wb->commit());

    auto res = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    EXPECT_TRUE(res.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Drop + recreate: stale R-tree must not leak into the new index
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RTreeCpuIntegrationTest, DropAndRecreate_StaleRTreeCleared) {
    // Insert data into the original index.
    ASSERT_TRUE(mgr_->insert(kTable, "berlin", pointSidecar(13.4, 52.5)));
    EXPECT_EQ(mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0)).size(), 1u);

    // Drop the index — must clear in-memory R-tree.
    ASSERT_TRUE(mgr_->dropSpatialIndex(kTable));

    // Recreate the index (empty) and query without inserting any new data.
    RTreeConfig rtcfg;
    rtcfg.total_bounds = MBR(-180.0, -90.0, 180.0, 90.0);
    ASSERT_TRUE(mgr_->createSpatialIndex(kTable, "geometry", rtcfg));

    // The new (empty) index must return no results.
    auto res = mgr_->searchIntersects(kTable, MBR(12.0, 51.0, 15.0, 54.0));
    EXPECT_TRUE(res.empty()) << "Stale R-tree entries leaked after drop+recreate";
}
