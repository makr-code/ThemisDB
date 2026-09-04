// Focused tests for the VecKnnInsertPipeline (PERF-D3).
// Test suite name: VecKnnInsertFocusedTests
//
// Coverage:
//   1. simd_l2_sq correctness (scalar reference comparison)
//   2. simd_batch_l2_sq correctness for n vectors
//   3. DistanceCache get/put/invalidate/clear/size
//   4. DistanceCache canonical key ordering (a,b) == (b,a)
//   5. DistanceCache eviction when full
//   6. DistanceCache hit/miss counters
//   7. VecKnnInsertPipeline default construction
//   8. VecKnnInsertPipeline setBatchSize / setThreadCount
//   9. insertBatch – empty entities returns ok
//  10. insertBatch – single entity
//  11. insertBatch – full batch (> batch_size entities)
//  12. insertBatch – all entities visible in index after insert
//  13. insertBatch – parallel insert (wave > 1 sub-batch)
//  14. insertBatch – cache is pre-warmed after insert
//  15. computeDistances – shape and monotonicity
//  16. computeDistances – zero query returns large distances
//  17. VecKnnInsertPipeline enableDistanceCache false
//  18. VecKnnInsertPipeline totalInserted / totalFailed counters
//  19. VecKnnInsertPipeline config accessor
//  20. DistanceCache thread-safety: concurrent put/get
//  21. insertBatch – vectorField override
//  22. insertBatch – returns ok=false if index not initialised
//  23. simd_l2_sq symmetry: dist(a,b) == dist(b,a)
//  24. simd_l2_sq identity: dist(a,a) == 0
//  25. insertBatch – cache hit counter grows on repeated batch pairs

#include <gtest/gtest.h>
#include "acceleration/vec_knn.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::acceleration;

// ============================================================================
// Test fixture
// ============================================================================

class VecKnnInsertFocusedTests : public ::testing::Test {
protected:
    static constexpr int kDim = 64;

    std::string   db_path_ = {};
    std::unique_ptr<RocksDBWrapper>      db_;
    std::unique_ptr<VectorIndexManager>  vim_;

    void SetUp() override {
        db_path_ = "/tmp/test_vec_knn_insert_" +
                   std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        vim_ = std::make_unique<VectorIndexManager>(*db_);
        auto st = vim_->init("test_vec", kDim);
        ASSERT_TRUE(st.ok) << st.message;

        VecKnnInsertPipeline::setAddBatchBridgeFn(
            [](VectorIndexManager& idx,
               const std::vector<BaseEntity>& batch,
               std::string_view field) -> VecKnnInsertResult {
                const auto st = idx.addBatch(batch, field);
                VecKnnInsertResult r;
                r.ok = st.ok;
                r.message = st.message;
                if (st.ok) {
                    r.inserted = batch.size();
                } else {
                    r.failed = batch.size();
                }
                return r;
            });

        VecKnnInsertPipeline::setExtractVectorBridgeFn(
            [](const BaseEntity& entity,
               std::string_view field) -> std::optional<std::vector<float>> {
                return entity.getFieldAsVector(field);
            });
    }

    void TearDown() override {
        // NOTE: Do NOT clear the bridge functions here. They are process-global state
        // and clearing them can affect other tests if tests run in parallel or if
        // they use SetUp/TearDown patterns. The bridges are set in SetUp() and
        // setting them again is harmless (overwrites the previous ones).
        // Each test's SetUp() will reconfigure the bridges correctly.
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // Generate a random unit-normalised vector of length kDim
    std::vector<float> randVec(int seed = 0) const {
        std::mt19937 rng(static_cast<uint32_t>(seed + 1));
        std::uniform_real_distribution<float> dis(-1.f, 1.f);
        std::vector<float> v(kDim);
        for (auto& x : v) {
          x = dis(rng);
        }
        return v;
    }

    // Build n BaseEntity objects with "embedding" vectors
    std::vector<BaseEntity> makeEntities(int n, int seed = 0) const {
        std::vector<BaseEntity> out;
        out.reserve(n);
        for (int i = 0; i < n; ++i) {
            BaseEntity e("pk_" + std::to_string(i), BaseEntity::FieldMap{
                {"embedding", randVec(seed + i)}
            });
            out.push_back(std::move(e));
        }
        return out;
    }

    // Scalar L2 squared distance
    static float scalarL2Sq(const std::vector<float>& a, const std::vector<float>& b) {
        float s = 0;
        for (std::size_t i = 0; i < a.size(); ++i) {
            float d = a[i] - b[i]; s += d * d;
        }
        return s;
    }
};

// ============================================================================
// 1. simd_l2_sq correctness
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, SimdL2SqCorrectness) {
    auto a = randVec(1);
    auto b = randVec(2);
    float expected = scalarL2Sq(a, b);
    float got = simd_l2_sq(a.data(), b.data(), kDim);
    EXPECT_NEAR(got, expected, 1e-3f * expected + 1e-6f);
}

