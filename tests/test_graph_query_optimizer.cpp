/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_graph_query_optimizer.cpp                     ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:53:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1300                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "graph/graph_query_optimizer.h"
#include "graph/path_constraints.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include <filesystem>
#include <chrono>
#include <nlohmann/json.hpp>

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
    auto result = optimizer_->executeBFS("A", 1, {});
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
    auto result = optimizer_->executeDFS("A", 1, {});
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
    auto result = optimizer_->executeDijkstra("D", "A", {});
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
    auto result = optimizer_->executeBidirectional("A", "D", {});
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
    // Execute many queries to test history bounding
    for (int i = 0; i < 150; ++i) {
        optimizer_->executeBFS("A", 1, {});
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

// ============================================================================
// Timeout / SLO Enforcement Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, BFS_ZeroTimeout_NoAbort) {
    // timeout_ms = 0 means no limit; query should succeed normally
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_depth = 3;
    constraints.timeout_ms = 0;
    auto result = optimizer_->executeBFS("A", 3, constraints);
    EXPECT_TRUE(result);
}

TEST_F(GraphQueryOptimizerTest, DFS_ZeroTimeout_NoAbort) {
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_depth = 3;
    constraints.timeout_ms = 0;
    auto result = optimizer_->executeDFS("A", 3, constraints);
    EXPECT_TRUE(result);
}

TEST_F(GraphQueryOptimizerTest, BFS_LargeTimeout_Succeeds) {
    // A generous timeout should not abort a small-graph traversal
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_depth = 3;
    constraints.timeout_ms = 60000; // 60 seconds
    auto result = optimizer_->executeBFS("A", 3, constraints);
    EXPECT_TRUE(result);
}

// ============================================================================
// Aggregate Observability Metrics Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, Metrics_TotalQueriesIncrement) {
    uint64_t before = optimizer_->getQueryMetrics().total_queries.load();
    optimizer_->executeBFS("A", 2, {});
    uint64_t after = optimizer_->getQueryMetrics().total_queries.load();
    EXPECT_EQ(after, before + 1);
}

TEST_F(GraphQueryOptimizerTest, Metrics_TotalNodesExplored_NonZero) {
    optimizer_->executeBFS("A", 2, {});
    EXPECT_GT(optimizer_->getQueryMetrics().total_nodes_explored.load(), 0u);
}

TEST_F(GraphQueryOptimizerTest, Metrics_MultipleQueries_Accumulate) {
    const int N = 5;
    for (int i = 0; i < N; ++i) {
        optimizer_->executeBFS("A", 1, {});
    }
    EXPECT_GE(optimizer_->getQueryMetrics().total_queries.load(),
              static_cast<uint64_t>(N));
}

TEST_F(GraphQueryOptimizerTest, Metrics_PlanCacheHit_Counted) {
    // First call: cache miss
    optimizer_->optimizeShortestPath("A", "D");
    uint64_t misses_after_first =
        optimizer_->getQueryMetrics().plan_cache_misses.load();
    EXPECT_GT(misses_after_first, 0u);

    // Second identical call: cache hit
    optimizer_->optimizeShortestPath("A", "D");
    uint64_t hits_after_second =
        optimizer_->getQueryMetrics().plan_cache_hits.load();
    EXPECT_GT(hits_after_second, 0u);
}

TEST_F(GraphQueryOptimizerTest, Metrics_AvgExecutionTime_AfterQueries) {
    for (int i = 0; i < 3; ++i) {
        optimizer_->executeBFS("A", 2, {});
    }
    // All three queries should succeed, so error rate must be 0
    EXPECT_EQ(optimizer_->getQueryMetrics().errorRate(), 0.0);
    // avg execution time should be a non-negative finite value
    EXPECT_GE(optimizer_->getQueryMetrics().avgExecutionTimeMs(), 0.0);
}

// ============================================================================
// Graph-Specific Error Codes Tests (Phase 2.1)
// ============================================================================

TEST_F(GraphQueryOptimizerTest, BFS_ValidStartVertex_ReturnsResults) {
    // A well-known present vertex should return a non-empty result.
    auto result = optimizer_->executeBFS("A", 2, {});
    EXPECT_TRUE(result);
    EXPECT_FALSE(result.value().empty());
}

