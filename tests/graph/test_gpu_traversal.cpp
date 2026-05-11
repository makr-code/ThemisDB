/**
 * @file test_gpu_traversal.cpp
 * @brief GPUGraphTraversal focused tests — GPU-01..GPU-19
 *
 * Tests cover:
 *   GPU-01  Empty graph: load() succeeds with vertex_count=0, edge_count=0
 *   GPU-02  Load graph via explicit vertex_ids list
 *   GPU-03  BFS linear chain (A→B→C→D): correct distances at each level
 *   GPU-04  BFS with max_depth=1 stops at depth 1 and does not revisit deeper nodes
 *   GPU-05  BFS with forbidden vertex: forbidden vertex and its subtree skipped
 *   GPU-06  BFS max_results truncation: visited_vertices.size() == max_results, truncated=true
 *   GPU-07  BFS from unknown start vertex returns error
 *   GPU-08  BFS before load() returns error
 *   GPU-09  DFS basic traversal: all reachable vertices visited, discovery order monotone
 *   GPU-10  DFS with max_depth=1: only direct neighbours of source visited
 *   GPU-11  DFS with forbidden vertex: forbidden vertex not in visited_vertices
 *   GPU-12  DFS max_results truncation: visited_vertices.size() == max_results, truncated=true
 *   GPU-13  DFS from unknown start vertex returns error
 *   GPU-14  getStats() reflects vertex and edge counts from load()
 *   GPU-15  used_cpu_fallback is true when no GPU hardware is present
 *   GPU-16  BFS disconnected graph: only the source component is visited
 *   GPU-17  BFS cyclic graph: terminates without infinite loop, no duplicate visits
 *   GPU-18  DFS cyclic graph: terminates without infinite loop, no duplicate visits
 *   GPU-19  BFS includes source vertex at distance 0
 */

#include <gtest/gtest.h>
#include "graph/gpu_traversal.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace themis {
namespace graph {
namespace {

// ---------------------------------------------------------------------------
// Test fixture — creates a fresh RocksDB database per test
// ---------------------------------------------------------------------------

class GPUTraversalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a unique path per test to avoid cross-test interference.
        const auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        db_path_ = std::filesystem::temp_directory_path() /
                   ("gpu_trav_" + std::to_string(tid) + "_" +
                    std::to_string(
                        std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_.string();
        cfg.memtable_size_mb    = 16;
        cfg.block_cache_size_mb = 32;
        cfg.max_background_jobs = 1;
        cfg.compression_default = "none";

        db_    = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        mgr_   = std::make_unique<themis::GraphIndexManager>(*db_);
    }

    void TearDown() override {
        mgr_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // Add a directed edge u → v.
    void addEdge(const std::string& u, const std::string& v,
                 const std::string& eid = "") {
        const std::string id = eid.empty() ? (u + "_" + v) : eid;
        BaseEntity e(id);
        e.setField("id", id);
        e.setField("_from", u);
        e.setField("_to", v);
        ASSERT_TRUE(mgr_->addEdge(e).ok);
    }

    std::filesystem::path                    db_path_;
    std::unique_ptr<themis::RocksDBWrapper>  db_;
    std::unique_ptr<themis::GraphIndexManager> mgr_;
};

// ---------------------------------------------------------------------------
// GPU-01: Empty graph
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU01_EmptyGraphLoadSucceeds) {
    GPUGraphTraversal t(*mgr_);
    auto res = t.load();
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_EQ(t.vertexCount(), 0u);
    EXPECT_EQ(t.edgeCount(), 0u);
}

// ---------------------------------------------------------------------------
// GPU-02: Load with explicit vertex_ids
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU02_LoadWithExplicitVertexIds) {
    addEdge("A", "B");
    addEdge("B", "C");

    GPUGraphTraversal t(*mgr_);
    auto res = t.load({"A", "B"});
    ASSERT_TRUE(res.has_value()) << res.error().message();
    // Explicit vertex IDs seed the load, but discovered neighbours are still
    // materialized into the CSR graph during adjacency expansion.
    EXPECT_EQ(t.vertexCount(), 3u);
}

// ---------------------------------------------------------------------------
// GPU-03: BFS correct distances in linear chain A→B→C→D
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU03_BFS_LinearChain_CorrectDistances) {
    addEdge("A", "B");
    addEdge("B", "C");
    addEdge("C", "D");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.bfs("A");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    const auto& r = res.value();

    EXPECT_EQ(r.distances.at("A"), 0);
    EXPECT_EQ(r.distances.at("B"), 1);
    EXPECT_EQ(r.distances.at("C"), 2);
    EXPECT_EQ(r.distances.at("D"), 3);
    EXPECT_FALSE(r.truncated);
    EXPECT_GE(r.nodes_explored, 4u);
}

