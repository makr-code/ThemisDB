#include <gtest/gtest.h>
#include "graph/gpu_traversal.h"
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "observability/metrics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class GPUGraphTraversalTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_gpu_traversal_test";
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
        trav_       = std::make_unique<themis::graph::GPUGraphTraversal>(*graph_mgr_);

        buildTestGraph();
    }

    void TearDown() override {
        trav_.reset();
        graph_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    /**
     * Test graph topology:
     *   A -> B -> C -> D
     *   A -> C
     *   E -> F
     * Two connected components: {A,B,C,D} and {E,F}.
     */
    void buildTestGraph() {
        auto addEdge = [&](const std::string& id,
                           const std::string& from,
                           const std::string& to) {
            themis::BaseEntity e(id);
            e.setField("id", id);
            e.setField("_from", from);
            e.setField("_to", to);
            e.setField("_weight", "1.0");
            graph_mgr_->addEdge(e);
        };

        addEdge("e1", "A", "B");
        addEdge("e2", "B", "C");
        addEdge("e3", "C", "D");
        addEdge("e4", "A", "C");
        addEdge("e5", "E", "F");
    }

    // Reload the CSR from the graph manager.
    void loadGraph() {
        auto res = trav_->load();
        ASSERT_TRUE(res.has_value()) << "load() failed";
    }

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_mgr_;
    std::unique_ptr<themis::graph::GPUGraphTraversal> trav_;
};

// ---------------------------------------------------------------------------
// load() tests
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, Load_SucceedsOnNonEmptyGraph) {
    auto res = trav_->load();
    EXPECT_TRUE(res.has_value());
}

TEST_F(GPUGraphTraversalTest, Load_PopulatesVertexAndEdgeCounts) {
    loadGraph();
    // A, B, C, D, E, F = 6 vertices; 5 edges
    EXPECT_EQ(trav_->vertexCount(), 6u);
    EXPECT_EQ(trav_->edgeCount(), 5u);
}

TEST_F(GPUGraphTraversalTest, Load_ReloadUpdatesState) {
    loadGraph();
    const size_t first_count = trav_->vertexCount();
    loadGraph(); // second load should produce the same result
    EXPECT_EQ(trav_->vertexCount(), first_count);
}

// ---------------------------------------------------------------------------
// getStats() tests
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, Stats_ReflectsLoadedGraph) {
    loadGraph();
    auto stats = trav_->getStats();
    EXPECT_EQ(stats.vertex_count, 6u);
    EXPECT_EQ(stats.edge_count,   5u);
}

// ---------------------------------------------------------------------------
// BFS – correctness tests
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, BFS_BeforeLoad_ReturnsError) {
    // trav_ was not loaded
    auto res = trav_->bfs("A");
    EXPECT_FALSE(res.has_value());
}

TEST_F(GPUGraphTraversalTest, BFS_UnknownVertex_ReturnsError) {
    loadGraph();
    auto res = trav_->bfs("Z");
    EXPECT_FALSE(res.has_value());
}

TEST_F(GPUGraphTraversalTest, BFS_SingleSource_ReachesAllInComponent) {
    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());

    const auto& visited = res->visited_vertices;
    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << v << " should be reachable from A";
    }
    // E and F are in a separate component
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "E"), visited.end());
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "F"), visited.end());
}

TEST_F(GPUGraphTraversalTest, BFS_NoDuplicateVertices) {
    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());

    std::unordered_set<std::string> seen;
    for (const auto& v : res->visited_vertices) {
        EXPECT_TRUE(seen.insert(v).second) << "Duplicate: " << v;
    }
}

TEST_F(GPUGraphTraversalTest, BFS_SourceVertexIsFirstVisited) {
    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());
    ASSERT_FALSE(res->visited_vertices.empty());
    EXPECT_EQ(res->visited_vertices.front(), "A");
}

TEST_F(GPUGraphTraversalTest, BFS_DistancesAreCorrect) {
    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());

    // A is at distance 0; B and C are at distance 1 (direct edges A->B, A->C)
    EXPECT_EQ(res->distances.at("A"), 0);
    EXPECT_EQ(res->distances.at("B"), 1);
    EXPECT_EQ(res->distances.at("C"), 1);
    // D is at distance 2 (A->C->D)
    EXPECT_EQ(res->distances.at("D"), 2);
}

