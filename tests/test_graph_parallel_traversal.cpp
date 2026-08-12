#include <gtest/gtest.h>
#include "graph/parallel_traversal.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

class ParallelTraversalTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_parallel_traversal_test";
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
        traversal_ = std::make_unique<themis::graph::ParallelTraversal>(*graph_mgr_);

        createTestGraph();
    }

    void TearDown() override {
        traversal_.reset();
        graph_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    /**
     * Creates the test graph:
     *   A -> B -> C -> D
     *   A -> C
     *   E -> F
     * (two connected components: {A,B,C,D} and {E,F})
     */
    void createTestGraph() {
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

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_mgr_;
    std::unique_ptr<themis::graph::ParallelTraversal> traversal_;
};

// ============================================================================
// multiSourceBFS – basic correctness
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_SingleSource_ReturnsReachable) {
    auto result = traversal_->multiSourceBFS({"A"});
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_FALSE(visited.empty());
    // All nodes reachable from A
    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << "Expected " << v << " to be reachable from A";
    }
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_TwoSources_UnionOfReachable) {
    // A reaches {A,B,C,D}; E reaches {E,F}
    auto result = traversal_->multiSourceBFS({"A", "E"});
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    for (const char* v : {"A", "B", "C", "D", "E", "F"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << "Expected " << v << " in multi-source result";
    }
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_NoDuplicates) {
    // Both A and B can reach C and D; result must deduplicate
    auto result = traversal_->multiSourceBFS({"A", "B"});
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    std::unordered_set<std::string> seen;
    for (const auto& v : visited) {
        EXPECT_TRUE(seen.insert(v).second) << "Duplicate vertex: " << v;
    }
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_VertexToSource_Populated) {
    auto result = traversal_->multiSourceBFS({"A", "E"});
    ASSERT_TRUE(result.has_value());

    const auto& v2s = result->vertex_to_source;
    // Each visited vertex must map to some source
    for (const auto& v : result->visited_vertices) {
        EXPECT_NE(v2s.find(v), v2s.end()) << "No source mapping for " << v;
    }
    // A is its own source
    ASSERT_NE(v2s.find("A"), v2s.end());
    EXPECT_EQ(v2s.at("A"), "A");
    // E is its own source
    ASSERT_NE(v2s.find("E"), v2s.end());
    EXPECT_EQ(v2s.at("E"), "E");
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_EmptySources_ReturnsError) {
    auto result = traversal_->multiSourceBFS({});
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// multiSourceBFS – depth constraint
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_MaxDepth_Respected) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_depth = 1;

    // A with depth=1 should find A, B, C (direct neighbors) but not D
    auto result = traversal_->multiSourceBFS({"A"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_NE(std::find(visited.begin(), visited.end(), "A"), visited.end());
    EXPECT_NE(std::find(visited.begin(), visited.end(), "B"), visited.end());
    EXPECT_NE(std::find(visited.begin(), visited.end(), "C"), visited.end());
    // D is 2 hops from A via B->C->D, and directly 2 hops via A->C->D
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "D"), visited.end());
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_ZeroDepth_OnlySources) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_depth = 0;

    auto result = traversal_->multiSourceBFS({"A", "E"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_EQ(visited.size(), 2u);
    EXPECT_NE(std::find(visited.begin(), visited.end(), "A"), visited.end());
    EXPECT_NE(std::find(visited.begin(), visited.end(), "E"), visited.end());
}

// ============================================================================
// multiSourceBFS – forbidden vertices
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_ForbiddenVertex_Excluded) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.forbidden_vertices = {"B"};

    auto result = traversal_->multiSourceBFS({"A"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "B"), visited.end())
        << "B must not be visited";
}

// ============================================================================
// multiSourceBFS – max_results constraint
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_MaxResults_Respected) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_results = 2;

    auto result = traversal_->multiSourceBFS({"A"}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_LE(result->visited_vertices.size(), 2u);
}

// ============================================================================
// multiSourceBFS – metrics
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_MetricsPopulated) {
    auto result = traversal_->multiSourceBFS({"A", "E"});
    ASSERT_TRUE(result.has_value());

    EXPECT_GT(result->total_nodes_explored, 0u);
    EXPECT_GE(result->total_edges_traversed, 0u);
    EXPECT_GE(result->execution_time_ms, 0.0);
    EXPECT_FALSE(result->timed_out);
}

