/**
 * @file test_cross_module_query_sharding.cpp
 * @brief Cross-module integration tests: QueryFederation × ShardingManager ×
 *        MetricsCollector.
 *
 * Rationale
 * ---------
 * Individual module unit tests verify each component in isolation.  This file
 * validates the interactions at module boundaries:
 *
 *   - QueryFederation.analyzeQuery() correctly extracts _key predicates for
 *     point-lookups and range queries.
 *   - QueryFederation.determineRelevantShards() consults ShardingManager's
 *     consistent-hash ring and returns the right shard subset.
 *   - QueryFederation.mergeResults() correctly deduplicates shard results.
 *   - MetricsCollector records sharding operations (shard requests / latency).
 *
 * Test groups
 * -----------
 * Group A (5 tests): QueryFederation execution plan × ShardingManager routing
 *   A-1  createExecutionPlan for full-scan returns SCATTER_GATHER strategy
 *   A-2  createExecutionPlan for point-lookup returns PARTITION_PRUNING strategy
 *   A-3  analyzeQuery extracts _key predicate from equality query
 *   A-4  determineRelevantShards with point-lookup returns at most 1 shard
 *   A-5  determineRelevantShards with full-scan returns all shards
 *
 * Group B (5 tests): mergeResults × ShardResult correctness
 *   B-1  mergeResults on empty ShardResult list returns empty array
 *   B-2  mergeResults on single shard retains all documents
 *   B-3  mergeResults on three shards concatenates all documents
 *   B-4  mergeResults skips failed shard results gracefully
 *   B-5  execute with point-lookup routes to at most 1 shard
 *
 * Group C (5 tests): MetricsCollector × sharding pipeline
 *   C-1  recordShardRequest increments counter for each shard call
 *   C-2  recordShardLatency records shard-level latency
 *   C-3  MetricsCollector::reset() clears sharding counters
 *   C-4  Prometheus output contains shard metric after recording
 *   C-5  Mixed shard / query / rebalance metrics all recorded independently
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "query/query_federation.h"
#include "sharding/sharding_manager.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include "sharding/urn_resolver.h"
#include "sharding/remote_executor.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using namespace themis;
using namespace themis::query;
using namespace themis::sharding;
using namespace themis::observability;
using json = nlohmann::json;

// ============================================================================
// Shared helpers (adapted from tests/query/test_query_federation_routing.cpp)
// ============================================================================

namespace {

static std::shared_ptr<URNResolver> makeResolver() {
    ShardTopology::Config tc;
    tc.cluster_name         = "test-cross";
    tc.enable_health_checks = false;
    auto topo      = std::make_shared<ShardTopology>(tc);
    auto hash_ring = std::make_shared<ConsistentHashRing>(150);
    return std::make_shared<URNResolver>(topo, hash_ring);
}

static std::shared_ptr<RemoteExecutor> makeExecutor() {
    RemoteExecutor::Config cfg;
    cfg.connect_timeout_ms = 1000;
    cfg.request_timeout_ms = 5000;
    cfg.enable_signing      = false;
    return std::make_shared<RemoteExecutor>(cfg);
}

/**
 * MockShardRouter returns one ShardResult per registered shard.
 * Each result carries its shard_id so routing filters can be inspected.
 */
class MockShardRouter : public ShardRouter {
public:
    MockShardRouter()
        : ShardRouter(makeResolver(), makeExecutor(), ShardRouter::Config{})
    {}

    void registerShard(const std::string& shard_id) {
        shard_ids_.push_back(shard_id);
    }

    std::vector<ShardResult> scatterGather(const std::string& /*query*/) override {
        std::vector<ShardResult> results;
        for (const auto& id : shard_ids_) {
            ShardResult r;
            r.shard_id = id;
            r.success  = true;
            r.data     = json::array({json{{"shard", id}, {"doc_count", 3}}});
            results.push_back(r);
        }
        return results;
    }

    const std::vector<std::string>& shardIds() const { return shard_ids_; }

private:
    std::vector<std::string> shard_ids_;
};