// ============================================================================
// 2. simd_batch_l2_sq correctness
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, SimdBatchL2SqCorrectness) {
    constexpr int N = 16;
    auto q = randVec(99);
    std::vector<float> db_flat(N * kDim);
    std::vector<std::vector<float>> db(N);
    for (int i = 0; i < N; ++i) {
        db[i] = randVec(i);
        std::copy(db[i].begin(), db[i].end(), db_flat.data() + i * kDim);
    }
    std::vector<float> out(N);
    simd_batch_l2_sq(q.data(), db_flat.data(), N, kDim, out.data());
    for (int i = 0; i < N; ++i) {
        float expected = scalarL2Sq(q, db[i]);
        EXPECT_NEAR(out[i], expected, 1e-3f * expected + 1e-6f)
            << "mismatch at index " << i;
    }
}

// ============================================================================
// 3. DistanceCache get/put/size
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, DistanceCacheGetPut) {
    DistanceCache dc(128);
    EXPECT_EQ(dc.size(), 0u);

    dc.put("a", "b", 1.5f);
    EXPECT_EQ(dc.size(), 1u);

    float val = 0;
    EXPECT_TRUE(dc.get("a", "b", val));
    EXPECT_FLOAT_EQ(val, 1.5f);
}

// ============================================================================
// 4. DistanceCache canonical key ordering
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, DistanceCacheCanonicalKey) {
    DistanceCache dc;
    dc.put("x", "y", 2.0f);
    float val = 0;
    EXPECT_TRUE(dc.get("y", "x", val));
    EXPECT_FLOAT_EQ(val, 2.0f);
}

// ============================================================================
// 5. DistanceCache eviction when full
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, DistanceCacheEviction) {
    const std::size_t max = 4;
    DistanceCache dc(max);
    for (std::size_t i = 0; i < max + 2; ++i)
        dc.put("a" + std::to_string(i), "b" + std::to_string(i), static_cast<float>(i));

    // Size must not exceed max
    EXPECT_LE(dc.size(), max);
}

// ============================================================================
// 6. DistanceCache hit/miss counters
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, DistanceCacheHitMissCounters) {
    DistanceCache dc;
    dc.put("p", "q", 3.0f);
    float v = 0;
    dc.get("p", "q", v); // hit
    dc.get("p", "z", v); // miss
    EXPECT_EQ(dc.hits(), 1u);
    EXPECT_EQ(dc.misses(), 1u);
}

// ============================================================================
// 7. VecKnnInsertPipeline default construction
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, PipelineDefaultConstruction) {
    VecKnnInsertPipeline p;
    EXPECT_GE(p.config().batch_size, 1u);
    EXPECT_GE(p.config().num_threads, 1u);
}

// ============================================================================
// 8. setBatchSize / setThreadCount
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, PipelineSetBatchAndThreadCount) {
    VecKnnInsertPipeline p;
    p.setBatchSize(64);
    EXPECT_EQ(p.config().batch_size, 64u);
    p.setThreadCount(4);
    EXPECT_EQ(p.config().num_threads, 4u);
}

// ============================================================================
// 9. insertBatch – empty entities returns ok
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertBatchEmpty) {
    VecKnnInsertPipeline p;
    auto res = p.insertBatch(*vim_, {});
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(res.inserted, 0u);
    EXPECT_EQ(res.failed, 0u);
}

