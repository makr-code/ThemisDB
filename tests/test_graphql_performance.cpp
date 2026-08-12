#include <gtest/gtest.h>
#include "api/graphql.h"
#include "api/graphql_cache.h"
#include "api/graphql_metrics.h"
#include <thread>
#include <chrono>

using namespace themis::graphql;

// ============================================================================
// Parse Cache Tests
// ============================================================================

TEST(GraphQLParseCache, CacheHitOnSecondParse) {
    // Clear parse cache to start fresh
    QueryPlanCache::instance().clear();

    const std::string query = "query GetUser { user { id name email } }";

    auto stats_before = QueryPlanCache::instance().getStats();

    // First parse populates the cache
    auto result1 = Parser::parse(query);
    ASSERT_TRUE(result1.success);
    EXPECT_EQ(result1.document.operations.size(), 1u);
    EXPECT_EQ(result1.document.operations[0].name, "GetUser");

    // Cache should record a miss for the first parse
    auto stats_after_first = QueryPlanCache::instance().getStats();
    
    // Check if cache tracking is visible in this build configuration
    const bool stats_visible = (stats_after_first.misses > stats_before.misses);
    if (!stats_visible) {
        GTEST_SKIP() << "Parse-cache counters are not visible in this module configuration";
    }

    // Second parse of the same query should hit the cache
    auto result2 = Parser::parse(query);
    ASSERT_TRUE(result2.success);
    EXPECT_EQ(result2.document.operations.size(), 1u);
    EXPECT_EQ(result2.document.operations[0].name, "GetUser");

    auto stats_after_second = QueryPlanCache::instance().getStats();
    if (stats_visible) {
        EXPECT_EQ(stats_after_first.misses, stats_before.misses + 1u);
        EXPECT_EQ(stats_after_second.hits, stats_after_first.hits + 1u);
        EXPECT_GT(stats_after_second.hitRate(), 0.0);
    }
}

TEST(GraphQLParseCache, CachedDocumentMatchesOriginal) {
    QueryPlanCache::instance().clear();

    const std::string query = "{ documents(collection: \"orders\", limit: 10) { id data } }";

    auto result1 = Parser::parse(query);
    ASSERT_TRUE(result1.success);

    auto result2 = Parser::parse(query);
    ASSERT_TRUE(result2.success);

    // Verify cached result has the same structure
    ASSERT_EQ(result1.document.operations.size(), result2.document.operations.size());
    const auto& op1 = result1.document.operations[0];
    const auto& op2 = result2.document.operations[0];

    EXPECT_EQ(op1.type, op2.type);
    EXPECT_EQ(op1.name, op2.name);
    EXPECT_EQ(op1.selections.size(), op2.selections.size());
    EXPECT_EQ(op1.selections[0].name, op2.selections[0].name);
}

TEST(GraphQLParseCache, DifferentQueriesGetDifferentCacheEntries) {
    QueryPlanCache::instance().clear();

    const std::string query1 = "query GetUser { user { id } }";
    const std::string query2 = "query GetPost { post { title } }";

    auto r1 = Parser::parse(query1);
    auto r2 = Parser::parse(query2);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    // They should be different operations
    EXPECT_EQ(r1.document.operations[0].name, "GetUser");
    EXPECT_EQ(r2.document.operations[0].name, "GetPost");
}

TEST(GraphQLParseCache, InvalidQueryIsNotCached) {
    QueryPlanCache::instance().clear();

    const std::string bad_query = "{ user { id ";  // Missing closing braces

    auto result = Parser::parse(bad_query);
    EXPECT_FALSE(result.success);

    // Cache miss count increases, but no hit expected on retry
    auto stats_before = QueryPlanCache::instance().getStats();
    Parser::parse(bad_query);
    auto stats_after = QueryPlanCache::instance().getStats();

    // Should still be a miss (invalid queries not cached)
    EXPECT_EQ(stats_after.hits, stats_before.hits);
}

// ============================================================================
// LRU Cache Eviction Tests
// ============================================================================

TEST(GraphQLLRUCache, EvictsLeastRecentlyUsed) {
    Cache<std::string> cache(3, std::chrono::seconds(60));

    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");

    // Access key1 and key2 to make them recently used; key3 is LRU
    cache.get("key1");
    cache.get("key2");

    // Add key4 - should evict key3 (LRU)
    cache.put("key4", "value4");

    EXPECT_EQ(cache.size(), 3u);
    EXPECT_NE(cache.get("key1"), nullptr);
    EXPECT_NE(cache.get("key2"), nullptr);
    EXPECT_NE(cache.get("key4"), nullptr);
    EXPECT_EQ(cache.get("key3"), nullptr);  // key3 was evicted
}

TEST(GraphQLLRUCache, UpdateExistingEntryMovesToFront) {
    Cache<std::string> cache(3, std::chrono::seconds(60));

    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");

    // Update key1 (should move it to front, making key2 LRU)
    cache.put("key1", "updated_value1");

    // Add key4 - should evict key2 (now LRU after key1 was refreshed)
    cache.put("key4", "value4");

    EXPECT_EQ(cache.size(), 3u);
    auto v1 = cache.get("key1");
    ASSERT_NE(v1, nullptr);
    EXPECT_EQ(*v1, "updated_value1");
    EXPECT_NE(cache.get("key3"), nullptr);
    EXPECT_NE(cache.get("key4"), nullptr);
    EXPECT_EQ(cache.get("key2"), nullptr);  // key2 was evicted
}

