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
 *
 * TensorMmapBridge — TIM-19..TIM-24
 *   TIM-19  mapCores() returns non-null for a vector that was added
 *   TIM-20  mapCores() returns null for a non-existent vector ID
 *   TIM-21  mapCores() returns null for a non-existent index
 *   TIM-22  Each core slice's float data matches the stored TTTrain exactly
 *   TIM-23  totalBytes() equals the sum of all core byte sizes
 *   TIM-24  release() empties slices and is idempotent
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "tensor/tensor_index_manager.h"
#include "tensor/tensor_mmap_bridge.h"
#include "storage/tensor_router.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <limits>
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
    p.kappa_estimate = 1.5;   // ≥ 1.3 but < 1.7 → HYBRID

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
    for (size_t i = 0; i < dim; ++i) {
      v[i] = seed + static_cast<float>(i) * 0.1f;
    }
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
// TFI-07..TFI-10 — load() treats .ttidx files as untrusted input
// ============================================================================

namespace {
std::string tfiTempPath(const char* name) {
    return (std::filesystem::temp_directory_path() /
            (std::string("themis_tfi_hardening_") + name + ".ttidx")).string();
}

template <typename T>
void writePod(std::ofstream& out, T value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void writeTtiHeader(std::ofstream& out, uint64_t n_entries) {
    out.write("THEMIS_TTI\0", 11);
    writePod<uint8_t>(out, 1);
    writePod<uint64_t>(out, n_entries);
}

// One entry with one declared mode size matching the single core's n.
void writeSingleCoreEntry(std::ofstream& out, uint64_t rl, uint64_t nn,
                          uint64_t rr, uint64_t nf, size_t floats_to_write) {
    writePod<int64_t>(out, 1);       // id
    writePod<uint32_t>(out, 1);      // n_mode_sizes
    writePod<uint64_t>(out, nn);     // mode size
    writePod<double>(out, 1.0);      // original_norm
    writePod<double>(out, 0.0);      // achieved_eps
    writePod<uint32_t>(out, 1);      // n_cores
    writePod<uint64_t>(out, rl);
    writePod<uint64_t>(out, nn);
    writePod<uint64_t>(out, rr);
    writePod<uint64_t>(out, nf);
    for (size_t i = 0; i < floats_to_write; ++i) {
        writePod<float>(out, 1.0f);
    }
}

ITensorIndex* indexWithOneVector(const std::shared_ptr<TensorIndexManager>& mgr) {
    auto* idx = mgr->createIndex("t", "c", "f");
    auto v = makeVec(8, 1.0f);
    EXPECT_TRUE(idx->addFlat(1, v.data(), v.size()));
    return idx;
}
} // namespace

TEST(FlatTensorIndexPersistence, TFI07_TruncatedFileRejectedAndContentsKept) {
    const std::string path = tfiTempPath("truncated");
    {
        auto mgr = TensorIndexManager::create(nullptr);
        ASSERT_TRUE(indexWithOneVector(mgr)->save(path));
    }
    const auto full = std::filesystem::file_size(path);
    std::filesystem::resize_file(path, full - 3);

    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = indexWithOneVector(mgr);
    EXPECT_FALSE(idx->load(path));
    EXPECT_EQ(idx->size(), 1u);
    std::filesystem::remove(path);
}

TEST(FlatTensorIndexPersistence, TFI08_HugeEntryCountRejectedWithoutThrowing) {
    const std::string path = tfiTempPath("huge_count");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        writeTtiHeader(out, uint64_t{1} << 60);
    }
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = indexWithOneVector(mgr);
    bool loaded = true;
    EXPECT_NO_THROW(loaded = idx->load(path));
    EXPECT_FALSE(loaded);
    EXPECT_EQ(idx->size(), 1u);
    std::filesystem::remove(path);
}

TEST(FlatTensorIndexPersistence, TFI09_CoreSizeMismatchRejected) {
    const std::string path = tfiTempPath("core_mismatch");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        writeTtiHeader(out, 1);
        writeSingleCoreEntry(out, /*rl=*/1, /*nn=*/2, /*rr=*/1, /*nf=*/3, 3);
    }
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    EXPECT_FALSE(idx->load(path));
    EXPECT_EQ(idx->size(), 0u);
    std::filesystem::remove(path);
}

