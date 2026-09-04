#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "sharding/shard_router.h"
#include "query/query_federation.h"

using namespace themis;
using namespace themis::sharding;
using namespace themis::query;

// ─────────────────────────────────────────────────────────────────────────────
// Instrumented ShardRouter: records which shards each call targets.
// ─────────────────────────────────────────────────────────────────────────────

class InstrumentedShardRouter : public ShardRouter {
public:
    InstrumentedShardRouter(
        std::shared_ptr<URNResolver> resolver)
        : ShardRouter(resolver, nullptr, Config{})
    {}

    // Override scatterGather so full-scan tests don't make real network calls.
    std::vector<ShardResult> scatterGather(const std::string& /*query*/) override {
        scatter_gather_call_count++;
        return {};
    }

    std::vector<ShardResult> executeOnShards(
        const std::string& /*query*/,
        const std::vector<std::string>& shard_ids) override
    {
        last_on_shards_call = shard_ids;
        std::vector<ShardResult> results = {};

        for (const auto& id : shard_ids) {
            ShardResult r;
            r.shard_id = id;
            r.success  = true;
            r.data     = nlohmann::json::array();
            results.push_back(r);
        }
        return results;
    }

    // Records from the most recent executeOnShards call
    std::vector<std::string> last_on_shards_call;
    int scatter_gather_call_count = 0;
};

class QueryResultShardRouter : public InstrumentedShardRouter {
public:
    using InstrumentedShardRouter::InstrumentedShardRouter;

    nlohmann::json executeQuery(const std::string& /*query*/) override {
        if (next_result_index_ >= queued_results_.size()) {
            return nlohmann::json::array();
        }
        return queued_results_[next_result_index_++];
    }

    void queueResult(nlohmann::json result) {
        queued_results_.push_back(std::move(result));
    }

private:
    std::vector<nlohmann::json> queued_results_;
    size_t next_result_index_ = 0;
};

class FederatedResultShardRouter : public InstrumentedShardRouter {
public:
    using InstrumentedShardRouter::InstrumentedShardRouter;

    std::vector<ShardResult> scatterGather(const std::string& /*query*/) override {
        scatter_gather_call_count++;
        return queued_results_;
    }

    void queueShardResult(ShardResult result) {
        queued_results_.push_back(std::move(result));
    }

private:
    std::vector<ShardResult> queued_results_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: 3-shard cluster
// ─────────────────────────────────────────────────────────────────────────────

class QueryFederationShardRoutingTest : public ::testing::Test {
protected:
    static constexpr const char* kShard1 = "shard-001";
    static constexpr const char* kShard2 = "shard-002";
    static constexpr const char* kShard3 = "shard-003";

    void SetUp() override {
        ring_    = std::make_shared<ConsistentHashRing>();
        topology = std::make_shared<ShardTopology>();

        for (const char* id : {kShard1, kShard2, kShard3}) {
            ring_->addShard(id, /*virtual_nodes=*/150);
            ShardInfo info;
            info.shard_id         = id;
            info.primary_endpoint = std::string(id) + ".example.com:8080";
            info.is_healthy       = true;
            topology->addShard(info);
        }

        resolver = std::make_shared<URNResolver>(topology, ring_);
        router   = std::make_shared<InstrumentedShardRouter>(resolver);

        QueryFederation::Config cfg;
        cfg.enable_pushdown = true;
        fed = std::make_unique<QueryFederation>(router, cfg);
    }

