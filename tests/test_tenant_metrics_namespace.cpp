/**
 * @file test_tenant_metrics_namespace.cpp
 * @brief Focused unit tests for TenantMetricsNamespace.
 *
 * Test suite: TenantMetricsNamespaceFocusedTests
 */

#include <gtest/gtest.h>
#include "observability/tenant_metrics_namespace.h"

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;

// ============================================================================
// Fixture
// ============================================================================

class TenantMetricsNamespaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        TenantMetricsConfig cfg;
        cfg.cardinality_limit_per_tenant = 50;
        reg_ = std::make_unique<TenantMetricsNamespace>(cfg);
    }

    std::unique_ptr<TenantMetricsNamespace> reg_;
};

// ============================================================================
// Lifecycle
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, RegisterTenant_NewTenant_ReturnsTrue) {
    EXPECT_TRUE(reg_->registerTenant("acme"));
    EXPECT_TRUE(reg_->hasTenant("acme"));
}

TEST_F(TenantMetricsNamespaceTest, RegisterTenant_Duplicate_ReturnsFalse) {
    reg_->registerTenant("acme");
    EXPECT_FALSE(reg_->registerTenant("acme"));
    EXPECT_EQ(1u, reg_->tenantCount());
}

TEST_F(TenantMetricsNamespaceTest, DeregisterTenant_Existing_ReturnsTrue) {
    reg_->registerTenant("acme");
    EXPECT_TRUE(reg_->deregisterTenant("acme"));
    EXPECT_FALSE(reg_->hasTenant("acme"));
}

TEST_F(TenantMetricsNamespaceTest, DeregisterTenant_Unknown_ReturnsFalse) {
    EXPECT_FALSE(reg_->deregisterTenant("unknown_tenant"));
}

TEST_F(TenantMetricsNamespaceTest, HasTenant_UnknownTenant_ReturnsFalse) {
    EXPECT_FALSE(reg_->hasTenant("nobody"));
}

TEST_F(TenantMetricsNamespaceTest, Tenants_ListsAllRegistered) {
    reg_->registerTenant("a");
    reg_->registerTenant("b");
    reg_->registerTenant("c");
    auto ids = reg_->tenants();
    EXPECT_EQ(3u, ids.size());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "a") != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "b") != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "c") != ids.end());
}

TEST_F(TenantMetricsNamespaceTest, TenantCount_Accurate) {
    EXPECT_EQ(0u, reg_->tenantCount());
    reg_->registerTenant("t1");
    EXPECT_EQ(1u, reg_->tenantCount());
    reg_->registerTenant("t2");
    EXPECT_EQ(2u, reg_->tenantCount());
    reg_->deregisterTenant("t1");
    EXPECT_EQ(1u, reg_->tenantCount());
}

TEST_F(TenantMetricsNamespaceTest, MaxTenants_Enforced) {
    TenantMetricsConfig cfg;
    cfg.max_tenants = 2;
    TenantMetricsNamespace reg2(cfg);
    EXPECT_TRUE(reg2.registerTenant("a"));
    EXPECT_TRUE(reg2.registerTenant("b"));
    EXPECT_FALSE(reg2.registerTenant("c")); // limit reached
    EXPECT_EQ(2u, reg2.tenantCount());
}

// ============================================================================
// increment
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, Increment_AutoRegistersInNonStrictMode) {
    reg_->increment("new_tenant", "queries_total");
    EXPECT_TRUE(reg_->hasTenant("new_tenant"));
    auto s = reg_->stats("new_tenant");
    EXPECT_EQ(1, s.total_observations);
}

TEST_F(TenantMetricsNamespaceTest, Increment_StrictMode_DropsUnknownTenant) {
    TenantMetricsConfig cfg;
    cfg.strict_tenant_registration = true;
    TenantMetricsNamespace strict(cfg);
    strict.increment("ghost", "queries_total");
    EXPECT_FALSE(strict.hasTenant("ghost"));
}

TEST_F(TenantMetricsNamespaceTest, Increment_CounterAccumulates) {
    reg_->registerTenant("acme");
    reg_->increment("acme", "req_total");
    reg_->increment("acme", "req_total");
    reg_->increment("acme", "req_total");
    auto s = reg_->stats("acme");
    EXPECT_EQ(3, s.total_observations);
}

TEST_F(TenantMetricsNamespaceTest, Increment_WithLabels) {
    reg_->registerTenant("t");
    reg_->increment("t", "query_total", {{"type", "select"}});
    reg_->increment("t", "query_total", {{"type", "insert"}});
    auto s = reg_->stats("t");
    EXPECT_EQ(2, s.total_observations);
}

// ============================================================================
// setGauge
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, SetGauge_StoresValue) {
    reg_->registerTenant("t");
    reg_->setGauge("t", "connections", 42.0);
    auto s = reg_->stats("t");
    EXPECT_EQ(1, s.total_observations);
    EXPECT_GE(s.active_series, 1u);
}

TEST_F(TenantMetricsNamespaceTest, SetGauge_OverwritesPreviousValue) {
    reg_->registerTenant("t");
    reg_->setGauge("t", "connections", 10.0);
    reg_->setGauge("t", "connections", 99.0);
    // Still counted as 2 observations
    EXPECT_EQ(2, reg_->stats("t").total_observations);
}

// ============================================================================
// observeHistogram
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, ObserveHistogram_RecordsObservation) {
    reg_->registerTenant("t");
    reg_->observeHistogram("t", "latency_ms", 12.5);
    reg_->observeHistogram("t", "latency_ms", 30.0);
    auto s = reg_->stats("t");
    EXPECT_EQ(2, s.total_observations);
}

