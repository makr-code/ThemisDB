#include <gtest/gtest.h>
#include "index/graph_analytics.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "query/functions/function_registry.h"
#include <filesystem>
#include <cmath>
#include <chrono>

using namespace themis;

class KShortestPathsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory
        testDbPath_ = std::filesystem::temp_directory_path() / "test_k_shortest_paths_db";
        std::filesystem::remove_all(testDbPath_);
        std::filesystem::create_directories(testDbPath_);

        // Initialize database
        RocksDBWrapper::Config cfg;
        cfg.db_path = testDbPath_.string();
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 256;
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        graphMgr_ = std::make_unique<GraphIndexManager>(*db_);
        analytics_ = std::make_unique<GraphAnalytics>(*graphMgr_);
    }

    void TearDown() override {
        analytics_.reset();
        graphMgr_.reset();
        db_.reset();
        std::filesystem::remove_all(testDbPath_);
    }

    // Helper: Create edge entity
    BaseEntity createEdge(const std::string& id, const std::string& from, const std::string& to, double weight = 1.0) {
        BaseEntity edge;
        edge.setPrimaryKey(id);
        edge.setField("id", Value(id));
        edge.setField("_from", Value(from));
        edge.setField("_to", Value(to));
        edge.setField("weight", Value(weight));
        return edge;
    }

    // Helper: Build a simple graph with multiple paths
    void buildMultiPathGraph() {
        // Graph structure:
        //   A -> B -> D -> E
        //   A -> C -> E
        //   A -> B -> C -> E
        //
        // Paths from A to E:
        // 1. A -> C -> E (length 2)
        // 2. A -> B -> D -> E (length 3)
        // 3. A -> B -> C -> E (length 3)
        
        auto e1 = createEdge("e1", "A", "B", 1.0);
        auto e2 = createEdge("e2", "A", "C", 1.0);
        auto e3 = createEdge("e3", "B", "D", 1.0);
        auto e4 = createEdge("e4", "B", "C", 1.0);
        auto e5 = createEdge("e5", "C", "E", 1.0);
        auto e6 = createEdge("e6", "D", "E", 1.0);

        ASSERT_TRUE(graphMgr_->addEdge(e1).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e2).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e3).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e4).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e5).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e6).ok);
    }

    // Helper: Build a weighted graph
    void buildWeightedGraph() {
        // Graph structure with weights:
        //   A -5-> B -3-> E
        //   A -2-> C -10-> E
        //   A -4-> D -1-> E
        //
        // Paths from A to E by weight:
        // 1. A -> D -> E (weight 5)
        // 2. A -> B -> E (weight 8)
        // 3. A -> C -> E (weight 12)
        
        auto e1 = createEdge("e1", "A", "B", 5.0);
        auto e2 = createEdge("e2", "A", "C", 2.0);
        auto e3 = createEdge("e3", "A", "D", 4.0);
        auto e4 = createEdge("e4", "B", "E", 3.0);
        auto e5 = createEdge("e5", "C", "E", 10.0);
        auto e6 = createEdge("e6", "D", "E", 1.0);

        ASSERT_TRUE(graphMgr_->addEdge(e1).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e2).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e3).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e4).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e5).ok);
        ASSERT_TRUE(graphMgr_->addEdge(e6).ok);
    }

    std::filesystem::path testDbPath_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> graphMgr_;
    std::unique_ptr<GraphAnalytics> analytics_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(KShortestPathsTest, FindsSingleShortestPath) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 1);
    ASSERT_TRUE(status.ok);
    
    ASSERT_EQ(paths.size(), 1);
    
    // Shortest path should be A -> C -> E (length 2)
    EXPECT_EQ(paths[0].vertices.size(), 3);
    EXPECT_EQ(paths[0].vertices[0], "A");
    EXPECT_EQ(paths[0].vertices[1], "C");
    EXPECT_EQ(paths[0].vertices[2], "E");
    EXPECT_EQ(paths[0].hop_count, 2);
}