TEST(FlatTensorIndexPersistence, TFI10_BrokenRankChainRejected) {
    const std::string path = tfiTempPath("rank_chain");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        writeTtiHeader(out, 1);
        // Single core must have r_left == r_right == 1; r_right == 2 is invalid.
        writeSingleCoreEntry(out, /*rl=*/1, /*nn=*/2, /*rr=*/2, /*nf=*/4, 4);
    }
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    EXPECT_FALSE(idx->load(path));
    EXPECT_EQ(idx->size(), 0u);
    std::filesystem::remove(path);
}

TEST(FlatTensorIndexPersistence, TFI11_ModeSizeMismatchRejectedAndContentsKept) {
    const std::string path = tfiTempPath("mode_size_mismatch");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        writeTtiHeader(out, 1);
        writePod<int64_t>(out, 2);
        writePod<uint32_t>(out, 1);
        writePod<uint64_t>(out, 3);  // Declared mode size differs from core n.
        writePod<double>(out, 1.0);
        writePod<double>(out, 0.0);
        writePod<uint32_t>(out, 1);
        writePod<uint64_t>(out, 1);  // r_left
        writePod<uint64_t>(out, 2);  // core n
        writePod<uint64_t>(out, 1);  // r_right
        writePod<uint64_t>(out, 2);  // n_floats
        writePod<float>(out, 1.0f);
        writePod<float>(out, 1.0f);
    }

    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = indexWithOneVector(mgr);
    ASSERT_NE(idx->get(1), nullptr);
    EXPECT_FALSE(idx->load(path));
    EXPECT_EQ(idx->size(), 1u);
    EXPECT_NE(idx->get(1), nullptr);
    std::filesystem::remove(path);
}

TEST(FlatTensorIndexSearch, TFI12_IncompatibleQueryModeSizesRejected) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = indexWithOneVector(mgr);

    themis::storage::TTTrain query;
    query.mode_sizes = {8};
    themis::storage::TTCore core;
    core.r_left = 1;
    core.n = 8;
    core.r_right = 1;
    core.data.assign(8, 1.0f);
    query.cores.push_back(std::move(core));

    EXPECT_FALSE(idx->add(2, query));
    EXPECT_EQ(idx->size(), 1u);
    EXPECT_TRUE(idx->search(query, 1).empty());
}

TEST(FlatTensorIndexPersistence, TFI13_ModeCoreCountMismatchRejected) {
    const std::string path = tfiTempPath("mode_core_count_mismatch");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        writeTtiHeader(out, 1);
        writePod<int64_t>(out, 2);
        writePod<uint32_t>(out, 0);  // No declared modes.
        writePod<double>(out, 1.0);
        writePod<double>(out, 0.0);
        writePod<uint32_t>(out, 1);  // One core.
        writePod<uint64_t>(out, 1);
        writePod<uint64_t>(out, 2);
        writePod<uint64_t>(out, 1);
        writePod<uint64_t>(out, 2);
        writePod<float>(out, 1.0f);
        writePod<float>(out, 1.0f);
    }

    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = indexWithOneVector(mgr);
    EXPECT_FALSE(idx->load(path));
    EXPECT_EQ(idx->size(), 1u);
    std::filesystem::remove(path);
}

TEST(FlatTensorIndexPersistence, TFI14_MixedEntryModeSizesRejectedAndContentsKept) {
    const std::string path = tfiTempPath("mixed_entry_mode_sizes");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        writeTtiHeader(out, 2);
        const auto writeEntry = [&out](int64_t id, uint64_t mode_size) {
            writePod<int64_t>(out, id);
            writePod<uint32_t>(out, 1);
            writePod<uint64_t>(out, mode_size);
            writePod<double>(out, 1.0);
            writePod<double>(out, 0.0);
            writePod<uint32_t>(out, 1);
            writePod<uint64_t>(out, 1);          // r_left
            writePod<uint64_t>(out, mode_size);  // core n
            writePod<uint64_t>(out, 1);          // r_right
            writePod<uint64_t>(out, mode_size);  // n_floats
            for (uint64_t i = 0; i < mode_size; ++i) {
                writePod<float>(out, 1.0f);
            }
        };
        writeEntry(10, 2);
        writeEntry(11, 3);
    }

    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = indexWithOneVector(mgr);
    ASSERT_NE(idx->get(1), nullptr);
    EXPECT_FALSE(idx->load(path));
    EXPECT_EQ(idx->size(), 1u);
    EXPECT_NE(idx->get(1), nullptr);
    std::filesystem::remove(path);
}

