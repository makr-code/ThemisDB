/**
 * @file test_vector_index_comprehensive.cpp
 * @brief Comprehensive unit tests for storage::IVectorIndexBackend and
 *        storage::InMemoryVectorIndex.
 *
 * Test IDs: VIB-01 … VIB-12
 */

#include <gtest/gtest.h>
#include "storage/vector_index_backend.h"

#include <thread>
#include <vector>

using namespace themis::storage;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<float> unitVec(std::size_t dim, std::size_t hot_dim)
{
    auto v = std::vector<float>(dim, 0.0f);
    if (hot_dim < dim) { v[hot_dim] = 1.0f; }
    return v;
}

// ============================================================================
// VIB-01 — Basic add and size
// ============================================================================
TEST(VectorIndexBackend, VIB_01_AddAndSize)
{
    VectorIndexConfig cfg;
    cfg.dim    = 4;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);
    EXPECT_EQ(idx.size(), 0u);

    idx.add("a", {1.0f, 0.0f, 0.0f, 0.0f});
    EXPECT_EQ(idx.size(), 1u);

    idx.add("b", {0.0f, 1.0f, 0.0f, 0.0f});
    EXPECT_EQ(idx.size(), 2u);
}

// ============================================================================
// VIB-02 — Replace (upsert) existing id
// ============================================================================
TEST(VectorIndexBackend, VIB_02_Upsert)
{
    VectorIndexConfig cfg;
    cfg.dim    = 2;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);
    idx.add("x", {1.0f, 0.0f});
    idx.add("x", {0.0f, 1.0f});   // replace
    EXPECT_EQ(idx.size(), 1u);

    auto results = idx.search({0.0f, 1.0f}, 1);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "x");
    EXPECT_NEAR(results[0].distance, 0.0f, 1e-5f);
}

// ============================================================================
// VIB-03 — Remove
// ============================================================================
TEST(VectorIndexBackend, VIB_03_Remove)
{
    VectorIndexConfig cfg;
    cfg.dim    = 3;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);
    idx.add("p", {1.0f, 2.0f, 3.0f});
    idx.add("q", {4.0f, 5.0f, 6.0f});
    EXPECT_EQ(idx.size(), 2u);

    idx.remove("p");
    EXPECT_EQ(idx.size(), 1u);

    auto results = idx.search({1.0f, 2.0f, 3.0f}, 2);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "q");

    // Remove non-existent id must not throw.
    EXPECT_NO_THROW(idx.remove("nonexistent"));
}

// ============================================================================
// VIB-04 — L2 search correctness
// ============================================================================
TEST(VectorIndexBackend, VIB_04_L2SearchCorrectness)
{
    VectorIndexConfig cfg;
    cfg.dim    = 3;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);
    idx.add("origin", {0.0f, 0.0f, 0.0f});
    idx.add("near",   {1.0f, 0.0f, 0.0f});
    idx.add("far",    {10.0f, 10.0f, 10.0f});

    auto results = idx.search({0.0f, 0.0f, 0.0f}, 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].id, "origin");
    EXPECT_EQ(results[1].id, "near");
    EXPECT_EQ(results[2].id, "far");
    EXPECT_LE(results[0].distance, results[1].distance);
    EXPECT_LE(results[1].distance, results[2].distance);
}

// ============================================================================
// VIB-05 — COSINE search correctness
// ============================================================================
TEST(VectorIndexBackend, VIB_05_CosineSearchCorrectness)
{
    VectorIndexConfig cfg;
    cfg.dim    = 3;
    cfg.metric = DistanceMetric::COSINE;

    InMemoryVectorIndex idx(cfg);
    idx.add("x_axis", unitVec(3, 0));
    idx.add("y_axis", unitVec(3, 1));
    idx.add("z_axis", unitVec(3, 2));

    auto results = idx.search({0.9f, 0.1f, 0.0f}, 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].id, "x_axis");
    EXPECT_NEAR(results[0].score, 1.0f, 0.15f);
}

// ============================================================================
// VIB-06 — DOT_PRODUCT metric
// ============================================================================
TEST(VectorIndexBackend, VIB_06_DotProductMetric)
{
    VectorIndexConfig cfg;
    cfg.dim    = 2;
    cfg.metric = DistanceMetric::DOT_PRODUCT;

    InMemoryVectorIndex idx(cfg);
    idx.add("high",  {10.0f, 10.0f});
    idx.add("low",   { 1.0f,  1.0f});
    idx.add("zero",  { 0.0f,  0.0f});

    auto results = idx.search({1.0f, 1.0f}, 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].id, "high");
}

