// Test suite for Gunrock GPU-accelerated graph processing

#include "performance/phase3/gunrock.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>

using namespace themis::performance::phase3;

class GunrockTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<GunrockProcessor>();
    }
    
    std::unique_ptr<GunrockProcessor> processor;
};

TEST_F(GunrockTest, LoadGraph) {
    // Create a simple graph: 0 -> 1 -> 2
    std::vector<std::vector<NodeID>> adj_list = {
        {1},      // 0 -> 1
        {2},      // 1 -> 2
        {}        // 2 -> nothing
    };
    
    processor->load_graph(adj_list);
    
    auto stats = processor->get_stats();
    EXPECT_EQ(stats.num_vertices, 3);
    EXPECT_EQ(stats.num_edges, 2);
}

TEST_F(GunrockTest, BFS_Simple) {
    // Create a simple graph: 0 -> 1 -> 2
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {2},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_bfs(0);
    
    ASSERT_EQ(distances.size(), 3);
    EXPECT_EQ(distances[0], 0);  // Start vertex
    EXPECT_EQ(distances[1], 1);  // Distance 1
    EXPECT_EQ(distances[2], 2);  // Distance 2
}

TEST_F(GunrockTest, BFS_Disconnected) {
    // Create a disconnected graph: 0 -> 1, 2 (isolated)
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_bfs(0);
    
    ASSERT_EQ(distances.size(), 3);
    EXPECT_EQ(distances[0], 0);
    EXPECT_EQ(distances[1], 1);
    EXPECT_EQ(distances[2], -1);  // Unreachable
}

TEST_F(GunrockTest, BFS_Cycle) {
    // Create a cycle: 0 -> 1 -> 2 -> 0
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {2},
        {0}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_bfs(0);
    
    ASSERT_EQ(distances.size(), 3);
    EXPECT_EQ(distances[0], 0);
    EXPECT_EQ(distances[1], 1);
    EXPECT_EQ(distances[2], 2);
}

TEST_F(GunrockTest, BFS_InvalidStartVertex) {
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {2},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_bfs(100);  // Invalid vertex
    
    ASSERT_EQ(distances.size(), 3);
    // All vertices should be unreachable
    for (int dist : distances) {
        EXPECT_EQ(dist, -1);
    }
}

TEST_F(GunrockTest, PageRank_Simple) {
    // Create a simple graph: 0 -> 1 -> 2 -> 1 (cycle)
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {2},
        {1}
    };
    
    processor->load_graph(adj_list);
    auto ranks = processor->gpu_pagerank(20, 0.85);
    
    ASSERT_EQ(ranks.size(), 3);
    
    // All ranks should be positive
    for (double rank : ranks) {
        EXPECT_GT(rank, 0.0);
    }
    
    // Sum of ranks should be approximately 1.0
    double sum = 0.0;
    for (double rank : ranks) {
        sum += rank;
    }
    EXPECT_NEAR(sum, 1.0, 0.01);
}

TEST_F(GunrockTest, PageRank_Star) {
    // Star graph: 0 -> {1, 2, 3}
    std::vector<std::vector<NodeID>> adj_list = {
        {1, 2, 3},
        {},
        {},
        {}
    };
    
    processor->load_graph(adj_list);
    auto ranks = processor->gpu_pagerank(20, 0.85);
    
    ASSERT_EQ(ranks.size(), 4);
    
    // Leaf nodes (1, 2, 3) should have similar ranks
    EXPECT_NEAR(ranks[1], ranks[2], 0.01);
    EXPECT_NEAR(ranks[2], ranks[3], 0.01);
    
    // Sum should be 1.0
    double sum = 0.0;
    for (double rank : ranks) {
        sum += rank;
    }
    EXPECT_NEAR(sum, 1.0, 0.01);
}

TEST_F(GunrockTest, PageRank_HighDamping) {
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {0}
    };
    
    processor->load_graph(adj_list);
    auto ranks = processor->gpu_pagerank(20, 0.95);  // High damping
    
    ASSERT_EQ(ranks.size(), 2);
    
    // Both nodes should have similar ranks (symmetric cycle)
    EXPECT_NEAR(ranks[0], ranks[1], 0.01);
    
    double sum = ranks[0] + ranks[1];
    EXPECT_NEAR(sum, 1.0, 0.01);
}

TEST_F(GunrockTest, SSSP_Simple) {
    // Create a simple path: 0 -> 1 -> 2
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {2},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_sssp(0);
    
    ASSERT_EQ(distances.size(), 3);
    EXPECT_DOUBLE_EQ(distances[0], 0.0);
    EXPECT_DOUBLE_EQ(distances[1], 1.0);
    EXPECT_DOUBLE_EQ(distances[2], 2.0);
}

