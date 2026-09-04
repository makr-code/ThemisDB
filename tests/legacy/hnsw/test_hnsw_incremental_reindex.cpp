/**
 * @file test_hnsw_incremental_reindex.cpp
 * @brief Unit tests for VectorIndexManager::incrementalReindex()
 *
 * Tests exercise the VectorIndexManager-level incremental re-index feature
 * using a real (temporary) RocksDB instance.  Seven test scenarios are covered:
 *   1. Unchanged vectors – no action taken (preserved HNSW graph)
 *   2. New vectors in storage – added to in-memory index
 *   3. Vectors deleted from storage – marked as deleted in HNSW
 *   4. Vectors changed in storage – updated in-place in HNSW
 *   5. Mixed scenario – all four categories in one call
 *   6. Idempotency – calling twice in a row has no extra effect
 *   7. Uninitialised manager – returns error without crashing
 */

#include <gtest/gtest.h>
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include <chrono>
#include <filesystem>
#include <memory>

namespace themis {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string makeTempPath(const std::string& tag) {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("themis_inc_reindex_" + tag + "_" + std::to_string(ns))).string();
}

static BaseEntity makeEntity(const std::string& pk, const std::vector<float>& emb) {
    BaseEntity e(pk);
    e.setField("embedding", emb);
    return e;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
struct IncrementalReindexFixture : ::testing::Test {
    static constexpr int kDim = 8;

    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper>    db;
    std::unique_ptr<VectorIndexManager> vim;

    void SetUp() override {
        db_path = makeTempPath("test");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed at " << db_path;
        vim = std::make_unique<VectorIndexManager>(*db);
        auto s = vim->init("items", kDim,
                           VectorIndexManager::Metric::L2,
                           /*M=*/4, /*efC=*/50, /*efS=*/16);
        ASSERT_TRUE(s.ok) << s.message;
    }

    void TearDown() override {
        vim.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    // Convenience: write an entity directly into RocksDB using the canonical
    // key format that addEntity() uses: KeySchema::makeVectorKey(objectName, pk)
    // = "vec:<objectName>:<pk>", matching the scan prefix in incrementalReindex().
    void storeDirect(const BaseEntity& e) {
        std::string key = KeySchema::makeVectorKey("items", e.getPrimaryKey());
        ASSERT_TRUE(db->put(key, e.serialize()));
    }

    // Convenience: delete an entity directly from RocksDB (canonical key format).
    void deleteDirect(const std::string& pk) {
        ASSERT_TRUE(db->del(KeySchema::makeVectorKey("items", pk)));
    }
};

// ---------------------------------------------------------------------------
// Test 1: Uninitialised manager → returns error, does not crash
// ---------------------------------------------------------------------------
TEST(IncrementalReindexBasic, UninitManagerReturnsError) {
    std::string path = makeTempPath("uninit");
    RocksDBWrapper::Config cfg;
    cfg.db_path       = path;
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    VectorIndexManager vim(db);  // NOT initialised via init()
    auto [status, stats] = vim.incrementalReindex();
    EXPECT_FALSE(status.ok);
    EXPECT_FALSE(status.message.empty());

    db.close();
    std::filesystem::remove_all(path);
}

// ---------------------------------------------------------------------------
// Test 2: Empty index + empty storage → all-zero stats
// ---------------------------------------------------------------------------
TEST_F(IncrementalReindexFixture, EmptyIndexEmptyStorage) {
    auto [s, stats] = vim->incrementalReindex();
    ASSERT_TRUE(s.ok) << s.message;
    EXPECT_EQ(stats.added,         0u);
    EXPECT_EQ(stats.removed,       0u);
    EXPECT_EQ(stats.updated,       0u);
    EXPECT_EQ(stats.unchanged,     0u);
    EXPECT_EQ(stats.total_scanned, 0u);
    EXPECT_FALSE(stats.full_rebuild_triggered);
}

// ---------------------------------------------------------------------------
// Test 3: All vectors already synced → all unchanged
// ---------------------------------------------------------------------------
TEST_F(IncrementalReindexFixture, AlreadySyncedAllUnchanged) {
    ASSERT_TRUE(vim->addEntity(makeEntity("pk1", {1,0,0,0,0,0,0,0})).ok);
    ASSERT_TRUE(vim->addEntity(makeEntity("pk2", {0,1,0,0,0,0,0,0})).ok);

    auto [s, stats] = vim->incrementalReindex();
    ASSERT_TRUE(s.ok) << s.message;
    EXPECT_EQ(stats.added,         0u);
    EXPECT_EQ(stats.removed,       0u);
    EXPECT_EQ(stats.updated,       0u);
    EXPECT_EQ(stats.unchanged,     2u);
    EXPECT_EQ(stats.total_scanned, 2u);
}

// ---------------------------------------------------------------------------
// Test 4: Vector deleted from storage → marked deleted in index
// ---------------------------------------------------------------------------
TEST_F(IncrementalReindexFixture, RemovesVectorDeletedFromStorage) {
    ASSERT_TRUE(vim->addEntity(makeEntity("pk_del", {1,2,3,4,5,6,7,8})).ok);
    EXPECT_EQ(vim->getVectorCount(), 1u);

    // Delete from storage but leave in-memory state untouched
    deleteDirect("pk_del");

    auto [s, stats] = vim->incrementalReindex();
    ASSERT_TRUE(s.ok) << s.message;
    EXPECT_EQ(stats.removed,       1u);
    EXPECT_EQ(stats.added,         0u);
    EXPECT_EQ(stats.total_scanned, 0u);
    // Cache entry must be gone after reindex
    EXPECT_EQ(vim->getVectorCount(), 0u);
}

// ---------------------------------------------------------------------------
// Test 5: New vector in storage (not yet in index) → added to index
// ---------------------------------------------------------------------------
TEST_F(IncrementalReindexFixture, AddsVectorNewInStorage) {
    // Write directly to RocksDB, bypassing VectorIndexManager
    storeDirect(makeEntity("pk_new", {0,0,1,0,0,0,0,0}));
    EXPECT_EQ(vim->getVectorCount(), 0u);

    auto [s, stats] = vim->incrementalReindex();
    ASSERT_TRUE(s.ok) << s.message;
    EXPECT_EQ(stats.added,         1u);
    EXPECT_EQ(stats.removed,       0u);
    EXPECT_EQ(stats.total_scanned, 1u);
    EXPECT_EQ(vim->getVectorCount(), 1u);
}

// ---------------------------------------------------------------------------
// Test 6: Mixed scenario – add + remove + update + unchanged in one call
// ---------------------------------------------------------------------------
TEST_F(IncrementalReindexFixture, MixedDiffCountersCorrect) {
    // Two entities in both index and storage
    ASSERT_TRUE(vim->addEntity(makeEntity("del_me",  {1,0,0,0,0,0,0,0})).ok);
    ASSERT_TRUE(vim->addEntity(makeEntity("change",  {0,1,0,0,0,0,0,0})).ok);
    ASSERT_TRUE(vim->addEntity(makeEntity("stable",  {0,0,0,0,0,0,0,1})).ok);

    // Simulate out-of-band storage changes:
    //  a) delete "del_me" from storage
    deleteDirect("del_me");
    //  b) overwrite "change" with a different vector
    storeDirect(makeEntity("change", {0,0,0,1,0,0,0,0}));
    //  c) add a brand-new entry to storage
    storeDirect(makeEntity("fresh", {0,0,1,0,0,0,0,0}));

    auto [s, stats] = vim->incrementalReindex();
    ASSERT_TRUE(s.ok) << s.message;

    EXPECT_EQ(stats.removed,       1u) << "del_me deleted from storage";
    EXPECT_EQ(stats.updated,       1u) << "change vector data changed";
    EXPECT_EQ(stats.added,         1u) << "fresh new in storage";
    EXPECT_EQ(stats.unchanged,     1u) << "stable identical in both";
    EXPECT_EQ(stats.total_scanned, 3u) << "change, fresh, stable scanned";
}

// ---------------------------------------------------------------------------
// Test 7: Calling incrementalReindex twice in a row is idempotent
// ---------------------------------------------------------------------------
TEST_F(IncrementalReindexFixture, IdempotentOnSecondCall) {
    ASSERT_TRUE(vim->addEntity(makeEntity("idem", {1,1,0,0,0,0,0,0})).ok);

    auto [s1, st1] = vim->incrementalReindex();
    ASSERT_TRUE(s1.ok);
    EXPECT_EQ(st1.added + st1.removed + st1.updated, 0u);
    EXPECT_EQ(st1.unchanged, 1u);

    auto [s2, st2] = vim->incrementalReindex();
    ASSERT_TRUE(s2.ok);
    EXPECT_EQ(st2.added + st2.removed + st2.updated, 0u);
    EXPECT_EQ(st2.unchanged, 1u);
}

} // namespace
} // namespace themis