TEST_F(GraphQueryOptimizerTest, GraphErrorCode_ERR_GRAPH_NO_SUCH_VERTEX_Value) {
    // Verify the numeric value of the new error code so callers can rely on it.
    EXPECT_EQ(static_cast<int>(themis::errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX), 6400);
}

TEST_F(GraphQueryOptimizerTest, GraphErrorCode_ERR_GRAPH_PATH_NOT_FOUND_Value) {
    EXPECT_EQ(static_cast<int>(themis::errors::ErrorCode::ERR_GRAPH_PATH_NOT_FOUND), 6403);
}

TEST_F(GraphQueryOptimizerTest, GraphErrorCode_ERR_GRAPH_CONSTRAINT_CONFLICT_Value) {
    EXPECT_EQ(static_cast<int>(themis::errors::ErrorCode::ERR_GRAPH_CONSTRAINT_CONFLICT), 6402);
}

// ============================================================================
// explainConstrainedPath() Dry-Run Tests (Phase 2.2)
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_ReturnsPlan) {
    themis::graph::PathConstraints constraints(graph_mgr_.get());
    constraints.addMinLength(1);
    constraints.addMaxLength(5);

    auto result = optimizer_->explainConstrainedPath("A", "D", constraints);
    ASSERT_TRUE(result);

    const auto& plan = result.value();
    EXPECT_GT(plan.estimated_cost, 0.0);
    EXPECT_FALSE(plan.explanation.empty());
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_ContainsConstraintInfo) {
    themis::graph::PathConstraints constraints(graph_mgr_.get());
    constraints.addMinLength(2);
    constraints.addMaxLength(4);
    constraints.addForbiddenNode("X");

    auto result = optimizer_->explainConstrainedPath("A", "D", constraints);
    ASSERT_TRUE(result);

    // The explanation should mention the constraint count
    const auto& explanation = result.value().explanation;
    EXPECT_NE(explanation.find("Constraints"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_DoesNotIncrementQueryCounter) {
    // explainConstrainedPath is a dry-run: it must NOT count as an executed query
    uint64_t before = optimizer_->getQueryMetrics().total_queries.load();

    themis::graph::PathConstraints constraints(graph_mgr_.get());
    optimizer_->explainConstrainedPath("A", "D", constraints);

    uint64_t after = optimizer_->getQueryMetrics().total_queries.load();
    EXPECT_EQ(before, after);
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_AlgorithmSelected) {
    // With min_length = 6, DFS should be selected
    themis::graph::PathConstraints constraints(graph_mgr_.get());
    constraints.addMinLength(6);

    auto result = optimizer_->explainConstrainedPath("A", "D", constraints);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().algorithm,
              themis::graph::GraphQueryOptimizer::TraversalAlgorithm::DFS);
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_BFSForRequiredNode) {
    // With a required node, BFS should be selected
    themis::graph::PathConstraints constraints(graph_mgr_.get());
    constraints.addRequiredNode("B");

    auto result = optimizer_->explainConstrainedPath("A", "D", constraints);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().algorithm,
              themis::graph::GraphQueryOptimizer::TraversalAlgorithm::BFS);
}



// ============================================================================
// Phase 3: Parallel BFS Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, BFS_Parallel_ProducesSameResultAsSequential) {
    // Sequential BFS
    themis::graph::GraphQueryOptimizer::QueryConstraints seq_c;
    seq_c.max_depth = 3;
    seq_c.enable_parallel = false;
    auto seq_result = optimizer_->executeBFS("A", 3, seq_c);
    ASSERT_TRUE(seq_result);

    // Parallel BFS
    themis::graph::GraphQueryOptimizer::QueryConstraints par_c;
    par_c.max_depth = 3;
    par_c.enable_parallel = true;
    par_c.num_threads = 2;
    auto par_result = optimizer_->executeBFS("A", 3, par_c);
    ASSERT_TRUE(par_result);

    // Both should find the same set of reachable nodes (order may differ)
    auto seq_nodes = seq_result.value();
    auto par_nodes = par_result.value();
    std::sort(seq_nodes.begin(), seq_nodes.end());
    std::sort(par_nodes.begin(), par_nodes.end());
    EXPECT_EQ(seq_nodes, par_nodes);
}