// ---------------------------------------------------------------------------
// GPU-04: BFS max_depth limits traversal depth
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU04_BFS_MaxDepth) {
    addEdge("A", "B");
    addEdge("B", "C");
    addEdge("C", "D");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.max_depth = 1;
    auto res = t.bfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    // Should see A (depth 0) and B (depth 1); C and D are beyond max_depth.
    EXPECT_NE(r.distances.count("A"), 0u);
    EXPECT_NE(r.distances.count("B"), 0u);
    EXPECT_EQ(r.distances.count("C"), 0u);
    EXPECT_EQ(r.distances.count("D"), 0u);
}

// ---------------------------------------------------------------------------
// GPU-05: BFS forbidden vertex
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU05_BFS_ForbiddenVertex) {
    //   A → B → C
    //       ↓
    //       D
    addEdge("A", "B");
    addEdge("B", "C");
    addEdge("B", "D");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.forbidden_vertices = {"B"};
    auto res = t.bfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    // A is visited; B is forbidden → neither B, C, nor D are in the result.
    EXPECT_NE(r.distances.count("A"), 0u);
    EXPECT_EQ(r.distances.count("B"), 0u);
    EXPECT_EQ(r.distances.count("C"), 0u);
    EXPECT_EQ(r.distances.count("D"), 0u);
}

// ---------------------------------------------------------------------------
// GPU-06: BFS max_results truncation
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU06_BFS_MaxResultsTruncation) {
    // Fan-out from S: S→A, S→B, S→C, S→D (4 neighbours)
    addEdge("S", "A");
    addEdge("S", "B");
    addEdge("S", "C");
    addEdge("S", "D");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.max_results = 2;
    auto res = t.bfs("S", cfg);
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    EXPECT_EQ(r.visited_vertices.size(), 2u);
    EXPECT_TRUE(r.truncated);
}

// ---------------------------------------------------------------------------
// GPU-07: BFS unknown start vertex returns error
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU07_BFS_UnknownVertex_ReturnsError) {
    addEdge("A", "B");
    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.bfs("NONEXISTENT");
    EXPECT_FALSE(res.has_value());
}

// ---------------------------------------------------------------------------
// GPU-08: BFS before load() returns error
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU08_BFS_BeforeLoad_ReturnsError) {
    GPUGraphTraversal t(*mgr_);
    // No call to load()
    auto res = t.bfs("A");
    EXPECT_FALSE(res.has_value());
}

// ---------------------------------------------------------------------------
// GPU-09: DFS basic traversal — all reachable vertices visited
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU09_DFS_BasicTraversal_AllReachableVisited) {
    addEdge("A", "B");
    addEdge("B", "C");
    addEdge("A", "D");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.dfs("A");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    const auto& r = res.value();

    std::unordered_set<std::string> visited(r.visited_vertices.begin(),
                                            r.visited_vertices.end());
    EXPECT_TRUE(visited.count("A"));
    EXPECT_TRUE(visited.count("B"));
    EXPECT_TRUE(visited.count("C"));
    EXPECT_TRUE(visited.count("D"));
    EXPECT_FALSE(r.truncated);
}

// ---------------------------------------------------------------------------
// GPU-10: DFS max_depth=1 limits to direct neighbours only
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU10_DFS_MaxDepth1_DirectNeighboursOnly) {
    addEdge("A", "B");
    addEdge("B", "C");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.max_depth = 1;
    auto res = t.dfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    std::unordered_set<std::string> visited(r.visited_vertices.begin(),
                                            r.visited_vertices.end());
    EXPECT_TRUE(visited.count("A"));
    EXPECT_TRUE(visited.count("B"));
    EXPECT_FALSE(visited.count("C"));
}

// ---------------------------------------------------------------------------
// GPU-11: DFS forbidden vertex not in result
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU11_DFS_ForbiddenVertex_NotVisited) {
    addEdge("A", "B");
    addEdge("B", "C");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.forbidden_vertices = {"B"};
    auto res = t.dfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    std::unordered_set<std::string> visited(r.visited_vertices.begin(),
                                            r.visited_vertices.end());
    EXPECT_TRUE(visited.count("A"));
    EXPECT_FALSE(visited.count("B"));
    EXPECT_FALSE(visited.count("C"));
}

