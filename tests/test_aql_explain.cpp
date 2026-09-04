// Tests for the explainAql / explainAqlText / explainAqlDot pipeline
// These tests exercise the full parse → translate → build-plan → render path
// without touching any real storage (null storage / mock index manager).

#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <vector>

#include "query/aql_runner.h"
#include "query/query_plan_visualizer.h"
#include "core/query_engine_builder.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "utils/expected.h"

using namespace themis;
using namespace themis::errors;

// ============================================================================
// Minimal mock implementations (no real storage / indexes needed for EXPLAIN)
// ============================================================================

class ExplainMockStorage : public IStorageEngine {
public:
    Result<void>        open(const std::string&)                          override { return OkVoid(); }
    void                close()                                           override {}
    Result<void>        put(const std::string&, const std::string&)       override { return OkVoid(); }
    Result<std::string> get(const std::string&)                           override {
        return Err<std::string>(ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    }
    Result<void>        del(const std::string&)                           override { return OkVoid(); }
};

class ExplainMockIndexManager : public IIndexManager {
public:
    Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view, std::string_view, const std::string&) override {
        return Err<ISecondaryIndex*>(ErrorCode::ERR_INDEX_CREATION_FAILED);
    }
    Result<IVectorIndex*> createVectorIndex(
        std::string_view, uint32_t, const std::string&) override {
        return Err<IVectorIndex*>(ErrorCode::ERR_INDEX_CREATION_FAILED);
    }
    Result<IGraphIndex*> createGraphIndex(
        std::string_view, const std::string&) override {
        return Err<IGraphIndex*>(ErrorCode::ERR_INDEX_CREATION_FAILED);
    }
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view) const override {
        return Err<ISecondaryIndex*>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
    Result<IVectorIndex*> getVectorIndex(std::string_view) const override {
        return Err<IVectorIndex*>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
    Result<IGraphIndex*> getGraphIndex(std::string_view) const override {
        return Err<IGraphIndex*>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
    Result<void>              dropIndex(std::string_view)              override { return OkVoid(); }
    std::vector<std::string>  listIndexes()                      const override { return {}; }
    Result<IndexType>         getIndexType(std::string_view)     const override {
        return Err<IndexType>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
};

// ============================================================================
// Test fixture
// ============================================================================

class ExplainAqlTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage_   = std::make_shared<ExplainMockStorage>();
        index_mgr_ = std::make_shared<ExplainMockIndexManager>();
        engine_    = QueryEngineBuilder()
                         .withStorage(storage_)
                         .withIndexManager(index_mgr_)
                         .build();
    }

    std::shared_ptr<ExplainMockStorage>      storage_;
    std::shared_ptr<ExplainMockIndexManager> index_mgr_;
    std::shared_ptr<query::QueryEngine>      engine_;
};

// ============================================================================
// explainAql (JSON)
// ============================================================================

TEST_F(ExplainAqlTest, SimpleConjunctive_ReturnsValidJson) {
    auto result = explainAql(
        "FOR u IN users FILTER u.age == 30 RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    const auto& j = *result;
    EXPECT_TRUE(j.contains("mode"));
    EXPECT_TRUE(j.contains("plan"));
    EXPECT_EQ(j["mode"].get<std::string>(), "EXPLAIN");
}

TEST_F(ExplainAqlTest, SimpleConjunctive_PlanRootIsReturn) {
    auto result = explainAql(
        "FOR u IN users FILTER u.city == \"Berlin\" RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value());
    const auto& plan = (*result)["plan"];
    EXPECT_EQ(plan["type"].get<std::string>(), "Return");
}

TEST_F(ExplainAqlTest, SimpleConjunctive_HasFilterChild) {
    auto result = explainAql(
        "FOR u IN users FILTER u.status == \"active\" RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value());
    const auto& plan = (*result)["plan"];
    ASSERT_FALSE(plan["children"].empty());
    // First child must be a Filter
    EXPECT_EQ(plan["children"][0]["type"].get<std::string>(), "Filter");
}

TEST_F(ExplainAqlTest, AnalyzeMode_IncludesActualTimeField) {
    auto result = explainAql(
        "FOR p IN products FILTER p.category == \"book\" RETURN p",
        *engine_,
        /*analyze=*/true
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)["mode"].get<std::string>(), "EXPLAIN ANALYZE");
    // The root plan node must carry the actual_time_ms sentinel (−1 = not measured)
    EXPECT_TRUE((*result)["plan"].contains("actual_time_ms"));
}

TEST_F(ExplainAqlTest, NoPredicates_ProducesSeqScan) {
    auto result = explainAql(
        "FOR d IN docs RETURN d",
        *engine_
    );
    ASSERT_TRUE(result.has_value());
    const auto& plan = (*result)["plan"];
    // Walk to leaf
    const nlohmann::json* node = &plan;
    while (!(*node)["children"].empty()) {
        node = &(*node)["children"][0];
    }
    EXPECT_EQ((*node)["type"].get<std::string>(), "SeqScan");
}

TEST_F(ExplainAqlTest, MultiplePredicates_MultipleFilterNodes) {
    auto result = explainAql(
        "FOR u IN users FILTER u.age == 30 AND u.city == \"Berlin\" RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value());
    const auto& plan = (*result)["plan"];
    // Count Filter nodes
    int filter_count = 0;
    std::function<void(const nlohmann::json&)> count_filters =
        [&](const nlohmann::json& n) {
            if (n["type"].get<std::string>() == "Filter") {
              ++filter_count;
            }
            for (const auto& child : n["children"]) {
              count_filters(child);
            }
        };
    count_filters(plan);
    EXPECT_EQ(filter_count, 2);
}

TEST_F(ExplainAqlTest, InvalidAql_ReturnsError) {
    auto result = explainAql("NOT VALID AQL !!!", *engine_);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// explainAqlText
// ============================================================================

TEST_F(ExplainAqlTest, Text_ContainsExplainHeader) {
    auto result = explainAqlText(
        "FOR u IN users FILTER u.role == \"admin\" RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE((*result).find("EXPLAIN"), std::string::npos);
}

TEST_F(ExplainAqlTest, Text_ContainsReturnAndFilter) {
    auto result = explainAqlText(
        "FOR u IN users FILTER u.age == 25 RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_NE((*result).find("Return"), std::string::npos);
    EXPECT_NE((*result).find("Filter"), std::string::npos);
}

TEST_F(ExplainAqlTest, Text_AnalyzeMode_ContainsActualTime) {
    auto result = explainAqlText(
        "FOR u IN users FILTER u.age == 25 RETURN u",
        *engine_,
        /*analyze=*/true
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_NE((*result).find("EXPLAIN ANALYZE"), std::string::npos);
}

TEST_F(ExplainAqlTest, Text_InvalidAql_ReturnsError) {
    auto result = explainAqlText("GARBAGE", *engine_);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// explainAqlDot
// ============================================================================

TEST_F(ExplainAqlTest, DOT_IsValidDigraph) {
    auto result = explainAqlDot(
        "FOR u IN users FILTER u.city == \"Hamburg\" RETURN u",
        *engine_
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE((*result).find("digraph QueryPlan"), std::string::npos);
    EXPECT_NE((*result).find("->"), std::string::npos);
}

TEST_F(ExplainAqlTest, DOT_ContainsNodeLabels) {
    auto result = explainAqlDot(
        "FOR o IN orders FILTER o.status == \"shipped\" RETURN o",
        *engine_
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_NE((*result).find("Return"), std::string::npos);
    EXPECT_NE((*result).find("Filter"), std::string::npos);
}

TEST_F(ExplainAqlTest, DOT_InvalidAql_ReturnsError) {
    auto result = explainAqlDot("GARBAGE", *engine_);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Non-conjunctive form: graph traversal
// ============================================================================

TEST_F(ExplainAqlTest, GraphTraversal_ProducesGraphTraversalNode) {
    const char* aql =
        "FOR v IN 1..3 OUTBOUND 'persons/alice' GRAPH 'social' RETURN v";
    auto result = explainAql(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    const auto& plan = (*result)["plan"];
    EXPECT_EQ(plan["type"].get<std::string>(), "GraphTraversal");
}

TEST_F(ExplainAqlTest, GraphTraversal_PlanHasCostAndRows) {
    const char* aql =
        "FOR v IN 1..3 OUTBOUND 'persons/alice' GRAPH 'social' RETURN v";
    auto result = explainAql(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    const auto& plan = (*result)["plan"];
    EXPECT_TRUE(plan.contains("estimated_cost"));
    EXPECT_TRUE(plan.contains("estimated_rows"));
    EXPECT_GT(plan["estimated_cost"].get<double>(), 0.0);
}

TEST_F(ExplainAqlTest, GraphTraversal_AttributesIncludeStartDepthDirectionAlgorithm) {
    const char* aql =
        "FOR v IN 2..4 OUTBOUND 'persons/alice' GRAPH 'social' RETURN v";
    auto result = explainAql(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    const auto& plan = (*result)["plan"];
    ASSERT_TRUE(plan.contains("attributes"));
    const auto& attrs = plan["attributes"];

    // Collect all attribute strings for easy searching
    std::vector<std::string> attr_list;
    for (const auto& a : attrs) {
      attr_list.push_back(a.get<std::string>());
    }

    auto has_attr = [&](const std::string& prefix) {
        for (const auto& a : attr_list)
            if (a.rfind(prefix, 0) == 0) {
              return true;
            }
        return false;
    };

    EXPECT_TRUE(has_attr("start:"))     << "missing 'start:' attribute";
    EXPECT_TRUE(has_attr("depth:"))     << "missing 'depth:' attribute";
    EXPECT_TRUE(has_attr("direction:")) << "missing 'direction:' attribute";
    EXPECT_TRUE(has_attr("algorithm:")) << "missing 'algorithm:' attribute";
}

TEST_F(ExplainAqlTest, GraphTraversal_ShortestPath_UsesBFS) {
    // SHORTEST_PATH with depth ≤ 5 → BFS
    const char* aql =
        "FOR v IN 1..3 OUTBOUND 'persons/alice' GRAPH 'social' "
        "SHORTEST_PATH TO 'persons/bob' RETURN v";
    auto result = explainAql(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    const auto& plan = (*result)["plan"];
    EXPECT_EQ(plan["type"].get<std::string>(), "GraphTraversal");
    // Algorithm attribute must be present
    ASSERT_TRUE(plan.contains("attributes"));
    bool found_algo = false;
    for (const auto& a : plan["attributes"]) {
        if (a.get<std::string>().rfind("algorithm:", 0) == 0) { found_algo = true; break; }
    }
    EXPECT_TRUE(found_algo) << "missing 'algorithm:' attribute in shortest-path plan";
}

TEST_F(ExplainAqlTest, GraphTraversal_Text_ContainsGraphTraversalKeyword) {
    const char* aql =
        "FOR v IN 1..2 OUTBOUND 'persons/alice' GRAPH 'social' RETURN v";
    auto result = explainAqlText(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE((*result).find("GraphTraversal"), std::string::npos);
}

TEST_F(ExplainAqlTest, GraphTraversal_DOT_ContainsGraphTraversalLabel) {
    const char* aql =
        "FOR v IN 1..2 OUTBOUND 'persons/alice' GRAPH 'social' RETURN v";
    auto result = explainAqlDot(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE((*result).find("GraphTraversal"), std::string::npos);
    EXPECT_NE((*result).find("digraph QueryPlan"), std::string::npos);
}

TEST_F(ExplainAqlTest, GraphTraversal_AnalyzeMode_IncludesActualTimeSentinel) {
    const char* aql =
        "FOR v IN 1..2 OUTBOUND 'persons/alice' GRAPH 'social' RETURN v";
    auto result = explainAql(aql, *engine_, /*analyze=*/true);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ((*result)["mode"].get<std::string>(), "EXPLAIN ANALYZE");
    // GraphTraversal node must carry actual_time_ms sentinel (-1 = not measured)
    EXPECT_TRUE((*result)["plan"].contains("actual_time_ms"));
    EXPECT_EQ((*result)["plan"]["actual_time_ms"].get<double>(), -1.0);
}

TEST_F(ExplainAqlTest, GraphTraversal_ShortestPath_AlgorithmIsBFS) {
    // SHORTEST_PATH without explicit depth uses BFS (maxDepth defaults to 1, ≤ 5)
    const char* aql =
        "FOR v IN 1..3 OUTBOUND 'persons/alice' GRAPH 'social' "
        "SHORTEST_PATH TO 'persons/bob' RETURN v";
    auto result = explainAql(aql, *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ((*result)["plan"]["type"].get<std::string>(), "GraphTraversal");
    // Algorithm should be BFS since depth ≤ 5
    ASSERT_TRUE((*result)["plan"].contains("attributes"));
    bool found_bfs = false;
    for (const auto& a : (*result)["plan"]["attributes"]) {
        if (a.get<std::string>() == "algorithm: BFS") { found_bfs = true; break; }
    }
    EXPECT_TRUE(found_bfs) << "expected algorithm: BFS for SHORTEST_PATH with default depth";
}
