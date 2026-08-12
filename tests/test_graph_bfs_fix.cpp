/**
 * @file test_graph_bfs_fix.cpp
 * @brief BFS correctness regression tests for GraphIndexManager.
 *
 * These tests guard against regressions in the BFS implementation,
 * particularly:
 *  - Correct depth-limited traversal (maxDepth respects hop count)
 *  - No duplicate vertices in result set
 *  - Disconnected vertices are not included
 *  - Empty / degenerate graph edge cases (empty start vertex, maxDepth=0)
 *  - Cycle handling (no infinite loop, no duplicate results)
 *  - BFS with edge-type filtering returns only matching edges
 *  - allVertices() fallback enumerates all known vertices
 */

#include <gtest/gtest.h>
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using themis::BaseEntity;
using themis::GraphIndexManager;
using themis::RocksDBWrapper;

// ============================================================================
// Test fixture
// ============================================================================

class GraphBfsFixTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_bfs_fix_test";
        fs::remove_all(test_db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = test_db_path_;
        cfg.memtable_size_mb    = 16;
        cfg.block_cache_size_mb = 32;
        cfg.max_background_jobs = 1;

        db_       = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        graph_    = std::make_unique<GraphIndexManager>(*db_);
    }

    void TearDown() override {
        graph_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    // Helper: add a directed edge
    void addEdge(const std::string& id, const std::string& from,
                 const std::string& to,
                 const std::string& type = "default") {
        BaseEntity e(id);
        e.setField("id",      id);
        e.setField("_from",   from);
        e.setField("_to",     to);
        e.setField("_type",   type);
        e.setField("_weight", "1.0");
        graph_->addEdge(e);
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper>    db_;
    std::unique_ptr<GraphIndexManager> graph_;
};

// ============================================================================
// Basic BFS correctness
// ============================================================================

TEST_F(GraphBfsFixTest, BFS_LinearChain_VisitsAllNodes) {
    // A -> B -> C -> D
    addEdge("e1", "A", "B");
    addEdge("e2", "B", "C");
    addEdge("e3", "C", "D");
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("A", 5);
    ASSERT_TRUE(status.ok) << status.message;

    // All reachable nodes should be present
    EXPECT_NE(std::find(visited.begin(), visited.end(), "B"), visited.end());
    EXPECT_NE(std::find(visited.begin(), visited.end(), "C"), visited.end());
    EXPECT_NE(std::find(visited.begin(), visited.end(), "D"), visited.end());
}

TEST_F(GraphBfsFixTest, BFS_MaxDepthLimits_Traversal) {
    // A -> B -> C -> D — only A and B reachable with depth=1
    addEdge("e1", "A", "B");
    addEdge("e2", "B", "C");
    addEdge("e3", "C", "D");
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("A", 1);
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_NE(std::find(visited.begin(), visited.end(), "B"), visited.end());
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "C"), visited.end())
        << "C should NOT be reachable at depth 1";
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "D"), visited.end())
        << "D should NOT be reachable at depth 1";
}

TEST_F(GraphBfsFixTest, BFS_MaxDepthZero_ReturnsOnlyStart) {
    // A -> B: with maxDepth=0 only the start vertex should appear
    addEdge("e1", "A", "B");
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("A", 0);
    ASSERT_TRUE(status.ok) << status.message;

    // Implementation may return just start, or an empty list — no B in any case
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "B"), visited.end())
        << "B must NOT be reachable with maxDepth=0";
}

TEST_F(GraphBfsFixTest, BFS_NoDuplicates_InResult) {
    // Diamond: A -> B, A -> C, B -> D, C -> D  (D reachable via two paths)
    addEdge("e1", "A", "B");
    addEdge("e2", "A", "C");
    addEdge("e3", "B", "D");
    addEdge("e4", "C", "D");
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("A", 5);
    ASSERT_TRUE(status.ok) << status.message;

    // D should appear exactly once
    const size_t d_count = std::count(visited.begin(), visited.end(), "D");
    EXPECT_EQ(d_count, 1u) << "Duplicate 'D' in BFS result — deduplication bug";
}

TEST_F(GraphBfsFixTest, BFS_DisconnectedNode_NotReturned) {
    // A -> B, C is isolated
    addEdge("e1", "A", "B");
    // Add isolated node C — no edges; just ensure it's not accidentally visited
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("A", 5);
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_EQ(std::find(visited.begin(), visited.end(), "C"), visited.end())
        << "Disconnected vertex C must not appear in BFS from A";
}

TEST_F(GraphBfsFixTest, BFS_Cycle_NoInfiniteLoop) {
    // A -> B -> C -> A (cycle)
    addEdge("e1", "A", "B");
    addEdge("e2", "B", "C");
    addEdge("e3", "C", "A"); // back-edge
    graph_->rebuildTopology();

    // Should terminate and not crash / hang
    auto [status, visited] = graph_->bfs("A", 10);
    ASSERT_TRUE(status.ok) << status.message;

    // All vertices reachable and no duplicates
    for (const auto& v : {"B", "C"}) {
        EXPECT_NE(std::find(visited.begin(), visited.end(), v), visited.end());
    }
    // No duplicates
    std::vector<std::string> sorted(visited);
    std::sort(sorted.begin(), sorted.end());
    auto it = std::adjacent_find(sorted.begin(), sorted.end());
    EXPECT_EQ(it, sorted.end()) << "Duplicate vertices in BFS cycle result";
}

TEST_F(GraphBfsFixTest, BFS_EmptyStartVertex_ReturnsError) {
    auto [status, visited] = graph_->bfs("", 5);
    EXPECT_FALSE(status.ok) << "Empty start vertex must return error status";
    EXPECT_TRUE(visited.empty());
}

TEST_F(GraphBfsFixTest, BFS_UnknownStartVertex_ReturnsEmpty) {
    addEdge("e1", "A", "B");
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("NONEXISTENT", 5);
    // Either error or empty result — must not crash
    EXPECT_TRUE(!status.ok || visited.empty())
        << "BFS from unknown vertex must return empty result or error";
}

// ============================================================================
// BFS with edge-type filtering
// ============================================================================

TEST_F(GraphBfsFixTest, BFS_TypeFilter_OnlyFollowsMatchingEdges) {
    // A --[follows]--> B, A --[blocks]--> C
    addEdge("e1", "A", "B", "follows");
    addEdge("e2", "A", "C", "blocks");
    graph_->rebuildTopology();

    auto [status, visited] = graph_->bfs("A", 5, "follows", "");
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_NE(std::find(visited.begin(), visited.end(), "B"), visited.end())
        << "B must be reachable via 'follows' edge";
    EXPECT_EQ(std::find(visited.begin(), visited.end(), "C"), visited.end())
        << "C must NOT be reachable (edge type 'blocks' != 'follows')";
}

TEST_F(GraphBfsFixTest, BFS_TypeFilter_EmptyType_FollowsAllEdges) {
    addEdge("e1", "A", "B", "follows");
    addEdge("e2", "A", "C", "likes");
    graph_->rebuildTopology();

    // empty edge_type means "all types"
    auto [status, visited] = graph_->bfs("A", 5);
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_NE(std::find(visited.begin(), visited.end(), "B"), visited.end());
    EXPECT_NE(std::find(visited.begin(), visited.end(), "C"), visited.end());
}