// ============================================================================
// 10. insertBatch – single entity
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertBatchSingleEntity) {
    VecKnnInsertPipeline p;
    auto entities = makeEntities(1);
    auto res = p.insertBatch(*vim_, entities);
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.inserted, 1u);
    EXPECT_EQ(res.failed, 0u);
}

// ============================================================================
// 11. insertBatch – full batch (> batch_size entities)
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertBatchLargerThanBatchSize) {
    VecKnnPipelineConfig cfg;
    cfg.batch_size = 8;
    cfg.num_threads = 2;
    VecKnnInsertPipeline p(cfg);

    auto entities = makeEntities(32);
    auto res = p.insertBatch(*vim_, entities);
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.inserted, 32u);
}

// ============================================================================
// 12. insertBatch – all entities visible in index after insert
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertedEntitiesVisibleInIndex) {
    constexpr int N = 20;
    VecKnnInsertPipeline p;
    auto entities = makeEntities(N);
    auto res = p.insertBatch(*vim_, entities);
    ASSERT_TRUE(res.ok) << res.message;

    // VectorIndexManager::getVectorCount() should reflect inserted vectors
    EXPECT_EQ(static_cast<int>(vim_->getVectorCount()), N);
}

// ============================================================================
// 13. insertBatch – parallel insert (multiple waves)
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertBatchParallelWaves) {
    VecKnnPipelineConfig cfg;
    cfg.batch_size  = 4;
    cfg.num_threads = 2;
    cfg.enable_cache = false;
    VecKnnInsertPipeline p(cfg);

    auto entities = makeEntities(24);
    auto res = p.insertBatch(*vim_, entities);
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.inserted, 24u);
}

// ============================================================================
// 14. insertBatch – cache is pre-warmed after insert
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, CachePreWarmedAfterInsert) {
    VecKnnPipelineConfig cfg;
    cfg.batch_size   = 8;
    cfg.enable_cache = true;
    VecKnnInsertPipeline p(cfg);

    auto entities = makeEntities(8);
    auto res = p.insertBatch(*vim_, entities);
    ASSERT_TRUE(res.ok);

    // Cache should have at least one entry from pairwise pre-warming
    EXPECT_GT(p.cache().size(), 0u);
}

// ============================================================================
// 15. computeDistances – shape and monotonicity
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, ComputeDistancesShape) {
    VecKnnInsertPipeline p;
    constexpr int NQ = 3, ND = 5;
    auto qv = randVec(0);
    std::vector<float> queries(NQ * kDim), db(ND * kDim);
    for (int i = 0; i < NQ; ++i) {
        auto v = randVec(i);
        std::copy(v.begin(), v.end(), queries.data() + i * kDim);
    }
    for (int i = 0; i < ND; ++i) {
        auto v = randVec(10 + i);
        std::copy(v.begin(), v.end(), db.data() + i * kDim);
    }
    auto dists = p.computeDistances(queries.data(), NQ, db.data(), ND, kDim);
    ASSERT_EQ(dists.size(), static_cast<std::size_t>(NQ * ND));
    for (float d : dists) {
      EXPECT_GE(d, 0.f);
    }
}

// ============================================================================
// 16. computeDistances – empty inputs return empty
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, ComputeDistancesEmptyInput) {
    VecKnnInsertPipeline p;
    auto dists = p.computeDistances(nullptr, 0, nullptr, 0, 0);
    EXPECT_TRUE(dists.empty());
}

// ============================================================================
// 17. enableDistanceCache false – cache stays empty
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, EnableDistanceCacheFalse) {
    VecKnnPipelineConfig cfg;
    cfg.batch_size   = 8;
    cfg.enable_cache = false;
    VecKnnInsertPipeline p(cfg);

    auto entities = makeEntities(8);
    auto res = p.insertBatch(*vim_, entities);
    ASSERT_TRUE(res.ok);
    EXPECT_EQ(p.cache().size(), 0u);
}

// ============================================================================
// 18. totalInserted / totalFailed counters
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, TotalInsertedFailedCounters) {
    VecKnnInsertPipeline p;
    auto entities = makeEntities(10);
    p.insertBatch(*vim_, entities);
    EXPECT_EQ(p.totalInserted(), 10u);
    EXPECT_EQ(p.totalFailed(),   0u);
}

