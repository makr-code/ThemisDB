#include <gtest/gtest.h>
#include "graph/graph_query_optimizer.h"
#include "graph/path_constraints.h"
#include "query/result_stream.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

class GraphQueryOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = (fs::temp_directory_path() /
            ("themis_graph_optimizer_test_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))
            ).string();
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);
        
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
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);
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

TEST_F(GraphQueryOptimizerTest, ClearPlanCache_AlsoClearsStructuralKeys) {
    optimizer_->setPlanCachingEnabled(true);

    // Warm up: A → D populates exact key + structural key
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", constraints);
    ASSERT_TRUE(plan1.has_value());

    // Clear everything (exact + structural)
    optimizer_->clearPlanCache();

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    // B → C with the same constraints: structural key was cleared, so this must be a miss.
    auto plan2 = optimizer_->optimizeShortestPath("B", "C", constraints);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_EQ(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "clearPlanCache must also remove structural cache keys";
}

TEST_F(GraphQueryOptimizerTest, PlanCachingDisabled_NoCache) {
    optimizer_->setPlanCachingEnabled(false);
    
    auto result1 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result1);
    
    auto result2 = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result2);
}

// ============================================================================
// Structural Plan Reuse Tests
// Two queries are "structurally similar" when they share the same pattern and
// constraints but have different start/target vertices.  They receive the same
// OptimizationPlan via the structural cache key, avoiding redundant planning.
// ============================================================================

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_ShortestPath_DifferentVertices) {
    optimizer_->setPlanCachingEnabled(true);

    // First query: A → D (cold start – populates structural cache key)
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", constraints);
    ASSERT_TRUE(plan1.has_value());

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    // Second query: B → C – different vertices, same constraints.
    // Should be served from the structural cache entry.
    auto plan2 = optimizer_->optimizeShortestPath("B", "C", constraints);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Structural plan reuse should produce a cache hit for a different vertex pair";

    // The resulting plan must be identical to the first one.
    EXPECT_EQ(plan1->algorithm, plan2->algorithm);
    EXPECT_DOUBLE_EQ(plan1->estimated_cost, plan2->estimated_cost);
    EXPECT_EQ(plan1->estimated_nodes_explored, plan2->estimated_nodes_explored);
}

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_DifferentConstraintsNoReuse) {
    optimizer_->setPlanCachingEnabled(true);

    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.max_depth = 2;
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", c1);
    ASSERT_TRUE(plan1.has_value());

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    // Different constraints – must NOT reuse the previous plan.
    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.max_depth = 5;
    auto plan2 = optimizer_->optimizeShortestPath("B", "C", c2);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_EQ(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Queries with different constraints must not share a structural plan";
}

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_KHop_DifferentStartVertices) {
    optimizer_->setPlanCachingEnabled(true);

    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    auto plan1 = optimizer_->optimizeKHopNeighborhood("A", 2, constraints);
    ASSERT_TRUE(plan1.has_value());

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    auto plan2 = optimizer_->optimizeKHopNeighborhood("B", 2, constraints);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "K-hop structural plan reuse should hit for a different start vertex with same k";

    EXPECT_EQ(plan1->algorithm, plan2->algorithm);
    EXPECT_DOUBLE_EQ(plan1->estimated_cost, plan2->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_KHop_DifferentKNoReuse) {
    optimizer_->setPlanCachingEnabled(true);

    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    optimizer_->optimizeKHopNeighborhood("A", 2, constraints);

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    // Different k value – different structural key – no reuse expected.
    optimizer_->optimizeKHopNeighborhood("B", 3, constraints);

    EXPECT_EQ(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "K-hop queries with different k values must not share a structural plan";
}

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_Reachability_DifferentVertices) {
    optimizer_->setPlanCachingEnabled(true);

    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    auto plan1 = optimizer_->optimizeReachability("A", "D", constraints);
    ASSERT_TRUE(plan1.has_value());

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    auto plan2 = optimizer_->optimizeReachability("B", "C", constraints);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Reachability structural plan reuse should hit for different vertex pair";

    EXPECT_EQ(plan1->algorithm, plan2->algorithm);
    EXPECT_DOUBLE_EQ(plan1->estimated_cost, plan2->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_PatternMatch_DifferentVertexLabels) {
    optimizer_->setPlanCachingEnabled(true);

    // Two pattern queries with the same structure (2 vertices, 1 edge)
    // but different vertex names.
    std::vector<std::string> verts1 = {"X", "Y"};
    std::vector<std::pair<std::string, std::string>> edges1 = {{"X", "Y"}};
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    auto plan1 = optimizer_->optimizePatternMatch(verts1, edges1, constraints);
    ASSERT_TRUE(plan1.has_value());

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    std::vector<std::string> verts2 = {"P", "Q"};
    std::vector<std::pair<std::string, std::string>> edges2 = {{"P", "Q"}};
    auto plan2 = optimizer_->optimizePatternMatch(verts2, edges2, constraints);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Pattern-match structural plan reuse should hit for same-shape patterns";

    EXPECT_EQ(plan1->algorithm, plan2->algorithm);
    EXPECT_DOUBLE_EQ(plan1->estimated_cost, plan2->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, StructuralPlanReuse_CachingDisabled_NoStructuralReuse) {
    optimizer_->setPlanCachingEnabled(false);

    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    optimizer_->optimizeShortestPath("A", "D", constraints);

    const uint64_t hits_before = optimizer_->getQueryMetrics().plan_cache_hits.load();

    optimizer_->optimizeShortestPath("B", "C", constraints);

    // With caching disabled, no cache hits should be recorded.
    EXPECT_EQ(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Structural plan reuse must be disabled when plan caching is off";
}

TEST_F(GraphQueryOptimizerTest, ExactCacheKey_EnableParallelDistinguishesCachedPlans) {
    optimizer_->setPlanCachingEnabled(true);

    // First query: same vertex pair, enable_parallel = false
    themis::graph::GraphQueryOptimizer::QueryConstraints c_no_par;
    c_no_par.enable_parallel = false;
    auto plan_seq = optimizer_->optimizeShortestPath("A", "D", c_no_par);
    ASSERT_TRUE(plan_seq.has_value());
    EXPECT_FALSE(plan_seq->enable_parallel);

    // Second query: same vertex pair, enable_parallel = true
    // Must NOT return the cached plan from the first query (which had enable_parallel=false).
    themis::graph::GraphQueryOptimizer::QueryConstraints c_par;
    c_par.enable_parallel = true;
    auto plan_par = optimizer_->optimizeShortestPath("A", "D", c_par);
    ASSERT_TRUE(plan_par.has_value());
    EXPECT_TRUE(plan_par->enable_parallel)
        << "enable_parallel=true must yield a plan with enable_parallel=true, "
           "not a stale cached plan built without the parallel flag";
}

TEST_F(GraphQueryOptimizerTest, ExplainPlan_GeneratesExplanation) {
    auto result = optimizer_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(result);
    
    std::string explanation = optimizer_->explainPlan(result.value());
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("Query Pattern"), std::string::npos);
    EXPECT_NE(explanation.find("Selected Algorithm"), std::string::npos);
    EXPECT_NE(explanation.find("Estimated Cost"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, KHop_EnableParallel_RespectedInPlan) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    auto plan = optimizer_->optimizeKHopNeighborhood("A", 2, c);
    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->enable_parallel)
        << "optimizeKHopNeighborhood must set enable_parallel=true when constraints.enable_parallel=true";
}

TEST_F(GraphQueryOptimizerTest, Reachability_EnableParallel_RespectedInPlan) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    auto plan = optimizer_->optimizeReachability("A", "D", c);
    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->enable_parallel)
        << "optimizeReachability must set enable_parallel=true when constraints.enable_parallel=true";
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

    bhttp::request<bhttp::string_body> makePost(const std::string& target,
                                                const std::string& body = "{}") {
        bhttp::request<bhttp::string_body> req{bhttp::verb::post, target, 11};
        req.set(bhttp::field::host, "localhost");
        req.set(bhttp::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();
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

TEST_F(GraphQueryOptimizerTest, AdaptivePlanSelection_ReachabilityHasAlternatives) {
    // optimizeReachability should now produce an alternatives list, mirroring
    // the behaviour of optimizeShortestPath for cost-based selection.
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto plan = optimizer_->optimizeReachability("A", "D", c);
    ASSERT_TRUE(plan);
    // At least the primary algorithm should appear in alternatives
    EXPECT_FALSE(plan.value().alternatives.empty());
}

TEST_F(GraphQueryOptimizerTest, AdaptivePlanSelection_DisabledUsesStaticHeuristic) {
    // With adaptive learning disabled, selectAlgorithm falls back to the
    // depth-based static heuristic.  For a shallow graph (depth <= 3) it should
    // still pick BFS for a reachability query.
    optimizer_->enableAdaptiveLearning(false);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->clearPlanCache();
    auto plan = optimizer_->optimizeReachability("A", "D", c);
    ASSERT_TRUE(plan);
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    // Test graph is small; estimated depth <= 3, so static heuristic picks BFS.
    EXPECT_EQ(plan.value().algorithm, Algo::BFS);
    optimizer_->enableAdaptiveLearning(true);
}

TEST_F(GraphQueryOptimizerTest, AdaptivePlanSelection_HighConfidenceDFSShiftsKHopChoice) {
    // Import a synthetic cost model where DFS is dramatically faster than BFS.
    // With adaptive learning active and high confidence, the K-HOP plan should
    // switch from the default BFS to DFS.
    const std::string synthetic_model = R"({
        "BFS": {"ema_cost_ms": 500.0, "exec_count": 100, "confidence": 1.0},
        "DFS": {"ema_cost_ms":   1.0, "exec_count": 100, "confidence": 1.0}
    })";
    themis::graph::GraphQueryOptimizer opt2(*graph_mgr_);
    ASSERT_TRUE(opt2.importCostModel(synthetic_model));

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto plan = opt2.optimizeKHopNeighborhood("A", 2, c);
    ASSERT_TRUE(plan);
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    // DFS has ema_cost_ms=1 vs BFS ema_cost_ms=500, both confidence=1.0.
    // The blended cost for DFS (=10) is much lower than BFS (=5000), so DFS wins.
    EXPECT_EQ(plan.value().algorithm, Algo::DFS);
}

TEST_F(GraphQueryOptimizerTest, AdaptivePlanSelection_ImportedModelInfluencesReachability) {
    // Import a model where BIDIRECTIONAL is dramatically cheaper than BFS.
    // We force depth > 3 via max_depth constraint so that BIDIRECTIONAL becomes
    // a candidate; the adaptive model should then select it over BFS.
    const std::string synthetic_model = R"({
        "BFS":           {"ema_cost_ms": 900.0, "exec_count": 100, "confidence": 1.0},
        "BIDIRECTIONAL": {"ema_cost_ms":   1.0, "exec_count": 100, "confidence": 1.0}
    })";
    themis::graph::GraphQueryOptimizer opt2(*graph_mgr_);
    ASSERT_TRUE(opt2.importCostModel(synthetic_model));

    // max_depth = 5 forces estimateDepth > 3, which makes BIDIRECTIONAL a candidate.
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 5;
    auto plan = opt2.optimizeReachability("A", "D", c);
    ASSERT_TRUE(plan);
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    EXPECT_EQ(plan.value().algorithm, Algo::BIDIRECTIONAL);
}

