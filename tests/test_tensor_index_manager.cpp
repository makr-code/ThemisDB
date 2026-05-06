/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_tensor_index_manager.cpp                      ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_tensor_index_manager.cpp
 * @brief Unit tests for TensorIndexManager and TensorRouter::decide().
 *
 * Acceptance criteria:
 *
 * TensorRouter::decide (static) — TIM-03..TIM-05
 *   TIM-03  Large dim (≥4096) with high κ → TENSOR_TRAIN
 *   TIM-04  Medium dim (1024) with medium κ → HYBRID
 *   TIM-05  Low κ (<1.3) → HNSW regardless of dim
 *
 * TensorIndexManager lifecycle — TIM-06..TIM-10
 *   TIM-06  createIndex returns non-null pointer
 *   TIM-07  createIndex twice for same key returns same pointer (idempotent)
 *   TIM-08  getIndex returns nullptr for non-existent key
 *   TIM-09  dropIndex removes the index (getIndex returns nullptr after drop)
 *   TIM-10  dropTenantIndexes removes all indexes for that tenant
 *
 * TensorRouter::routeFor heuristics — TIM-11..TIM-13
 *   TIM-11  routeFor with dim ≥ 4096 returns TENSOR_TRAIN
 *   TIM-12  routeFor with dim 1024..4095 returns HYBRID
 *   TIM-13  routeFor with dim < 256 returns HNSW
 *
 * listIndexes — TIM-14
 *   TIM-14  listIndexes(tenant_id) returns only that tenant's indexes
 *
 * File-based persistence (setDataDir/flushAll) — TIM-15..TIM-18
 *   TIM-15  flushAll() writes a .ttidx file to the configured data dir
 *   TIM-16  createIndex() restores data from disk when data dir is set
 *   TIM-17  dropIndex() deletes the corresponding .ttidx file
 *   TIM-18  flushAll() on empty manager returns 0
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "tensor/tensor_index_manager.h"
#include "storage/tensor_router.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace themis::tensor;
using namespace themis::storage;

// ============================================================================
// TIM-03..TIM-05 — TensorRouter::decide (static heuristic)
// ============================================================================

TEST(TensorRouterDecide, TIM03_LargeDimHighKappaYieldsTensorTrain) {
    TensorRouter::DataProfile p;
    p.dim            = 4096;
    p.num_vectors    = 1000000;
    p.kappa_estimate = 4.5;   // >> 1.7 threshold

    EXPECT_EQ(TensorRouter::decide(p), TensorRouter::Route::TENSOR_TRAIN);
}

TEST(TensorRouterDecide, TIM04_MediumDimMediumKappaYieldsHybrid) {
    TensorRouter::DataProfile p;
    p.dim            = 1024;
    p.num_vectors    = 50000;
    p.kappa_estimate = 2.8;   // ≥ 1.3 but < 1.7 → HYBRID

    EXPECT_EQ(TensorRouter::decide(p), TensorRouter::Route::HYBRID);
}

TEST(TensorRouterDecide, TIM05_LowKappaYieldsHNSW) {
    TensorRouter::DataProfile p;
    p.dim            = 768;
    p.num_vectors    = 10000;
    p.kappa_estimate = 1.2;   // < 1.3 → HNSW

    EXPECT_EQ(TensorRouter::decide(p), TensorRouter::Route::HNSW);
}

TEST(TensorRouterDecide, TIM05b_HighKappaButSmallDimYieldsHybridNotTensorTrain) {
    // κ ≥ 1.7 but dim < 256 → HYBRID (TT overhead not justified for tiny vecs)
    TensorRouter::DataProfile p;
    p.dim            = 64;
    p.num_vectors    = 10000;
    p.kappa_estimate = 2.0;

    EXPECT_EQ(TensorRouter::decide(p), TensorRouter::Route::HYBRID);
}

TEST(TensorRouterDecide, TIM05c_ZeroKappaAlwaysHNSW) {
    TensorRouter::DataProfile p;
    p.dim            = 8192;
    p.num_vectors    = 1000;
    p.kappa_estimate = 0.0;

    EXPECT_EQ(TensorRouter::decide(p), TensorRouter::Route::HNSW);
}

// ============================================================================
// TIM-06..TIM-10 — TensorIndexManager lifecycle
// ============================================================================