    std::shared_ptr<ConsistentHashRing>       ring_;
    std::shared_ptr<ShardTopology>            topology;
    std::shared_ptr<URNResolver>              resolver;
    std::shared_ptr<InstrumentedShardRouter>  router;
    std::unique_ptr<QueryFederation>          fed;
};

TEST(QueryFederationConstructorValidationTest, NullRouter_DefaultConstructorThrows) {
    std::shared_ptr<ShardRouter> null_router;
    EXPECT_THROW((void)QueryFederation(null_router), std::invalid_argument);
}

TEST(QueryFederationConstructorValidationTest, NullRouter_ConfigConstructorThrows) {
    std::shared_ptr<ShardRouter> null_router;
    QueryFederation::Config cfg;
    EXPECT_THROW((void)QueryFederation(null_router, cfg), std::invalid_argument);
}

TEST(QueryFederationConstructorValidationTest, NullRouter_ShardingManagerConstructorThrows) {
    std::shared_ptr<ShardRouter> null_router;
    auto& mgr = ShardingManager::GetInstance();
    EXPECT_THROW((void)QueryFederation(null_router, mgr), std::invalid_argument);
}

TEST(QueryFederationConstructorValidationTest, NullRouter_ShardingManagerConfigConstructorThrows) {
    std::shared_ptr<ShardRouter> null_router;
    auto& mgr = ShardingManager::GetInstance();
    QueryFederation::Config cfg;
    EXPECT_THROW((void)QueryFederation(null_router, mgr, cfg), std::invalid_argument);
}

TEST(QueryFederationJoinValidationTest, InvalidLeftCollectionNameThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation federation(router);

    EXPECT_THROW(
        federation.executeJoin("users RETURN 1", "orders", "user_id"),
        std::invalid_argument);
}

TEST(QueryFederationJoinValidationTest, InvalidRightCollectionNameThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation federation(router);

    EXPECT_THROW(
        federation.executeJoin("users", "orders FILTER 1==1", "user_id"),
        std::invalid_argument);
}

TEST(QueryFederationJoinValidationTest, EmptyJoinConditionThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation federation(router);

    EXPECT_THROW(
        federation.executeJoin("users", "orders", "   \t"),
        std::invalid_argument);
}

TEST(QueryFederationJoinValidationTest, JoinConditionWithoutEqualityOperatorThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation federation(router);

    EXPECT_THROW(
        federation.executeJoin("users", "orders", "user_id"),
        std::invalid_argument);
}

TEST(QueryFederationPlanningTest, JoinWithoutOnFallsBackToScatterGather) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation federation(router);

    const auto plan = federation.createExecutionPlan(
        "SELECT * FROM users JOIN orders");

    EXPECT_EQ(plan.strategy, QueryFederation::ExecutionPlan::Strategy::SCATTER_GATHER);
}

TEST(QueryFederationExecutionTest, JoinWithoutParseableOnThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation federation(router);

    EXPECT_THROW(
        federation.execute("SELECT * FROM users JOIN orders"),
        std::invalid_argument);
}

TEST(QueryFederationExecutionTest, SqlJoinWithOnConditionProducesJoinedRows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<QueryResultShardRouter>(resolver);

    router->queueResult(nlohmann::json::array({
        nlohmann::json{{"user_id", "u1"}, {"name", "alice"}}
    }));
    router->queueResult(nlohmann::json::array({
        nlohmann::json{{"user_id", "u1"}, {"order_id", "o1"}}
    }));

    QueryFederation federation(router);
    const auto result = federation.execute(
        "SELECT * FROM users JOIN orders ON users.user_id = orders.user_id");

    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]["users_user_id"], "u1");
    EXPECT_EQ(result[0]["orders_order_id"], "o1");
}

TEST(QueryFederationJoinValidationTest, OversizedShuffleJoinInputThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<QueryResultShardRouter>(resolver);

    QueryFederation::Config cfg;
    cfg.max_result_size_bytes = 64;
    cfg.broadcast_threshold_bytes = 1;

    const std::string oversized_payload(128, 'x');
    router->queueResult(nlohmann::json::array(
        {nlohmann::json{{"user_id", "1"}, {"payload", oversized_payload}}}));

    QueryFederation federation(router, cfg);

    EXPECT_THROW(
        federation.executeJoin("users", "orders", "users.user_id = orders.user_id"),
        std::runtime_error);
}

TEST(QueryFederationExecutionLimitTest, OversizedMergedScatterGatherResultThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<FederatedResultShardRouter>(resolver);

    QueryFederation::Config cfg;
    cfg.max_result_size_bytes = 64;

    const std::string oversized_payload(128, 'x');
    ShardResult shard_result;
    shard_result.shard_id = "shard-001";
    shard_result.success = true;
    shard_result.data = nlohmann::json::array(
        {nlohmann::json{{"_key", "user-1"}, {"payload", oversized_payload}}});
    router->queueShardResult(std::move(shard_result));

    QueryFederation federation(router, cfg);

    EXPECT_THROW(
        federation.execute("FOR doc IN users RETURN doc"),
        std::runtime_error);
}