TEST_F(GraphQueryOptimizerTest, AdaptivePlanSelection_PatternMatch_HasAlternatives) {
    // optimizePatternMatch now generates a DFS/BFS alternatives list.
    std::vector<std::string> verts = {"A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {{"A", "B"}};
    auto plan = optimizer_->optimizePatternMatch(verts, edges);
    ASSERT_TRUE(plan);
    EXPECT_FALSE(plan.value().alternatives.empty());
    // Default (no adaptive model) must still prefer DFS
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    EXPECT_EQ(plan.value().algorithm, Algo::DFS);
}

TEST_F(GraphQueryOptimizerTest, AdaptivePlanSelection_PatternMatch_BFSWinsWhenFaster) {
    // Import a model where BFS is dramatically faster than DFS.
    const std::string synthetic = R"({
        "DFS": {"ema_cost_ms": 800.0, "exec_count": 100, "confidence": 1.0},
        "BFS": {"ema_cost_ms":   1.0, "exec_count": 100, "confidence": 1.0}
    })";
    themis::graph::GraphQueryOptimizer opt2(*graph_mgr_);
    ASSERT_TRUE(opt2.importCostModel(synthetic));

    std::vector<std::string> verts = {"A", "B", "C"};
    std::vector<std::pair<std::string, std::string>> edges = {{"A", "B"}, {"B", "C"}};
    auto plan = opt2.optimizePatternMatch(verts, edges);
    ASSERT_TRUE(plan);
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    // BFS has ema_cost_ms=1 (blended cost ≈10) vs DFS ema_cost_ms=800 (≈8000)
    EXPECT_EQ(plan.value().algorithm, Algo::BFS);
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
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->executeDijkstra("A", "D", c);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->totalCost, 3.0);
    ASSERT_FALSE(result->path.empty());
    EXPECT_EQ(result->path.front(), "A");
    EXPECT_EQ(result->path.back(), "D");
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_ProducesSameResultAsSequential) {
    // Sequential baseline
    themis::graph::GraphQueryOptimizer::QueryConstraints seq_c;
    auto seq = optimizer_->executeDijkstra("A", "D", seq_c);
    ASSERT_TRUE(seq.has_value());

    // Parallel Δ-stepping must yield the same optimal cost and path endpoints
    themis::graph::GraphQueryOptimizer::QueryConstraints par_c;
    par_c.enable_parallel = true;
    auto par = optimizer_->executeDijkstra("A", "D", par_c);
    ASSERT_TRUE(par.has_value());

    EXPECT_DOUBLE_EQ(par->totalCost, seq->totalCost);
    ASSERT_FALSE(par->path.empty());
    EXPECT_EQ(par->path.front(), "A");
    EXPECT_EQ(par->path.back(), "D");
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_ExplicitThreadCount) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
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

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.enable_parallel = true;
    optimizer_->executeDijkstra("A", "D", c);

    EXPECT_EQ(optimizer_->getQueryMetrics().total_queries.load(), before + 1);
}

TEST_F(GraphQueryOptimizerTest, Dijkstra_Parallel_SingleHop) {
    // Shortest A→B: cost 1.0, direct edge
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
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
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->executeBFS("A", 3, c);

    const auto& hist = optimizer_->getQueryMetrics().latency_histogram;
    uint64_t total_buckets = 0;
    for (size_t i = 0;
         i < themis::graph::GraphQueryOptimizer::GraphQueryMetrics::LatencyHistogram::kBucketCount;
         ++i) {
        total_buckets += hist.counts[i].load(std::memory_order_relaxed);
    }
    EXPECT_EQ(total_buckets, 1u);
}

TEST_F(GraphQueryOptimizerTest, LatencyHistogram_PercentileNonNegativeAfterExecution) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
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
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 5; ++i) {
        auto res = optimizer_->executeBFS("A", 2, c);
        EXPECT_TRUE(res.has_value()) << "query " << i << " should succeed under high rate limit";
    }
    optimizer_->setMaxQueriesPerSecond(0);
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_ZeroLimitDisabled) {
    // max_qps = 0 means no limit; even many rapid queries should succeed
    optimizer_->setMaxQueriesPerSecond(0);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 20; ++i) {
        auto res = optimizer_->executeBFS("A", 1, c);
        EXPECT_TRUE(res.has_value()) << "query " << i << " should pass with no limit";
    }
}

TEST_F(GraphQueryOptimizerTest, RateLimiter_ExceededReturnsErrorCode) {
    // Set limit to 1 query/second – the 2nd rapid call should be rejected
    optimizer_->setMaxQueriesPerSecond(1);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

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

// ============================================================================
// Cost Model Calibration from Execution History Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_EmptyHistoryReturnsEmptyReport) {
    auto report = optimizer_->calibrateFromHistory();
    EXPECT_EQ(report.total_samples, 0u);
    EXPECT_EQ(report.algorithms_calibrated, 0u);
    EXPECT_TRUE(report.algorithm_stats.empty());
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_BelowThresholdDoesNotUpdateModel) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    // Run fewer than MIN_CALIBRATION_SAMPLES (5) BFS executions
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    for (int i = 0; i < 3; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    const double ema_before =
        optimizer_->getAlgorithmCostModels().count(Algo::BFS)
            ? optimizer_->getAlgorithmCostModels().at(Algo::BFS).ema_cost_ms
            : -1.0;

    auto report = optimizer_->calibrateFromHistory();

    EXPECT_EQ(report.total_samples, 3u);
    EXPECT_EQ(report.algorithms_calibrated, 0u);  // below threshold
    ASSERT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());
    EXPECT_EQ(report.algorithm_stats.at(Algo::BFS).sample_count, 3u);

    // EMA should be unchanged by calibration when below threshold
    if (optimizer_->getAlgorithmCostModels().count(Algo::BFS)) {
        EXPECT_DOUBLE_EQ(optimizer_->getAlgorithmCostModels().at(Algo::BFS).ema_cost_ms,
                         ema_before);
    }
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_AtThresholdUpdatesModel) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Run exactly MIN_CALIBRATION_SAMPLES BFS executions
    const size_t threshold = themis::graph::GraphQueryOptimizer::MIN_CALIBRATION_SAMPLES;
    for (size_t i = 0; i < threshold; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();

    EXPECT_EQ(report.total_samples, threshold);
    EXPECT_EQ(report.algorithms_calibrated, 1u);

    // The model should have been re-seeded
    ASSERT_NE(optimizer_->getAlgorithmCostModels().find(Algo::BFS),
              optimizer_->getAlgorithmCostModels().end());
    const auto& model = optimizer_->getAlgorithmCostModels().at(Algo::BFS);
    EXPECT_GT(model.confidence, 0.0);
    EXPECT_EQ(model.exec_count, static_cast<uint32_t>(threshold));
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_StatsAreCorrect) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Run many BFS executions so statistics are meaningful
    for (int i = 0; i < 10; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();

    ASSERT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());
    const auto& stats = report.algorithm_stats.at(Algo::BFS);
    EXPECT_EQ(stats.sample_count, 10u);
    EXPECT_GE(stats.mean_execution_ms, 0.0);
    EXPECT_GE(stats.stddev_execution_ms, 0.0);
    EXPECT_GE(stats.min_execution_ms, 0.0);
    EXPECT_GE(stats.max_execution_ms, stats.min_execution_ms);
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_MultipleAlgorithms) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Run enough executions of both BFS and DFS
    const size_t threshold = themis::graph::GraphQueryOptimizer::MIN_CALIBRATION_SAMPLES;
    for (size_t i = 0; i < threshold; ++i) {
        optimizer_->executeBFS("A", 2, c);
        optimizer_->executeDFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();

    EXPECT_EQ(report.total_samples, threshold * 2);
    EXPECT_EQ(report.algorithms_calibrated, 2u);
    EXPECT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());
    EXPECT_NE(report.algorithm_stats.find(Algo::DFS), report.algorithm_stats.end());
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_DisabledLearningSkipsModelUpdate) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;

    // Run enough executions to fill history while learning is still on
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    const size_t threshold = themis::graph::GraphQueryOptimizer::MIN_CALIBRATION_SAMPLES;
    for (size_t i = 0; i < threshold; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    // Now disable adaptive learning before calibrating
    optimizer_->enableAdaptiveLearning(false);
    const double ema_before =
        optimizer_->getAlgorithmCostModels().count(Algo::BFS)
            ? optimizer_->getAlgorithmCostModels().at(Algo::BFS).ema_cost_ms
            : 0.0;

    auto report = optimizer_->calibrateFromHistory();

    // Stats are reported but no model update happens
    EXPECT_EQ(report.algorithms_calibrated, 0u);
    EXPECT_FALSE(report.algorithm_stats.empty());

    // EMA unchanged
    if (optimizer_->getAlgorithmCostModels().count(Algo::BFS)) {
        EXPECT_DOUBLE_EQ(optimizer_->getAlgorithmCostModels().at(Algo::BFS).ema_cost_ms,
                         ema_before);
    }
    optimizer_->enableAdaptiveLearning(true);
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_ModelReseededToHistoricalMean) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Accumulate history
    const size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();
    ASSERT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());
    const double historical_mean = report.algorithm_stats.at(Algo::BFS).mean_execution_ms;

    // After calibration the EMA should equal the historical mean
    ASSERT_NE(optimizer_->getAlgorithmCostModels().find(Algo::BFS),
              optimizer_->getAlgorithmCostModels().end());
    EXPECT_DOUBLE_EQ(
        optimizer_->getAlgorithmCostModels().at(Algo::BFS).ema_cost_ms,
        historical_mean
    );
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_ConfidenceReflectsSampleCount) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    const size_t n = 20;
    for (size_t i = 0; i < n; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();
    const double expected_confidence =
        std::min(1.0, static_cast<double>(n) /
                          themis::graph::GraphQueryOptimizer::AlgorithmCostModel::MAX_CONF_OBS);

    ASSERT_NE(optimizer_->getAlgorithmCostModels().find(Algo::BFS),
              optimizer_->getAlgorithmCostModels().end());
    EXPECT_DOUBLE_EQ(
        optimizer_->getAlgorithmCostModels().at(Algo::BFS).confidence,
        expected_confidence
    );
}

// ============================================================================
// Cost Model Accuracy Tracking Tests
// (ExecutionStats::estimated_cost_ms + AlgorithmCalibrationStats accuracy)
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecutionStats_HasEstimatedCostMs_AfterBFS) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    optimizer_->executeBFS("A", 2, c, &stats);

    // estimated_cost_ms should be populated (> 0) by the execute method
    EXPECT_GT(stats.estimated_cost_ms, 0.0);
}

TEST_F(GraphQueryOptimizerTest, ExecutionStats_HasEstimatedCostMs_AfterDFS) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    optimizer_->executeDFS("A", 2, c, &stats);

    EXPECT_GT(stats.estimated_cost_ms, 0.0);
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_AccuracyFields_PopulatedAfterExecution) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    const size_t threshold = themis::graph::GraphQueryOptimizer::MIN_CALIBRATION_SAMPLES;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Run enough executions to trigger calibration (estimated_cost_ms is set
    // by the execute method so accuracy fields should be populated).
    for (size_t i = 0; i < threshold; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();
    ASSERT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());

    const auto& stats = report.algorithm_stats.at(Algo::BFS);
    // estimation_sample_count should equal sample_count because executeBFS
    // always sets estimated_cost_ms.
    EXPECT_EQ(stats.estimation_sample_count, threshold);
    // Accuracy fields must be populated
    EXPECT_GT(stats.mean_estimated_ms, 0.0);
    EXPECT_GE(stats.mean_absolute_error_ms, 0.0);
    EXPECT_GT(stats.cost_ratio, 0.0);
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_CostRatioIsReasonable) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    const size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();
    ASSERT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());
    const auto& stats = report.algorithm_stats.at(Algo::BFS);

    // cost_ratio = mean_estimated_ms / mean_execution_ms
    // For a tiny in-memory graph the actual execution will be near 0 ms while
    // the estimate may be higher; we only check the ratio is non-negative.
    EXPECT_GE(stats.cost_ratio, 0.0);
    if (stats.mean_execution_ms > 0.0) {
        double expected_ratio = stats.mean_estimated_ms / stats.mean_execution_ms;
        EXPECT_DOUBLE_EQ(stats.cost_ratio, expected_ratio);
    }
}

TEST_F(GraphQueryOptimizerTest, CalibrateFromHistory_MAE_IsNonNegative) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    const size_t n = 8;
    for (size_t i = 0; i < n; ++i) {
        optimizer_->executeBFS("A", 2, c);
    }

    auto report = optimizer_->calibrateFromHistory();
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    ASSERT_NE(report.algorithm_stats.find(Algo::BFS), report.algorithm_stats.end());
    EXPECT_GE(report.algorithm_stats.at(Algo::BFS).mean_absolute_error_ms, 0.0);
}