// ============================================================================
// multiSourceBFS – parallel thread counts
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_ExplicitThreadCount_Succeeds) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.num_threads = 2;

    auto result = traversal_->multiSourceBFS({"A", "E"}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->visited_vertices.empty());
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_AutoThreadCount_Succeeds) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.num_threads = 0; // auto

    auto result = traversal_->multiSourceBFS({"A", "E"}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->visited_vertices.empty());
}

// ============================================================================
// multiSourceBFS vs sequential – result consistency
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_ConsistentWithSingleSourceBFS) {
    // Running multi-source with one source must produce the same vertex set as
    // running it with a single thread.
    themis::graph::ParallelTraversal::Config cfg1;
    cfg1.num_threads = 1;

    themis::graph::ParallelTraversal::Config cfg2;
    cfg2.num_threads = 4;

    auto r1 = traversal_->multiSourceBFS({"A"}, cfg1);
    auto r2 = traversal_->multiSourceBFS({"A"}, cfg2);

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());

    auto v1 = r1->visited_vertices;
    auto v2 = r2->visited_vertices;
    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());
    EXPECT_EQ(v1, v2);
}

// ============================================================================
// multiSourceDFS – basic correctness
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceDFS_SingleSource_ReturnsReachable) {
    auto result = traversal_->multiSourceDFS({"A"});
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_FALSE(visited.empty());

    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << v << " should be reachable from A via DFS";
    }
}

TEST_F(ParallelTraversalTest, MultiSourceDFS_TwoSources_UnionOfReachable) {
    auto result = traversal_->multiSourceDFS({"A", "E"});
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    for (const char* v : {"A", "B", "C", "D", "E", "F"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end())
            << v << " should appear in multi-source DFS result";
    }
}

TEST_F(ParallelTraversalTest, MultiSourceDFS_NoDuplicates) {
    auto result = traversal_->multiSourceDFS({"A", "B"});
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    std::unordered_set<std::string> seen;
    for (const auto& v : visited) {
        EXPECT_TRUE(seen.insert(v).second) << "Duplicate vertex in DFS result: " << v;
    }
}

TEST_F(ParallelTraversalTest, MultiSourceDFS_EmptySources_ReturnsError) {
    auto result = traversal_->multiSourceDFS({});
    EXPECT_FALSE(result.has_value());
}

TEST_F(ParallelTraversalTest, MultiSourceDFS_MaxDepth_Respected) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_depth = 1;

    auto result = traversal_->multiSourceDFS({"A"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    // D is at least 2 hops away from A
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "D"), visited.end());
}

TEST_F(ParallelTraversalTest, MultiSourceDFS_ForbiddenVertex_Excluded) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.forbidden_vertices = {"C"};

    auto result = traversal_->multiSourceDFS({"A"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "C"), visited.end());
    // D is only reachable through C, so it should also be absent
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "D"), visited.end());
}

TEST_F(ParallelTraversalTest, MultiSourceDFS_MetricsPopulated) {
    auto result = traversal_->multiSourceDFS({"A", "E"});
    ASSERT_TRUE(result.has_value());

    EXPECT_GT(result->total_nodes_explored, 0u);
    EXPECT_GE(result->execution_time_ms, 0.0);
    EXPECT_FALSE(result->timed_out);
}

// ============================================================================
// BFS vs DFS – same vertex set for full traversal
// ============================================================================

TEST_F(ParallelTraversalTest, BFS_And_DFS_SameVertexSet_FullTraversal) {
    auto bfs = traversal_->multiSourceBFS({"A"});
    auto dfs = traversal_->multiSourceDFS({"A"});

    ASSERT_TRUE(bfs.has_value());
    ASSERT_TRUE(dfs.has_value());

    auto bfs_v = bfs->visited_vertices;
    auto dfs_v = dfs->visited_vertices;
    std::sort(bfs_v.begin(), bfs_v.end());
    std::sort(dfs_v.begin(), dfs_v.end());

    EXPECT_EQ(bfs_v, dfs_v)
        << "BFS and DFS from the same source should visit the same vertices";
}

// ============================================================================
// Config defaults
// ============================================================================