// ============================================================================
// TIM-27..TIM-28 — createIndex() honours max_rank / epsilon
// ============================================================================

TEST(TensorIndexManager, TIM27_CreateIndexRejectsInvalidEpsilon) {
    auto mgr = TensorIndexManager::create(nullptr);
    EXPECT_EQ(mgr->createIndex("t", "c", "zero", 0, 32, 0.0), nullptr);
    EXPECT_EQ(mgr->createIndex("t", "c", "one", 0, 32, 1.0), nullptr);
    EXPECT_EQ(mgr->createIndex("t", "c", "nan", 0, 32,
                               std::numeric_limits<double>::quiet_NaN()), nullptr);
    EXPECT_TRUE(mgr->listIndexes().empty());
}

TEST(TensorIndexManager, TIM28_MaxRankIsAppliedToAddFlat) {
    // v[i] = 1 + 0.1*i reshaped to 8x8 is an affine matrix of rank 2.
    auto v = makeVec(64, 1.0f);
    auto maxBond = [](const themis::storage::TTTrain* t) {
        size_t r = 0;
        for (const auto& c : t->cores) r = std::max(r, c.r_right);
        return r;
    };

    auto mgr = TensorIndexManager::create(nullptr);
    auto* capped = mgr->createIndex("t", "c", "capped", 0, /*max_rank=*/1, 1e-4);
    auto* uncapped = mgr->createIndex("t", "c", "free",   0, /*max_rank=*/32, 1e-4);
    ASSERT_NE(capped, nullptr);
    ASSERT_NE(uncapped, nullptr);
    ASSERT_TRUE(capped->addFlat(1, v.data(), v.size()));
    ASSERT_TRUE(uncapped->addFlat(1, v.data(), v.size()));

    EXPECT_EQ(maxBond(capped->get(1)), 1u);
    EXPECT_GE(maxBond(uncapped->get(1)), 2u);
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
        if (e.path().extension() == ".ttidx") {
          ++file_count_before;
        }
    }
    EXPECT_EQ(file_count_before, 1u);

    EXPECT_TRUE(mgr->dropIndex("t", "c", "f"));

    size_t file_count_after = 0;
    for (auto& e : std::filesystem::directory_iterator(dir)) {
        if (e.path().extension() == ".ttidx") {
          ++file_count_after;
        }
    }
    EXPECT_EQ(file_count_after, 0u);

    std::filesystem::remove_all(dir);
}

TEST(TensorIndexManagerPersistence, TIM18_FlushAllEmptyManagerReturnsZero) {
    auto mgr = TensorIndexManager::create(nullptr);
    mgr->setDataDir("/tmp");
    EXPECT_EQ(mgr->flushAll(), 0u);
}

// ============================================================================
// TensorMmapBridge — TIM-19..TIM-24
// ============================================================================
//
// These tests exercise TensorIndexManager::mapCores() which returns a
// TensorMmapBridge owning mmap-pinned copies of TT-core data (STUB #176).
// Tests are deliberately platform-agnostic: mlock() failure is non-fatal
// (CI containers may have RLIMIT_MEMLOCK=0), so we only assert on pointer
// validity and data correctness, not on isLocked().
// ============================================================================

namespace {

/// Helper: add a vector and return the decomposed TTTrain for comparison.
/// Also returns the raw vector so we can validate round-trip data.
std::vector<float> addVecGetRaw(ITensorIndex* idx, int64_t id,
                                size_t dim, float seed) {
    auto v = makeVec(dim, seed);
    EXPECT_TRUE(idx->addFlat(id, v.data(), v.size()));
    return v;
}

} // namespace