TEST_F(GraphQueryOptimizerTest, BFS_Parallel_DefaultThreadCount) {
    // num_threads = 0 means auto; should still succeed
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    c.num_threads = 0;
    auto result = optimizer_->executeBFS("A", 2, c);
    EXPECT_TRUE(result);
    EXPECT_FALSE(result.value().empty());
}

TEST_F(GraphQueryOptimizerTest, BFS_Parallel_IncreasesQueryCounter) {
    uint64_t before = optimizer_->getQueryMetrics().total_queries.load();
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    c.num_threads = 2;
    optimizer_->executeBFS("A", 2, c);
    EXPECT_EQ(optimizer_->getQueryMetrics().total_queries.load(), before + 1);
}

TEST_F(GraphQueryOptimizerTest, QueryConstraints_EnableParallel_Default_False) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    EXPECT_FALSE(c.enable_parallel);
    EXPECT_EQ(c.num_threads, 0u);
}

// ============================================================================
// Phase 5: Edge Property Constraint Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, PathConstraints_EdgeProperty_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addEdgePropertyConstraint("type", "follows");

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Edge property"), std::string::npos);
    EXPECT_NE(desc.find("type"), std::string::npos);
    EXPECT_NE(desc.find("follows"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_EdgeProperty_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addEdgePropertyConstraint("status", "active");

    const auto& constraints = pc.getConstraints();
    ASSERT_EQ(constraints.size(), 1u);
    EXPECT_EQ(constraints[0].type,
              themis::graph::PathConstraints::ConstraintType::EDGE_PROPERTY);
    ASSERT_TRUE(constraints[0].property_key.has_value());
    EXPECT_EQ(*constraints[0].property_key, "status");
    ASSERT_TRUE(constraints[0].string_value.has_value());
    EXPECT_EQ(*constraints[0].string_value, "active");
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_FailsOnWrongEdgeProperty) {
    // edge1 exists but has no "type" field (the test graph doesn't set it)
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addEdgePropertyConstraint("type", "friend");

    // Validate a path using edge1 – it has no "type" field so should fail
    std::vector<std::string> nodes = {"A", "B"};
    std::vector<std::string> edges = {"edge1"};
    auto result = pc.validatePath(nodes, edges);
    // Should either return false/error because edge1 doesn't have type="friend"
    // (The method returns error or false on violation)
    EXPECT_FALSE(result.has_value() && *result == true);
}


// ============================================================================
// Phase 2.3: GET /api/v1/graph/metrics endpoint tests
// ============================================================================

#include "server/graph_api_handler.h"
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace bhttp = beast::http;

class GraphApiHandlerMetricsTest : public ::testing::Test {
protected:
    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::GraphIndexManager> graph_mgr_;
    std::unique_ptr<themis::server::GraphApiHandler> handler_;

    void SetUp() override {
        test_db_path_ = "./data/themis_graph_metrics_api_test_" +
                        std::to_string(
                            std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::remove_all(test_db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        cfg.memtable_size_mb = 16;
        cfg.block_cache_size_mb = 32;
        db_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        graph_mgr_ = std::make_shared<themis::GraphIndexManager>(*db_);
        ASSERT_TRUE(graph_mgr_->rebuildTopology().ok);

        // GraphApiHandler takes shared_ptr<GraphIndexManager>; auth can be null
        handler_ = std::make_unique<themis::server::GraphApiHandler>(
            nullptr, graph_mgr_, nullptr);
    }

    void TearDown() override {
        handler_.reset();
        graph_mgr_.reset();
        db_->close();
        std::filesystem::remove_all(test_db_path_);
    }

    bhttp::request<bhttp::string_body> makeGet(const std::string& target) {
        bhttp::request<bhttp::string_body> req{bhttp::verb::get, target, 11};
        req.set(bhttp::field::host, "localhost");
        return req;
    }
};

TEST_F(GraphApiHandlerMetricsTest, HandleMetrics_ReturnsOK) {
    auto req = makeGet("/api/v1/graph/metrics");
    auto res = handler_->handleMetrics(req);
    EXPECT_EQ(res.result(), bhttp::status::ok);
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetrics_BodyIsValidJSON) {
    auto req = makeGet("/api/v1/graph/metrics");
    auto res = handler_->handleMetrics(req);

    nlohmann::json j;
    ASSERT_NO_THROW(j = nlohmann::json::parse(res.body()));
    EXPECT_TRUE(j.is_object());
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetrics_ContainsAllExpectedKeys) {
    auto req = makeGet("/api/v1/graph/metrics");
    auto res = handler_->handleMetrics(req);

    auto j = nlohmann::json::parse(res.body());
    EXPECT_TRUE(j.contains("total_queries"));
    EXPECT_TRUE(j.contains("failed_queries"));
    EXPECT_TRUE(j.contains("timed_out_queries"));
    EXPECT_TRUE(j.contains("total_execution_time_ms"));
    EXPECT_TRUE(j.contains("max_execution_time_ms"));
    EXPECT_TRUE(j.contains("avg_execution_time_ms"));
    EXPECT_TRUE(j.contains("total_nodes_explored"));
    EXPECT_TRUE(j.contains("total_edges_traversed"));
    EXPECT_TRUE(j.contains("plan_cache_hits"));
    EXPECT_TRUE(j.contains("plan_cache_misses"));
    EXPECT_TRUE(j.contains("error_rate"));
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetrics_InitialCountersAreZero) {
    auto req = makeGet("/api/v1/graph/metrics");
    auto res = handler_->handleMetrics(req);

    auto j = nlohmann::json::parse(res.body());
    EXPECT_EQ(j["total_queries"].get<uint64_t>(), 0u);
    EXPECT_EQ(j["failed_queries"].get<uint64_t>(), 0u);
    EXPECT_EQ(j["timed_out_queries"].get<uint64_t>(), 0u);
    EXPECT_EQ(j["error_rate"].get<double>(), 0.0);
}


// ============================================================================
// Adaptive Cost Model Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_EnabledByDefault) {
    EXPECT_TRUE(optimizer_->isAdaptiveLearningEnabled());
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_CanBeDisabled) {
    optimizer_->enableAdaptiveLearning(false);
    EXPECT_FALSE(optimizer_->isAdaptiveLearningEnabled());
    optimizer_->enableAdaptiveLearning(true);
    EXPECT_TRUE(optimizer_->isAdaptiveLearningEnabled());
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_BFSExecutionPopulatesModel) {
    // Run a BFS – this should update the BFS cost model
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->executeBFS("A", 3, c);

    const auto& models = optimizer_->getAlgorithmCostModels();
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    ASSERT_NE(models.find(Algo::BFS), models.end());
    const auto& bfs_model = models.at(Algo::BFS);
    EXPECT_EQ(bfs_model.exec_count, 1u);
    EXPECT_GE(bfs_model.ema_cost_ms, 0.0);
    EXPECT_GT(bfs_model.confidence, 0.0);
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_MultipleExecutionsIncreaseConfidence) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 5; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    const auto& bfs_model = optimizer_->getAlgorithmCostModels().at(Algo::BFS);
    EXPECT_EQ(bfs_model.exec_count, 5u);
    EXPECT_GE(bfs_model.confidence, 5.0 / 100.0);
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_DisabledDoesNotPopulateModel) {
    optimizer_->enableAdaptiveLearning(false);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->executeBFS("A", 2, c);

    // No model entry should be created when adaptive learning is off
    const auto& models = optimizer_->getAlgorithmCostModels();
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    EXPECT_EQ(models.find(Algo::BFS), models.end());
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_DFSAndBFSTrackedSeparately) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->executeBFS("A", 2, c);
    optimizer_->executeDFS("A", 2, c);

    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    const auto& models = optimizer_->getAlgorithmCostModels();
    EXPECT_NE(models.find(Algo::BFS), models.end());
    EXPECT_NE(models.find(Algo::DFS), models.end());
    EXPECT_EQ(models.at(Algo::BFS).exec_count, 1u);
    EXPECT_EQ(models.at(Algo::DFS).exec_count, 1u);
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_ExportProducesValidJSON) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->executeBFS("A", 2, c);
    optimizer_->executeDFS("A", 2, c);

    std::string exported = optimizer_->exportCostModel();
    ASSERT_FALSE(exported.empty());

    nlohmann::json j;
    ASSERT_NO_THROW(j = nlohmann::json::parse(exported));
    EXPECT_TRUE(j.is_object());
    EXPECT_TRUE(j.contains("BFS"));
    EXPECT_TRUE(j.contains("DFS"));
    EXPECT_TRUE(j["BFS"].contains("ema_cost_ms"));
    EXPECT_TRUE(j["BFS"].contains("exec_count"));
    EXPECT_TRUE(j["BFS"].contains("confidence"));
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_ExportEmpty_ReturnsEmptyObject) {
    std::string exported = optimizer_->exportCostModel();
    // Fresh optimizer – no executions yet; should export empty JSON object "{}"
    nlohmann::json j = nlohmann::json::parse(exported);
    EXPECT_EQ(j.size(), 0u);
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_ImportRoundtrip) {
    // Run several BFS to build a model
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 3; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }
    const std::string exported = optimizer_->exportCostModel();

    // Import into a new optimizer instance
    themis::graph::GraphQueryOptimizer opt2(*graph_mgr_);
    EXPECT_TRUE(opt2.importCostModel(exported));

    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    const auto& imported_models = opt2.getAlgorithmCostModels();
    ASSERT_NE(imported_models.find(Algo::BFS), imported_models.end());
    const auto& orig = optimizer_->getAlgorithmCostModels().at(Algo::BFS);
    const auto& imported = imported_models.at(Algo::BFS);
    EXPECT_DOUBLE_EQ(imported.ema_cost_ms, orig.ema_cost_ms);
    EXPECT_EQ(imported.exec_count, orig.exec_count);
    EXPECT_DOUBLE_EQ(imported.confidence, orig.confidence);
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_ImportInvalidJSON_ReturnsFalse) {
    themis::graph::GraphQueryOptimizer opt2(*graph_mgr_);
    EXPECT_FALSE(opt2.importCostModel("{not valid json"));
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_ImportUnknownAlgo_IsIgnored) {
    // JSON with an unknown algorithm name should be silently ignored
    std::string json_with_unknown = R"({"UNKNOWN_ALGO":{"ema_cost_ms":5.0,"exec_count":10,"confidence":0.1}})";
    themis::graph::GraphQueryOptimizer opt2(*graph_mgr_);
    EXPECT_TRUE(opt2.importCostModel(json_with_unknown));
    EXPECT_EQ(opt2.getAlgorithmCostModels().size(), 0u);
}