// ---------------------------------------------------------------------------
// GPU-12: DFS max_results truncation
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU12_DFS_MaxResultsTruncation) {
    addEdge("A", "B");
    addEdge("A", "C");
    addEdge("A", "D");
    addEdge("A", "E");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.max_results = 2;
    auto res = t.dfs("A", cfg);
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    EXPECT_EQ(r.visited_vertices.size(), 2u);
    EXPECT_TRUE(r.truncated);
}

// ---------------------------------------------------------------------------
// GPU-13: DFS unknown start vertex returns error
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU13_DFS_UnknownVertex_ReturnsError) {
    addEdge("A", "B");
    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.dfs("NONEXISTENT");
    EXPECT_FALSE(res.has_value());
}

// ---------------------------------------------------------------------------
// GPU-14: getStats() reflects loaded vertex/edge counts
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU14_GetStats_ReflectsLoadedGraph) {
    // 3 edges: A→B, A→C, B→C (3 vertices)
    addEdge("A", "B");
    addEdge("A", "C");
    addEdge("B", "C");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto s = t.getStats();
    EXPECT_EQ(s.vertex_count, 3u);
    EXPECT_EQ(s.edge_count, 3u);
}

// ---------------------------------------------------------------------------
// GPU-15: used_cpu_fallback is true without GPU hardware
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU15_BFS_UsesCPUFallback_WhenNoGPU) {
    addEdge("X", "Y");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    GPUGraphTraversal::Config cfg;
    cfg.min_vertices_for_gpu = std::numeric_limits<size_t>::max();
    auto res = t.bfs("X", cfg);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res.value().used_cpu_fallback);
}

// ---------------------------------------------------------------------------
// GPU-16: BFS disconnected graph — only reachable component visited
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU16_BFS_DisconnectedGraph_OnlyReachableComponent) {
    // Component 1: A → B
    // Component 2: C → D  (not reachable from A)
    addEdge("A", "B");
    addEdge("C", "D");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.bfs("A");
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    std::unordered_set<std::string> visited(r.visited_vertices.begin(),
                                            r.visited_vertices.end());
    EXPECT_TRUE(visited.count("A"));
    EXPECT_TRUE(visited.count("B"));
    EXPECT_FALSE(visited.count("C"));
    EXPECT_FALSE(visited.count("D"));
}

// ---------------------------------------------------------------------------
// GPU-17: BFS cyclic graph terminates without infinite loop
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU17_BFS_CyclicGraph_Terminates) {
    // Cycle: A → B → C → A
    addEdge("A", "B");
    addEdge("B", "C");
    addEdge("C", "A");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.bfs("A");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    const auto& r = res.value();

    // All 3 vertices visited exactly once.
    EXPECT_EQ(r.visited_vertices.size(), 3u);

    // No duplicates in visited_vertices.
    std::unordered_set<std::string> visited_set(r.visited_vertices.begin(),
                                                r.visited_vertices.end());
    EXPECT_EQ(visited_set.size(), r.visited_vertices.size());
}

// ---------------------------------------------------------------------------
// GPU-18: DFS cyclic graph terminates without infinite loop
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU18_DFS_CyclicGraph_Terminates) {
    // Cycle: A → B → C → A
    addEdge("A", "B");
    addEdge("B", "C");
    addEdge("C", "A");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.dfs("A");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    const auto& r = res.value();

    EXPECT_EQ(r.visited_vertices.size(), 3u);

    // No duplicates.
    std::unordered_set<std::string> visited_set(r.visited_vertices.begin(),
                                                r.visited_vertices.end());
    EXPECT_EQ(visited_set.size(), r.visited_vertices.size());
}

// ---------------------------------------------------------------------------
// GPU-19: BFS includes source vertex at distance 0
// ---------------------------------------------------------------------------

TEST_F(GPUTraversalTest, GPU19_BFS_SourceVertex_AtDistanceZero) {
    addEdge("A", "B");

    GPUGraphTraversal t(*mgr_);
    ASSERT_TRUE(t.load().has_value());

    auto res = t.bfs("A");
    ASSERT_TRUE(res.has_value());
    const auto& r = res.value();

    ASSERT_TRUE(r.distances.count("A") > 0);
    EXPECT_EQ(r.distances.at("A"), 0);

    // Source must be first in visited_vertices.
    ASSERT_FALSE(r.visited_vertices.empty());
    EXPECT_EQ(r.visited_vertices.front(), "A");
}

} // namespace
} // namespace graph
} // namespace themis