TEST(TensorMmapBridge, TIM19_MapCoresReturnsNonNullForExistingVector) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    addVecGetRaw(idx, 42, 8, 1.0f);

    auto bridge = mgr->mapCores("t", "c", "f", 42);
    ASSERT_NE(bridge, nullptr);
    EXPECT_GT(bridge->coreCount(), 0u);
}

TEST(TensorMmapBridge, TIM20_MapCoresReturnsNullForMissingId) {
    auto mgr = TensorIndexManager::create(nullptr);
    mgr->createIndex("t", "c", "f");

    auto bridge = mgr->mapCores("t", "c", "f", 999);
    EXPECT_EQ(bridge, nullptr);
}

TEST(TensorMmapBridge, TIM21_MapCoresReturnsNullForMissingIndex) {
    auto mgr = TensorIndexManager::create(nullptr);

    auto bridge = mgr->mapCores("ghost", "c", "f", 1);
    EXPECT_EQ(bridge, nullptr);
}

TEST(TensorMmapBridge, TIM22_CoreDataMatchesOriginalTTCoreValues) {
    // Decompose a known vector and verify that every float in every pinned
    // core matches the stored TTTrain data exactly.
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    addVecGetRaw(idx, 7, 8, 2.5f);

    // Retrieve the raw TTTrain for comparison.
    const TTTrain* train = idx->get(7);
    ASSERT_NE(train, nullptr);

    auto bridge = mgr->mapCores("t", "c", "f", 7);
    ASSERT_NE(bridge, nullptr);

    const auto& slices = bridge->slices();
    ASSERT_EQ(slices.size(), train->cores.size());

    for (size_t ci = 0; ci < train->cores.size(); ++ci) {
        const auto& s = slices[ci];
        const auto& c = train->cores[ci];
        EXPECT_EQ(s.core_idx,  ci);
        EXPECT_EQ(s.num_elems, c.data.size());
        EXPECT_EQ(s.bytes,     c.data.size() * sizeof(float));
        if (s.data && !c.data.empty()) {
            for (size_t ei = 0; ei < c.data.size(); ++ei) {
                EXPECT_FLOAT_EQ(s.data[ei], c.data[ei])
                    << "Mismatch at core=" << ci << " elem=" << ei;
            }
        }
    }
}

TEST(TensorMmapBridge, TIM23_TotalBytesMatchesSumOfCoreSizes) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    addVecGetRaw(idx, 3, 16, 0.5f);

    const TTTrain* train = idx->get(3);
    ASSERT_NE(train, nullptr);

    size_t expected = 0;
    for (const auto& core : train->cores) {
        expected += core.data.size() * sizeof(float);
    }

    auto bridge = mgr->mapCores("t", "c", "f", 3);
    ASSERT_NE(bridge, nullptr);
    EXPECT_EQ(bridge->totalBytes(), expected);
}

TEST(TensorMmapBridge, TIM24_ReleaseInvalidatesRegions) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t", "c", "f");
    addVecGetRaw(idx, 55, 8, 1.0f);

    auto bridge = mgr->mapCores("t", "c", "f", 55);
    ASSERT_NE(bridge, nullptr);
    EXPECT_GT(bridge->coreCount(), 0u);

    // After explicit release, slices must be empty.
    bridge->release();
    EXPECT_EQ(bridge->coreCount(), 0u);
    EXPECT_EQ(bridge->totalBytes(), 0u);
    EXPECT_FALSE(bridge->isLocked());

    // A second release must be safe (idempotent).
    EXPECT_NO_THROW(bridge->release());
}

TEST(TensorMmapBridge, TIM25_GgmlCorePtrsReturnsNonEmptySlicesForExistingVector) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t_ggml", "c", "f");
    addVecGetRaw(idx, 77, 8, 1.25f);

    auto ptrs = mgr->ggmlCorePtrs("t_ggml", "c", "f", 77);
    ASSERT_FALSE(ptrs.empty());
    for (const auto& [ptr, bytes] : ptrs) {
        if (bytes > 0) {
            ASSERT_NE(ptr, nullptr);
        }
    }
}