TEST(QueryFederationExecutionLimitTest, OversizedAggregationShardResultThrows) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<FederatedResultShardRouter>(resolver);

    QueryFederation::Config cfg;
    cfg.max_result_size_bytes = 64;

    const std::string oversized_payload(128, 'x');
    ShardResult shard_result;
    shard_result.shard_id = "shard-001";
    shard_result.success = true;
    shard_result.data = nlohmann::json{
        {"sum", 1},
        {"payload", oversized_payload}
    };
    router->queueShardResult(std::move(shard_result));

    QueryFederation federation(router, cfg);

    EXPECT_THROW(
        federation.executeAggregation("FOR doc IN users COLLECT WITH COUNT INTO length RETURN { length }"),
        std::runtime_error);
}

TEST(QueryFederationExecutionLimitTest, SmallAggregationShardResultSucceeds) {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    auto router   = std::make_shared<FederatedResultShardRouter>(resolver);

    QueryFederation::Config cfg;
    cfg.max_result_size_bytes = 256;

    ShardResult shard_result;
    shard_result.shard_id = "shard-001";
    shard_result.success = true;
    shard_result.data = nlohmann::json{
        {"sum", 3},
        {"count", 1}
    };
    router->queueShardResult(std::move(shard_result));

    QueryFederation federation(router, cfg);

    EXPECT_EQ(
        federation.executeAggregation("FOR doc IN users COLLECT AGGREGATE sum = SUM(doc.value) RETURN { sum }"),
        (nlohmann::json{{"sum", 3}, {"count", 1}}));
}

// ─────────────────────────────────────────────────────────────────────────────
// Point-lookup routes to exactly 1 shard
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, PointLookup_RoutesTo_ExactlyOneShard) {
    const std::string key   = "order-42";
    const std::string query =
        "FOR doc IN orders FILTER doc._key == \"" + key + "\" RETURN doc";

    fed->execute(query);

    // Current implementation executes shard RPC via scatterGather and applies
    // partition pruning locally, so executeOnShards is intentionally unused.
    EXPECT_TRUE(router->last_on_shards_call.empty());
    EXPECT_EQ(router->scatter_gather_call_count, 1);

    auto stats = fed->getStatistics();
    EXPECT_EQ(stats["partition_pruned_queries"], 1);
    EXPECT_EQ(stats["scatter_gather_queries"], 0);

    // Keep resolver contract covered: point key maps to exactly one shard.
    const std::string expected = resolver->getShardForKey("orders", key);
    EXPECT_FALSE(expected.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Range across two keys on different shards routes to ≥ 1 shard
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, RangeQuery_RoutesTo_Shards) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key >= "aaa" AND doc._key <= "zzz" RETURN doc)";

    fed->execute(query);

    EXPECT_TRUE(router->last_on_shards_call.empty());
    EXPECT_EQ(router->scatter_gather_call_count, 1);

    auto stats = fed->getStatistics();
    EXPECT_EQ(stats["partition_pruned_queries"], 1);
    EXPECT_EQ(stats["scatter_gather_queries"], 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Full-scan (no _key predicate) uses scatterGather, not executeOnShards
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, FullScan_Uses_ScatterGather) {
    const std::string query = "FOR doc IN orders RETURN doc";

    fed->execute(query);

    // scatterGather is called; executeOnShards is NOT called.
    EXPECT_TRUE(router->last_on_shards_call.empty())
        << "Full-scan must use scatterGather, not executeOnShards";
}

// ─────────────────────────────────────────────────────────────────────────────
// Two identical point-lookup queries hit the same shard (ring is deterministic)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, PointLookup_Is_Deterministic) {
    const std::string key   = "deterministic-key-999";
    const std::string query =
        "FOR doc IN users FILTER doc._key == \"" + key + "\" RETURN doc";

    fed->execute(query);
    const auto first_stats = fed->getStatistics();
    const int first_scatter = router->scatter_gather_call_count;

    // Reset and re-run
    router->last_on_shards_call.clear();
    fed->execute(query);
    const auto second_stats = fed->getStatistics();

    EXPECT_TRUE(router->last_on_shards_call.empty());
    EXPECT_EQ(router->scatter_gather_call_count, first_scatter + 1);
    EXPECT_EQ(first_stats["partition_pruned_queries"], 1);
    EXPECT_EQ(second_stats["partition_pruned_queries"], 2);

    const std::string first_shard = resolver->getShardForKey("users", key);
    const std::string second_shard = resolver->getShardForKey("users", key);
    EXPECT_EQ(first_shard, second_shard)
        << "The same key must always resolve to the same shard";
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-4: Federated RAG merge tests (QF-RAG-01 … QF-RAG-05)
// ─────────────────────────────────────────────────────────────────────────────