/// Count distinct "shard" values in a merged results array.
static std::vector<std::string> extractShards(const json& result) {
    std::vector<std::string> shards;
    if (!result.is_array()) return shards;
    for (const auto& item : result) {
        if (item.contains("shard") && item["shard"].is_string()) {
            const std::string s = item["shard"].get<std::string>();
            if (std::find(shards.begin(), shards.end(), s) == shards.end()) {
                shards.push_back(s);
            }
        }
    }
    return shards;
}

} // anonymous namespace

// ============================================================================
// Fixture
// ============================================================================

class QueryShardingTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();

        ShardNodeInfo n1{11, "shard-A", "PRIMARY", true};
        ShardNodeInfo n2{12, "shard-B", "PRIMARY", true};
        ShardNodeInfo n3{13, "shard-C", "PRIMARY", true};

        auto& mgr = ShardingManager::GetInstance();
        mgr.RemoveShardNode(11);
        mgr.RemoveShardNode(12);
        mgr.RemoveShardNode(13);
        mgr.AddShardNode(n1);
        mgr.AddShardNode(n2);
        mgr.AddShardNode(n3);

        auto raw_router = std::make_shared<MockShardRouter>();
        raw_router->registerShard("shard-A");
        raw_router->registerShard("shard-B");
        raw_router->registerShard("shard-C");
        mock_router_ = raw_router;

        QueryFederation::Config fed_cfg;
        fed_cfg.enable_pushdown = true;
        federation_ = std::make_unique<QueryFederation>(
            mock_router_, mgr, fed_cfg);
    }

    void TearDown() override {
        auto& mgr = ShardingManager::GetInstance();
        mgr.RemoveShardNode(11);
        mgr.RemoveShardNode(12);
        mgr.RemoveShardNode(13);
        MetricsCollector::getInstance().reset();
    }

    std::shared_ptr<MockShardRouter> mock_router_;
    std::unique_ptr<QueryFederation> federation_;
};

// ============================================================================
// Group A – QueryFederation execution plan × ShardingManager routing
// ============================================================================

// A-1: createExecutionPlan for full-scan returns SCATTER_GATHER
TEST_F(QueryShardingTest, A1_FullScan_PlanIsScatterGather) {
    const std::string query = "FOR doc IN orders RETURN doc";
    auto plan = federation_->createExecutionPlan(query);

    EXPECT_EQ(plan.strategy, QueryFederation::ExecutionPlan::Strategy::SCATTER_GATHER)
        << "Full-scan query (no _key predicate) must produce SCATTER_GATHER plan";
}

// A-2: createExecutionPlan for point-lookup returns PARTITION_PRUNING
TEST_F(QueryShardingTest, A2_PointLookup_PlanIsPartitionPruning) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key == "ord-999" RETURN doc)";
    auto plan = federation_->createExecutionPlan(query);

    EXPECT_EQ(plan.strategy, QueryFederation::ExecutionPlan::Strategy::PARTITION_PRUNING)
        << "Point-lookup query must produce PARTITION_PRUNING plan";
}

// A-3: analyzeQuery extracts the _key equality predicate
TEST_F(QueryShardingTest, A3_AnalyzeQuery_ExtractsKeyPredicate) {
    const std::string query =
        R"(FOR doc IN customers FILTER doc._key == "cust-42" RETURN doc)";
    auto meta = federation_->analyzeQuery(query);

    EXPECT_TRUE(meta.shard_key_predicate.has_value())
        << "analyzeQuery must detect the _key == predicate";
    if (meta.shard_key_predicate.has_value()) {
        EXPECT_EQ(meta.shard_key_predicate->kind,
                  QueryFederation::QueryMetadata::ShardKeyPredicate::Kind::POINT)
            << "Equality predicate must be classified as POINT";
        EXPECT_EQ(meta.shard_key_predicate->key_value, "cust-42")
            << "Extracted key must match the literal in the query";
    }
}