TEST_F(GPUGraphTraversalTest, BFS_MaxDepthZero_OnlySource) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.max_depth = 0;

    auto res = trav_->bfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->visited_vertices.size(), 1u);
    EXPECT_EQ(res->visited_vertices.front(), "A");
}

TEST_F(GPUGraphTraversalTest, BFS_MaxDepthOne_OnlyDirectNeighbors) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.max_depth = 1;

    auto res = trav_->bfs("A", cfg);
    ASSERT_TRUE(res.has_value());

    const auto& v = res->visited_vertices;
    EXPECT_NE(std::find(v.begin(), v.end(), "A"), v.end());
    EXPECT_NE(std::find(v.begin(), v.end(), "B"), v.end());
    EXPECT_NE(std::find(v.begin(), v.end(), "C"), v.end());
    // D is 2 hops from A
    EXPECT_EQ(std::find(v.begin(), v.end(), "D"), v.end());
}

TEST_F(GPUGraphTraversalTest, BFS_MaxResults_EnforcedEarlyTermination) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.max_results = 2;

    auto res = trav_->bfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    EXPECT_LE(res->visited_vertices.size(), 2u);
    EXPECT_TRUE(res->truncated);
}

TEST_F(GPUGraphTraversalTest, BFS_ForbiddenVertex_NotVisited) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.forbidden_vertices = {"B"};

    auto res = trav_->bfs("A", cfg);
    ASSERT_TRUE(res.has_value());

    const auto& v = res->visited_vertices;
    EXPECT_EQ(std::find(v.begin(), v.end(), "B"), v.end())
        << "B must not be visited";
}

TEST_F(GPUGraphTraversalTest, BFS_MetricsPopulated) {
    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());

    EXPECT_GT(res->nodes_explored, 0u);
    EXPECT_GE(res->edges_traversed, 0u);
    EXPECT_GE(res->execution_time_ms, 0.0);
}

TEST_F(GPUGraphTraversalTest, BFS_UsedCPUFallback_IsTrue) {
    // In a no-GPU environment the CPU fallback flag must be set.
    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->used_cpu_fallback);
}

TEST_F(GPUGraphTraversalTest, BFS_NegativeMaxDepth_ReturnsInvalidInputAndMetric) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.max_depth = -1;

    auto res = trav_->bfs("A", cfg);
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_acceleration_routes_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"bfs\""), std::string::npos);
    EXPECT_NE(exported.find("route=\"rejected\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"invalid_config\""), std::string::npos);
}

TEST_F(GPUGraphTraversalTest, DFS_NegativeGpuDevice_ReturnsInvalidInputAndMetric) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.gpu_device = -1;

    auto res = trav_->dfs("A", cfg);
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_acceleration_routes_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"dfs\""), std::string::npos);
    EXPECT_NE(exported.find("route=\"rejected\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"invalid_config\""), std::string::npos);
}

TEST_F(GPUGraphTraversalTest, BFS_EmitsCpuRouteMetricWhenGpuUnavailable) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    loadGraph();
    auto res = trav_->bfs("A");
    ASSERT_TRUE(res.has_value());

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_acceleration_routes_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"bfs\""), std::string::npos);
    EXPECT_NE(exported.find("route=\"cpu\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"gpu_unavailable\""), std::string::npos);
}

// ---------------------------------------------------------------------------
// DFS – correctness tests
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, DFS_BeforeLoad_ReturnsError) {
    auto res = trav_->dfs("A");
    EXPECT_FALSE(res.has_value());
}

TEST_F(GPUGraphTraversalTest, DFS_UnknownVertex_ReturnsError) {
    loadGraph();
    auto res = trav_->dfs("Z");
    EXPECT_FALSE(res.has_value());
}

TEST_F(GPUGraphTraversalTest, DFS_SingleSource_ReachesAllInComponent) {
    loadGraph();
    auto res = trav_->dfs("A");
    ASSERT_TRUE(res.has_value());

    const auto& visited = res->visited_vertices;
    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << v << " should be reachable via DFS from A";
    }
}

TEST_F(GPUGraphTraversalTest, DFS_NoDuplicateVertices) {
    loadGraph();
    auto res = trav_->dfs("A");
    ASSERT_TRUE(res.has_value());

    std::unordered_set<std::string> seen;
    for (const auto& v : res->visited_vertices) {
        EXPECT_TRUE(seen.insert(v).second) << "Duplicate in DFS: " << v;
    }
}