TEST_F(KShortestPathsTest, FindsMultiplePaths) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 3);
    ASSERT_TRUE(status.ok);
    
    ASSERT_GE(paths.size(), 2);  // Should find at least 2 paths
    ASSERT_LE(paths.size(), 3);  // Should not exceed k
    
    // Paths should be sorted by length
    for (size_t i = 1; i < paths.size(); ++i) {
        EXPECT_LE(paths[i-1].length, paths[i].length) 
            << "Paths should be sorted by increasing length";
    }
}

TEST_F(KShortestPathsTest, PathsAreUnique) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 5);
    ASSERT_TRUE(status.ok);
    
    // Check that all paths are unique
    for (size_t i = 0; i < paths.size(); ++i) {
        for (size_t j = i + 1; j < paths.size(); ++j) {
            EXPECT_NE(paths[i].vertices, paths[j].vertices)
                << "Paths " << i << " and " << j << " are duplicates";
        }
    }
}

TEST_F(KShortestPathsTest, PathsAreLoopless) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 5);
    ASSERT_TRUE(status.ok);
    
    // Check that no path contains loops
    for (const auto& path : paths) {
        std::set<std::string> visited;
        for (const auto& vertex : path.vertices) {
            EXPECT_EQ(visited.count(vertex), 0) 
                << "Path contains loop through vertex " << vertex;
            visited.insert(vertex);
        }
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(KShortestPathsTest, NoPathExists) {
    // Create disconnected graph
    auto e1 = createEdge("e1", "A", "B");
    auto e2 = createEdge("e2", "C", "D");
    ASSERT_TRUE(graphMgr_->addEdge(e1).ok);
    ASSERT_TRUE(graphMgr_->addEdge(e2).ok);
    
    auto [status, paths] = analytics_->kShortestPaths("A", "D", 3);
    ASSERT_TRUE(status.ok);
    
    EXPECT_EQ(paths.size(), 0) << "Should return empty when no path exists";
}

TEST_F(KShortestPathsTest, KIsZero) {
    buildMultiPathGraph();
    
    auto [status_zero, paths_zero] = analytics_->kShortestPaths("A", "E", 0);
    
    EXPECT_FALSE(status_zero.ok) << "Should return error for k=0";
}

TEST_F(KShortestPathsTest, KIsNegative) {
    buildMultiPathGraph();
    
    auto [status_negative, paths_negative] = analytics_->kShortestPaths("A", "E", -1);
    
    EXPECT_FALSE(status_negative.ok) << "Should return error for negative k";
}

TEST_F(KShortestPathsTest, KExceedsAvailablePaths) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 100);
    ASSERT_TRUE(status.ok);
    
    EXPECT_GT(paths.size(), 0) << "Should return available paths";
    EXPECT_LT(paths.size(), 100) << "Should not return more paths than exist";
}

TEST_F(KShortestPathsTest, SourceEqualsTarget) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "A", 3);
    ASSERT_TRUE(status.ok);
    
    // Depending on implementation, this might return empty or a single-node path
    // For now, we just check it doesn't crash
}

TEST_F(KShortestPathsTest, SingleEdgePath) {
    auto e1 = createEdge("e1", "A", "B");
    ASSERT_TRUE(graphMgr_->addEdge(e1).ok);
    
    auto [status, paths] = analytics_->kShortestPaths("A", "B", 1);
    ASSERT_TRUE(status.ok);
    
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].vertices.size(), 2);
    EXPECT_EQ(paths[0].hop_count, 1);
}

// ============================================================================
// Complex Graph Tests
// ============================================================================