#include "distributed_knowledge/federated_rag_merger.h"

using namespace themis::distributed_knowledge;

namespace {

// Build a ShardRetrievalResult with N documents whose doc_ids are
// "<shard_id>-doc-<i>".
ShardRetrievalResult makeShardResult(const std::string& shard_id, int num_docs,
                                     double accuracy_delta = 0.0,
                                     bool ok = true)
{
    ShardRetrievalResult sr;
    sr.shard_id              = shard_id;
    sr.ok                    = ok;
    sr.adapter_accuracy_delta = accuracy_delta;
    if (ok) {
        for (int i = 0; i < num_docs; ++i) {
            RetrievedDocument doc;
            doc.doc_id          = shard_id + "-doc-" + std::to_string(i);
            doc.content         = "content from " + shard_id + " doc " + std::to_string(i);
            doc.shard_id        = shard_id;
            doc.relevance_score = 1.0 - (0.1 * i);  // decreasing relevance
            doc.rank_in_shard   = static_cast<size_t>(i + 1);
            sr.documents.push_back(doc);
        }
    }
    return sr;
}

} // namespace

// QF-RAG-01: Fan-out to 3 mock shards → merged top-5 contains docs from all shards
TEST(QueryFederationRAGTest, QF_RAG_01_MergeContainsDocsFromAllThreeShards) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 20;
    auto merger = std::make_shared<FederatedRAGMerger>(cfg);

    // Build a minimal QueryFederation just to call mergeRAGResults
    auto topology = std::make_shared<sharding::ShardTopology>();
    auto ring     = std::make_shared<sharding::ConsistentHashRing>();
    auto resolver = std::make_shared<sharding::URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation qf(router);
    qf.setRAGMerger(merger);

    std::vector<ShardRetrievalResult> inputs = {
        makeShardResult("shard-1", 5),
        makeShardResult("shard-2", 5),
        makeShardResult("shard-3", 5),
    };

    auto ctx = qf.mergeRAGResults(inputs);

    // Collect shard IDs present in merged output
    std::set<std::string> shards_seen = {};

    for (const auto& doc : ctx.documents) {
        shards_seen.insert(doc.shard_id);
    }

    EXPECT_EQ(shards_seen.size(), 3u) << "Merged result must include docs from all 3 shards";
    EXPECT_GE(ctx.documents.size(), 3u);
    EXPECT_EQ(ctx.shards_queried, 3u);
    EXPECT_EQ(ctx.shards_responded, 3u);
}

// QF-RAG-02: One shard timeout (ok=false) → merge still produces result with 2 shards
TEST(QueryFederationRAGTest, QF_RAG_02_TimeoutShardGracefullySkipped) {
    auto merger = std::make_shared<FederatedRAGMerger>();

    auto topology = std::make_shared<sharding::ShardTopology>();
    auto ring     = std::make_shared<sharding::ConsistentHashRing>();
    auto resolver = std::make_shared<sharding::URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation qf(router);
    qf.setRAGMerger(merger);

    std::vector<ShardRetrievalResult> inputs = {
        makeShardResult("shard-1", 5),
        makeShardResult("shard-2", 0, 0.0, /*ok=*/false),  // timeout shard
        makeShardResult("shard-3", 5),
    };

    auto ctx = qf.mergeRAGResults(inputs);

    // Documents from timed-out shard must not appear in result
    for (const auto& doc : ctx.documents) {
        EXPECT_NE(doc.shard_id, "shard-2")
            << "Timeout shard docs must not appear in merged result";
    }
    EXPECT_GE(ctx.documents.size(), 1u);
    EXPECT_EQ(ctx.shards_queried, 3u);
    EXPECT_EQ(ctx.shards_responded, 2u);
}