// ============================================================================
// VIB-07 — Dimension mismatch throws
// ============================================================================
TEST(VectorIndexBackend, VIB_07_DimensionMismatch)
{
    VectorIndexConfig cfg;
    cfg.dim    = 4;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);

    EXPECT_THROW(([&idx] {
        idx.add("bad", {1.0f, 2.0f});
    }()), std::invalid_argument);
    try {
        auto results = idx.search({1.0f, 2.0f}, 1);
        (void)results;
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument&) {
        SUCCEED();
    } catch (...) {
        FAIL() << "Expected std::invalid_argument";
    }
}

// ============================================================================
// VIB-08 — dim=0 throws at construction
// ============================================================================
TEST(VectorIndexBackend, VIB_08_ZeroDimThrows)
{
    VectorIndexConfig cfg;
    cfg.dim = 0;
    EXPECT_THROW(InMemoryVectorIndex{cfg}, std::invalid_argument);
}

// ============================================================================
// VIB-09 — k > size returns all elements
// ============================================================================
TEST(VectorIndexBackend, VIB_09_KLargerThanSize)
{
    VectorIndexConfig cfg;
    cfg.dim    = 2;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);
    idx.add("a", {1.0f, 0.0f});
    idx.add("b", {0.0f, 1.0f});

    auto results = idx.search({0.5f, 0.5f}, 100);
    EXPECT_EQ(results.size(), 2u);
}

// ============================================================================
// VIB-10 — Thread-safety: concurrent adds and searches
// ============================================================================
TEST(VectorIndexBackend, VIB_10_ConcurrentAddSearch)
{
    VectorIndexConfig cfg;
    cfg.dim    = 4;
    cfg.metric = DistanceMetric::L2;

    auto idx = std::make_shared<InMemoryVectorIndex>(cfg);

    constexpr int kWriters   = 4;
    constexpr int kPerWriter = 50;

    std::vector<std::thread> writers;
    writers.reserve(kWriters);
    for (int t = 0; t < kWriters; ++t) {
        writers.emplace_back([&idx, t]() {
            for (int i = 0; i < kPerWriter; ++i) {
                std::string id = "w" + std::to_string(t) + "_" + std::to_string(i);
                idx->add(id, {static_cast<float>(t), static_cast<float>(i), 0.0f, 0.0f});
            }
        });
    }

    std::thread reader([&idx]() {
        size_t sink = 0;
        for (int i = 0; i < 20; ++i) {
            auto results = idx->search({0.0f, 0.0f, 0.0f, 0.0f}, 5);
            sink += results.size();
        }
        (void)sink;
    });

    for (auto& w : writers) { w.join(); }
    reader.join();

    EXPECT_EQ(idx->size(), static_cast<std::size_t>(kWriters * kPerWriter));
}

// ============================================================================
// VIB-11 — score ∈ [0, 1] for L2
// ============================================================================
TEST(VectorIndexBackend, VIB_11_L2ScoreRange)
{
    VectorIndexConfig cfg;
    cfg.dim    = 3;
    cfg.metric = DistanceMetric::L2;

    InMemoryVectorIndex idx(cfg);
    idx.add("a", {100.0f, 200.0f, 300.0f});
    idx.add("b", {0.0f, 0.0f, 0.0f});

    auto results = idx.search({0.0f, 0.0f, 0.0f}, 2);
    for (const auto& r : results) {
        EXPECT_GE(r.score, 0.0f);
        EXPECT_LE(r.score, 1.0f);
    }
}

// ============================================================================
// VIB-12 — config() reflects construction parameters
// ============================================================================
TEST(VectorIndexBackend, VIB_12_ConfigAccessor)
{
    VectorIndexConfig cfg;
    cfg.dim    = 128;
    cfg.metric = DistanceMetric::COSINE;
    cfg.hnsw_M = 32;

    InMemoryVectorIndex idx(cfg);
    EXPECT_EQ(idx.config().dim, 128u);
    EXPECT_EQ(idx.config().metric, DistanceMetric::COSINE);
    EXPECT_EQ(idx.config().hnsw_M, 32u);
    EXPECT_EQ(idx.name(), "in_memory");
}