// ---------------------------------------------------------------------------
// IndexMaintenanceManager integration tests
// ---------------------------------------------------------------------------
#include "storage/index_maintenance.h"

namespace themis {
namespace {

struct MaintenanceFixture : ::testing::Test {
    static constexpr int kDim = 8;

    std::string db_path = {};
    std::shared_ptr<RocksDBWrapper>          db;   // shared for IndexMaintenanceManager
    std::shared_ptr<VectorIndexManager>      vim;
    std::unique_ptr<IndexMaintenanceManager> maint;

    void SetUp() override {
        db_path = makeTempPath("maint");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());

        vim = std::make_shared<VectorIndexManager>(*db);
        auto s = vim->init("things", kDim, VectorIndexManager::Metric::L2,
                           /*M=*/4, /*efC=*/50, /*efS=*/16);
        ASSERT_TRUE(s.ok) << s.message;

        maint = std::make_unique<IndexMaintenanceManager>(db);
        maint->setVectorIndexManager(vim);
    }

    void TearDown() override {
        maint.reset();
        vim.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }
};

// Test 8: vectorIncrementalReindex without VIM set → error
TEST(MaintenanceVectorReindex, WithoutVIM_ReturnsError) {
    std::string path = makeTempPath("novim");
    RocksDBWrapper::Config cfg;
    cfg.db_path       = path;
    cfg.enable_blobdb = false;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());