TEST_F(GPUGraphTraversalTest, DFS_MaxDepthOne_LimitsReachability) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.max_depth = 1;

    auto res = trav_->dfs("A", cfg);
    ASSERT_TRUE(res.has_value());

    const auto& v = res->visited_vertices;
    // D is at least 2 hops from A
    EXPECT_EQ(std::find(v.begin(), v.end(), "D"), v.end());
}

TEST_F(GPUGraphTraversalTest, DFS_ForbiddenVertex_NotVisited) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.forbidden_vertices = {"C"};

    auto res = trav_->dfs("A", cfg);
    ASSERT_TRUE(res.has_value());

    const auto& v = res->visited_vertices;
    EXPECT_EQ(std::find(v.begin(), v.end(), "C"), v.end());
    // D is only reachable through C so it must also be absent
    EXPECT_EQ(std::find(v.begin(), v.end(), "D"), v.end());
}

TEST_F(GPUGraphTraversalTest, DFS_MaxResults_EnforcedEarlyTermination) {
    loadGraph();
    themis::graph::GPUGraphTraversal::Config cfg;
    cfg.max_results = 1;

    auto res = trav_->dfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    EXPECT_LE(res->visited_vertices.size(), 1u);
    EXPECT_TRUE(res->truncated);
}

TEST_F(GPUGraphTraversalTest, DFS_MetricsPopulated) {
    loadGraph();
    auto res = trav_->dfs("A");
    ASSERT_TRUE(res.has_value());

    EXPECT_GT(res->nodes_explored, 0u);
    EXPECT_GE(res->execution_time_ms, 0.0);
}

// ---------------------------------------------------------------------------
// BFS vs DFS – same reachable set
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, BFS_And_DFS_SameVertexSet) {
    loadGraph();
    auto bfs_res = trav_->bfs("A");
    auto dfs_res = trav_->dfs("A");

    ASSERT_TRUE(bfs_res.has_value());
    ASSERT_TRUE(dfs_res.has_value());

    auto bv = bfs_res->visited_vertices;
    auto dv = dfs_res->visited_vertices;
    std::sort(bv.begin(), bv.end());
    std::sort(dv.begin(), dv.end());

    EXPECT_EQ(bv, dv) << "BFS and DFS must reach the same vertex set";
}

// ---------------------------------------------------------------------------
// Config defaults
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, Config_DefaultValues) {
    themis::graph::GPUGraphTraversal::Config cfg;
    EXPECT_EQ(cfg.gpu_device, 0);
    EXPECT_GT(cfg.min_vertices_for_gpu, 0u);
    EXPECT_EQ(cfg.max_depth, 10);
    EXPECT_EQ(cfg.max_results, 0u);
    EXPECT_TRUE(cfg.forbidden_vertices.empty());
}

// ---------------------------------------------------------------------------
// GraphIndexManager::allVertices() — unit tests
// ---------------------------------------------------------------------------

TEST_F(GPUGraphTraversalTest, AllVertices_ReturnsAllVerticesBeforeRebuildTopology) {
    // allVertices() must enumerate vertices even without rebuildTopology().
    auto [status, verts] = graph_mgr_->allVertices();
    EXPECT_TRUE(status.ok);
    EXPECT_FALSE(verts.empty());
    // All 6 vertices from the test graph must be present.
    std::unordered_set<std::string> vset(verts.begin(), verts.end());
    for (const char* v : {"A", "B", "C", "D", "E", "F"}) {
        EXPECT_TRUE(vset.count(v) > 0) << v << " missing from allVertices()";
    }
}

TEST_F(GPUGraphTraversalTest, AllVertices_CountMatchesExpected) {
    auto [status, verts] = graph_mgr_->allVertices();
    EXPECT_TRUE(status.ok);
    EXPECT_EQ(verts.size(), 6u);
}

// ---------------------------------------------------------------------------
// GraphQueryOptimizer integration — use_gpu = true
// ---------------------------------------------------------------------------

class GPUGraphOptimizerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_gpu_optimizer_integration_test";
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
        graph_mgr_   = std::make_unique<themis::GraphIndexManager>(*db_);
        optimizer_   = std::make_unique<themis::graph::GraphQueryOptimizer>(*graph_mgr_);

        buildGraph();
    }

    void TearDown() override {
        optimizer_.reset();
        graph_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    void buildGraph() {
        auto addEdge = [&](const std::string& id,
                           const std::string& from, const std::string& to) {
            themis::BaseEntity e(id);
            e.setField("id",     id);
            e.setField("_from",  from);
            e.setField("_to",    to);
            e.setField("_weight","1.0");
            graph_mgr_->addEdge(e);
        };
        addEdge("e1", "A", "B");
        addEdge("e2", "B", "C");
        addEdge("e3", "C", "D");
        addEdge("e4", "A", "C");
    }

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper>               db_;
    std::unique_ptr<themis::GraphIndexManager>            graph_mgr_;
    std::unique_ptr<themis::graph::GraphQueryOptimizer>   optimizer_;
};

TEST_F(GPUGraphOptimizerIntegrationTest, ExecuteBFS_UseGpu_ReturnsReachableVertices) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.use_gpu = true;

    auto result = optimizer_->executeBFS("A", 5, c);
    ASSERT_TRUE(result.has_value());

    const auto& visited = *result;
    EXPECT_FALSE(visited.empty());
    // A, B, C, D should all be reachable from A
    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << v << " should be in BFS result (use_gpu=true)";
    }
}

TEST_F(GPUGraphOptimizerIntegrationTest, ExecuteBFS_UseGpu_ProducesSameResultAsCpu) {
    themis::graph::GraphQueryOptimizer::QueryConstraints gpu_c;
    gpu_c.use_gpu = true;

    themis::graph::GraphQueryOptimizer::QueryConstraints cpu_c;
    cpu_c.use_gpu = false;

    auto gpu_res = optimizer_->executeBFS("A", 5, gpu_c);
    auto cpu_res = optimizer_->executeBFS("A", 5, cpu_c);

    ASSERT_TRUE(gpu_res.has_value());
    ASSERT_TRUE(cpu_res.has_value());

    auto gpu_v = *gpu_res;
    auto cpu_v = *cpu_res;
    std::sort(gpu_v.begin(), gpu_v.end());
    std::sort(cpu_v.begin(), cpu_v.end());
    EXPECT_EQ(gpu_v, cpu_v)
        << "GPU and CPU BFS must reach identical vertex sets";
}

TEST_F(GPUGraphOptimizerIntegrationTest, ExecuteDFS_UseGpu_ReturnsReachableVertices) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.use_gpu = true;

    auto result = optimizer_->executeDFS("A", 5, c);
    ASSERT_TRUE(result.has_value());

    const auto& visited = *result;
    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << v << " should be in DFS result (use_gpu=true)";
    }
}

TEST_F(GPUGraphOptimizerIntegrationTest, ExecuteDFS_UseGpu_ProducesSameResultAsCpu) {
    themis::graph::GraphQueryOptimizer::QueryConstraints gpu_c;
    gpu_c.use_gpu = true;

    themis::graph::GraphQueryOptimizer::QueryConstraints cpu_c;
    cpu_c.use_gpu = false;

    auto gpu_res = optimizer_->executeDFS("A", 5, gpu_c);
    auto cpu_res = optimizer_->executeDFS("A", 5, cpu_c);

    ASSERT_TRUE(gpu_res.has_value());
    ASSERT_TRUE(cpu_res.has_value());

    auto gpu_v = *gpu_res;
    auto cpu_v = *cpu_res;
    std::sort(gpu_v.begin(), gpu_v.end());
    std::sort(cpu_v.begin(), cpu_v.end());
    EXPECT_EQ(gpu_v, cpu_v)
        << "GPU and CPU DFS must reach identical vertex sets";
}

TEST_F(GPUGraphOptimizerIntegrationTest, ExecuteBFS_UseGpu_UnknownVertex_ReturnsError) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.use_gpu = true;

    auto result = optimizer_->executeBFS("DOES_NOT_EXIST", 3, c);
    EXPECT_FALSE(result.has_value());
}

TEST_F(GPUGraphOptimizerIntegrationTest, ExecuteBFS_UseGpu_MaxDepth_Respected) {
    themis::graph::GraphQueryOptimizer::QueryConstraints c;
    c.use_gpu    = true;
    c.max_depth  = 1;

    auto result = optimizer_->executeBFS("A", 1, c);
    ASSERT_TRUE(result.has_value());
    // D is 2+ hops from A; must not appear with max_depth=1
    EXPECT_EQ(std::find(result->begin(), result->end(), "D"), result->end());
}