TEST(TensorIndexManager, TIM06_CreateIndexReturnsNonNull) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("tenant1", "docs", "embedding");
    ASSERT_NE(idx, nullptr);
}

TEST(TensorIndexManager, TIM07_CreateIndexIdempotent) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx1 = mgr->createIndex("tenant1", "docs", "embedding");
    auto* idx2 = mgr->createIndex("tenant1", "docs", "embedding");
    EXPECT_EQ(idx1, idx2);
}

TEST(TensorIndexManager, TIM08_GetIndexReturnsNullForMissing) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->getIndex("no_such_tenant", "no_collection", "no_field");
    EXPECT_EQ(idx, nullptr);
}

TEST(TensorIndexManager, TIM09_DropIndexRemovesFromRegistry) {
    auto mgr = TensorIndexManager::create(nullptr);
    mgr->createIndex("t1", "c1", "f1");
    ASSERT_NE(mgr->getIndex("t1", "c1", "f1"), nullptr);

    bool dropped = mgr->dropIndex("t1", "c1", "f1");
    EXPECT_TRUE(dropped);
    EXPECT_EQ(mgr->getIndex("t1", "c1", "f1"), nullptr);
}

TEST(TensorIndexManager, TIM09b_DropNonExistentReturnsFalse) {
    auto mgr = TensorIndexManager::create(nullptr);
    EXPECT_FALSE(mgr->dropIndex("ghost", "g", "g"));
}

TEST(TensorIndexManager, TIM10_DropTenantIndexesRemovesAll) {
    auto mgr = TensorIndexManager::create(nullptr);
    mgr->createIndex("tenantX", "c1", "f1");
    mgr->createIndex("tenantX", "c2", "f2");
    mgr->createIndex("tenantY", "c1", "f1");

    mgr->dropTenantIndexes("tenantX");

    EXPECT_EQ(mgr->getIndex("tenantX", "c1", "f1"), nullptr);
    EXPECT_EQ(mgr->getIndex("tenantX", "c2", "f2"), nullptr);
    // Other tenant must be unaffected
    EXPECT_NE(mgr->getIndex("tenantY", "c1", "f1"), nullptr);
}

// ============================================================================
// TIM-11..TIM-13 — TensorIndexManager::routeFor heuristics
// ============================================================================

TEST(TensorIndexManager, TIM11_RouteForLargeDimYieldsTensorTrain) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto route = mgr->routeFor("t", "c", "f", 4096, 1000000);
    EXPECT_EQ(route, TensorRouter::Route::TENSOR_TRAIN);
}

TEST(TensorIndexManager, TIM12_RouteForMediumDimYieldsHybrid) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto route = mgr->routeFor("t", "c", "f", 1024, 50000);
    EXPECT_EQ(route, TensorRouter::Route::HYBRID);
}

TEST(TensorIndexManager, TIM13_RouteForSmallDimYieldsHNSW) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto route = mgr->routeFor("t", "c", "f", 128, 1000);
    EXPECT_EQ(route, TensorRouter::Route::HNSW);
}

// ============================================================================
// TIM-14 — listIndexes(tenant_id) isolation
// ============================================================================

TEST(TensorIndexManager, TIM14_ListIndexesFiltersbyTenant) {
    auto mgr = TensorIndexManager::create(nullptr);
    mgr->createIndex("alpha", "c1", "f1");
    mgr->createIndex("alpha", "c2", "f2");
    mgr->createIndex("beta",  "c1", "f1");

    auto alpha_indexes = mgr->listIndexes("alpha");
    EXPECT_EQ(alpha_indexes.size(), 2u);
    for (const auto& h : alpha_indexes) {
        EXPECT_EQ(h.tenant_id, "alpha");
    }

    auto beta_indexes = mgr->listIndexes("beta");
    EXPECT_EQ(beta_indexes.size(), 1u);

    auto all_indexes = mgr->listIndexes();
    EXPECT_EQ(all_indexes.size(), 3u);
}

// ============================================================================
// TFI-01..TFI-06 — FlatTensorIndex binary persistence (save / load)
//
//   TFI-01  save() on empty index succeeds and creates a file
//   TFI-02  load() of empty-index file restores size 0
//   TFI-03  save() of index with vectors succeeds
//   TFI-04  load() after save() restores all vectors (size matches)
//   TFI-05  loaded index is searchable (search returns non-empty results)
//   TFI-06  load() with a non-existent path returns false
// ============================================================================