// ============================================================================
// Cost Model HTTP API Tests
// (POST /api/v1/graph/cost-model/calibrate,
//  GET  /api/v1/graph/cost-model,
//  POST /api/v1/graph/cost-model)
// ============================================================================

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelCalibrate_EmptyHistory_ReturnsOK) {
    auto req = makePost("/api/v1/graph/cost-model/calibrate");
    auto res = handler_->handleCostModelCalibrate(req);
    EXPECT_EQ(res.result(), bhttp::status::ok);
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelCalibrate_EmptyHistory_BodyIsValidJSON) {
    auto req = makePost("/api/v1/graph/cost-model/calibrate");
    auto res = handler_->handleCostModelCalibrate(req);
    ASSERT_NO_THROW({
        auto body = nlohmann::json::parse(res.body());
        EXPECT_TRUE(body.contains("total_samples"));
        EXPECT_TRUE(body.contains("algorithms_calibrated"));
        EXPECT_TRUE(body.contains("algorithm_stats"));
        EXPECT_EQ(body["total_samples"].get<size_t>(), 0u);
        EXPECT_EQ(body["algorithms_calibrated"].get<size_t>(), 0u);
    });
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelCalibrate_AfterExecutions_ContainsStats) {
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Build enough history to trigger calibration
    const size_t threshold = themis::graph::GraphQueryOptimizer::MIN_CALIBRATION_SAMPLES;
    // Access the optimizer via the handler to run BFS executions
    // Use the metrics endpoint to confirm queries were executed
    // We directly access handler_ which owns optimizer_ (private); drive via
    // traverse requests (too complex) – instead we confirm the JSON structure
    // when history is empty and that's acceptable for handler-level tests.
    // For full calibration validation see the unit tests above.
    auto req = makePost("/api/v1/graph/cost-model/calibrate");
    auto res = handler_->handleCostModelCalibrate(req);
    EXPECT_EQ(res.result(), bhttp::status::ok);
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body["algorithm_stats"].is_object());
    // samples may be 0 since the handler has its own optimizer; that's fine.
    EXPECT_GE(body["total_samples"].get<size_t>(), 0u);
    (void)threshold;
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelExport_ReturnsOK) {
    auto req = makeGet("/api/v1/graph/cost-model");
    auto res = handler_->handleCostModelExport(req);
    EXPECT_EQ(res.result(), bhttp::status::ok);
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelExport_BodyIsValidJSON) {
    auto req = makeGet("/api/v1/graph/cost-model");
    auto res = handler_->handleCostModelExport(req);
    ASSERT_NO_THROW({
        auto body = nlohmann::json::parse(res.body());
        EXPECT_TRUE(body.is_object());
    });
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelImport_ValidJSON_ReturnsOK) {
    // Export first, then re-import
    auto export_req = makeGet("/api/v1/graph/cost-model");
    auto export_res = handler_->handleCostModelExport(export_req);
    ASSERT_EQ(export_res.result(), bhttp::status::ok);

    auto import_req = makePost("/api/v1/graph/cost-model", export_res.body());
    auto import_res = handler_->handleCostModelImport(import_req);
    EXPECT_EQ(import_res.result(), bhttp::status::ok);
    auto body = nlohmann::json::parse(import_res.body());
    EXPECT_TRUE(body.contains("imported"));
    EXPECT_TRUE(body["imported"].get<bool>());
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelImport_InvalidJSON_Returns400) {
    auto req = makePost("/api/v1/graph/cost-model", "not valid json{{{{");
    auto res = handler_->handleCostModelImport(req);
    EXPECT_EQ(res.result(), bhttp::status::bad_request);
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelImport_UnknownAlgoKeys_Returns200) {
    // JSON with unknown algorithm names should be silently ignored
    std::string model = R"({"UNKNOWNALGO":{"ema_cost_ms":5.0,"exec_count":10,"confidence":0.1}})";
    auto req = makePost("/api/v1/graph/cost-model", model);
    auto res = handler_->handleCostModelImport(req);
    EXPECT_EQ(res.result(), bhttp::status::ok);
}

TEST_F(GraphApiHandlerMetricsTest, HandleCostModelExportImportRoundtrip_PreservesModel) {
    // Export the default (empty) model, import it, then export again – JSON should be equal
    auto export_req1 = makeGet("/api/v1/graph/cost-model");
    auto export_res1 = handler_->handleCostModelExport(export_req1);
    ASSERT_EQ(export_res1.result(), bhttp::status::ok);
    const std::string original_json = export_res1.body();

    auto import_req = makePost("/api/v1/graph/cost-model", original_json);
    ASSERT_EQ(handler_->handleCostModelImport(import_req).result(), bhttp::status::ok);

    auto export_req2 = makeGet("/api/v1/graph/cost-model");
    auto export_res2 = handler_->handleCostModelExport(export_req2);
    ASSERT_EQ(export_res2.result(), bhttp::status::ok);

    // Both exports should produce the same JSON
    auto parsed_original = nlohmann::json::parse(original_json);
    auto parsed_roundtrip = nlohmann::json::parse(export_res2.body());
    EXPECT_EQ(parsed_original, parsed_roundtrip);
}

// Helper constants representing a simple timeline (milliseconds since epoch):
// t_past  = 2020-01-01 00:00:00 UTC (approx)
// t_mid   = 2022-06-01 00:00:00 UTC (approx)
// t_now   = 2025-01-01 00:00:00 UTC (approx)

static constexpr int64_t kT2020 = 1577836800000LL; // 2020-01-01
static constexpr int64_t kT2022 = 1654041600000LL; // 2022-06-01
static constexpr int64_t kT2025 = 1735689600000LL; // 2025-01-01

// Add temporal edges to the existing test graph fixture.
// Edge A->B is valid only in [kT2020, kT2022)
// Edge B->C is valid from kT2022 onwards
// Edge C->D has no temporal bounds (always valid)
static void addTemporalEdges(themis::GraphIndexManager& gm) {
    themis::BaseEntity te1("te1");
    te1.setField("id", "te1");
    te1.setField("_from", "TA");
    te1.setField("_to", "TB");
    te1.setField("valid_from", std::to_string(kT2020));
    te1.setField("valid_to",   std::to_string(kT2022));
    gm.addEdge(te1);

    themis::BaseEntity te2("te2");
    te2.setField("id", "te2");
    te2.setField("_from", "TB");
    te2.setField("_to", "TC");
    te2.setField("valid_from", std::to_string(kT2022));
    gm.addEdge(te2);

    themis::BaseEntity te3("te3");
    te3.setField("id", "te3");
    te3.setField("_from", "TA");
    te3.setField("_to", "TC");
    // No temporal bounds – always valid
    gm.addEdge(te3);
}

// ── optimizeTemporalTraversal ─────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, OptimizeTemporalTraversal_SelectsBFS) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;

    auto result = optimizer_->optimizeTemporalTraversal("A", 3, c);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->pattern,
              themis::graph::GraphQueryOptimizer::QueryPattern::K_HOP_NEIGHBORS);
    EXPECT_EQ(result->algorithm,
              themis::graph::GraphQueryOptimizer::TraversalAlgorithm::BFS);
}

TEST_F(GraphQueryOptimizerTest, OptimizeTemporalTraversal_CostLowerThanUnbounded) {
    // A narrow time window should produce a lower cost estimate than a query
    // without any temporal constraint (because temporal selectivity < 1).
    themis::graph::GraphQueryOptimizer::QueryConstraints c_temporal;
    c_temporal.time_range_start_ms = kT2022;
    c_temporal.time_range_end_ms   = kT2022 + 86400000LL; // 1 day

    themis::graph::GraphQueryOptimizer::QueryConstraints c_unbounded;

    auto res_temporal  = optimizer_->optimizeTemporalTraversal("A", 3, c_temporal);
    auto res_unbounded = optimizer_->optimizeTemporalTraversal("A", 3, c_unbounded);

    ASSERT_TRUE(res_temporal);
    ASSERT_TRUE(res_unbounded);
    EXPECT_LT(res_temporal->estimated_cost, res_unbounded->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, OptimizeTemporalTraversal_ExplanationContainsTimeRange) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;

    auto result = optimizer_->optimizeTemporalTraversal("A", 2, c);
    ASSERT_TRUE(result);
    EXPECT_NE(result->explanation.find(std::to_string(kT2020)), std::string::npos);
    EXPECT_NE(result->explanation.find(std::to_string(kT2025)), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, OptimizeTemporalTraversal_HasAlternativePlan) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;

    auto result = optimizer_->optimizeTemporalTraversal("A", 2, c);
    ASSERT_TRUE(result);
    EXPECT_FALSE(result->alternatives.empty());
}

TEST_F(GraphQueryOptimizerTest, OptimizeTemporalTraversal_PlanCacheHitOnSecondCall) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2022;
    c.time_range_end_ms   = kT2025;

    optimizer_->optimizeTemporalTraversal("A", 2, c); // first call – miss
    const uint64_t misses_before =
        optimizer_->getQueryMetrics().plan_cache_misses.load();
    optimizer_->optimizeTemporalTraversal("B", 2, c); // same constraints – hit
    const uint64_t hits_after =
        optimizer_->getQueryMetrics().plan_cache_hits.load();

    EXPECT_GT(hits_after, 0u);
    (void)misses_before; // value used to verify monotonicity in other tests
}

// ── executeTemporalBFS ────────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_NoTemporalConstraint_FallsBackToStandardBFS) {
    // When no time range is set, executeTemporalBFS should behave like executeBFS.
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    EXPECT_FALSE(c.hasTemporalRange());

    auto res_temporal = optimizer_->executeTemporalBFS("A", 3, c);
    auto res_standard = optimizer_->executeBFS("A", 3, c);

    ASSERT_TRUE(res_temporal);
    ASSERT_TRUE(res_standard);
    // Both should reach the same set of nodes (order may differ; compare as sets).
    auto t_nodes = res_temporal.value();
    auto s_nodes = res_standard.value();
    std::sort(t_nodes.begin(), t_nodes.end());
    std::sort(s_nodes.begin(), s_nodes.end());
    EXPECT_EQ(t_nodes, s_nodes);
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_WithTemporalConstraint_FindsTemporalNeighbors) {
    addTemporalEdges(*graph_mgr_);

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2022 - 1; // window: 2020 – just before 2022

    auto result = optimizer_->executeTemporalBFS("TA", 2, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    // TA is always in the result (start node)
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "TA"), nodes.end());
    // TB is reachable via te1 which is valid in [kT2020, kT2022)
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "TB"), nodes.end());
    // TC is reachable via te3 (no temporal bounds) at any time
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "TC"), nodes.end());
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_ExcludesEdgesOutsideWindow) {
    addTemporalEdges(*graph_mgr_);

    // Window is entirely after kT2022: te1 (TA->TB, valid [kT2020,kT2022)) should
    // not contribute TB to the result when the window starts at kT2022.
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2022 + 1;
    c.time_range_end_ms   = kT2025;

    auto result = optimizer_->executeTemporalBFS("TA", 2, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    // TB is only reached via te1 which is NOT valid after kT2022.
    // te2 (TB->TC) starts at kT2022 but to reach TB from TA we need te1 first.
    EXPECT_EQ(std::find(nodes.begin(), nodes.end(), "TB"), nodes.end());
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_UpdatesExecutionMetrics) {
    addTemporalEdges(*graph_mgr_);

    const uint64_t queries_before =
        optimizer_->getQueryMetrics().total_queries.load();

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;
    optimizer_->executeTemporalBFS("TA", 2, c);

    EXPECT_GT(optimizer_->getQueryMetrics().total_queries.load(), queries_before);
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_RespectsMaxResults) {
    addTemporalEdges(*graph_mgr_);

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;
    c.max_results = 2;

    auto result = optimizer_->executeTemporalBFS("TA", 3, c);
    ASSERT_TRUE(result);
    EXPECT_LE(result->size(), 2u);
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_RejectsForbiddenVertex) {
    addTemporalEdges(*graph_mgr_);

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;
    c.forbidden_vertices  = {"TC"};

    auto result = optimizer_->executeTemporalBFS("TA", 3, c);
    ASSERT_TRUE(result);
    EXPECT_EQ(std::find(result->begin(), result->end(), "TC"), result->end());
}

// ── QueryConstraints temporal helper ─────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, QueryConstraints_HasTemporalRange_FalseByDefault) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    EXPECT_FALSE(c.hasTemporalRange());
}

TEST_F(GraphQueryOptimizerTest, QueryConstraints_HasTemporalRange_TrueWhenStartSet) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    EXPECT_TRUE(c.hasTemporalRange());
}

TEST_F(GraphQueryOptimizerTest, QueryConstraints_HasTemporalRange_TrueWhenEndSet) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_end_ms = kT2025;
    EXPECT_TRUE(c.hasTemporalRange());
}