TEST_F(GunrockTest, SSSP_Unreachable) {
    // Disconnected: 0 -> 1, 2 (isolated)
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_sssp(0);
    
    ASSERT_EQ(distances.size(), 3);
    EXPECT_DOUBLE_EQ(distances[0], 0.0);
    EXPECT_DOUBLE_EQ(distances[1], 1.0);
    EXPECT_TRUE(std::isinf(distances[2]));  // Unreachable
}

TEST_F(GunrockTest, SSSP_MultiPath) {
    // Multiple paths: 0 -> {1, 2}, 1 -> 3, 2 -> 3
    std::vector<std::vector<NodeID>> adj_list = {
        {1, 2},
        {3},
        {3},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_sssp(0);
    
    ASSERT_EQ(distances.size(), 4);
    EXPECT_DOUBLE_EQ(distances[0], 0.0);
    EXPECT_DOUBLE_EQ(distances[1], 1.0);
    EXPECT_DOUBLE_EQ(distances[2], 1.0);
    EXPECT_DOUBLE_EQ(distances[3], 2.0);  // Shortest path through 1 or 2
}

TEST_F(GunrockTest, SSSP_InvalidStartVertex) {
    std::vector<std::vector<NodeID>> adj_list = {
        {1},
        {2},
        {}
    };
    
    processor->load_graph(adj_list);
    auto distances = processor->gpu_sssp(100);  // Invalid
    
    ASSERT_EQ(distances.size(), 3);
    // All should be infinite
    for (double dist : distances) {
        EXPECT_TRUE(std::isinf(dist));
    }
}

TEST_F(GunrockTest, EmptyGraph) {
    std::vector<std::vector<NodeID>> adj_list;
    
    processor->load_graph(adj_list);
    
    auto stats = processor->get_stats();
    EXPECT_EQ(stats.num_vertices, 0);
    EXPECT_EQ(stats.num_edges, 0);
}

TEST_F(GunrockTest, LargeGraph) {
    // Create a larger graph: chain of 100 nodes
    std::vector<std::vector<NodeID>> adj_list(100);
    for (size_t i = 0; i < 99; ++i) {
        adj_list[i].push_back(i + 1);
    }
    
    processor->load_graph(adj_list);
    
    auto stats = processor->get_stats();
    EXPECT_EQ(stats.num_vertices, 100);
    EXPECT_EQ(stats.num_edges, 99);
    
    auto distances = processor->gpu_bfs(0);
    ASSERT_EQ(distances.size(), 100);
    EXPECT_EQ(distances[0], 0);
    EXPECT_EQ(distances[50], 50);
    EXPECT_EQ(distances[99], 99);
}

TEST_F(GunrockTest, Stats) {
    std::vector<std::vector<NodeID>> adj_list = {
        {1, 2},
        {2},
        {0}
    };
    
    processor->load_graph(adj_list);
    
    auto stats = processor->get_stats();
    EXPECT_EQ(stats.num_vertices, 3);
    EXPECT_EQ(stats.num_edges, 4);
    EXPECT_FALSE(stats.gpu_available);  // CPU fallback
    EXPECT_EQ(stats.gpu_memory_mb, 0);
}

// ===========================================================================
// GAP-021 — BFS frontier size cap (CWE-400)
// ===========================================================================

// GAP-021-01: A graph with 2,000,000 nodes in a long chain must not exhaust
// memory — gpu_bfs() must return before visiting all nodes when the frontier
// cap (1,000,000 nodes) is reached.
TEST_F(GunrockTest, GAP021_LargeChainGraph_FrontierCapRespected) {
    // Build a 2M-node chain: 0→1→2→…→N-1
    constexpr int N = 2'000'000;
    std::vector<std::vector<NodeID>> adj(N);
    for (int i = 0; i < N - 1; ++i) {
        adj[i].push_back(i + 1);
    }
    processor->load_graph(adj);

    auto distances = processor->gpu_bfs(0);
    ASSERT_EQ(static_cast<int>(distances.size()), N);

    // Node 0 must always be at distance 0.
    EXPECT_EQ(distances[0], 0);

    // Nodes beyond the cap (1M) should be -1 (unreachable / truncated).
    // We check the last node: on a capped run it must be -1.
    EXPECT_EQ(distances[N - 1], -1)
        << "BFS should have been truncated; last node must remain unreachable";
}

// GAP-021-02: Small graphs (< cap) must still return correct distances.
TEST_F(GunrockTest, GAP021_SmallGraph_FullTraversalUnaffected) {
    std::vector<std::vector<NodeID>> adj = {
        {1, 2},  // 0 → 1, 2
        {3},     // 1 → 3
        {3},     // 2 → 3
        {}       // 3 (leaf)
    };
    processor->load_graph(adj);

    auto distances = processor->gpu_bfs(0);
    ASSERT_EQ(distances.size(), 4u);
    EXPECT_EQ(distances[0], 0);
    EXPECT_EQ(distances[1], 1);
    EXPECT_EQ(distances[2], 1);
    EXPECT_EQ(distances[3], 2);
}
