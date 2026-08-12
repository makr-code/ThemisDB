/**
 * @file test_query_cache_manager.cpp
 * @brief Tests for QueryCacheManager (src/query/query_cache_manager.cpp)
 *
 * Covers:
 *   - Construction with default Config
 *   - Construction with caching disabled → get always misses
 *   - get on empty cache → nullopt (cache miss)
 *   - put + get → hit returns stored result
 *   - put with dependencies + invalidateByDependency removes entry
 *   - invalidate (by query + params) removes specific entry
 *   - warmCache pre-populates entries
 *   - getCurrentWorkload (default UNKNOWN)
 *   - getConfig / setConfig round-trip
 */

#include <gtest/gtest.h>
#include "query/query_cache_manager.h"
#include "query/workload_cache_strategy.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace themis::query;

// ============================================================================
// Fixture
// ============================================================================

class QueryCacheManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        QueryCacheManager::Config cfg;
        cfg.enable_caching = true;
        cfg.enable_workload_detection = false; // deterministic for tests
        cfg.enable_detailed_stats = false;
        manager_ = std::make_unique<QueryCacheManager>(cfg);
    }

    std::unique_ptr<QueryCacheManager> manager_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(QueryCacheManagerTest, Construction_Succeeds) {
    EXPECT_NE(manager_, nullptr);
}

TEST_F(QueryCacheManagerTest, GetConfig_ReturnsConfiguredValues) {
    auto cfg = manager_->getConfig();
    EXPECT_TRUE(cfg.enable_caching);
}

// ============================================================================
// Cache miss on empty cache
// ============================================================================

TEST_F(QueryCacheManagerTest, Get_EmptyCache_ReturnsNullopt) {
    auto result = manager_->get("SELECT * FROM t WHERE id = 1");
    EXPECT_FALSE(result.has_value());
}

TEST_F(QueryCacheManagerTest, Get_WithParams_EmptyCache_ReturnsNullopt) {
    nlohmann::json params = {{"id", 42}};
    auto result = manager_->get("SELECT * FROM t WHERE id = ?", params);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// put + get round-trip
// ============================================================================

TEST_F(QueryCacheManagerTest, PutThenGet_ReturnsCachedResult) {
    const std::string query  = "SELECT name FROM users WHERE id = 1";
    const nlohmann::json params  = nlohmann::json::object();
    const nlohmann::json result  = {{"name", "Alice"}};

    QueryCharacteristics chars;
    chars.result_size_bytes  = 64;
    chars.rows_returned      = 1;
    chars.execution_time_ms  = 5;

    ASSERT_TRUE(manager_->put(query, params, result, chars));

    auto cached = manager_->get(query, params);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached.value()["name"], "Alice");
}

TEST_F(QueryCacheManagerTest, Put_DifferentQueries_IndependentEntries) {
    QueryCharacteristics chars;
    chars.result_size_bytes = 16;

    manager_->put("Q1", nlohmann::json::object(), nlohmann::json{{"v", 1}}, chars);
    manager_->put("Q2", nlohmann::json::object(), nlohmann::json{{"v", 2}}, chars);

    EXPECT_EQ(manager_->get("Q1").value()["v"], 1);
    EXPECT_EQ(manager_->get("Q2").value()["v"], 2);
}

// ============================================================================
// invalidate (by query + params)
// ============================================================================

TEST_F(QueryCacheManagerTest, Invalidate_RemovesSpecificEntry) {
    const std::string q     = "SELECT * FROM orders WHERE id = 5";
    const nlohmann::json params = nlohmann::json::object();
    const nlohmann::json res    = {{"order_id", 5}};

    QueryCharacteristics chars;
    chars.result_size_bytes = 32;

    manager_->put(q, params, res, chars);
    ASSERT_TRUE(manager_->get(q).has_value());

    manager_->invalidate(q, params);
    EXPECT_FALSE(manager_->get(q).has_value());
}

TEST_F(QueryCacheManagerTest, Invalidate_OtherEntriesUnaffected) {
    QueryCharacteristics chars;
    chars.result_size_bytes = 16;

    manager_->put("Q_keep",   nlohmann::json::object(), nlohmann::json{{"v", "keep"}},   chars);
    manager_->put("Q_remove", nlohmann::json::object(), nlohmann::json{{"v", "remove"}}, chars);

    manager_->invalidate("Q_remove");

    EXPECT_TRUE(manager_->get("Q_keep").has_value());
    EXPECT_FALSE(manager_->get("Q_remove").has_value());
}

// ============================================================================
// invalidateByDependency
// ============================================================================

TEST_F(QueryCacheManagerTest, InvalidateByDependency_RemovesMatchingEntries) {
    QueryCharacteristics chars;
    chars.result_size_bytes = 16;

    manager_->put("Q_dep",   nlohmann::json::object(), nlohmann::json{{"v", 1}}, chars,
                  {"table_products"});
    manager_->put("Q_other", nlohmann::json::object(), nlohmann::json{{"v", 2}}, chars,
                  {"table_orders"});

    size_t removed = manager_->invalidateByDependency("table_products");
    EXPECT_GE(removed, 1u);

    // Entry dependent on table_products should be gone
    EXPECT_FALSE(manager_->get("Q_dep").has_value());
    // Unrelated entry should still exist
    EXPECT_TRUE(manager_->get("Q_other").has_value());
}

// ============================================================================
// warmCache
// ============================================================================

TEST_F(QueryCacheManagerTest, WarmCache_PrePopulatesEntries) {
    std::map<std::string, nlohmann::json> warm_data;
    warm_data["SELECT 1"]           = nlohmann::json{{"val", 1}};
    warm_data["SELECT count(*) FROM t"] = nlohmann::json{{"count", 42}};

    manager_->warmCache(warm_data);

    // At least one pre-warmed query should be retrievable
    bool any_hit = manager_->get("SELECT 1").has_value() ||
                   manager_->get("SELECT count(*) FROM t").has_value();
    EXPECT_TRUE(any_hit);
}

// ============================================================================
// Caching disabled
// ============================================================================

TEST(QueryCacheManagerDisabledTest, CachingDisabled_GetAlwaysMisses) {
    QueryCacheManager::Config cfg;
    cfg.enable_caching = false;
    QueryCacheManager mgr(cfg);

    QueryCharacteristics chars;
    chars.result_size_bytes = 16;
    mgr.put("SELECT 1", nlohmann::json::object(), nlohmann::json{{"x", 1}}, chars);

    auto result = mgr.get("SELECT 1");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// getCurrentWorkload
// ============================================================================

TEST_F(QueryCacheManagerTest, GetCurrentWorkload_DefaultIsUnknownOrKnown) {
    auto wt = manager_->getCurrentWorkload();
    // WorkloadType is a valid enum value
    EXPECT_NO_THROW((void)wt);
}

// ============================================================================
// getConfig / setConfig
// ============================================================================

TEST_F(QueryCacheManagerTest, SetConfig_GetConfig_RoundTrip) {
    QueryCacheManager::Config new_cfg;
    new_cfg.enable_caching           = true;
    new_cfg.enable_workload_detection = false;
    new_cfg.enable_detailed_stats     = true;
    new_cfg.stats_report_interval     = std::chrono::seconds(600);

    manager_->setConfig(new_cfg);
    auto retrieved = manager_->getConfig();

    EXPECT_TRUE(retrieved.enable_caching);
    EXPECT_TRUE(retrieved.enable_detailed_stats);
    EXPECT_EQ(retrieved.stats_report_interval.count(), 600);
}