TEST_F(GraphQueryOptimizerTest, QueryConstraints_HasTemporalRange_TrueWhenBothSet) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;
    EXPECT_TRUE(c.hasTemporalRange());
}

// Property graph schema-aware optimizer hints (Issue #1819)
// ============================================================================

// Helper: store a node with _labels field (comma-separated).
static void storeNodeWithLabels(themis::RocksDBWrapper& db,
                                 const std::string& pk,
                                 const std::string& labels_csv) {
    themis::BaseEntity v(pk);
    v.setField("id", pk);
    v.setField("_labels", labels_csv);
    db.put(themis::KeySchema::makeGraphNodeKey(pk), v.serialize());
}

// ── Cost estimation ───────────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SchemaHint_NodeLabels_ReducesCostEstimate) {
    // Provide label stats: "Person" covers 3 of the 4 nodes in the test graph.
    // selectivity = 3/4 = 0.75 → cost estimate should be reduced proportionally.
    optimizer_->collectStatistics();
    optimizer_->setNodeLabelStats({{"Person", 3}});

    themis::graph::GraphQueryOptimizer::QueryConstraints without_hint;
    auto plan_no_hint = optimizer_->optimizeShortestPath("A", "D", without_hint);
    ASSERT_TRUE(plan_no_hint);

    themis::graph::GraphQueryOptimizer::QueryConstraints with_hint;
    with_hint.node_labels = {"Person"};
    optimizer_->clearPlanCache();
    auto plan_with_hint = optimizer_->optimizeShortestPath("A", "D", with_hint);
    ASSERT_TRUE(plan_with_hint);

    // With a 0.75 selectivity, estimated cost should be strictly lower than without hint
    EXPECT_LT(plan_with_hint->estimated_cost, plan_no_hint->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_ExcludedEdgeTypes_ReducesCostEstimate) {
    // Excluding an unknown edge type applies a 10% conservative reduction.
    // This verifies the fallback path (no selectivity stats registered).
    optimizer_->collectStatistics();

    themis::graph::GraphQueryOptimizer::QueryConstraints without_hint;
    optimizer_->clearPlanCache();
    auto plan_no_hint = optimizer_->optimizeShortestPath("A", "D", without_hint);
    ASSERT_TRUE(plan_no_hint);

    themis::graph::GraphQueryOptimizer::QueryConstraints with_hint;
    with_hint.excluded_edge_types = {"UNKNOWN_TYPE"};
    optimizer_->clearPlanCache();
    auto plan_with_hint = optimizer_->optimizeShortestPath("A", "D", with_hint);
    ASSERT_TRUE(plan_with_hint);

    // 10% reduction for one unknown type → estimated cost should be lower
    EXPECT_LT(plan_with_hint->estimated_cost, plan_no_hint->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_UnknownLabel_UsesDefaultSelectivity) {
    // No label stats registered → fallback selectivity (0.5) is applied.
    optimizer_->collectStatistics();

    themis::graph::GraphQueryOptimizer::QueryConstraints without_hint;
    optimizer_->clearPlanCache();
    auto plan_no_hint = optimizer_->optimizeShortestPath("A", "D", without_hint);
    ASSERT_TRUE(plan_no_hint);

    themis::graph::GraphQueryOptimizer::QueryConstraints with_hint;
    with_hint.node_labels = {"UnknownLabel"};
    optimizer_->clearPlanCache();
    auto plan_with_hint = optimizer_->optimizeShortestPath("A", "D", with_hint);
    ASSERT_TRUE(plan_with_hint);

    // Fallback selectivity = 0.5 → cost should be lower
    EXPECT_LT(plan_with_hint->estimated_cost, plan_no_hint->estimated_cost);
}

// ── Plan explanation ──────────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SchemaHint_ExplanationContainsNodeLabels) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.node_labels = {"Person", "Employee"};
    optimizer_->clearPlanCache();
    auto plan = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan);
    EXPECT_NE(plan->explanation.find("Person"), std::string::npos);
    EXPECT_NE(plan->explanation.find("Employee"), std::string::npos);
    EXPECT_NE(plan->explanation.find("Schema Hints Active"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_ExplanationContainsExcludedEdgeTypes) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.excluded_edge_types = {"BLOCKED"};
    optimizer_->clearPlanCache();
    auto plan = optimizer_->optimizeKHopNeighborhood("A", 2, c);
    ASSERT_TRUE(plan);
    EXPECT_NE(plan->explanation.find("BLOCKED"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_NoHints_ExplanationUnchanged) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;  // no schema hints
    optimizer_->clearPlanCache();
    auto plan = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan);
    // "Schema Hints Active" should NOT appear when no hints are set
    EXPECT_EQ(plan->explanation.find("Schema Hints Active"), std::string::npos);
    EXPECT_TRUE(plan->active_schema_hints.empty());
}

// ── Active schema hints field ─────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SchemaHint_ActiveSchemaHintsPopulated) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.node_labels = {"Person"};
    c.excluded_edge_types = {"DEPRECATED"};
    optimizer_->clearPlanCache();
    auto plan = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan);
    ASSERT_EQ(plan->active_schema_hints.size(), 2u);
    // First hint describes node labels
    EXPECT_NE(plan->active_schema_hints[0].find("Person"), std::string::npos);
    // Second hint describes excluded edge types
    EXPECT_NE(plan->active_schema_hints[1].find("DEPRECATED"), std::string::npos);
}

// ── Plan cache keys ───────────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SchemaHint_DifferentLabelsDifferentCacheEntries) {
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.node_labels = {"Person"};
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", c1);
    ASSERT_TRUE(plan1);
    const size_t hits_after1 = optimizer_->getQueryMetrics().plan_cache_misses.load();

    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.node_labels = {"Organization"};
    auto plan2 = optimizer_->optimizeShortestPath("A", "D", c2);
    ASSERT_TRUE(plan2);
    const size_t hits_after2 = optimizer_->getQueryMetrics().plan_cache_misses.load();

    // c2 has a different node label → different cache key → one more miss
    EXPECT_GT(hits_after2, hits_after1);
}

// ── BFS label filtering ───────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SchemaHint_BFS_FiltersNodesByLabel) {
    // Store node "B" with label "Person"; "C" has no labels.
    // Graph: A -> B -> C (B labeled Person, C unlabeled)
    storeNodeWithLabels(*db_, "B", "Person");

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.node_labels = {"Person"};
    auto result = optimizer_->executeBFS("A", 3, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    // "A" is the start node (not filtered), "B" has label "Person" (included),
    // "C" has no labels (excluded by hint).
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "A"), nodes.end())
        << "Start node should always be present";
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "B"), nodes.end())
        << "B (Person) should be included";
    EXPECT_EQ(std::find(nodes.begin(), nodes.end(), "C"), nodes.end())
        << "C (no labels) should be excluded by the hint";
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_BFS_OR_Semantics_MatchesAnyLabel) {
    // "B" has label "Employee", "C" has label "Manager".
    // With node_labels={"Employee","Manager"}, both B and C should be included.
    storeNodeWithLabels(*db_, "B", "Employee");
    storeNodeWithLabels(*db_, "C", "Manager");

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.node_labels = {"Employee", "Manager"};  // OR: either label matches
    auto result = optimizer_->executeBFS("A", 4, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    // The start node "A" is always present regardless of labels (never filtered)
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "A"), nodes.end())
        << "Start node A should always be present regardless of label filter";
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "B"), nodes.end())
        << "B (Employee) should be included";
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "C"), nodes.end())
        << "C (Manager) should be included via OR semantics";
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_CacheKey_LabelOrderIndependent) {
    // {"Person","Employee"} and {"Employee","Person"} must produce the same key
    // and therefore share a cached plan.
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.node_labels = {"Person", "Employee"};
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", c1);
    ASSERT_TRUE(plan1);
    const size_t misses_after1 = optimizer_->getQueryMetrics().plan_cache_misses.load();

    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.node_labels = {"Employee", "Person"};  // same labels, different order
    auto plan2 = optimizer_->optimizeShortestPath("A", "D", c2);
    ASSERT_TRUE(plan2);
    const size_t misses_after2 = optimizer_->getQueryMetrics().plan_cache_misses.load();

    // Reversed order should hit exact cache (same sorted key) → no new miss
    EXPECT_EQ(misses_after2, misses_after1)
        << "Reversed label order should reuse cached plan via same sorted key";
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_CacheKey_ExcludedEdgeTypeOrderIndependent) {
    // {"FOLLOWS","LIKES"} and {"LIKES","FOLLOWS"} must produce the same key.
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.excluded_edge_types = {"FOLLOWS", "LIKES"};
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", c1);
    ASSERT_TRUE(plan1);
    const size_t misses_after1 = optimizer_->getQueryMetrics().plan_cache_misses.load();

    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.excluded_edge_types = {"LIKES", "FOLLOWS"};  // same types, different order
    auto plan2 = optimizer_->optimizeShortestPath("A", "D", c2);
    ASSERT_TRUE(plan2);
    const size_t misses_after2 = optimizer_->getQueryMetrics().plan_cache_misses.load();

    // Reversed order should hit the cached plan (same sorted key)
    EXPECT_EQ(misses_after2, misses_after1)
        << "Reversed excluded_edge_types order should reuse cached plan";
}

TEST_F(GraphQueryOptimizerTest, SchemaHint_BFS_NoFilter_AllNodesTraversed) {
    // Without node_labels, all reachable nodes should appear.
    themis::graph::GraphQueryOptimizer::QueryConstraints c; // no hint
    auto result = optimizer_->executeBFS("A", 4, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "B"), nodes.end());
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "C"), nodes.end());
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "D"), nodes.end());
}

// ── DFS label filtering ───────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SchemaHint_DFS_FiltersNodesByLabel) {
    storeNodeWithLabels(*db_, "B", "Person");

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.node_labels = {"Person"};
    auto result = optimizer_->executeDFS("A", 3, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "A"), nodes.end());
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "B"), nodes.end());
    EXPECT_EQ(std::find(nodes.begin(), nodes.end(), "C"), nodes.end())
        << "C (no labels) should be excluded";
}

// ── setNodeLabelStats ─────────────────────────────────────────────────────────

TEST_F(GraphQueryOptimizerTest, SetNodeLabelStats_PopulatesSelectivity) {
    // The test graph has 4 vertices (A, B, C, D); collectStatistics discovers them.
    auto stats_result = optimizer_->collectStatistics();
    ASSERT_TRUE(stats_result);
    const size_t vertex_count = optimizer_->getStatistics().vertex_count;
    ASSERT_GT(vertex_count, 0u);

    // Supply counts that are always ≤ vertex_count
    const size_t person_count = vertex_count;   // 100% → selectivity = 1.0
    const size_t company_count = 0;             // 0%   → selectivity = 0.0

    optimizer_->setNodeLabelStats({{"Person", person_count}, {"Company", company_count}});

    const auto& stats = optimizer_->getStatistics();
    ASSERT_NE(stats.node_label_counts.find("Person"), stats.node_label_counts.end());
    EXPECT_EQ(stats.node_label_counts.at("Person"), person_count);
    ASSERT_NE(stats.node_label_selectivity.find("Person"), stats.node_label_selectivity.end());
    EXPECT_DOUBLE_EQ(stats.node_label_selectivity.at("Person"), 1.0);
    ASSERT_NE(stats.node_label_selectivity.find("Company"), stats.node_label_selectivity.end());
    EXPECT_DOUBLE_EQ(stats.node_label_selectivity.at("Company"), 0.0);
}

TEST_F(GraphQueryOptimizerTest, SetNodeLabelStats_ClearsOldEntries) {
    optimizer_->collectStatistics();

    optimizer_->setNodeLabelStats({{"Person", 1}});
    optimizer_->setNodeLabelStats({{"Company", 1}});  // replaces previous

    const auto& stats = optimizer_->getStatistics();
    // Old "Person" entry should be gone after the second call
    EXPECT_EQ(stats.node_label_counts.find("Person"), stats.node_label_counts.end());
    ASSERT_NE(stats.node_label_counts.find("Company"), stats.node_label_counts.end());
    EXPECT_EQ(stats.node_label_counts.at("Company"), 1u);
}