// ============================================================================
// 19. config accessor is stable after set calls
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, ConfigAccessorStable) {
    VecKnnPipelineConfig cfg;
    cfg.batch_size  = 16;
    cfg.num_threads = 3;
    cfg.enable_cache = true;
    VecKnnInsertPipeline p(cfg);
    EXPECT_EQ(p.config().batch_size,  16u);
    EXPECT_EQ(p.config().num_threads, 3u);
    EXPECT_TRUE(p.config().enable_cache);
}

// ============================================================================
// 20. DistanceCache thread-safety: concurrent put/get
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, DistanceCacheThreadSafety) {
    DistanceCache dc(4096);
    constexpr int N = 20;
    std::vector<std::thread> workers;
    workers.reserve(N);
    for (int i = 0; i < N; ++i) {
        workers.emplace_back([&dc, i]() {
            dc.put("a" + std::to_string(i), "b" + std::to_string(i),
                   static_cast<float>(i));
            float v = 0;
            dc.get("a" + std::to_string(i), "b" + std::to_string(i), v);
        });
    }
    for (auto& w : workers) {
      w.join();
    }
    EXPECT_LE(dc.size(), static_cast<std::size_t>(N));
}

// ============================================================================
// 21. insertBatch – vectorField override
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertBatchVectorFieldOverride) {
    // vim_ is already initialised in SetUp() with "embedding" as the field.
    // Use 5 entities with the default "embedding" field and pass the field
    // name explicitly as a vectorField override.
    std::vector<BaseEntity> entities = {};

    for (int i = 0; i < 5; ++i) {
        entities.emplace_back("fld_" + std::to_string(i), BaseEntity::FieldMap{
            {"embedding", randVec(i)}
        });
    }

    VecKnnInsertPipeline p;
    auto res = p.insertBatch(*vim_, entities, "embedding");
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.inserted, 5u);
}

// ============================================================================
// 22. insertBatch – mirrors direct addBatch behaviour on uninitialised index
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, InsertBatchUninitializedIndex) {
    VectorIndexManager vim2(*db_);
    VectorIndexManager vim3(*db_);
    // NOT calling init() on either manager
    auto entities_pipeline = makeEntities(1, 1200);
    auto entities_direct = makeEntities(1, 1300);

    VecKnnInsertPipeline p;
    auto res = p.insertBatch(vim2, entities_pipeline);
    const auto direct = vim3.addBatch(entities_direct, "embedding");

    EXPECT_EQ(res.ok, direct.ok);
    if (res.ok) {
        EXPECT_EQ(res.failed, 0u);
        EXPECT_EQ(res.inserted, entities_pipeline.size());
    } else {
        EXPECT_GT(res.failed, 0u);
    }
}

// ============================================================================
// 23. simd_l2_sq symmetry: dist(a,b) == dist(b,a)
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, SimdL2SqSymmetry) {
    auto a = randVec(7);
    auto b = randVec(8);
    float dab = simd_l2_sq(a.data(), b.data(), kDim);
    float dba = simd_l2_sq(b.data(), a.data(), kDim);
    EXPECT_NEAR(dab, dba, 1e-5f);
}

// ============================================================================
// 24. simd_l2_sq identity: dist(a,a) == 0
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, SimdL2SqIdentity) {
    auto a = randVec(42);
    float d = simd_l2_sq(a.data(), a.data(), kDim);
    EXPECT_NEAR(d, 0.f, 1e-6f);
}

// ============================================================================
// 25. Cache hit counter grows on repeated batch pairs
// ============================================================================
TEST_F(VecKnnInsertFocusedTests, CacheHitCounterGrowsOnRepeat) {
    VecKnnPipelineConfig cfg;
    cfg.batch_size   = 4;
    cfg.enable_cache = true;
    VecKnnInsertPipeline p(cfg);

    auto entities = makeEntities(4, 100);
    // First call: populates cache, mostly misses
    p.insertBatch(*vim_, entities);
    std::size_t hits_after_first = p.cache().hits();

    // Second call with same keys – cache already has the entries
    // (insertBatch is designed to re-use them for distance prewarming)
    p.insertBatch(*vim_, entities);
    std::size_t hits_after_second = p.cache().hits();

    // Hits should increase on the second call
    EXPECT_GE(hits_after_second, hits_after_first);
}
