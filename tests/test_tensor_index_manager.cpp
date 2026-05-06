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
 */

#include <gtest/gtest.h>

#include "tensor/tensor_index_manager.h"
#include "storage/tensor_router.h"

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