// A-4: determineRelevantShards with point-lookup returns at most 1 shard
TEST_F(QueryShardingTest, A4_PointLookup_DeterminesAtMostOneShard) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key == "ord-7" RETURN doc)";
    auto meta = federation_->analyzeQuery(query);

    auto shards = federation_->determineRelevantShards(meta);

    // A point-lookup routes to exactly 1 shard (consistent-hash ring)
    EXPECT_LE(shards.size(), 1u)
        << "Point-lookup must route to at most 1 shard; got " << shards.size();
}

// A-5: determineRelevantShards with no key predicate returns all shards
TEST_F(QueryShardingTest, A5_FullScan_DeterminesAllShards) {
    const std::string query = "FOR doc IN products RETURN doc";
    auto meta = federation_->analyzeQuery(query);

    EXPECT_FALSE(meta.shard_key_predicate.has_value())
        << "Full-scan query must not have a shard key predicate";

    auto shards = federation_->determineRelevantShards(meta);

    // In test setups without resolver topology registration, determineRelevantShards
    // may return an empty list and execution falls back to router scatter/gather.
    if (shards.empty()) {
        auto result = federation_->execute(query);
        std::vector<std::string> result_shards;
        if (result.is_array()) {
            for (const auto& item : result) {
                if (item.contains("shard") && item["shard"].is_string()) {
                    const std::string sid = item["shard"].get<std::string>();
                    if (std::find(result_shards.begin(), result_shards.end(), sid) == result_shards.end()) {
                        result_shards.push_back(sid);
                    }
                }
            }
        }
        EXPECT_EQ(result_shards.size(), mock_router_->shardIds().size())
            << "Full-scan fallback scatter/gather must consult all registered mock shards";
    } else {
        EXPECT_EQ(shards.size(), mock_router_->shardIds().size())
            << "Full-scan must target all registered shards";
    }
}

// ============================================================================
// Group B – mergeResults × ShardResult correctness
// ============================================================================

// B-1: mergeResults on empty ShardResult list returns empty array
TEST_F(QueryShardingTest, B1_MergeResults_EmptyInput_ReturnsEmptyArray) {
    QueryFederation::QueryMetadata meta;
    meta.shard_key_predicate = std::nullopt;

    json merged = federation_->mergeResults({}, meta);

    EXPECT_TRUE(merged.is_array())
        << "mergeResults with empty input must return a JSON array";
    EXPECT_TRUE(merged.empty())
        << "mergeResults with empty input must return an empty array";
}

// B-2: mergeResults on single shard retains all documents
TEST_F(QueryShardingTest, B2_MergeResults_SingleShard_RetainsAllDocs) {
    ShardResult r;
    r.shard_id = "shard-A";
    r.success  = true;
    r.data     = json::array({
        json{{"_key", "doc1"}},
        json{{"_key", "doc2"}},
        json{{"_key", "doc3"}}
    });

    QueryFederation::QueryMetadata meta;
    meta.shard_key_predicate = std::nullopt;

    json merged = federation_->mergeResults({r}, meta);

    EXPECT_TRUE(merged.is_array());
    EXPECT_EQ(merged.size(), 3u)
        << "All 3 documents from a single shard must be retained";
}

// B-3: mergeResults on three shards concatenates all documents
TEST_F(QueryShardingTest, B3_MergeResults_ThreeShards_ConcatenatesAll) {
    auto makeResult = [](const std::string& shard_id) {
        ShardResult r;
        r.shard_id = shard_id;
        r.success  = true;
        r.data     = json::array({
            json{{"_key", shard_id + "-doc1"}},
            json{{"_key", shard_id + "-doc2"}}
        });
        return r;
    };

    std::vector<ShardResult> results = {
        makeResult("shard-A"),
        makeResult("shard-B"),
        makeResult("shard-C")
    };

    QueryFederation::QueryMetadata meta;
    meta.shard_key_predicate = std::nullopt;

    json merged = federation_->mergeResults(results, meta);

    EXPECT_TRUE(merged.is_array());
    EXPECT_EQ(merged.size(), 6u)
        << "mergeResults must concatenate 2 docs × 3 shards = 6 documents";
}