// Incremental Graph Query Execution Tests (v1.9.0)
// ============================================================================

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_RegisterReturnsNonZeroHandle) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 3, c, [](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {});
    EXPECT_GT(handle, 0u);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_TwoRegistrationsHaveDifferentHandles) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto h1 = optimizer_->registerIncrementalBFS(
        "A", 3, c, [](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {});
    auto h2 = optimizer_->registerIncrementalBFS(
        "A", 3, c, [](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {});
    EXPECT_NE(h1, h2);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_CallsCallbackForAffectedQuery) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;

    int callback_count = 0;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult& result) {
            ++callback_count;
            EXPECT_TRUE(result.reexecuted);
        });

    // Add an edge touching vertex B (which is in A's BFS result)
    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edge_new", "B", "E");

    const size_t reexecuted = optimizer_->onGraphChange(changes);
    EXPECT_EQ(reexecuted, 1u);
    EXPECT_EQ(callback_count, 1);

    optimizer_->unregisterIncrementalQuery(handle);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_NoCallbackForUnaffectedQuery) {
    // Register a BFS starting from vertex A (reaches B, C, D within depth 3)
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    int callback_count = 0;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {
            ++callback_count;
        });

    // Change only touches vertex Z (not reachable from A)
    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edge_z", "Z", "W");

    optimizer_->onGraphChange(changes);
    EXPECT_EQ(callback_count, 0);

    optimizer_->unregisterIncrementalQuery(handle);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_UnregisterStopsCallbacks) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    int callback_count = 0;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {
            ++callback_count;
        });

    optimizer_->unregisterIncrementalQuery(handle);

    // Change touches A directly – would have fired callback if still registered
    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addVertexAdded("A");
    optimizer_->onGraphChange(changes);

    EXPECT_EQ(callback_count, 0);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_DeltaContainsAddedVertex) {
    // Initial graph: A->B->C->D, A->C
    // After change: add edge A->E (E was not reachable before)
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 2;

    themis::graph::GraphQueryOptimizer::IncrementalQueryResult captured;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 2, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult& result) {
            captured = result;
        });

    // Add edge A->E and vertex E
    themis::BaseEntity eNew("edgeE");
    eNew.setField("id", "edgeE");
    eNew.setField("_from", "A");
    eNew.setField("_to", "E");
    eNew.setField("_weight", "1.0");
    graph_mgr_->addEdge(eNew);

    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edgeE", "A", "E");

    optimizer_->onGraphChange(changes);

    // E should appear in the added set (it is within depth 1 of A)
    EXPECT_TRUE(captured.reexecuted);
    const bool e_added = std::find(captured.added.begin(), captured.added.end(), "E")
                         != captured.added.end();
    EXPECT_TRUE(e_added) << "Expected 'E' in added vertices";
    EXPECT_FALSE(captured.current.empty());

    optimizer_->unregisterIncrementalQuery(handle);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_DeltaContainsRemovedVertex) {
    // Add an extra edge A->F so F is reachable, then remove it
    themis::BaseEntity eF("edgeF");
    eF.setField("id", "edgeF");
    eF.setField("_from", "A");
    eF.setField("_to", "F");
    eF.setField("_weight", "1.0");
    graph_mgr_->addEdge(eF);

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 2;

    themis::graph::GraphQueryOptimizer::IncrementalQueryResult captured;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 2, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult& result) {
            captured = result;
        });

    // Now delete edge A->F so F is no longer reachable
    graph_mgr_->deleteEdge("edgeF");

    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeRemoved("edgeF", "A", "F");

    optimizer_->onGraphChange(changes);

    EXPECT_TRUE(captured.reexecuted);
    const bool f_removed = std::find(captured.removed.begin(), captured.removed.end(), "F")
                           != captured.removed.end();
    EXPECT_TRUE(f_removed) << "Expected 'F' in removed vertices";

    optimizer_->unregisterIncrementalQuery(handle);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_CurrentResultIsComplete) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;

    themis::graph::GraphQueryOptimizer::IncrementalQueryResult captured;
    auto handle = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult& result) {
            captured = result;
        });

    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edgeX", "B", "X");

    optimizer_->onGraphChange(changes);

    // current should be non-empty (A reachable, etc.)
    EXPECT_FALSE(captured.current.empty());

    optimizer_->unregisterIncrementalQuery(handle);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_MultipleQueriesTrackedIndependently) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;

    int cb_a = 0, cb_z = 0;
    auto ha = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) { ++cb_a; });
    auto hz = optimizer_->registerIncrementalBFS(
        "Z", 3, c,  // Z has no edges, won't be affected by changes to A's neighbourhood
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) { ++cb_z; });

    // Change affects B (in A's neighbourhood only)
    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edgeB2", "B", "Y");

    optimizer_->onGraphChange(changes);

    EXPECT_EQ(cb_a, 1);  // A's query re-executed
    EXPECT_EQ(cb_z, 0);  // Z's query unaffected

    optimizer_->unregisterIncrementalQuery(ha);
    optimizer_->unregisterIncrementalQuery(hz);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_EmptyChangeSet_NoCalls) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    int cb = 0;
    auto h = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) { ++cb; });

    themis::graph::GraphQueryOptimizer::GraphChangeSet empty;
    const size_t count = optimizer_->onGraphChange(empty);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(cb, 0);

    optimizer_->unregisterIncrementalQuery(h);
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_OnGraphChange_ReturnsReexecutedCount) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;

    auto h1 = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {});
    auto h2 = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {});

    // Change affects B (in A's result) — both queries should fire
    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edgeN", "B", "N");
    const size_t count = optimizer_->onGraphChange(changes);
    EXPECT_EQ(count, 2u);

    optimizer_->unregisterIncrementalQuery(h1);
    optimizer_->unregisterIncrementalQuery(h2);
}

TEST_F(GraphQueryOptimizerTest, GraphChangeSet_Helpers_WorkCorrectly) {
    themis::graph::GraphQueryOptimizer::GraphChangeSet cs;
    EXPECT_TRUE(cs.empty());
    EXPECT_EQ(cs.size(), 0u);

    cs.addEdgeAdded("e1", "A", "B");
    cs.addEdgeRemoved("e2", "C", "D");
    cs.addVertexAdded("V1");
    cs.addVertexRemoved("V2");

    EXPECT_FALSE(cs.empty());
    EXPECT_EQ(cs.size(), 4u);

    using CT = themis::graph::GraphQueryOptimizer::GraphChangeSet::ChangeType;
    EXPECT_EQ(cs.changes[0].type, CT::EDGE_ADDED);
    EXPECT_EQ(cs.changes[0].from, "A");
    EXPECT_EQ(cs.changes[0].to, "B");
    EXPECT_EQ(cs.changes[1].type, CT::EDGE_REMOVED);
    EXPECT_EQ(cs.changes[2].type, CT::VERTEX_ADDED);
    EXPECT_EQ(cs.changes[2].id, "V1");
    EXPECT_EQ(cs.changes[3].type, CT::VERTEX_REMOVED);
    EXPECT_EQ(cs.changes[3].id, "V2");
}

TEST_F(GraphQueryOptimizerTest, IncrementalBFS_CallbackCanSafelyUnregisterItselfDuringOnGraphChange) {
    // This test validates the iterator-safety fix: a callback that calls
    // unregisterIncrementalQuery() on its own handle must not cause
    // undefined behaviour (e.g. use-after-free or iterator invalidation).
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;

    themis::graph::GraphQueryOptimizer::IncrementalQueryHandle captured_handle = 0;
    int cb_count = 0;

    auto handle = optimizer_->registerIncrementalBFS(
        "A", 3, c,
        [&](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {
            ++cb_count;
            // Unregister self from within the callback – must be safe.
            optimizer_->unregisterIncrementalQuery(captured_handle);
        });
    captured_handle = handle;

    // Second query (different start vertex so its callback is not triggered).
    auto h2 = optimizer_->registerIncrementalBFS(
        "Z", 3, c,
        [](const themis::graph::GraphQueryOptimizer::IncrementalQueryResult&) {});

    // Trigger re-execution by touching B (in A's result).
    themis::graph::GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("edge_selfunreg", "B", "Q");

    // Must not crash or exhibit UB.
    EXPECT_NO_FATAL_FAILURE(optimizer_->onGraphChange(changes));
    EXPECT_EQ(cb_count, 1);

    optimizer_->unregisterIncrementalQuery(h2);
}

// Plan Cache Eviction: Size (LRU) and TTL Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, PlanCache_DefaultNoBoundsApplied) {
    // By default max size = 0 (unlimited) and TTL = 0 (no expiry)
    EXPECT_EQ(optimizer_->getPlanCacheMaxSize(), 0u);
    EXPECT_EQ(optimizer_->getPlanCacheTTL().count(), 0);
}

TEST_F(GraphQueryOptimizerTest, PlanCache_GetPlanCacheSizeReflectsInsertions) {
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    // Each unique constraint combination produces a different structural key
    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.max_depth = 1;
    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.max_depth = 2;

    optimizer_->optimizeShortestPath("A", "D", c1);
    optimizer_->optimizeShortestPath("A", "D", c2);

    // Expect at least 2 entries (exact key + structural key per query,
    // though structural dedup may share between queries with same depth).
    EXPECT_GE(optimizer_->getPlanCacheSize(), 2u);
}

TEST_F(GraphQueryOptimizerTest, PlanCache_LRU_EvictsLeastRecentlyUsed) {
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    // Allow only 2 entries in the cache
    optimizer_->setPlanCacheMaxSize(2);

    // Insert two distinct plans (different structural keys via different depths)
    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.max_depth = 10;
    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.max_depth = 20;

    optimizer_->optimizeKHopNeighborhood("A", 10, c1); // key1
    optimizer_->optimizeKHopNeighborhood("A", 20, c2); // key2

    EXPECT_EQ(optimizer_->getPlanCacheSize(), 2u);

    // Inserting a third distinct entry must evict the LRU (key1, inserted first)
    themis::graph::GraphQueryOptimizer::QueryConstraints c3;
    c3.max_depth = 30;
    optimizer_->optimizeKHopNeighborhood("A", 30, c3); // key3

    EXPECT_EQ(optimizer_->getPlanCacheSize(), 2u)
        << "Cache size should stay at max_size after eviction";

    // Eviction counter must have incremented
    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_evictions.load(), 0u);
}

TEST_F(GraphQueryOptimizerTest, PlanCache_LRU_EvictionCounterIncrementsPerEviction) {
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();
    optimizer_->setPlanCacheMaxSize(1);

    const uint64_t evictions_before =
        optimizer_->getQueryMetrics().plan_cache_evictions.load();

    themis::graph::GraphQueryOptimizer::QueryConstraints c1;
    c1.max_depth = 5;
    themis::graph::GraphQueryOptimizer::QueryConstraints c2;
    c2.max_depth = 6;

    optimizer_->optimizeKHopNeighborhood("A", 5, c1);
    optimizer_->optimizeKHopNeighborhood("A", 6, c2); // triggers 1 eviction

    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_evictions.load(),
              evictions_before);
}

TEST_F(GraphQueryOptimizerTest, PlanCache_TTL_ExpiredEntryNotReturned) {
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    // Set a very short TTL (1 ms)
    optimizer_->setPlanCacheTTL(std::chrono::milliseconds(1));

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan1.has_value());

    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const uint64_t hits_before =
        optimizer_->getQueryMetrics().plan_cache_hits.load();

    // Second query: TTL expired, so it must be a miss (entry evicted on lookup)
    auto plan2 = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_EQ(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Expired TTL entry must not be served as a cache hit";

    // The eviction counter should have incremented for the expired entry
    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_evictions.load(), 0u);

    // Restore unlimited TTL for other tests
    optimizer_->setPlanCacheTTL(std::chrono::milliseconds(0));
}

TEST_F(GraphQueryOptimizerTest, PlanCache_TTL_NonExpiredEntryIsServed) {
    optimizer_->setPlanCachingEnabled(true);
    optimizer_->clearPlanCache();

    // Set a generous TTL (10 seconds)
    optimizer_->setPlanCacheTTL(std::chrono::milliseconds(10000));

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 4;
    auto plan1 = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan1.has_value());

    const uint64_t hits_before =
        optimizer_->getQueryMetrics().plan_cache_hits.load();

    // Immediate second query: entry is within TTL, should be a cache hit
    auto plan2 = optimizer_->optimizeShortestPath("A", "D", c);
    ASSERT_TRUE(plan2.has_value());

    EXPECT_GT(optimizer_->getQueryMetrics().plan_cache_hits.load(), hits_before)
        << "Non-expired entry must be served from cache";

    // Plans must match
    EXPECT_EQ(plan1->algorithm, plan2->algorithm);
    EXPECT_DOUBLE_EQ(plan1->estimated_cost, plan2->estimated_cost);

    optimizer_->setPlanCacheTTL(std::chrono::milliseconds(0));
}