// ============================================================================
// Cardinality enforcement
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, CardinalityLimit_DropsExcessSeries) {
    TenantMetricsConfig cfg;
    cfg.cardinality_limit_per_tenant = 3;
    TenantMetricsNamespace reg2(cfg);
    reg2.registerTenant("t");

    for (int i = 0; i < 10; ++i) {
        reg2.increment("t", "metric", {{"i", std::to_string(i)}});
    }

    auto s = reg2.stats("t");
    EXPECT_GT(s.dropped_observations, 0);
}

TEST_F(TenantMetricsNamespaceTest, CardinalityLimit_IndependentPerTenant) {
    TenantMetricsConfig cfg;
    cfg.cardinality_limit_per_tenant = 2;
    TenantMetricsNamespace reg2(cfg);
    reg2.registerTenant("a");
    reg2.registerTenant("b");

    // Fill cardinality for tenant "a"
    for (int i = 0; i < 5; ++i) {
        reg2.increment("a", "m", {{"k", std::to_string(i)}});
    }
    // Tenant "b" should still be unaffected
    reg2.increment("b", "m", {{"k", "0"}});
    EXPECT_EQ(0, reg2.stats("b").dropped_observations);
}

// ============================================================================
// Export
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, ExportTenant_ContainsTenantIdPrefix) {
    reg_->registerTenant("acme");
    reg_->increment("acme", "req_total");
    std::string out = reg_->exportTenant("acme");
    EXPECT_NE(std::string::npos, out.find("themis_acme_"));
    EXPECT_NE(std::string::npos, out.find("tenant_id=\"acme\""));
}

TEST_F(TenantMetricsNamespaceTest, ExportTenant_UnknownTenant_ReturnsEmpty) {
    EXPECT_TRUE(reg_->exportTenant("nobody").empty());
}

TEST_F(TenantMetricsNamespaceTest, ExportAll_ContainsAllTenants) {
    reg_->registerTenant("a");
    reg_->registerTenant("b");
    reg_->increment("a", "ops_total");
    reg_->increment("b", "ops_total");
    std::string all = reg_->exportAll();
    EXPECT_NE(std::string::npos, all.find("themis_a_"));
    EXPECT_NE(std::string::npos, all.find("themis_b_"));
}

TEST_F(TenantMetricsNamespaceTest, ExportAll_EmptyRegistry_ReturnsEmpty) {
    EXPECT_TRUE(reg_->exportAll().empty());
}

TEST_F(TenantMetricsNamespaceTest, ExportTenant_IncludesHistogramSumAndCount) {
    reg_->registerTenant("t");
    reg_->observeHistogram("t", "lat", 10.0);
    reg_->observeHistogram("t", "lat", 20.0);
    std::string out = reg_->exportTenant("t");
    EXPECT_NE(std::string::npos, out.find("_sum"));
    EXPECT_NE(std::string::npos, out.find("_count"));
}

// ============================================================================
// Stats
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, Stats_UnknownTenant_ReturnsZeroed) {
    auto s = reg_->stats("ghost");
    EXPECT_EQ(0, s.total_observations);
    EXPECT_EQ(0, s.dropped_observations);
    EXPECT_EQ(0u, s.active_series);
}

TEST_F(TenantMetricsNamespaceTest, AllStats_ReturnsStatsForAllTenants) {
    reg_->registerTenant("x");
    reg_->registerTenant("y");
    reg_->increment("x", "m");
    auto all = reg_->allStats();
    EXPECT_EQ(2u, all.size());
}

TEST_F(TenantMetricsNamespaceTest, ActiveSeriesTracked) {
    reg_->registerTenant("t");
    reg_->increment("t", "c1");
    reg_->setGauge("t", "g1", 1.0);
    reg_->observeHistogram("t", "h1", 1.0);
    auto s = reg_->stats("t");
    EXPECT_GE(s.active_series, 3u);
}

// ============================================================================
// Reset
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, Reset_ClearsAllMetricData) {
    reg_->registerTenant("t");
    reg_->increment("t", "c");
    reg_->setGauge("t", "g", 5.0);
    reg_->reset();
    auto s = reg_->stats("t");
    EXPECT_EQ(0, s.total_observations);
    EXPECT_EQ(0u, s.active_series);
    // Tenant itself should still exist
    EXPECT_TRUE(reg_->hasTenant("t"));
}

// ============================================================================
// Config
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, Config_RoundTrip) {
    TenantMetricsConfig cfg;
    cfg.cardinality_limit_per_tenant = 77;
    cfg.max_tenants = 5;
    cfg.strict_tenant_registration = true;
    TenantMetricsNamespace reg2(cfg);
    auto c = reg2.config();
    EXPECT_EQ(77u, c.cardinality_limit_per_tenant);
    EXPECT_EQ(5u, c.max_tenants);
    EXPECT_TRUE(c.strict_tenant_registration);
}

// ============================================================================
// Thread safety (smoke test)
// ============================================================================

TEST_F(TenantMetricsNamespaceTest, ThreadSafety_ConcurrentIncrement) {
    reg_->registerTenant("shared");
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 100;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < kOpsPerThread; ++j) {
                reg_->increment("shared", "counter",
                                {{"thread", std::to_string(i)}});
            }
        });
    }
    for (auto& t : threads) t.join();
    auto s = reg_->stats("shared");
    EXPECT_LE(s.total_observations, (int64_t)(kThreads * kOpsPerThread));
}

TEST_F(TenantMetricsNamespaceTest, ThreadSafety_ConcurrentRegisterAndExport) {
    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads * 2);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            reg_->registerTenant("t" + std::to_string(i));
            reg_->increment("t" + std::to_string(i), "ops");
        });
        threads.emplace_back([&]() {
            reg_->exportAll();
        });
    }
    for (auto& t : threads) t.join();
    // No crash or UB is the pass criterion
    SUCCEED();
}