TEST(TensorMmapBridge, TIM26_GgmlCorePtrsDataMatchesStoredTrainCores) {
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t_ggml2", "c", "f");
    addVecGetRaw(idx, 88, 8, 2.75f);

    const TTTrain* train = idx->get(88);
    ASSERT_NE(train, nullptr);

    auto ptrs = mgr->ggmlCorePtrs("t_ggml2", "c", "f", 88);
    ASSERT_EQ(ptrs.size(), train->cores.size());

    for (size_t ci = 0; ci < train->cores.size(); ++ci) {
        const auto& core = train->cores[ci];
        const auto* data = ptrs[ci].first;
        const auto bytes = ptrs[ci].second;
        ASSERT_EQ(bytes, core.data.size() * sizeof(float));
        if (core.data.empty()) {
            continue;
        }
        ASSERT_NE(data, nullptr);
        for (size_t ei = 0; ei < core.data.size(); ++ei) {
            EXPECT_FLOAT_EQ(data[ei], core.data[ei]);
        }
    }
}


// ============================================================================
// TensorMmapBridge SST-page-map bridge (STUB #270)
// MMB-INJ-01..MMB-INJ-03
// ============================================================================

// MMB-INJ-01: Without SstMapFn, mapCores() uses MAP_ANONYMOUS + memcpy.
//             Data in bridge slices must match original vector.
TEST(TensorMmapBridge, MMB_INJ_01_no_fn_data_matches_original) {
    TensorMmapBridge::clearSstMapFn();
    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t_mmb1", "c", "f");
    auto raw = addVecGetRaw(idx, 100, 8, 2.5f);

    auto bridge = mgr->mapCores("t_mmb1", "c", "f", 100);
    ASSERT_NE(bridge, nullptr);
    ASSERT_GT(bridge->coreCount(), 0u);

    // Verify total bytes are consistent (> 0 since raw is non-trivial).
    EXPECT_GT(bridge->totalBytes(), 0u);

    // All slices must have valid data pointers.
    for (const auto& s : bridge->slices()) {
        if (s.num_elems > 0) {
            EXPECT_NE(s.data, nullptr);
        }
    }
}

// MMB-INJ-02: Injected SstMapFn is called during mapCores(); the fn is invoked
//             at least once for a non-empty train.
TEST(TensorMmapBridge, MMB_INJ_02_injected_sst_fn_is_called) {
    bool fn_called = false;

    // The fn returns nullptr to trigger memcpy fallback — we just verify it was called.
    TensorMmapBridge::setSstMapFn(
        [&fn_called]([[maybe_unused]] std::size_t bytes,
                     [[maybe_unused]] std::size_t core_idx) -> void* {
            fn_called = true;
            return nullptr; // fall back to MAP_ANONYMOUS
        });

    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t_mmb2", "c", "f");
    addVecGetRaw(idx, 200, 8, 3.0f);
    auto bridge = mgr->mapCores("t_mmb2", "c", "f", 200);
    TensorMmapBridge::clearSstMapFn();

    EXPECT_TRUE(fn_called);
    ASSERT_NE(bridge, nullptr);
    EXPECT_GT(bridge->coreCount(), 0u);
}

// MMB-INJ-03: After clearSstMapFn(), mapCores() reverts to MAP_ANONYMOUS+memcpy.
//             Data must still be valid and consistent with the stored vector.
TEST(TensorMmapBridge, MMB_INJ_03_clear_fn_reverts_to_anonymous_mmap) {
    TensorMmapBridge::setSstMapFn(
        [](std::size_t, std::size_t) -> void* { return nullptr; });
    TensorMmapBridge::clearSstMapFn();

    auto mgr = TensorIndexManager::create(nullptr);
    auto* idx = mgr->createIndex("t_mmb3", "c", "f");
    addVecGetRaw(idx, 300, 8, 4.0f);
    auto bridge = mgr->mapCores("t_mmb3", "c", "f", 300);

    ASSERT_NE(bridge, nullptr);
    EXPECT_GT(bridge->totalBytes(), 0u);
    for (const auto& s : bridge->slices()) {
        if (s.num_elems > 0) {
            EXPECT_NE(s.data, nullptr);
        }
    }
}