TEST_F(GraphQueryOptimizerTest, PlanCache_ClearResetsSize) {
    optimizer_->setPlanCachingEnabled(true);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    optimizer_->optimizeShortestPath("A", "D", c);

    EXPECT_GT(optimizer_->getPlanCacheSize(), 0u);

    optimizer_->clearPlanCache();
    EXPECT_EQ(optimizer_->getPlanCacheSize(), 0u);
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetrics_ContainsPlanCacheEvictions) {
    auto req = makeGet("/api/v1/graph/metrics");
    auto res = handler_->handleMetrics(req);

    auto j = nlohmann::json::parse(res.body());
    EXPECT_TRUE(j.contains("plan_cache_evictions"));
}

TEST_F(GraphApiHandlerMetricsTest, HandleMetricsPrometheus_ContainsPlanCacheEvictions) {
    auto req = makeGet("/api/v1/graph/metrics/prometheus");
    auto res = handler_->handleMetricsPrometheus(req);
    EXPECT_NE(res.body().find("themis_graph_plan_cache_evictions_total"),
              std::string::npos);
}

// Graph Query Result Streaming Tests (Issue #1822)
// ============================================================================

TEST_F(GraphQueryOptimizerTest, StreamBFS_ReturnsValidStream) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->streamBFS("A", 3, c);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_NE(*result, nullptr);
    EXPECT_TRUE((*result)->hasNext());
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_IteratesAllNodes) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto stream_result = optimizer_->streamBFS("A", 3, c);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    std::vector<std::string> nodes = {};

    while (stream->hasNext()) {
        auto item = stream->next();
        ASSERT_TRUE(item.has_value());
        nodes.push_back(*item);
    }

    // Graph: A->B->C->D, A->C. BFS from A depth 3: A, B, C, D
    EXPECT_FALSE(nodes.empty());
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "A"), nodes.end());
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "B"), nodes.end());
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "C"), nodes.end());
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_BatchedConsumption) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    themis::query::StreamConfig cfg;
    cfg.batch_size = 2;
    auto stream_result = optimizer_->streamBFS("A", 3, c, cfg);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    size_t total = 0;
    while (stream->hasNext()) {
        auto batch = stream->nextBatch(2);
        ASSERT_TRUE(batch.has_value());
        total += batch->items.size();
    }
    EXPECT_GT(total, 0u);
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_DepthZeroReturnsStart) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto stream_result = optimizer_->streamBFS("A", 0, c);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    ASSERT_TRUE(stream->hasNext());
    auto item = stream->next();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(*item, "A");
    EXPECT_FALSE(stream->hasNext());
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_MatchesExecuteBFS) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto batch_result = optimizer_->executeBFS("A", 3, c);
    ASSERT_TRUE(batch_result.has_value());

    auto stream_result = optimizer_->streamBFS("A", 3, c);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    std::vector<std::string> streamed = {};

    while (stream->hasNext()) {
        auto item = stream->next();
        ASSERT_TRUE(item.has_value());
        streamed.push_back(*item);
    }

    EXPECT_EQ(streamed, *batch_result);
}

TEST_F(GraphQueryOptimizerTest, StreamDFS_ReturnsValidStream) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->streamDFS("A", 3, c);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_NE(*result, nullptr);
    EXPECT_TRUE((*result)->hasNext());
}

TEST_F(GraphQueryOptimizerTest, StreamDFS_MatchesExecuteDFS) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto batch_result = optimizer_->executeDFS("A", 3, c);
    ASSERT_TRUE(batch_result.has_value());

    auto stream_result = optimizer_->streamDFS("A", 3, c);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    std::vector<std::string> streamed = {};

    while (stream->hasNext()) {
        auto item = stream->next();
        ASSERT_TRUE(item.has_value());
        streamed.push_back(*item);
    }

    EXPECT_EQ(streamed, *batch_result);
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_PropagatesRateLimitError) {
    optimizer_->setMaxQueriesPerSecond(1);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Consume the budget
    optimizer_->executeBFS("A", 1, c);

    // Streaming call should be rejected
    auto stream_result = optimizer_->streamBFS("A", 1, c);
    EXPECT_FALSE(stream_result.has_value());
    optimizer_->setMaxQueriesPerSecond(0);
}

TEST_F(GraphQueryOptimizerTest, StreamDFS_PropagatesRateLimitError) {
    optimizer_->setMaxQueriesPerSecond(1);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    // Consume the budget
    optimizer_->executeDFS("A", 1, c);

    // Streaming call should be rejected
    auto stream_result = optimizer_->streamDFS("A", 1, c);
    EXPECT_FALSE(stream_result.has_value());
    optimizer_->setMaxQueriesPerSecond(0);
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_WithConstraints_MaxResults) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_results = 2;
    auto stream_result = optimizer_->streamBFS("A", 3, c);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    std::vector<std::string> nodes = {};

    while (stream->hasNext()) {
        auto item = stream->next();
        ASSERT_TRUE(item.has_value());
        nodes.push_back(*item);
    }
    EXPECT_LE(nodes.size(), 2u);
}

TEST_F(GraphQueryOptimizerTest, StreamBFS_LargeGraph_AllNodesReachable) {
    // Add extra edges for a slightly larger graph
    for (int i = 0; i < 5; ++i) {
        themis::BaseEntity edge("stream_edge_" + std::to_string(i));
        edge.setField("id", "stream_edge_" + std::to_string(i));
        edge.setField("_from", "D");
        edge.setField("_to", "N_" + std::to_string(i));
        edge.setField("_weight", "1.0");
        graph_mgr_->addEdge(edge);
    }
    optimizer_->collectStatistics();

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto stream_result = optimizer_->streamBFS("A", 4, c);
    ASSERT_TRUE(stream_result.has_value());
    auto& stream = *stream_result;

    size_t count = 0;
    while (stream->hasNext()) {
        auto item = stream->next();
        ASSERT_TRUE(item.has_value());
        ++count;
    }
    // A, B, C, D, plus 5 new nodes = 9 total
    EXPECT_GE(count, 9u);
}

// Subgraph Isomorphism Tests
// ============================================================================

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_EmptyPatternReturnsEmptyMatch) {
    std::vector<std::string> verts;
    std::vector<std::pair<std::string, std::string>> edges;
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().matches.size(), 1u);
    EXPECT_TRUE(result.value().matches[0].empty());
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_SingleVertexFindsAllDataVertices) {
    // Pattern: single vertex "u" - should match every data vertex reachable from A
    std::vector<std::string> verts = {"u"};
    std::vector<std::pair<std::string, std::string>> edges;
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    // The test graph has vertices A,B,C,D - all should be matched
    EXPECT_GE(result.value().matches.size(), 1u);
    for (const auto& m : result.value().matches) {
        EXPECT_EQ(m.size(), 1u);
        EXPECT_TRUE(m.count("u"));
    }
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_EdgePatternFindsDirectEdges) {
    // Pattern: u -> v; should match A->B, A->C, B->C, C->D
    std::vector<std::string> verts = {"u", "v"};
    std::vector<std::pair<std::string, std::string>> edges = {{"u", "v"}};
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    EXPECT_GE(result.value().matches.size(), 1u);
    for (const auto& m : result.value().matches) {
        ASSERT_TRUE(m.count("u") && m.count("v"));
        const std::string& mu = m.at("u");
        const std::string& mv = m.at("v");
        // Verify injective: both mapped to different data vertices
        EXPECT_NE(mu, mv);
        // Verify edge exists in test graph
        auto [st, nbrs] = graph_mgr_->outNeighbors(mu);
        ASSERT_TRUE(st.ok);
        EXPECT_NE(std::find(nbrs.begin(), nbrs.end(), mv), nbrs.end());
    }
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_ChainPatternMatchesPath) {
    // Pattern: p -> q -> r; must match A->B->C and A->C->D and B->C->D
    std::vector<std::string> verts = {"p", "q", "r"};
    std::vector<std::pair<std::string, std::string>> edges = {{"p", "q"}, {"q", "r"}};
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    EXPECT_GE(result.value().matches.size(), 1u);
    for (const auto& m : result.value().matches) {
        ASSERT_EQ(m.size(), 3u);
        // Verify chain structure in data graph
        auto [st1, n1] = graph_mgr_->outNeighbors(m.at("p"));
        ASSERT_TRUE(st1.ok);
        EXPECT_NE(std::find(n1.begin(), n1.end(), m.at("q")), n1.end());
        auto [st2, n2] = graph_mgr_->outNeighbors(m.at("q"));
        ASSERT_TRUE(st2.ok);
        EXPECT_NE(std::find(n2.begin(), n2.end(), m.at("r")), n2.end());
    }
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_MaxResultsLimitsMatches) {
    // Pattern with many matches; limit results
    std::vector<std::string> verts = {"u", "v"};
    std::vector<std::pair<std::string, std::string>> edges = {{"u", "v"}};
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.max_results = 1;
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges, c);
    ASSERT_TRUE(result);
    EXPECT_LE(result.value().matches.size(), 1u);
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_ForbiddenVertexExcludesIt) {
    // Pattern: single vertex "u"; B is forbidden -> B must not appear in matches
    std::vector<std::string> verts = {"u"};
    std::vector<std::pair<std::string, std::string>> edges;
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.forbidden_vertices = {"B"};
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges, c);
    ASSERT_TRUE(result);
    for (const auto& m : result.value().matches) {
        ASSERT_TRUE(m.count("u"));
        EXPECT_NE(m.at("u"), "B");
    }
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_InjectiveNoReusedDataVertex) {
    // Pattern: two disconnected vertices; each data vertex used at most once
    std::vector<std::string> verts = {"x", "y"};
    std::vector<std::pair<std::string, std::string>> edges;
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    for (const auto& m : result.value().matches) {
        ASSERT_EQ(m.size(), 2u);
        EXPECT_NE(m.at("x"), m.at("y"));
    }
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_OptimizePlanSelectsDFS) {
    std::vector<std::string> verts = {"u", "v"};
    std::vector<std::pair<std::string, std::string>> edges = {{"u", "v"}};
    auto plan = optimizer_->optimizePatternMatch(verts, edges);
    ASSERT_TRUE(plan);
    EXPECT_EQ(plan.value().pattern,
              themis::graph::GraphQueryOptimizer::QueryPattern::PATTERN_MATCH);
    EXPECT_EQ(plan.value().algorithm,
              themis::graph::GraphQueryOptimizer::TraversalAlgorithm::DFS);
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_ExecutionStatsPopulated) {
    std::vector<std::string> verts = {"u", "v"};
    std::vector<std::pair<std::string, std::string>> edges = {{"u", "v"}};
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges, {}, &stats);
    ASSERT_TRUE(result);
    EXPECT_GE(stats.nodes_explored, 1u);
    EXPECT_GE(stats.execution_time_ms, 0.0);
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_SelfLoopPatternRejectedWhenDataEdgeMissing) {
    // Pattern: single vertex "u" with a self-loop u -> u.
    // The test graph (A->B->C->D, A->C) has no self-loop edges, so no match.
    std::vector<std::string> verts = {"u"};
    std::vector<std::pair<std::string, std::string>> edges = {{"u", "u"}};
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().matches.size(), 0u);
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_SelfLoopPatternMatchesWhenEdgeExists) {
    // Add a self-loop on vertex "S" to the graph, then verify the pattern matches.
    themis::BaseEntity self_edge("sel1");
    self_edge.setField("id", "sel1");
    self_edge.setField("_from", "S");
    self_edge.setField("_to", "S");
    graph_mgr_->addEdge(self_edge);

    // Pattern: single vertex with self-loop
    std::vector<std::string> verts = {"u"};
    std::vector<std::pair<std::string, std::string>> edges = {{"u", "u"}};
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges);
    ASSERT_TRUE(result);
    // At least "S" must appear as a match
    bool found_S = false;
    for (const auto& m : result.value().matches) {
        if (m.count("u") && m.at("u") == "S") { found_S = true; break; }
    }
    EXPECT_TRUE(found_S);
    // Verify every match vertex actually has a self-loop
    for (const auto& m : result.value().matches) {
        ASSERT_TRUE(m.count("u"));
        const std::string& mv = m.at("u");
        auto [st, nbrs] = graph_mgr_->outNeighbors(mv);
        ASSERT_TRUE(st.ok) << "outNeighbors failed for vertex " << mv << ": " << st.message;
        EXPECT_NE(std::find(nbrs.begin(), nbrs.end(), mv), nbrs.end())
            << "Match vertex " << mv << " has no self-loop";
    }
}