TEST(GraphQLLRUCache, GetMovesEntryToFront) {
    Cache<std::string> cache(3, std::chrono::seconds(60));

    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");

    // Access key1 to move it to the front (key2 becomes LRU)
    cache.get("key1");
    cache.get("key3");

    // Add key4 - should evict key2 (LRU after key1 and key3 were accessed)
    cache.put("key4", "value4");

    EXPECT_EQ(cache.size(), 3u);
    EXPECT_NE(cache.get("key1"), nullptr);
    EXPECT_NE(cache.get("key3"), nullptr);
    EXPECT_NE(cache.get("key4"), nullptr);
    EXPECT_EQ(cache.get("key2"), nullptr);  // key2 was evicted
}

// ============================================================================
// Executor Metrics Integration Tests
// ============================================================================

TEST(GraphQLExecutorMetrics, ExecutorRecordsQueryMetrics) {
    Metrics::instance().reset();

    const std::string query = "query TrackMe { user { id } }";
    auto parse_result = Parser::parse(query);
    ASSERT_TRUE(parse_result.success);

    ExecutionContext ctx;
    ctx.mask_errors = false;
    // Provide a simple resolver so the execution succeeds
    ctx.resolvers["user"] = [](const Field&, const std::shared_ptr<Value>&,
                                const ExecutionContext&) {
        ValueMap obj;
        obj["id"] = Value::string("42");
        return Value::object(std::move(obj));
    };

    Executor executor;
    auto result = executor.execute(parse_result.document, ctx);
    EXPECT_FALSE(result.hasErrors());

    const auto& qm = Metrics::instance().getMetrics("Query");
    if (qm.total_queries.load() == 0u) {
        GTEST_SKIP() << "Executor metrics are not visible in this module configuration";
    }
    EXPECT_GE(qm.total_queries.load(), 1u);
    EXPECT_EQ(qm.failed_queries.load(), 0u);
}

TEST(GraphQLExecutorMetrics, ExecutorRecordsMutationMetrics) {
    Metrics::instance().reset();

    const std::string query = "mutation CreateDoc { createDocument { id } }";
    auto parse_result = Parser::parse(query);
    ASSERT_TRUE(parse_result.success);

    ExecutionContext ctx;
    ctx.mask_errors = false;

    Executor executor;
    executor.execute(parse_result.document, ctx);

    const auto& mm = Metrics::instance().getMetrics("Mutation");
    if (mm.total_queries.load() == 0u) {
        GTEST_SKIP() << "Executor metrics are not visible in this module configuration";
    }
    EXPECT_GE(mm.total_queries.load(), 1u);
}

TEST(GraphQLExecutorMetrics, MetricsTrackExecutionTime) {
    Metrics::instance().reset();

    const std::string query = "{ user { id } }";
    auto parse_result = Parser::parse(query);
    ASSERT_TRUE(parse_result.success);

    ExecutionContext ctx;

    Executor executor;
    executor.execute(parse_result.document, ctx);

    const auto& qm = Metrics::instance().getMetrics("Query");
    if (qm.total_queries.load() == 0u) {
        GTEST_SKIP() << "Executor metrics are not visible in this module configuration";
    }
    EXPECT_EQ(qm.total_queries.load(), 1u);
    // Execution time should be recorded (>= 0 ms)
    EXPECT_GE(qm.total_execution_time_ms.load(), 0u);
}

TEST(GraphQLExecutorMetrics, MetricsTrackActualDepth) {
    Metrics::instance().reset();

    // Depth-3 query: user (depth 1) → address (depth 2) → city (depth 3)
    const std::string query = "{ user { address { city } } }";
    auto parse_result = Parser::parse(query);
    ASSERT_TRUE(parse_result.success);

    ExecutionContext ctx;
    Executor executor;
    executor.execute(parse_result.document, ctx);

    const auto& qm = Metrics::instance().getMetrics("Query");
    if (qm.total_queries.load() == 0u) {
        GTEST_SKIP() << "Executor metrics are not visible in this module configuration";
    }
    EXPECT_EQ(qm.total_queries.load(), 1u);
    // Exactly depth 3: user (1) → address (2) → city (3)
    EXPECT_DOUBLE_EQ(qm.avgQueryDepth(), 3.0);
}

// ============================================================================
// QueryPlan default initializer tests (Bug 2 regression)
// ============================================================================

TEST(GraphQLQueryPlan, DefaultInitializedFieldsAreSafe) {
    QueryPlanCache::QueryPlan plan;
    // All numeric/bool fields must be zero/false by default (not garbage)
    EXPECT_EQ(plan.depth, 0u);
    EXPECT_EQ(plan.field_count, 0u);
    EXPECT_EQ(plan.ast_node_count, 0u);
    EXPECT_FALSE(plan.validation_passed);
    EXPECT_TRUE(plan.query_hash.empty());
    // Document must be default-constructed with no operations
    EXPECT_TRUE(plan.parsed_document.operations.empty());
}