TEST_F(KShortestPathsTest, GridGraph) {
    // Create a 3x3 grid:
    //   A - B - C
    //   |   |   |
    //   D - E - F
    //   |   |   |
    //   G - H - I
    //
    // Multiple paths from A to I
    
    // Horizontal edges
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e1", "A", "B")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e2", "B", "C")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e3", "D", "E")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e4", "E", "F")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e5", "G", "H")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e6", "H", "I")).ok);
    
    // Vertical edges
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e7", "A", "D")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e8", "D", "G")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e9", "B", "E")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e10", "E", "H")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e11", "C", "F")).ok);
    ASSERT_TRUE(graphMgr_->addEdge(createEdge("e12", "F", "I")).ok);
    
    auto [status, paths] = analytics_->kShortestPaths("A", "I", 5);
    ASSERT_TRUE(status.ok);
    
    EXPECT_GT(paths.size(), 0);
    
    // Shortest path should have length 4 (minimum distance in grid)
    EXPECT_EQ(paths[0].hop_count, 4);
}

// ============================================================================
// Path Information Tests
// ============================================================================

TEST_F(KShortestPathsTest, PathContainsCorrectVertices) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 1);
    ASSERT_TRUE(status.ok);
    
    ASSERT_EQ(paths.size(), 1);
    
    // Check vertices list
    EXPECT_EQ(paths[0].vertices.front(), "A") << "Path should start at source";
    EXPECT_EQ(paths[0].vertices.back(), "E") << "Path should end at target";
    EXPECT_EQ(paths[0].vertices.size(), paths[0].edges.size() + 1)
        << "Vertices should be edges + 1";
}

TEST_F(KShortestPathsTest, PathContainsCorrectEdges) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 1);
    ASSERT_TRUE(status.ok);
    
    ASSERT_EQ(paths.size(), 1);
    
    // Verify edges connect consecutive vertices
    for (size_t i = 0; i < paths[0].edges.size(); ++i) {
        EXPECT_EQ(paths[0].edges[i].first, paths[0].vertices[i]);
        EXPECT_EQ(paths[0].edges[i].second, paths[0].vertices[i + 1]);
    }
}

TEST_F(KShortestPathsTest, HopCountMatchesEdgeCount) {
    buildMultiPathGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 3);
    ASSERT_TRUE(status.ok);
    
    for (const auto& path : paths) {
        EXPECT_EQ(path.hop_count, static_cast<int>(path.edges.size()))
            << "Hop count should equal number of edges";
    }
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(KShortestPathsTest, PerformanceSmallGraph) {
    buildMultiPathGraph();
    
    auto start = std::chrono::high_resolution_clock::now();
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 3);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_TRUE(status.ok);
    EXPECT_LT(duration.count(), 50) << "Small graph should complete in < 50ms";
}

// ============================================================================
// Weighted Graph Tests
// ============================================================================

TEST_F(KShortestPathsTest, WeightedGraphShortestByWeight) {
    buildWeightedGraph();
    
    auto [status, paths] = analytics_->kShortestPaths("A", "E", 3, "weight");
    ASSERT_TRUE(status.ok);
    
    ASSERT_GE(paths.size(), 3);
    
    // Paths should be sorted by total weight, not hop count
    // Expected order by weight:
    // 1. A -> D -> E (weight 5)
    // 2. A -> B -> E (weight 8)
    // 3. A -> C -> E (weight 12)
    
    EXPECT_LE(paths[0].length, 5.1) << "First path should have weight ~5";
    EXPECT_GE(paths[0].length, 4.9);
    
    EXPECT_LE(paths[1].length, 8.1) << "Second path should have weight ~8";
    EXPECT_GE(paths[1].length, 7.9);
    
    EXPECT_LE(paths[2].length, 12.1) << "Third path should have weight ~12";
    EXPECT_GE(paths[2].length, 11.9);
}