TEST_F(ParallelTraversalTest, Config_DefaultValues) {
    themis::graph::ParallelTraversal::Config cfg;
    EXPECT_EQ(cfg.max_depth, 10);
    EXPECT_EQ(cfg.max_results, 0u);
    EXPECT_EQ(cfg.num_threads, 0u);
    EXPECT_EQ(cfg.timeout_ms, 0u);
    EXPECT_TRUE(cfg.forbidden_vertices.empty());
}

// ============================================================================
// Large multi-source fan-out
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_ManySourcesAllInGraph) {
    // A, B, C, D are all present; starting from all of them should find the
    // same vertex set as starting from A alone (since A reaches all of them).
    auto result = traversal_->multiSourceBFS({"A", "B", "C", "D"});
    ASSERT_TRUE(result.has_value());

    for (const char* v : {"A", "B", "C", "D"}) {
        EXPECT_NE(std::find(result->visited_vertices.begin(),
                            result->visited_vertices.end(), v),
                  result->visited_vertices.end());
    }
}

// ============================================================================
// Fan-out threshold: parallel frontier expansion within a single source BFS
// ============================================================================

TEST_F(ParallelTraversalTest, MultiSourceBFS_FanOutThreshold_SameResultAsSequential) {
    // With fan_out_threshold=1 every level triggers parallel expansion;
    // the result must still be the same as with sequential expansion.
    themis::graph::ParallelTraversal::Config cfg_parallel;
    cfg_parallel.fan_out_threshold = 1; // always use parallel expansion
    cfg_parallel.num_threads = 2;

    themis::graph::ParallelTraversal::Config cfg_sequential;
    // fan_out_threshold = 0 (default) → sequential expansion

    auto r_par = traversal_->multiSourceBFS({"A"}, cfg_parallel);
    auto r_seq = traversal_->multiSourceBFS({"A"}, cfg_sequential);

    ASSERT_TRUE(r_par.has_value());
    ASSERT_TRUE(r_seq.has_value());

    auto v_par = r_par->visited_vertices;
    auto v_seq = r_seq->visited_vertices;
    std::sort(v_par.begin(), v_par.end());
    std::sort(v_seq.begin(), v_seq.end());

    EXPECT_EQ(v_par, v_seq)
        << "Parallel fan-out BFS must visit the same vertices as sequential BFS";
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_FanOutThreshold_NoDuplicates) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.fan_out_threshold = 1; // always use parallel expansion
    cfg.num_threads = 2;

    auto result = traversal_->multiSourceBFS({"A", "B"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    std::unordered_set<std::string> seen;
    for (const auto& v : visited) {
        EXPECT_TRUE(seen.insert(v).second) << "Duplicate vertex with fan-out expansion: " << v;
    }
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_FanOutThreshold_ForbiddenRespected) {
    themis::graph::ParallelTraversal::Config cfg;
    cfg.fan_out_threshold = 1;
    cfg.num_threads = 2;
    cfg.forbidden_vertices = {"B"};

    auto result = traversal_->multiSourceBFS({"A"}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& visited = result->visited_vertices;
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "B"), visited.end())
        << "B must be excluded even with parallel fan-out expansion";
}

TEST_F(ParallelTraversalTest, MultiSourceBFS_FanOutThreshold_LargeThresholdBehavesSequential) {
    // A very large threshold means sequential path is always taken.
    themis::graph::ParallelTraversal::Config cfg_large;
    cfg_large.fan_out_threshold = 10000;

    themis::graph::ParallelTraversal::Config cfg_seq;
    // default: fan_out_threshold = 0 → sequential

    auto r_large = traversal_->multiSourceBFS({"A"}, cfg_large);
    auto r_seq   = traversal_->multiSourceBFS({"A"}, cfg_seq);

    ASSERT_TRUE(r_large.has_value());
    ASSERT_TRUE(r_seq.has_value());

    auto v_large = r_large->visited_vertices;
    auto v_seq   = r_seq->visited_vertices;
    std::sort(v_large.begin(), v_large.end());
    std::sort(v_seq.begin(), v_seq.end());

    EXPECT_EQ(v_large, v_seq);
}

TEST_F(ParallelTraversalTest, Config_FanOutThreshold_DefaultZero) {
    themis::graph::ParallelTraversal::Config cfg;
    EXPECT_EQ(cfg.fan_out_threshold, 0u);
}