    IndexMaintenanceManager maint(db);
    auto result = maint.vectorIncrementalReindex();
    EXPECT_FALSE(result.has_value());

    db->close();
    std::filesystem::remove_all(path);
}

// Test 9: vectorIncrementalReindex with VIM set → OK job returned
TEST_F(MaintenanceFixture, WithVIM_ReturnsJobStatus) {
    ASSERT_TRUE(vim->addEntity(makeEntity("m1", {1,0,0,0,0,0,0,0})).ok);

    auto result = maint->vectorIncrementalReindex();
    ASSERT_TRUE(result.has_value()) << "Expected successful job";
    const auto& job = *result;

    EXPECT_FALSE(job.job_id.empty());
    EXPECT_TRUE(job.is_completed);
    EXPECT_FALSE(job.is_failed);
    EXPECT_EQ(job.operation, MaintenanceOperation::VECTOR_INCREMENTAL_REINDEX);
    EXPECT_GE(job.end_time_ms, job.start_time_ms);
    EXPECT_EQ(job.duration_ms, job.end_time_ms - job.start_time_ms);
}

// Test 10: vectorIncrementalReindex stats flow through job message
TEST_F(MaintenanceFixture, StatsFlowThroughJobMessage) {
    // Add a vector via addEntity (stored at "vec:things:gone"),
    // then delete that canonical key so incrementalReindex sees it in cache
    // but not in the storage scan → counted as "removed".
    ASSERT_TRUE(vim->addEntity(makeEntity("gone", {1,0,0,0,0,0,0,0})).ok);
    db->del(KeySchema::makeVectorKey("things", "gone")); // canonical key format

    auto result = maint->vectorIncrementalReindex(0.0f); // disable auto full-rebuild
    ASSERT_TRUE(result.has_value());
    const auto& job = *result;

    // The result_summary field carries the stats summary
    EXPECT_NE(job.result_summary.find("removed=1"), std::string::npos);
}

} // namespace
} // namespace themis
