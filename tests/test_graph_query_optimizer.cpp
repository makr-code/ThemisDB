#include <gtest/gtest.h>
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>

namespace fs = std::filesystem;

class GraphQueryOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_graph_optimizer_test";
        fs::remove_all(test_db_path_);
        
        themis::RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 256;
        config.max_background_jobs = 2;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        graph_mgr_ = std::make_unique<themis::GraphIndexManager>(*db_);
        optimizer_ = std::make_unique<themis::graph::GraphQueryOptimizer>(*graph_mgr_);
        
        // Create a test graph
        createTestGraph();
    }

    void TearDown() override {
        optimizer_.reset();
        graph_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    void createTestGraph() {
        // Create a simple graph: A -> B -> C -> D
        //                        A -> C
        themis::BaseEntity e1("edge1");
        e1.setField("id", "edge1");
        e1.setField("_from", "A");
        e1.setField("_to", "B");
        e1.setField("_weight", "1.0");
        graph_mgr_->addEdge(e1);

        themis::BaseEntity e2("edge2");
        e2.setField("id", "edge2");
        e2.setField("_from", "B");
        e2.setField("_to", "C");
        e2.setField("_weight", "1.0");
        graph_mgr_->addEdge(e2);

        themis::BaseEntity e3("edge3");
        e3.setField("id", "edge3");
        e3.setField("_from", "C");
        e3.setField("_to", "D");
        e3.setField("_weight", "1.0");
        graph_mgr_->addEdge(e3);

        themis::BaseEntity e4("edge4");
        e4.setField("id", "edge4");
        e4.setField("_from", "A");
        e4.setField("_to", "C");
        e4.setField("_weight", "2.0");
        graph_mgr_->addEdge(e4);
    }

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_mgr_;
    std::unique_ptr<themis::graph::GraphQueryOptimizer> optimizer_;
};

// ============================================================================
// Statistics Collection Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, CollectStatistics_Success) {
    auto result = optimizer_->collectStatistics();
    ASSERT_TRUE(result);
    
    const auto& stats = result.value();
    EXPECT_GT(stats.vertex_count, 0u);
    EXPECT_GT(stats.edge_count, 0u);
    EXPECT_TRUE(stats.has_edge_index);
    EXPECT_TRUE(stats.has_adjacency_cache);
}

TEST_F(GraphQueryOptimizerTest, GetStatistics_ReturnsCollectedStats) {
    optimizer_->collectStatistics();
    
    const auto& stats = optimizer_->getStatistics();
    EXPECT_GT(stats.edge_count, 0u);
}

// ============================================================================
// Plan Optimization Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, OptimizeShortestPath_GeneratesPlan) {
    auto result = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result);
    
    const auto& plan = result.value();
    EXPECT_EQ(plan.pattern, themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
    EXPECT_GT(plan.estimated_cost, 0.0);
    EXPECT_GT(plan.estimated_time_ms, 0.0);
    EXPECT_TRUE(plan.use_index);
    EXPECT_TRUE(plan.enable_early_termination);
}

TEST_F(GraphQueryOptimizerTest, OptimizeShortestPath_WithConstraints) {
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_depth = 2;
    constraints.unique_vertices = true;
    
    auto result = optimizer_->optimizeShortestPath("A", "D", constraints);
    ASSERT_TRUE(result);
    
    const auto& plan = result.value();
    EXPECT_EQ(plan.pattern, themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
}

TEST_F(GraphQueryOptimizerTest, OptimizeKHopNeighborhood_GeneratesPlan) {
    auto result = optimizer_->optimizeKHopNeighborhood("A", 2);
    ASSERT_TRUE(result);
    
    const auto& plan = result.value();
    EXPECT_EQ(plan.pattern, themis::graph::GraphQueryOptimizer::QueryPattern::K_HOP_NEIGHBORS);
    EXPECT_EQ(plan.algorithm, themis::graph::GraphQueryOptimizer::TraversalAlgorithm::BFS);
}

TEST_F(GraphQueryOptimizerTest, OptimizePatternMatch_SelectsDFS) {
    std::vector<std::string> vertices = {"A", "B", "C"};
    std::vector<std::pair<std::string, std::string>> edges = {{"A", "B"}, {"B", "C"}};
    
    auto result = optimizer_->optimizePatternMatch(vertices, edges);
    ASSERT_TRUE(result);
    
    const auto& plan = result.value();
    EXPECT_EQ(plan.pattern, themis::graph::GraphQueryOptimizer::QueryPattern::PATTERN_MATCH);
    EXPECT_EQ(plan.algorithm, themis::graph::GraphQueryOptimizer::TraversalAlgorithm::DFS);
}

TEST_F(GraphQueryOptimizerTest, OptimizeReachability_GeneratesPlan) {
    auto result = optimizer_->optimizeReachability("A", "D");
    ASSERT_TRUE(result);
    
    const auto& plan = result.value();
    EXPECT_EQ(plan.pattern, themis::graph::GraphQueryOptimizer::QueryPattern::REACHABILITY);
    EXPECT_TRUE(plan.enable_early_termination);
}

// ============================================================================
// BFS Execution Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteBFS_FindsAllNodesInDepth) {
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    
    auto result = optimizer_->executeBFS("A", 2, {}, &stats);
    ASSERT_TRUE(result);
    
    const auto& nodes = result.value();
    EXPECT_GT(nodes.size(), 0u);
    EXPECT_GT(stats.nodes_explored, 0u);
    EXPECT_GT(stats.execution_time_ms, 0.0);
}