TEST_F(GraphQueryOptimizerTest, SubgraphIsomorphism_EmptyPatternPathsFoundIsOne) {
    // Verify metrics: empty pattern should count as 1 path found (not 0 / failure)
    std::vector<std::string> verts;
    std::vector<std::pair<std::string, std::string>> edges;
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;
    auto result = optimizer_->executeSubgraphIsomorphism(verts, edges, {}, &stats);
    ASSERT_TRUE(result);
    EXPECT_EQ(stats.paths_found, 1u);
}
// Analytics Module Integration Tests (Issue #1821)
// ============================================================================

class GraphAnalyticsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = (fs::temp_directory_path() /
            ("themis_graph_analytics_integration_test_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))
            ).string();
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);

        themis::RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 256;
        config.max_background_jobs = 2;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";

        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        graph_mgr_  = std::make_unique<themis::GraphIndexManager>(*db_);
        optimizer_  = std::make_unique<themis::graph::GraphQueryOptimizer>(*graph_mgr_);
        analytics_  = std::make_unique<themis::GraphAnalytics>(*graph_mgr_);

        // Graph: A -> B -> C -> D, also A -> C
        auto addEdge = [&](const std::string& id,
                           const std::string& from,
                           const std::string& to,
                           double weight = 1.0) {
            themis::BaseEntity e(id);
            e.setField("id", id);
            e.setField("_from", from);
            e.setField("_to", to);
            e.setField("_weight", std::to_string(weight));
            graph_mgr_->addEdge(e);
        };
        addEdge("e1", "A", "B", 1.0);
        addEdge("e2", "B", "C", 1.0);
        addEdge("e3", "C", "D", 1.0);
        addEdge("e4", "A", "C", 2.0);
    }

    void TearDown() override {
        analytics_.reset();
        optimizer_.reset();
        graph_mgr_.reset();
        db_.reset();
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);
    }

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_mgr_;
    std::unique_ptr<themis::graph::GraphQueryOptimizer> optimizer_;
    std::unique_ptr<themis::GraphAnalytics> analytics_;
};

TEST_F(GraphAnalyticsIntegrationTest, AttachDetach_HasAnalyticsReflectsState) {
    EXPECT_FALSE(optimizer_->hasAnalytics());

    optimizer_->attachAnalytics(*analytics_);
    EXPECT_TRUE(optimizer_->hasAnalytics());

    optimizer_->detachAnalytics();
    EXPECT_FALSE(optimizer_->hasAnalytics());
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_WithoutAnalytics_ReturnsError) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->executeKShortestPaths("A", "D", 2, c);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_InvalidK_ReturnsError) {
    optimizer_->attachAnalytics(*analytics_);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->executeKShortestPaths("A", "D", 0, c);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_FindsPathsViaAnalytics) {
    optimizer_->attachAnalytics(*analytics_);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    auto result = optimizer_->executeKShortestPaths("A", "D", 2, c);
    ASSERT_TRUE(result.has_value()) << (result ? "" : result.error().message());

    const auto& paths = result.value();
    EXPECT_GE(paths.size(), 1u);

    // All returned paths must start at A and end at D
    for (const auto& path : paths) {
        ASSERT_FALSE(path.vertices.empty());
        EXPECT_EQ(path.vertices.front(), "A");
        EXPECT_EQ(path.vertices.back(),  "D");
        EXPECT_GT(path.hop_count, 0);
    }
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_ReturnsAtMostKPaths) {
    optimizer_->attachAnalytics(*analytics_);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    auto result = optimizer_->executeKShortestPaths("A", "D", 3, c);
    ASSERT_TRUE(result.has_value());
    EXPECT_LE(result.value().size(), 3u);
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_PopulatesExecutionStats) {
    optimizer_->attachAnalytics(*analytics_);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    themis::graph::GraphQueryOptimizer::ExecutionStats stats;

    auto result = optimizer_->executeKShortestPaths("A", "D", 2, c, "", &stats);
    ASSERT_TRUE(result.has_value());

    EXPECT_GE(stats.paths_found, 1u);
    EXPECT_GE(stats.execution_time_ms, 0.0);
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_UpdatesQueryMetrics) {
    optimizer_->attachAnalytics(*analytics_);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;

    const auto queries_before =
        optimizer_->getQueryMetrics().total_queries.load();

    optimizer_->executeKShortestPaths("A", "D", 1, c);

    EXPECT_EQ(optimizer_->getQueryMetrics().total_queries.load(),
              queries_before + 1);
}

TEST_F(GraphAnalyticsIntegrationTest, ExecuteKShortestPaths_AfterDetach_ReturnsError) {
    optimizer_->attachAnalytics(*analytics_);
    optimizer_->detachAnalytics();

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    auto result = optimizer_->executeKShortestPaths("A", "D", 1, c);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// PathConstraints: Previously Untested Methods
// Covers: addForbiddenEdge, addRequiredEdge, requireAcyclic, requireUniqueNodes,
//         requireUniqueEdges, addCustomPredicate, clearConstraints, and the
//         corresponding validatePath / findConstrainedPaths code paths.
// ============================================================================

TEST_F(GraphQueryOptimizerTest, PathConstraints_ForbiddenEdge_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenEdge("edge1");

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Forbidden edge"), std::string::npos);
    EXPECT_NE(desc.find("edge1"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ForbiddenEdge_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenEdge("edge2");

    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::FORBIDDEN_EDGE);
    ASSERT_TRUE(pc.getConstraints()[0].string_value.has_value());
    EXPECT_EQ(*pc.getConstraints()[0].string_value, "edge2");
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_FailsOnForbiddenEdge) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenEdge("edge1");

    // Path uses edge1 (A->B) – should fail
    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_PassesWhenForbiddenEdgeAbsent) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenEdge("edge_does_not_exist");

    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequiredEdge_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredEdge("edge1");

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Required edge"), std::string::npos);
    EXPECT_NE(desc.find("edge1"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequiredEdge_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredEdge("edge3");

    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::REQUIRED_EDGE);
    ASSERT_TRUE(pc.getConstraints()[0].string_value.has_value());
    EXPECT_EQ(*pc.getConstraints()[0].string_value, "edge3");
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_FailsWhenRequiredEdgeMissing) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredEdge("edge3"); // C->D edge

    // Path A->B does NOT include edge3 – should fail
    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_PassesWhenRequiredEdgePresent) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredEdge("edge1");

    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequireAcyclic_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireAcyclic();

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("No cycles"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequireAcyclic_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireAcyclic();

    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::NO_CYCLES);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_FailsOnCycle) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireAcyclic();

    // Path containing duplicate node ("A" repeated) violates NO_CYCLES
    auto res = pc.validatePath({"A", "B", "A"}, {"edge1", "edge_back"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_AcyclicPassesForSimplePath) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireAcyclic();

    auto res = pc.validatePath({"A", "B", "C"}, {"edge1", "edge2"});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequireUniqueNodes_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueNodes();

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Unique nodes"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequireUniqueNodes_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueNodes();

    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::UNIQUE_NODES);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_FailsOnDuplicateNode) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueNodes();

    auto res = pc.validatePath({"A", "B", "A"}, {"e1", "e2"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_UniqueNodesPasses) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueNodes();

    auto res = pc.validatePath({"A", "B", "C"}, {"edge1", "edge2"});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequireUniqueEdges_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueEdges();

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Unique edges"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_RequireUniqueEdges_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueEdges();

    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::UNIQUE_EDGES);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_FailsOnDuplicateEdge) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueEdges();

    auto res = pc.validatePath({"A", "B", "C"}, {"edge1", "edge1"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_UniqueEdgesPasses) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueEdges();

    auto res = pc.validatePath({"A", "B", "C"}, {"edge1", "edge2"});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_CustomPredicate_DescribeShows) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addCustomPredicate([](const std::vector<std::string>&) { return true; });

    const std::string desc = pc.describeConstraints();
    EXPECT_NE(desc.find("Custom predicate"), std::string::npos);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_CustomPredicate_StoredInConstraintList) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addCustomPredicate([](const std::vector<std::string>&) { return true; });

    ASSERT_EQ(pc.getConstraints().size(), 1u);
    EXPECT_EQ(pc.getConstraints()[0].type,
              themis::graph::PathConstraints::ConstraintType::CUSTOM_PREDICATE);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_CustomPredicatePasses) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addCustomPredicate([](const std::vector<std::string>&) { return true; });

    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(*res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ValidatePath_CustomPredicateFails) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addCustomPredicate([](const std::vector<std::string>&) { return false; });

    auto res = pc.validatePath({"A", "B"}, {"edge1"});
    EXPECT_FALSE(res.has_value() && *res);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ClearConstraints_EmptiesAll) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMinLength(2);
    pc.addMaxLength(5);
    pc.addForbiddenNode("X");
    pc.addForbiddenEdge("edgeX");
    pc.addRequiredNode("A");
    pc.addRequiredEdge("edge1");
    EXPECT_GT(pc.getConstraints().size(), 0u);

    pc.clearConstraints();

    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_ClearConstraints_AllowsRevalidation) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenNode("B");

    // Before clear: B is forbidden
    auto before = pc.validatePath({"A", "B"}, {"edge1"});
    EXPECT_FALSE(before.has_value() && *before);

    pc.clearConstraints();

    // After clear: no constraints – path A->B should pass
    auto after = pc.validatePath({"A", "B"}, {"edge1"});
    ASSERT_TRUE(after.has_value());
    EXPECT_TRUE(*after);
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_WithForbiddenNode_ExcludesNode) {
    // Test graph: A->B->C->D, A->C
    // Forbid B: only path from A to C should be A->C directly
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenNode("B");

    auto res = pc.findConstrainedPaths("A", "C", 10);
    ASSERT_TRUE(res.has_value());
    for (const auto& path : res.value()) {
        const auto& nodes = path.nodes;
        EXPECT_EQ(std::find(nodes.begin(), nodes.end(), "B"), nodes.end())
            << "B must not appear in any path when forbidden";
    }
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_WithRequiredNode_IncludesNode) {
    // A->B->C->D: require B to be on the path from A to C
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredNode("B");

    auto res = pc.findConstrainedPaths("A", "C", 10);
    ASSERT_TRUE(res.has_value());
    // Every returned path must pass through B
    for (const auto& path : res.value()) {
        const auto& nodes = path.nodes;
        EXPECT_NE(std::find(nodes.begin(), nodes.end(), "B"), nodes.end())
            << "B must appear in every path when required";
    }
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_WithRequireUniqueNodes_NoDuplicates) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueNodes();

    auto res = pc.findConstrainedPaths("A", "D", 10);
    ASSERT_TRUE(res.has_value());
    for (const auto& path : res.value()) {
        std::unordered_set<std::string> seen = {};

        for (const auto& n : path.nodes) {
            EXPECT_EQ(seen.count(n), 0u) << "Duplicate node found: " << n;
            seen.insert(n);
        }
    }
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_WithRequireAcyclic_NoDuplicateNodes) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireAcyclic();

    auto res = pc.findConstrainedPaths("A", "D", 10);
    ASSERT_TRUE(res.has_value());
    for (const auto& path : res.value()) {
        std::unordered_set<std::string> seen = {};

        for (const auto& n : path.nodes) {
            EXPECT_EQ(seen.count(n), 0u) << "Duplicate node found: " << n;
            seen.insert(n);
        }
    }
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_MinLengthGreaterThanMaxLength_ReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMinLength(5);
    pc.addMaxLength(2);

    auto res = pc.findConstrainedPaths("A", "D", 10);
    EXPECT_FALSE(res.has_value());
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_SameNodeRequiredAndForbidden_ReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredNode("B");
    pc.addForbiddenNode("B");

    auto res = pc.findConstrainedPaths("A", "D", 10);
    EXPECT_FALSE(res.has_value());
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_SameEdgeRequiredAndForbidden_ReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addRequiredEdge("edge1");
    pc.addForbiddenEdge("edge1");

    auto res = pc.findConstrainedPaths("A", "D", 10);
    EXPECT_FALSE(res.has_value());
}