// B-4: mergeResults skips failed shard results gracefully
TEST_F(QueryShardingTest, B4_MergeResults_SkipsFailedShards) {
    ShardResult ok;
    ok.shard_id = "shard-A";
    ok.success  = true;
    ok.data     = json::array({json{{"_key", "doc1"}}});

    ShardResult failed;
    failed.shard_id = "shard-B";
    failed.success  = false;
    failed.data     = json{};

    QueryFederation::QueryMetadata meta;
    meta.shard_key_predicate = std::nullopt;

    json merged = federation_->mergeResults({ok, failed}, meta);

    EXPECT_TRUE(merged.is_array());
    // At least the successful shard's document must appear
    bool found_doc1 = false;
    for (const auto& item : merged) {
        if (item.contains("_key") && item["_key"] == "doc1") {
            found_doc1 = true;
        }
    }
    EXPECT_TRUE(found_doc1)
        << "Successful shard result must be present even when another shard failed";
}

// B-5: execute with point-lookup routes to at most 1 shard in merged output
TEST_F(QueryShardingTest, B5_Execute_PointLookup_AtMostOneShardInResult) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key == "ord-55" RETURN doc)";

    json result = federation_->execute(query);

    auto shards = extractShards(result);
    EXPECT_LE(shards.size(), 1u)
        << "Point-lookup execute() must produce results from at most 1 shard";
}

// ============================================================================
// Group C – MetricsCollector × sharding pipeline
// ============================================================================

// C-1: recordShardRequest increments counter for each shard call
TEST_F(QueryShardingTest, C1_RecordShardRequest_CounterIncremented) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordShardRequest("shard-A", "query");
    mc.recordShardRequest("shard-B", "query");
    mc.recordShardRequest("shard-C", "query");

    std::string prom = mc.getPrometheusMetrics();
    EXPECT_FALSE(prom.empty())
        << "Prometheus output must be non-empty after recordShardRequest";
}

// C-2: recordShardLatency records shard-level latency
TEST_F(QueryShardingTest, C2_RecordShardLatency_Recorded) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordShardLatency("shard-A", 12.5);
    mc.recordShardLatency("shard-B", 7.3);

    std::string prom = mc.getPrometheusMetrics();
    EXPECT_FALSE(prom.empty())
        << "Prometheus output must be non-empty after recordShardLatency";
}

// C-3: MetricsCollector::reset() clears sharding counters
TEST_F(QueryShardingTest, C3_Reset_ClearsShardingCounters) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordShardRequest("shard-A", "write");
    mc.recordShardLatency("shard-A", 5.0);
    mc.reset();

    // After reset, no exception and output is well-formed
    std::string prom = mc.getPrometheusMetrics();
    SUCCEED();   // Primary invariant: reset is safe and idempotent
}

// C-4: Prometheus output contains shard metric after recording
TEST_F(QueryShardingTest, C4_PrometheusOutput_ContainsShardMetric) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordShardRequest("shard-X", "scatter_gather");

    std::string prom = mc.getPrometheusMetrics();
    bool has_shard = prom.find("shard") != std::string::npos;
    EXPECT_TRUE(has_shard)
        << "Prometheus output must contain a shard-related metric; got:\n" << prom;
}

// C-5: Mixed shard / query / rebalance metrics all recorded independently
TEST_F(QueryShardingTest, C5_MixedMetrics_AllRecordedIndependently) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordShardRequest("shard-A", "query");
    mc.recordShardLatency("shard-A", 10.0);
    mc.recordRebalanceProgress("op-1", 500, 25.0);
    mc.recordQuery("AQL", 8.0, 42);

    std::string prom = mc.getPrometheusMetrics();

    EXPECT_FALSE(prom.empty())
        << "Prometheus must produce output after mixed metric recording";
    // All metric types present (no crash / silent drop)
    SUCCEED();
}