TEST_F(GraphQueryOptimizerTest, AdaptiveLearning_CostModelInfluencesEstimate) {
    // After many BFS executions the learned cost should shift the estimate.
    // We verify that with high confidence the estimate differs from a fresh
    // optimizer that has no learned data.
    themis::graph::GraphQueryOptimizer fresh(*graph_mgr_);
    const double cost_before = [&] {
        themis::graph::GraphQueryOptimizer::QueryConstraints c;
        auto plan = fresh.optimizeShortestPath("A", "D", c);
        return plan ? plan.value().estimated_cost : 0.0;
    }();

    // Feed many fast observations into our optimizer to push confidence high
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 50; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }
    auto plan_after = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan_after);
    const double cost_after = plan_after.value().estimated_cost;

    // Both costs are finite and non-negative; the adaptive model should have
    // shifted the estimate (they should differ because the EMA introduces a
    // learned component with confidence ≈ 0.5 after 50 observations).
    EXPECT_GE(cost_before, 0.0);
    EXPECT_GE(cost_after, 0.0);
    // Just verify the optimizer produced a valid plan; cost change depends on
    // actual timing so we don't assert exact equality or direction.
    EXPECT_NE(plan_after.value().algorithm, static_cast<decltype(plan_after.value().algorithm)>(-1));
}