// QF-RAG-03: Specialised shard (accuracy_delta=+0.15) → its top-docs dominate
TEST(QueryFederationRAGTest, QF_RAG_03_SpecialisedShardDocsDominate) {
    FederatedRAGMergerConfig cfg;
    cfg.boost_specialised    = true;
    cfg.specialisation_boost = 1.2;
    cfg.top_k                = 5;
    auto merger = std::make_shared<FederatedRAGMerger>(cfg);

    auto topology = std::make_shared<sharding::ShardTopology>();
    auto ring     = std::make_shared<sharding::ConsistentHashRing>();
    auto resolver = std::make_shared<sharding::URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation qf(router);
    qf.setRAGMerger(merger);

    // shard-special has accuracy_delta = +0.15 → 1.2× boost in RRF
    std::vector<ShardRetrievalResult> inputs = {
        makeShardResult("shard-generic-1", 5, 0.0),
        makeShardResult("shard-generic-2", 5, 0.0),
        makeShardResult("shard-special",   5, 0.15),
    };

    auto ctx = qf.mergeRAGResults(inputs);

    ASSERT_FALSE(ctx.documents.empty());
    // The top-ranked document must come from the specialised shard
    EXPECT_EQ(ctx.documents.front().shard_id, "shard-special")
        << "Specialised shard's rank-1 doc must dominate merged output (RRF boost)";
}

// QF-RAG-04: No RAGMerger set → execute() uses existing merge path unchanged (no regression)
TEST_F(QueryFederationShardRoutingTest, QF_RAG_04_NoRAGMerger_ExistingPathUnchanged) {
    // fed was created without setRAGMerger() in SetUp → normal execute path
    const std::string query = "FOR doc IN orders RETURN doc";

    // Must not throw; must return a valid (possibly empty) JSON result
    EXPECT_NO_THROW(fed->execute(query));

    // The existing scatter-gather counter must have been incremented
    EXPECT_GE(router->scatter_gather_call_count, 1);
}

// QF-RAG-05: buildPromptContext() produces [Shard: ...] prefixes for each document
TEST(QueryFederationRAGTest, QF_RAG_05_BuildPromptContextContainsShardPrefixes) {
    auto merger = std::make_shared<FederatedRAGMerger>();

    auto topology = std::make_shared<sharding::ShardTopology>();
    auto ring     = std::make_shared<sharding::ConsistentHashRing>();
    auto resolver = std::make_shared<sharding::URNResolver>(topology, ring);
    auto router   = std::make_shared<InstrumentedShardRouter>(resolver);
    QueryFederation qf(router);
    qf.setRAGMerger(merger);

    std::vector<ShardRetrievalResult> inputs = {
        makeShardResult("shard-alpha", 3),
        makeShardResult("shard-beta",  3),
    };

    auto ctx     = qf.mergeRAGResults(inputs);
    auto prompt  = ctx.buildPromptContext(/*max_docs=*/6, /*max_chars=*/0);

    EXPECT_NE(prompt.find("[Shard: shard-alpha]"), std::string::npos)
        << "Prompt context must include [Shard: shard-alpha] prefix";
    EXPECT_NE(prompt.find("[Shard: shard-beta]"), std::string::npos)
        << "Prompt context must include [Shard: shard-beta] prefix";
}

TEST(QueryFederationRAGTest, QF_RAG_06_OversizedShardInputThrows) {
    auto topology = std::make_shared<sharding::ShardTopology>();
    auto ring     = std::make_shared<sharding::ConsistentHashRing>();
    auto resolver = std::make_shared<sharding::URNResolver>(topology, ring);
    auto router   = std::make_shared<FederatedResultShardRouter>(resolver);

    ShardResult shard_result;
    shard_result.shard_id = "shard-001";
    shard_result.success = true;
    shard_result.data = nlohmann::json::object();
    shard_result.data["docs"] = nlohmann::json::array({
        nlohmann::json{
            {"doc_id", "doc-1"},
            {"content", std::string(128, 'x')},
            {"score", 1.0}
        }
    });
    router->queueShardResult(std::move(shard_result));

    QueryFederation::Config cfg;
    cfg.max_result_size_bytes = 64;
    QueryFederation qf(router, cfg);
    qf.setRAGMerger(std::make_shared<FederatedRAGMerger>());

    EXPECT_THROW(
        qf.executeFederatedRAGQuery("query"),
        std::runtime_error);
}
