#include <gtest/gtest.h>
#include "query/functions/graph_functions.h"
#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>

using namespace themis::query::functions;
using json = nlohmann::json;

class PageRankFunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        function_ = std::make_unique<PageRankFunction>();
    }

    // Helper: Create edge JSON
    json createEdge(const std::string& from, const std::string& to, double weight = 1.0) {
        return json{
            {"_from", from},
            {"_to", to},
            {"weight", weight}
        };
    }

    // Helper: Build simple test graph
    // Graph structure:
    //   A -> B -> C
    //   A -> C
    //   B -> D
    //   C -> D
    json buildSimpleGraph() {
        return json::array({
            createEdge("A", "B"),
            createEdge("A", "C"),
            createEdge("B", "C"),
            createEdge("B", "D"),
            createEdge("C", "D")
        });
    }

    // Helper: Build hub-and-spoke graph
    // Graph structure:
    //   A -> Hub
    //   B -> Hub
    //   C -> Hub
    //   D -> Hub
    //   Hub -> E
    //   Hub -> F
    json buildHubGraph() {
        return json::array({
            createEdge("A", "Hub"),
            createEdge("B", "Hub"),
            createEdge("C", "Hub"),
            createEdge("D", "Hub"),
            createEdge("Hub", "E"),
            createEdge("Hub", "F")
        });
    }

    std::unique_ptr<PageRankFunction> function_;
    FunctionContext ctx_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(PageRankFunctionTest, SimpleGraph_DetailedFormat) {
    auto edges = buildSimpleGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 4);  // 4 nodes: A, B, C, D
    
    // Verify structure of first result
    ASSERT_TRUE(result[0].contains("node_id"));
    ASSERT_TRUE(result[0].contains("rank"));
    ASSERT_TRUE(result[0].contains("in_degree"));
    ASSERT_TRUE(result[0].contains("out_degree"));
    
    // Verify all ranks are positive
    for (const auto& node : result) {
        EXPECT_GT(node["rank"].get<double>(), 0.0);
    }
    
    // Verify ranks sum to ~1.0
    double sum = 0.0;
    for (const auto& node : result) {
        sum += node["rank"].get<double>();
    }
    EXPECT_NEAR(sum, 1.0, 0.01);
}

TEST_F(PageRankFunctionTest, SimpleGraph_SortedByRank) {
    auto edges = buildSimpleGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    ASSERT_TRUE(result.is_array());
    ASSERT_GT(result.size(), 1);
    
    // Verify results are sorted by rank descending
    for (size_t i = 1; i < result.size(); ++i) {
        double prev_rank = result[i-1]["rank"].get<double>();
        double curr_rank = result[i]["rank"].get<double>();
        EXPECT_GE(prev_rank, curr_rank) 
            << "Results not sorted: rank[" << i-1 << "]=" << prev_rank
            << " < rank[" << i << "]=" << curr_rank;
    }
}

TEST_F(PageRankFunctionTest, SimpleGraph_DegreeInformation) {
    auto edges = buildSimpleGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    // Find specific nodes and verify degrees
    std::map<std::string, json> nodeMap = {};

    for (const auto& node : result) {
        nodeMap[node["node_id"].get<std::string>()] = node;
    }
    
    // A: out=2, in=0
    ASSERT_TRUE(nodeMap.contains("A"));
    EXPECT_EQ(nodeMap["A"]["out_degree"].get<int64_t>(), 2);
    EXPECT_EQ(nodeMap["A"]["in_degree"].get<int64_t>(), 0);
    
    // B: out=2, in=1
    ASSERT_TRUE(nodeMap.contains("B"));
    EXPECT_EQ(nodeMap["B"]["out_degree"].get<int64_t>(), 2);
    EXPECT_EQ(nodeMap["B"]["in_degree"].get<int64_t>(), 1);
    
    // C: out=1, in=2
    ASSERT_TRUE(nodeMap.contains("C"));
    EXPECT_EQ(nodeMap["C"]["out_degree"].get<int64_t>(), 1);
    EXPECT_EQ(nodeMap["C"]["in_degree"].get<int64_t>(), 2);
    
    // D: out=0, in=2
    ASSERT_TRUE(nodeMap.contains("D"));
    EXPECT_EQ(nodeMap["D"]["out_degree"].get<int64_t>(), 0);
    EXPECT_EQ(nodeMap["D"]["in_degree"].get<int64_t>(), 2);
}