namespace {
// Build a trivial flat vector for testing
std::vector<float> makeVec(size_t dim, float seed) {
    std::vector<float> v(dim);
    for (size_t i = 0; i < dim; ++i) v[i] = seed + static_cast<float>(i) * 0.1f;
    return v;
}

std::string tmpPath(const char* suffix) {
    return std::string("/tmp/themis_tfi_test_") + suffix + ".bin";
}
} // namespace

TEST(FlatTensorIndexPersistence, TFI01_SaveEmptyIndexSucceeds) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    ASSERT_NE(idx, nullptr);

    const std::string path = tmpPath("empty");
    std::remove(path.c_str());
    EXPECT_TRUE(idx->save(path));

    // File should now exist
    std::ifstream chk(path, std::ios::binary);
    EXPECT_TRUE(chk.good());
}

TEST(FlatTensorIndexPersistence, TFI02_LoadEmptyIndexRestoresSizeZero) {
    // Save an empty index first
    auto mgr1 = TensorIndexManager::create(nullptr);
    auto* idx1 = mgr1->createIndex("t", "c", "f");
    const std::string path = tmpPath("empty2");
    std::remove(path.c_str());
    ASSERT_TRUE(idx1->save(path));

    // Load into a fresh index
    auto mgr2 = TensorIndexManager::create(nullptr);
    auto* idx2 = mgr2->createIndex("t", "c", "f");
    EXPECT_TRUE(idx2->load(path));
    EXPECT_EQ(idx2->size(), 0u);
}

TEST(FlatTensorIndexPersistence, TFI03_SaveNonEmptyIndexSucceeds) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    ASSERT_NE(idx, nullptr);

    auto v1 = makeVec(8, 1.0f);
    auto v2 = makeVec(8, 2.0f);
    ASSERT_TRUE(idx->addFlat(1, v1.data(), v1.size()));
    ASSERT_TRUE(idx->addFlat(2, v2.data(), v2.size()));

    const std::string path = tmpPath("nonempty");
    std::remove(path.c_str());
    EXPECT_TRUE(idx->save(path));
}

TEST(FlatTensorIndexPersistence, TFI04_LoadAfterSaveRestoresAllVectors) {
    const std::string path = tmpPath("roundtrip");
    std::remove(path.c_str());

    auto v1 = makeVec(8, 1.0f);
    auto v2 = makeVec(8, 2.0f);
    auto v3 = makeVec(8, 3.0f);

    {
        auto mgr = TensorIndexManager::create(nullptr);
        auto* idx = mgr->createIndex("t", "c", "f");
        ASSERT_TRUE(idx->addFlat(10, v1.data(), v1.size()));
        ASSERT_TRUE(idx->addFlat(20, v2.data(), v2.size()));
        ASSERT_TRUE(idx->addFlat(30, v3.data(), v3.size()));
        ASSERT_EQ(idx->size(), 3u);
        ASSERT_TRUE(idx->save(path));
    }

    auto mgr2 = TensorIndexManager::create(nullptr);
    auto* idx2 = mgr2->createIndex("t", "c", "f");
    EXPECT_TRUE(idx2->load(path));
    EXPECT_EQ(idx2->size(), 3u);
}

TEST(FlatTensorIndexPersistence, TFI05_LoadedIndexIsSearchable) {
    const std::string path = tmpPath("search");
    std::remove(path.c_str());

    auto v1 = makeVec(8, 0.0f);
    auto v2 = makeVec(8, 5.0f);

    {
        auto mgr = TensorIndexManager::create(nullptr);
        auto* idx = mgr->createIndex("t", "c", "f");
        ASSERT_TRUE(idx->addFlat(1, v1.data(), v1.size()));
        ASSERT_TRUE(idx->addFlat(2, v2.data(), v2.size()));
        ASSERT_TRUE(idx->save(path));
    }

    auto mgr2 = TensorIndexManager::create(nullptr);
    auto* idx2 = mgr2->createIndex("t", "c", "f");
    ASSERT_TRUE(idx2->load(path));

    auto query = makeVec(8, 0.1f);
    auto results = idx2->searchFlat(query.data(), query.size(), 2);
    EXPECT_FALSE(results.empty());
    EXPECT_LE(results.size(), 2u);
}