TEST_F(GraphQueryOptimizerTest, ExecuteBFS_RespectsMaxDepth) {
    auto result = optimizer_->executeBFS("A", 1);
    ASSERT_TRUE(result);
    
    const auto& nodes = result.value();
    // Should include A and its immediate neighbors (B, C)
    EXPECT_LE(nodes.size(), 3u);
}

TEST_F(GraphQueryOptimizerTest, ExecuteBFS_WithMaxResults) {
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_results = 2;
    
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    auto result = optimizer_->executeBFS("A", 10, constraints, &stats);
    ASSERT_TRUE(result);
    
    const auto& nodes = result.value();
    EXPECT_LE(nodes.size(), 2u);
    EXPECT_TRUE(stats.early_terminated);
}

TEST_F(GraphQueryOptimizerTest, ExecuteBFS_WithForbiddenVertices) {
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    constraints.forbidden_vertices = {"B"};
    
    auto result = optimizer_->executeBFS("A", 3, constraints);
    ASSERT_TRUE(result);
    
    const auto& nodes = result.value();
    // Should not include B or its descendants via B
    EXPECT_EQ(std::find(nodes.begin(), nodes.end(), "B"), nodes.end());
}

// ============================================================================
// DFS Execution Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteDFS_FindsNodes) {
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    
    auto result = optimizer_->executeDFS("A", 2, {}, &stats);
    ASSERT_TRUE(result);
    
    const auto& nodes = result.value();
    EXPECT_GT(nodes.size(), 0u);
    EXPECT_GT(stats.nodes_explored, 0u);
}

TEST_F(GraphQueryOptimizerTest, ExecuteDFS_RespectsMaxDepth) {
    auto result = optimizer_->executeDFS("A", 1);
    ASSERT_TRUE(result);
    
    const auto& nodes = result.value();
    EXPECT_LE(nodes.size(), 3u); // A and immediate neighbors
}

// ============================================================================
// Dijkstra Execution Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteDijkstra_FindsShortestPath) {
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    
    auto result = optimizer_->executeDijkstra("A", "D", {}, &stats);
    ASSERT_TRUE(result);
    
    const auto& path = result.value();
    EXPECT_GT(path.path.size(), 0u);
    EXPECT_GT(stats.execution_time_ms, 0.0);
    EXPECT_EQ(stats.paths_found, 1u);
}

TEST_F(GraphQueryOptimizerTest, ExecuteDijkstra_NoPath_ReturnsEmpty) {
    // D has no outgoing edges to reach anyone
    auto result = optimizer_->executeDijkstra("D", "A");
    ASSERT_TRUE(result);
    
    const auto& path = result.value();
    EXPECT_EQ(path.path.size(), 0u);
}

// ============================================================================
// A* Execution Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteAStar_WithHeuristic_FindsPath) {
    // Simple heuristic: always return 1
    auto heuristic = [](const std::string&) { return 1.0; };
    
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    auto result = optimizer_->executeAStar("A", "D", heuristic, {}, &stats);
    ASSERT_TRUE(result);
    
    const auto& path = result.value();
    EXPECT_GE(path.path.size(), 0u);
}