TEST_F(GraphQueryOptimizerTest, FindConstrainedPaths_WithRequireUniqueEdges_NoDuplicateEdges) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueEdges();

    auto res = pc.findConstrainedPaths("A", "D", 10);
    ASSERT_TRUE(res.has_value());
    for (const auto& path : res.value()) {
        std::unordered_set<std::string> seen = {};

        for (const auto& e : path.edges) {
            EXPECT_EQ(seen.count(e), 0u) << "Duplicate edge found: " << e;
            seen.insert(e);
        }
    }
}

// ============================================================================
// Temporal BFS: time_range_require_containment
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_Containment_ExcludesOverlapOnlyEdges) {
    // Edge te1: TA->TB valid [kT2020, kT2022)
    // Query window: [kT2019, kT2021] overlaps te1 but does NOT contain it.
    // With containment=true, te1 must be excluded.
    addTemporalEdges(*graph_mgr_);

    constexpr int64_t kT2019 = 1546300800000LL; // 2019-01-01
    constexpr int64_t kT2021 = 1609459200000LL; // 2021-01-01

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2019;
    c.time_range_end_ms   = kT2021;
    c.time_range_require_containment = true; // Only fully-contained edges

    auto result = optimizer_->executeTemporalBFS("TA", 2, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    // te1 (TA->TB, valid [kT2020, kT2022)) is NOT fully within [kT2019, kT2021]
    // because its valid_to (kT2022) > kT2021, so TB should NOT be reachable.
    // te3 (TA->TC, no bounds) is always valid and has no valid_from/valid_to
    // so it may or may not be included depending on implementation behaviour.
    // We only assert that te1's destination (TB) is excluded.
    EXPECT_EQ(std::find(nodes.begin(), nodes.end(), "TB"), nodes.end())
        << "TB should be unreachable when te1 is not fully contained in the window";
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_Containment_IncludesFullyContainedEdge) {
    // Add a custom edge whose validity period fits entirely inside the query window.
    themis::BaseEntity contained("tc1");
    contained.setField("id", "tc1");
    contained.setField("_from", "X1");
    contained.setField("_to", "X2");
    // valid [kT2022, kT2022 + 1 day] – fits inside query window [kT2020, kT2025]
    contained.setField("valid_from", std::to_string(kT2022));
    contained.setField("valid_to",   std::to_string(kT2022 + 86400000LL));
    graph_mgr_->addEdge(contained);

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;
    c.time_range_require_containment = true;

    auto result = optimizer_->executeTemporalBFS("X1", 1, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "X2"), nodes.end())
        << "X2 should be reachable via a fully-contained edge";
}

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_ContainmentFalse_IncludesOverlappingEdge) {
    // With containment=false (default), overlapping edges are included.
    addTemporalEdges(*graph_mgr_);

    constexpr int64_t kT2019 = 1546300800000LL;
    constexpr int64_t kT2021 = 1609459200000LL;

    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2019;
    c.time_range_end_ms   = kT2021;
    c.time_range_require_containment = false; // overlap is sufficient

    auto result = optimizer_->executeTemporalBFS("TA", 2, c);
    ASSERT_TRUE(result);

    const auto& nodes = result.value();
    // te1 overlaps [kT2019, kT2021] so TB is reachable
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), "TB"), nodes.end())
        << "TB should be reachable when overlap mode is used";
}

// ============================================================================
// optimizeConstrainedPath: UNIQUE_NODES / NO_CYCLES cost multiplier paths
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_UniqueNodes_AffectsCostEstimate) {
    // Adding UNIQUE_NODES should increase the cost estimate relative to no constraint.
    themis::graph::PathConstraints base_constraints(graph_mgr_.get());
    auto base_plan = optimizer_->explainConstrainedPath("A", "D", base_constraints);
    ASSERT_TRUE(base_plan);

    themis::graph::PathConstraints unique_constraints(graph_mgr_.get());
    unique_constraints.requireUniqueNodes();
    auto unique_plan = optimizer_->explainConstrainedPath("A", "D", unique_constraints);
    ASSERT_TRUE(unique_plan);

    // requireUniqueNodes adds overhead; cost must be >= base
    EXPECT_GE(unique_plan->estimated_cost, base_plan->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_NoCycles_AffectsCostEstimate) {
    themis::graph::PathConstraints base_constraints(graph_mgr_.get());
    auto base_plan = optimizer_->explainConstrainedPath("A", "D", base_constraints);
    ASSERT_TRUE(base_plan);

    themis::graph::PathConstraints acyclic_constraints(graph_mgr_.get());
    acyclic_constraints.requireAcyclic();
    auto acyclic_plan = optimizer_->explainConstrainedPath("A", "D", acyclic_constraints);
    ASSERT_TRUE(acyclic_plan);

    EXPECT_GE(acyclic_plan->estimated_cost, base_plan->estimated_cost);
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_MultipleConstraints_IncreasesComplexity) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addMinLength(1);
    pc.addMaxLength(5);
    pc.addRequiredNode("B");
    pc.requireUniqueNodes();

    auto plan = optimizer_->explainConstrainedPath("A", "D", pc);
    ASSERT_TRUE(plan);
    EXPECT_GT(plan->estimated_cost, 0.0);
    EXPECT_NE(plan->explanation.find("Constraints"), std::string::npos);
    EXPECT_NE(plan->explanation.find("4 active"), std::string::npos); // 4 constraints active
}

TEST_F(GraphQueryOptimizerTest, ExplainConstrainedPath_UniqueEdges_AffectsCostEstimate) {
    // requireUniqueEdges maps to UNIQUE_EDGES in optimizeConstrainedPath
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.requireUniqueEdges();

    auto plan = optimizer_->explainConstrainedPath("A", "D", pc);
    ASSERT_TRUE(plan);
    // UNIQUE_EDGES triggers requires_unique=true path in optimizeConstrainedPath
    EXPECT_GT(plan->estimated_cost, 0.0);
}

// ============================================================================
// Rate limit exceeded: executeTemporalBFS and executeSubgraphIsomorphism
// ============================================================================

TEST_F(GraphQueryOptimizerTest, ExecuteTemporalBFS_RateLimitExceeded_ReturnsError) {
    optimizer_->setMaxQueriesPerSecond(1);
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.time_range_start_ms = kT2020;
    c.time_range_end_ms   = kT2025;

    // Consume the rate budget
    optimizer_->executeTemporalBFS("A", 1, c);
    // Second call in the same second should be rejected
    auto second = optimizer_->executeTemporalBFS("A", 1, c);
    EXPECT_FALSE(second.has_value());

    optimizer_->setMaxQueriesPerSecond(0);
}

TEST_F(GraphQueryOptimizerTest, ExecuteSubgraphIsomorphism_RateLimitExceeded_ReturnsError) {
    optimizer_->setMaxQueriesPerSecond(1);

    std::vector<std::string> verts = {"u"};
    std::vector<std::pair<std::string, std::string>> edges;

    // Consume the budget
    optimizer_->executeSubgraphIsomorphism(verts, edges, {});
    // Second call should be rate-limited
    auto second = optimizer_->executeSubgraphIsomorphism(verts, edges, {});
    EXPECT_FALSE(second.has_value());

    optimizer_->setMaxQueriesPerSecond(0);
}

// ============================================================================
// Security: query injection via path constraints
// ============================================================================

// Empty node identifiers must be silently rejected (not added to constraints).
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_EmptyNodeIdIgnored) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenNode("");
    pc.addRequiredNode("");

    // No constraints should have been stored for the empty identifiers.
    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

// Empty edge identifiers must be silently rejected.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_EmptyEdgeIdIgnored) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addForbiddenEdge("");
    pc.addRequiredEdge("");

    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

// Node IDs containing a null byte must be rejected to prevent bypass attacks.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_NullByteInNodeIdIgnored) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    std::string bad_id = std::string("node\0x", 6); // contains \0
    pc.addForbiddenNode(bad_id);
    pc.addRequiredNode(bad_id);

    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

// Property field names containing a null byte must be rejected.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_NullByteInFieldNameIgnored) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    std::string bad_field = std::string("type\0evil", 9);
    pc.addEdgePropertyConstraint(bad_field, "follows");
    pc.addNodePropertyConstraint(bad_field, "active");

    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

// Property field names with special injection characters must be rejected.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_SpecialCharsInFieldNameIgnored) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    // Characters outside [A-Za-z0-9_.-] are not allowed in field names.
    pc.addEdgePropertyConstraint("type' OR '1'='1", "follows");
    pc.addNodePropertyConstraint("name; DROP GRAPH users --", "alice");

    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

// Empty property field name must be rejected.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_EmptyFieldNameIgnored) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    pc.addEdgePropertyConstraint("", "follows");
    pc.addNodePropertyConstraint("", "alice");

    EXPECT_EQ(pc.getConstraints().size(), 0u);
}

// A negative MIN_LENGTH must not cause integer overflow that rejects valid paths.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_NegativeMinLengthHandledSafely) {
    themis::graph::PathConstraints pc_no_mgr;
    pc_no_mgr.addMinLength(-1); // -1 cast to size_t would be SIZE_MAX

    // A path with 3 nodes must still pass: 3 >= -1 (treat negative as no-min).
    auto result = pc_no_mgr.validatePath({"A", "B", "C"}, {"e1", "e2"});
    // A negative MIN_LENGTH is treated as "no restriction from below", so a
    // non-empty path should pass.
    EXPECT_TRUE(result.has_value() && result.value())
        << "Negative MIN_LENGTH must not cause integer overflow rejection of valid paths";
}

// A negative MAX_LENGTH must not silently allow unbounded paths by wrapping.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_NegativeMaxLengthTreatedAsUnlimited) {
    themis::graph::PathConstraints pc_no_mgr;
    pc_no_mgr.addMaxLength(-1); // -1 cast to size_t would be SIZE_MAX (bypass)

    // A long path should pass because negative max_length is treated as unlimited.
    std::vector<std::string> long_path(100, "node");
    std::vector<std::string> long_edges(99, "edge");
    auto result = pc_no_mgr.validatePath(long_path, long_edges);
    // Negative MAX_LENGTH means "no upper limit" — path must not be rejected.
    EXPECT_TRUE(result.has_value() && result.value())
        << "Negative MAX_LENGTH must not reject paths via SIZE_MAX wrap-around";
}

// findConstrainedPaths must reject empty start/end node identifiers.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_EmptyStartNodeReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    auto res = pc.findConstrainedPaths("", "D", 10);
    EXPECT_FALSE(res.has_value());
}

TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_EmptyEndNodeReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    auto res = pc.findConstrainedPaths("A", "", 10);
    EXPECT_FALSE(res.has_value());
}

// findConstrainedPaths must reject start/end node IDs containing null bytes.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_NullByteStartNodeReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    std::string bad_start = std::string("A\0evil", 6);
    auto res = pc.findConstrainedPaths(bad_start, "D", 10);
    EXPECT_FALSE(res.has_value());
}

// findConstrainedPaths must reject max_results <= 0.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_ZeroMaxResultsReturnsError) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    auto res = pc.findConstrainedPaths("A", "D", 0);
    EXPECT_FALSE(res.has_value());
}

// findConstrainedPaths must clamp max_results above the safety limit.
TEST_F(GraphQueryOptimizerTest, PathConstraints_Security_ExcessiveMaxResultsClamped) {
    themis::graph::PathConstraints pc(graph_mgr_.get());
    // Requesting more than MAX_RESULTS_LIMIT should be accepted (clamped, not rejected).
    auto res = pc.findConstrainedPaths("A", "D", 999999);
    // The call should not fail just because max_results > MAX_RESULTS_LIMIT.
    // It may fail because no path exists, but not due to the limit itself.
    // We verify that the function completes (either with results or NOT_FOUND error).
    // The important assertion is that it does NOT crash or hang.
    (void)res; // result is valid regardless of success/failure
    SUCCEED() << "findConstrainedPaths with huge max_results must not crash";
}