TEST(FlatTensorIndexPersistence, TFI06_LoadNonExistentPathReturnsFalse) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    EXPECT_FALSE(idx->load("/tmp/this_file_does_not_exist_themis_tfi.bin"));
}

// ============================================================================
// TIM-15..TIM-18 — TensorIndexManager file-based persistence (setDataDir/flushAll)
// ============================================================================

TEST(TensorIndexManagerPersistence, TIM15_FlushAllWritesFile) {
    const std::string dir = std::filesystem::temp_directory_path().string() + "/themis_tim15_" + std::to_string(std::hash<std::string>{}("TIM15"));
    std::filesystem::create_directories(dir);

    auto mgr = TensorIndexManager::create(nullptr);
    mgr->setDataDir(dir);

    auto* idx = mgr->createIndex("tenant1", "coll", "field");
    auto v = makeVec(8, 1.0f);
    ASSERT_TRUE(idx->addFlat(100, v.data(), v.size()));

    size_t saved = mgr->flushAll();
    EXPECT_EQ(saved, 1u);

    // At least one .ttidx file must exist in the directory
    bool found = false;
    for (auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".ttidx") { found = true; break; }
    }
    EXPECT_TRUE(found);
    std::filesystem::remove_all(dir);
}

TEST(TensorIndexManagerPersistence, TIM16_CreateIndexLoadsExistingFile) {
    const std::string dir = std::filesystem::temp_directory_path().string() + "/themis_tim16_" + std::to_string(std::hash<std::string>{}("TIM16"));
    std::filesystem::create_directories(dir);

    auto v = makeVec(8, 2.0f);

    // First manager: insert data and flush
    {
        auto mgr = TensorIndexManager::create(nullptr);
        mgr->setDataDir(dir);
        auto* idx = mgr->createIndex("t", "c", "f");
        ASSERT_TRUE(idx->addFlat(42, v.data(), v.size()));
        EXPECT_EQ(mgr->flushAll(), 1u);
    }

    // Second manager: createIndex should restore from disk
    {
        auto mgr2 = TensorIndexManager::create(nullptr);
        mgr2->setDataDir(dir);
        auto* idx2 = mgr2->createIndex("t", "c", "f");
        ASSERT_NE(idx2, nullptr);

        auto result = idx2->searchFlat(v.data(), v.size(), 1);
        EXPECT_FALSE(result.empty());
        if (!result.empty()) {
            EXPECT_EQ(result[0].id, 42);
        }
    }

    std::filesystem::remove_all(dir);
}

TEST(TensorIndexManagerPersistence, TIM17_DropIndexDeletesFile) {
    const std::string dir = std::filesystem::temp_directory_path().string() + "/themis_tim17_" + std::to_string(std::hash<std::string>{}("TIM17"));
    std::filesystem::create_directories(dir);

    auto mgr = TensorIndexManager::create(nullptr);
    mgr->setDataDir(dir);

    auto* idx = mgr->createIndex("t", "c", "f");
    auto v = makeVec(8, 3.0f);
    ASSERT_TRUE(idx->addFlat(7, v.data(), v.size()));
    EXPECT_EQ(mgr->flushAll(), 1u);

    // Verify file exists, then drop and verify it is gone
    size_t file_count_before = 0;
    for (auto& e : std::filesystem::directory_iterator(dir)) {
        if (e.path().extension() == ".ttidx") ++file_count_before;
    }
    EXPECT_EQ(file_count_before, 1u);

    EXPECT_TRUE(mgr->dropIndex("t", "c", "f"));

    size_t file_count_after = 0;
    for (auto& e : std::filesystem::directory_iterator(dir)) {
        if (e.path().extension() == ".ttidx") ++file_count_after;
    }
    EXPECT_EQ(file_count_after, 0u);

    std::filesystem::remove_all(dir);
}

TEST(TensorIndexManagerPersistence, TIM18_FlushAllEmptyManagerReturnsZero) {
    auto mgr = TensorIndexManager::create(nullptr);
    mgr->setDataDir("/tmp");
    EXPECT_EQ(mgr->flushAll(), 0u);
}