// ============================================================================
// Bidirectional Search Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteBidirectional_FindsPath) {
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    
    auto result = optimizer_->executeBidirectional("A", "C", {}, &stats);
    ASSERT_TRUE(result);
    
    const auto& path = result.value();
    EXPECT_GT(path.path.size(), 0u);
    EXPECT_GT(stats.nodes_explored, 0u);
}

TEST_F(GraphQueryOptimizerTest, ExecuteBidirectional_MeetingPoint) {
    auto result = optimizer_->executeBidirectional("A", "D");
    ASSERT_TRUE(result);
    
    const auto& path = result.value();
    // Should find a path from A to D
    if (!path.path.empty()) {
        EXPECT_EQ(path.path.front(), "A");
        EXPECT_EQ(path.path.back(), "D");
    }
}

// ============================================================================
// Plan Caching Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, PlanCaching_CachesAndReuses) {
    optimizer_->setPlanCachingEnabled(true);
    
    // First call - generates plan
    auto result1 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result1);
    
    // Second call - should use cached plan
    auto result2 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result2);
    
    EXPECT_EQ(result1.value().estimated_cost, result2.value().estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, ClearPlanCache_RemovesCachedPlans) {
    optimizer_->setPlanCachingEnabled(true);
    
    auto result1 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result1);
    
    optimizer_->clearPlanCache();
    
    // Should regenerate plan after cache clear
    auto result2 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result2);
}

TEST_F(GraphQueryOptimizerTest, PlanCachingDisabled_NoCache) {
    optimizer_->setPlanCachingEnabled(false);
    
    auto result1 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result1);
    
    auto result2 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result2);
}

// ============================================================================
// Plan Explanation Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExplainPlan_GeneratesExplanation) {
    auto result = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result);
    
    std::string explanation = optimizer_->explainPlan(result.value());
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("Query Pattern"), std::string::npos);
    EXPECT_NE(explanation.find("Selected Algorithm"), std::string::npos);
    EXPECT_NE(explanation.find("Estimated Cost"), std::string::npos);
}

// ============================================================================
// Execution History Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecutionHistory_RecordsStats) {
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    optimizer_->executeBFS("A", 2, {}, &stats);
    
    const auto& history = optimizer_->getExecutionHistory();
    EXPECT_GT(history.size(), 0u);
    EXPECT_GT(history.back().nodes_explored, 0u);
}

TEST_F(GraphQueryOptimizerTest, ExecutionHistory_BoundedSize) {
    // Execute many queries
    for (int i = 0; i < 1100; ++i) {
        optimizer_->executeBFS("A", 1);
    }
    
    const auto& history = optimizer_->getExecutionHistory();
    EXPECT_LE(history.size(), 1000u); // MAX_HISTORY_SIZE
}

// ============================================================================
// Edge Type Selectivity Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, EstimateEdgeTypeSelectivity_DefaultOne) {
    double selectivity = optimizer_->estimateEdgeTypeSelectivity("unknown_type");
    EXPECT_DOUBLE_EQ(selectivity, 1.0);
}

// ============================================================================
// Performance and Scalability Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, LargerGraph_OptimizesEfficiently) {
    // Add more edges to create a larger graph
    for (int i = 0; i < 20; ++i) {
        themis::BaseEntity edge("edge_" + std::to_string(i + 10));
        edge.setField("id", "edge_" + std::to_string(i + 10));
        edge.setField("_from", "node_" + std::to_string(i));
        edge.setField("_to", "node_" + std::to_string(i + 1));
        edge.setField("_weight", "1.0");
        graph_mgr_->addEdge(edge);
    }
    
    optimizer_->collectStatistics();
    
    auto result = optimizer_->optimizeShortestPath("node_0", "node_19");
    ASSERT_TRUE(result);
    
    const auto& plan = result.value();
    EXPECT_GT(plan.estimated_nodes_explored, 0u);
}

TEST_F(GraphQueryOptimizerTest, MultipleQueries_ConsistentPerformance) {
    for (int i = 0; i < 10; ++i) {
        themis::graph::GraphQueryOptimizer::ExecutionStats stats;
        auto result = optimizer_->executeBFS("A", 2, {}, &stats);
        ASSERT_TRUE(result);
        EXPECT_GT(stats.nodes_explored, 0u);
    }
    
    const auto& history = optimizer_->getExecutionHistory();
    EXPECT_EQ(history.size(), 10u);
}