TEST_F(KShortestPathsTest, WeightedVsUnweightedDifferentOrder) {
    buildWeightedGraph();
    
    // Get paths with weight attribute
    auto [status_weighted, paths_weighted] = analytics_->kShortestPaths("A", "E", 3, "weight");
    
    // Get paths without weight attribute (uses default _weight or 1.0)
    auto [status_unweighted, paths_unweighted] = analytics_->kShortestPaths("A", "E", 3);
    
    ASSERT_TRUE(status_weighted.ok);
    ASSERT_TRUE(status_unweighted.ok);
    
    // Weighted should prefer A->D->E (weight 5, hops 2)
    // Unweighted should prefer A->B->E or A->C->E (hops 2)
    
    if (paths_weighted.size() > 0 && paths_unweighted.size() > 0) {
        // Verify that weighted considers actual weights
        EXPECT_LT(paths_weighted[0].length, 10.0) << "Weighted first path should have low weight";
    }
}

// ============================================================================
// Function Integration Tests
// ============================================================================

TEST_F(KShortestPathsTest, FunctionIntegrationWithContext) {
    buildMultiPathGraph();
    
    // Test the K_SHORTEST_PATHS function through FunctionRegistry
    auto& registry = themis::query::functions::FunctionRegistry::instance();
    
    // Create function context with graph analytics
    themis::query::functions::FunctionContext ctx;
    ctx.setGraphAnalytics(analytics_.get());
    
    // Prepare arguments
    std::vector<nlohmann::json> args;
    args.push_back(nlohmann::json("A"));  // start vertex
    args.push_back(nlohmann::json("E"));  // end vertex
    args.push_back(nlohmann::json(3));    // k
    
    // Call the function
    auto result = registry.call("K_SHORTEST_PATHS", args, ctx);
    
    // Verify result structure
    ASSERT_TRUE(result.is_array());
    EXPECT_GT(result.size(), 0) << "Should return at least one path";
    
    // Check first path structure
    if (result.size() > 0) {
        auto path = result[0];
        EXPECT_TRUE(path.contains("rank"));
        EXPECT_TRUE(path.contains("vertices"));
        EXPECT_TRUE(path.contains("edges"));
        EXPECT_TRUE(path.contains("length"));
        EXPECT_TRUE(path.contains("distance"));
        
        EXPECT_TRUE(path["vertices"].is_array());
        EXPECT_TRUE(path["edges"].is_array());
        EXPECT_EQ(path["rank"], 1);
    }
}

TEST_F(KShortestPathsTest, FunctionIntegrationWeightedGraph) {
    buildWeightedGraph();
    
    auto& registry = themis::query::functions::FunctionRegistry::instance();
    
    themis::query::functions::FunctionContext ctx;
    ctx.setGraphAnalytics(analytics_.get());
    
    // Test with weight attribute
    std::vector<nlohmann::json> args;
    args.push_back(nlohmann::json("A"));
    args.push_back(nlohmann::json("E"));
    args.push_back(nlohmann::json(3));
    
    nlohmann::json options = nlohmann::json::object();
    options["weightAttribute"] = "weight";
    args.push_back(options);
    
    auto result = registry.call("K_SHORTEST_PATHS", args, ctx);
    
    ASSERT_TRUE(result.is_array());
    EXPECT_GE(result.size(), 1);
    
    // Verify paths are sorted by distance
    if (result.size() >= 2) {
        double dist1 = result[0]["distance"].get<double>();
        double dist2 = result[1]["distance"].get<double>();
        EXPECT_LE(dist1, dist2) << "Paths should be sorted by distance";
    }
}

TEST_F(KShortestPathsTest, FunctionGracefulDegradationWithoutContext) {
    buildMultiPathGraph();
    
    auto& registry = themis::query::functions::FunctionRegistry::instance();
    
    // Create context without graph analytics (simulates missing infrastructure)
    themis::query::functions::FunctionContext ctx;
    
    std::vector<nlohmann::json> args;
    args.push_back(nlohmann::json("A"));
    args.push_back(nlohmann::json("E"));
    args.push_back(nlohmann::json(3));
    
    // Should not crash, returns empty array
    auto result = registry.call("K_SHORTEST_PATHS", args, ctx);
    
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0) << "Should return empty array when analytics not available";
}

// ============================================================================
// Main
// ============================================================================