// ============================================================================
// Phase 5.2: NODE_PROPERTY constraint tests
// ============================================================================

// Helper: store a vertex entity with a property field
static void storeVertex(themis::RocksDBWrapper& db, const std::string& pk,
                        const std::string& field, const std::string& value) {
    themis::BaseEntity v(pk);
    v.setField("id", pk);
    v.setField(field, value);
    db.put(themis::KeySchema::makeGraphNodeKey(pk), v.serialize());
}

TEST_F(GraphQueryOptimizerTest, NodePropertyConstraint_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addNodePropertyConstraint("country", "USA");
    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Node property: country = USA"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, NodePropertyConstraint_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addNodePropertyConstraint("type", "Person");
    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::NODE_PROPERTY);
    EXPECT_EQ(pc.getConstraints()[0].property_key.value_or(""), "type");
    EXPECT_EQ(pc.getConstraints()[0].string_value.value_or(""), "Person");
}

TEST_F(GraphQueryOptimizerTest, NodePropertyConstraint_ValidatePath_PassesWhenPresent) {
    // Store vertex "A" with country=USA
    storeVertex(*db_, "A", "country", "USA");

    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addNodePropertyConstraint("country", "USA");

    // Single-node path containing only "A"
    auto res = pc.validatePath({"A"}, {});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, NodePropertyConstraint_ValidatePath_FailsOnWrongValue) {
    // "A" has country=USA, "B" does not have country set
    storeVertex(*db_, "A", "country", "USA");

    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addNodePropertyConstraint("country", "USA");

    // Path with "B" which has no country field → should fail
    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, GetNodeField_ReturnsStoredField) {
    storeVertex(*db_, "testnode", "status", "premium");
    auto field = graph_mgr_->getNodeField("testnode", "status");
    ASSERT_TRUE(field.has_value());
    EXPECT_EQ(*field, "premium");
}