TEST_F(PageRankFunctionTest, HubGraph_CentralNodeHighestRank) {
    auto edges = buildHubGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 7);  // A, B, C, D, Hub, E, F
    
    // First result should be Hub (highest rank)
    EXPECT_EQ(result[0]["node_id"].get<std::string>(), "Hub");
    
    // Hub should have in_degree=4, out_degree=2
    EXPECT_EQ(result[0]["in_degree"].get<int64_t>(), 4);
    EXPECT_EQ(result[0]["out_degree"].get<int64_t>(), 2);
}

// ============================================================================
// Format Options Tests
// ============================================================================

TEST_F(PageRankFunctionTest, SimpleFormat_ReturnsObject) {
    auto edges = buildSimpleGraph();
    json options = {{"format", "simple"}};
    std::vector<json> args = {edges, 0.85, 100, options};
    
    auto result = function_->execute(args, ctx_);
    
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 4);
    
    // Verify it contains node_id -> rank mappings
    EXPECT_TRUE(result.contains("A"));
    EXPECT_TRUE(result.contains("B"));
    EXPECT_TRUE(result.contains("C"));
    EXPECT_TRUE(result.contains("D"));
    
    // Verify all values are numbers (ranks)
    for (auto& [key, value] : result.items()) {
        EXPECT_TRUE(value.is_number());
        EXPECT_GT(value.get<double>(), 0.0);
    }
}

TEST_F(PageRankFunctionTest, DetailedFormat_ReturnsArray) {
    auto edges = buildSimpleGraph();
    json options = {{"format", "detailed"}};
    std::vector<json> args = {edges, 0.85, 100, options};
    
    auto result = function_->execute(args, ctx_);
    
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 4);
}

TEST_F(PageRankFunctionTest, DefaultFormat_IsDetailed) {
    auto edges = buildSimpleGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    // Default format should be detailed (array)
    ASSERT_TRUE(result.is_array());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PageRankFunctionTest, EmptyGraph) {
    json edges = json::array();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    // Empty graph should return empty array
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0);
}

TEST_F(PageRankFunctionTest, EmptyGraph_SimpleFormat) {
    json edges = json::array();
    json options = {{"format", "simple"}};
    std::vector<json> args = {edges, 0.85, 100, options};
    
    auto result = function_->execute(args, ctx_);
    
    // Empty graph should return empty object in simple format
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result.size(), 0);
}

TEST_F(PageRankFunctionTest, DisconnectedNodes) {
    // Create graph with two disconnected components:
    // A -> B and C -> D
    json edges = json::array({
        createEdge("A", "B"),
        createEdge("C", "D")
    });
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 4);
    
    // All nodes should still have valid ranks
    for (const auto& node : result) {
        EXPECT_GT(node["rank"].get<double>(), 0.0);
    }
}

// ============================================================================
// Normalization Tests
// ============================================================================

TEST_F(PageRankFunctionTest, Normalization_RanksSumToOne) {
    auto edges = buildHubGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    double sum = 0.0;
    for (const auto& node : result) {
        sum += node["rank"].get<double>();
    }
    
    // Ranks should be normalized to sum to 1.0
    EXPECT_NEAR(sum, 1.0, 0.001);
}

TEST_F(PageRankFunctionTest, RankOrdering_SimpleGraph) {
    auto edges = buildSimpleGraph();
    std::vector<json> args = {edges, 0.85, 100};
    
    auto result = function_->execute(args, ctx_);
    
    std::map<std::string, double> ranks = {};

    for (const auto& node : result) {
        ranks[node["node_id"].get<std::string>()] = node["rank"].get<double>();
    }
    
    // D should have highest rank (sink node with most incoming)
    EXPECT_GT(ranks["D"], ranks["A"]);
    EXPECT_GT(ranks["D"], ranks["B"]);
    
    // C should have higher rank than B (more incoming)
    EXPECT_GT(ranks["C"], ranks["B"]);
    
    // A should have lowest rank (source node, no incoming)
    EXPECT_LT(ranks["A"], ranks["B"]);
    EXPECT_LT(ranks["A"], ranks["C"]);
    EXPECT_LT(ranks["A"], ranks["D"]);
}
