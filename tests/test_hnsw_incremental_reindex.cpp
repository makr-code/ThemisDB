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

    std::string db_path;
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

    // Convenience: write an entity directly into RocksDB under the "items:" prefix.
    void storeDirect(const BaseEntity& e) {
        std::string key = "items:" + e.getPrimaryKey();
        ASSERT_TRUE(db->put(key, e.serialize()));
    }

    // Convenience: delete a key directly from RocksDB.
    void deleteDirect(const std::string& pk) {
        ASSERT_TRUE(db->del("items:" + pk));
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