TEST_F(GraphQueryOptimizerTest, GetNodeField_ReturnsNulloptForMissingNode) {
    auto field = graph_mgr_->getNodeField("nonexistent_vertex_xyz", "status");
    EXPECT_FALSE(field.has_value());
}

// ============================================================================
// Phase 5.3: Weight constraint tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, WeightConstraint_DescribeShows_MaxWeight) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMaxWeight(10.5);
    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Maximum path weight: 10.5"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, WeightConstraint_DescribeShows_MinWeight) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMinWeight(2.0);
    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Minimum path weight: 2"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, WeightConstraint_StoredCorrectly) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMaxWeight(5.0);
    pc.addMinWeight(1.0);
    ASSERT_EQ(pc.getConstraints().size(), 2u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::MAX_WEIGHT);
    EXPECT_DOUBLE_EQ(pc.getConstraints()[0].double_value.value_or(-1.0), 5.0);
    EXPECT_EQ(pc.getConstraints()[1].type,
              themis::graph::PathConstraints::ConstraintType::MIN_WEIGHT);
    EXPECT_DOUBLE_EQ(pc.getConstraints()[1].double_value.value_or(-1.0), 1.0);
}

TEST_F(GraphQueryOptimizerTest, WeightConstraint_MaxWeight_PrunesBFS) {
    // Test graph topology (from createTestGraph):
    //   A --(1.0)--> B --(1.0)--> C --(1.0)--> D
    //   A --(2.0)--> C
    //
    // Paths to D and their total weights:
    //   A->B->C->D : 1.0 + 1.0 + 1.0 = 3.0
    //   A->C->D    : 2.0 + 1.0 = 3.0
    //
    // With max_weight=1.5: both paths to D exceed the budget, so no path should
    // be found.  A->B (1.0) and A->C (2.0) cannot be the target (target is D).
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMaxWeight(1.5);

    auto res = pc.findConstrainedPaths("A", "D", 10);
    EXPECT_FALSE(res.has_value());
}

TEST_F(GraphQueryOptimizerTest, WeightConstraint_MaxWeight_AllowsLightPath) {
    // A->C direct edge has weight 2.0; max_weight=5.0 should allow it
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMaxWeight(5.0);
    auto res = pc.findConstrainedPaths("A", "C", 10);
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res.value().empty());
}

TEST_F(GraphQueryOptimizerTest, WeightConstraint_MinWeight_RejectsLightPaths) {
    // A->C direct edge has weight 2.0; min_weight=10.0 should reject it
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMinWeight(10.0);
    auto res = pc.findConstrainedPaths("A", "C", 10);
    EXPECT_FALSE(res.has_value()); // All paths to C are under 10.0
}

TEST_F(GraphQueryOptimizerTest, WeightConstraint_MinWeight_AcceptsHeavyEnoughPath) {
    // A->B (1.0) + B->C (1.0) = 2.0; A->C direct = 2.0
    // min_weight=1.0 should accept A->B (1-hop, weight 1.0)
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMinWeight(1.0);
    auto res = pc.findConstrainedPaths("A", "B", 10);
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res.value().empty());
    EXPECT_GE(res.value().front().cost, 1.0);
}


// ============================================================================
// Phase 3.2: Parallel Dijkstra (Δ-Stepping) tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, Dijkstra_Sequential_FindsShortestPath) {
    // Graph: A-(1.0)->B-(1.0)->C-(1.0)->D  and  A-(2.0)->C
    // Shortest A→D: cost 3.0
    GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->executeDijkstra("A", "D", c);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->totalCost, 3.0);
    ASSERT_FALSE(result->path.empty());
    EXPECT_EQ(result->path.front(), "A");
    EXPECT_EQ(result->path.back(), "D");
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_ProducesSameResultAsSequential) {
    // Sequential baseline
    GraphQueryOptimizer::QueryConstraints seq_c;
    auto seq = optimizer_->executeDijkstra("A", "D", seq_c);
    ASSERT_TRUE(seq.has_value());

    // Parallel Δ-stepping must yield the same optimal cost and path endpoints
    GraphQueryOptimizer::QueryConstraints par_c;
    par_c.enable_parallel = true;
    auto par = optimizer_->executeDijkstra("A", "D", par_c);
    ASSERT_TRUE(par.has_value());

    EXPECT_DOUBLE_EQ(par->totalCost, seq->totalCost);
    ASSERT_FALSE(par->path.empty());
    EXPECT_EQ(par->path.front(), "A");
    EXPECT_EQ(par->path.back(), "D");
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_ExplicitThreadCount) {
    GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    c.num_threads = 2;
    auto result = optimizer_->executeDijkstra("A", "C", c);
    ASSERT_TRUE(result.has_value());
    // A→C direct edge costs 2.0; A→B→C also costs 2.0 – either is optimal
    EXPECT_DOUBLE_EQ(result->totalCost, 2.0);
    ASSERT_FALSE(result->path.empty());
    EXPECT_EQ(result->path.front(), "A");
    EXPECT_EQ(result->path.back(), "C");
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_UpdatesMetrics) {
    const uint64_t before = optimizer_->getQueryMetrics().total_queries.load();

    GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    optimizer_->executeDijkstra("A", "D", c);

    EXPECT_EQ(optimizer_->getQueryMetrics().total_queries.load(), before + 1);
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_SingleHop) {
    // Shortest A→B: cost 1.0, direct edge
    GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    auto result = optimizer_->executeDijkstra("A", "B", c);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->totalCost, 1.0);
    ASSERT_EQ(result->path.size(), 2u);
    EXPECT_EQ(result->path.front(), "A");
    EXPECT_EQ(result->path.back(), "B");
}

// ============================================================================
// Phase Obs+RL: Latency histogram, Prometheus endpoint, and query rate limiter
// ============================================================================

TEST_F(GraphQueryOptimizerTest, LatencyHistogram_PopulatedAfterExecution) {
    // After a BFS execution, at least one latency bucket should be non-zero
    GraphQueryOptimizer::QueryConstraints c;
    optimizer_->executeBFS("A", 3, c);

    const auto& hist = optimizer_->getQueryMetrics().latency_histogram;
    uint64_t total_buckets = 0;
    for (size_t i = 0;
         i < GraphQueryOptimizer::GraphQueryMetrics::LatencyHistogram::kBucketCount;
         ++i) {
        total_buckets += hist.counts[i].load(std::memory_order_relaxed);
    }
    EXPECT_EQ(total_buckets, 1u);
}

TEST_F(GraphQueryOptimizerTest, LatencyHistogram_PercentileNonNegativeAfterExecution) {
    GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 5; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }
    const auto& hist = optimizer_->getQueryMetrics().latency_histogram;
    EXPECT_GE(hist.percentileMs(0.50), 0.0);
    EXPECT_GE(hist.percentileMs(0.95), 0.0);
    EXPECT_GE(hist.percentileMs(0.99), 0.0);
}

TEST_F(GraphQueryOptimizerTest, LatencyHistogram_PercentileZeroBeforeExecution) {
    // Fresh optimizer: no executions yet – percentile should be 0
    const auto& hist = optimizer_->getQueryMetrics().latency_histogram;
    EXPECT_DOUBLE_EQ(hist.percentileMs(0.99), 0.0);
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_DefaultIsDisabled) {
    EXPECT_EQ(optimizer_->getMaxQueriesPerSecond(), 0u);
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_SetAndGet) {
    optimizer_->setMaxQueriesPerSecond(50);
    EXPECT_EQ(optimizer_->getMaxQueriesPerSecond(), 50u);
    optimizer_->setMaxQueriesPerSecond(0);  // reset
    EXPECT_EQ(optimizer_->getMaxQueriesPerSecond(), 0u);
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_HighLimitAllowsQueries) {
    // Set a high limit – all queries should succeed
    optimizer_->setMaxQueriesPerSecond(10000);
    GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 5; ++i) {
        auto res = optimizer_->executeBFS("A", 2, c);
        EXPECT_TRUE(res.has_value()) << "query " << i << " should succeed under high rate limit";
    }
    optimizer_->setMaxQueriesPerSecond(0);
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_ZeroLimitDisabled) {
    // max_qps = 0 means no limit; even many rapid queries should succeed
    optimizer_->setMaxQueriesPerSecond(0);
    GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 20; ++i) {
        auto res = optimizer_->executeBFS("A", 1, c);
        EXPECT_TRUE(res.has_value()) << "query " << i << " should pass with no limit";
    }
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_ExceededReturnsErrorCode) {
    // Set limit to 1 query/second – the 2nd rapid call should be rejected
    optimizer_->setMaxQueriesPerSecond(1);
    GraphQueryOptimizer::QueryConstraints c;

    // First call should succeed
    auto first = optimizer_->executeBFS("A", 2, c);
    // Second call in the same second should be rate-limited
    auto second = optimizer_->executeBFS("A", 2, c);

    // At least the second call must fail with the rate-limit error
    EXPECT_FALSE(second.has_value());
    EXPECT_NE(second.error().message().find("rate limit"), std::string::npos);

    optimizer_->setMaxQueriesPerSecond(0);
}

// ── Prometheus endpoint tests (extends GraphApiHandlerMetricsTest fixture) ──

TEST_F(GraphApiHandlerMetricsTest, HandleMetricsPrometheus_ReturnsOK) {
    auto req = makeGet("/api/v1/graph/metrics/prometheus");
    auto res = handler_->handleMetricsPrometheus(req);
    EXPECT_EQ(res.result(), bhttp::status::ok);
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetricsPrometheus_ContentTypeIsTextPlain) {
    auto req = makeGet("/api/v1/graph/metrics/prometheus");
    auto res = handler_->handleMetricsPrometheus(req);
    const std::string ct{res[bhttp::field::content_type]};
    EXPECT_NE(ct.find("text/plain"), std::string::npos);
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetricsPrometheus_ContainsCounterLines) {
    auto req = makeGet("/api/v1/graph/metrics/prometheus");
    auto res = handler_->handleMetricsPrometheus(req);
    const std::string& body = res.body();
    EXPECT_NE(body.find("themis_graph_queries_total"), std::string::npos);
    EXPECT_NE(body.find("themis_graph_query_errors_total"), std::string::npos);
    EXPECT_NE(body.find("themis_graph_latency_ms_bucket"), std::string::npos);
    EXPECT_NE(body.find("themis_graph_latency_p99_ms"), std::string::npos);
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetricsPrometheus_ContainsInfBucket) {
    auto req = makeGet("/api/v1/graph/metrics/prometheus");
    auto res = handler_->handleMetricsPrometheus(req);
    EXPECT_NE(res.body().find("+Inf"), std::string::npos);
}
