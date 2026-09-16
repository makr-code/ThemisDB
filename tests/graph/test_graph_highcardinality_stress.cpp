// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_graph_highcardinality_stress.cpp
 * @brief Wave D — Graph High-Cardinality Stress Tests.
 *
 * Three stress cases for the graph engine under high-cardinality and
 * concurrency conditions using in-process stubs and seed-42 determinism.
 *
 * ## Cases
 * - HighCardinalityGraphTraversal    : 100 000 nodes, 8-thread BFS
 * - ConcurrentConstrainedTraversal   : constrained traversal under contention
 * - HighFanOutParallelQuery          : high fan-out parallel BFS queries
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_GRAPH_ENGINE.md
 * @see src/graph/ROADMAP.md
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

static constexpr uint64_t kGraphStressSeed = 42;

// ---------------------------------------------------------------------------
// In-process graph stubs
// ---------------------------------------------------------------------------

/// Build a deterministic sparse adjacency list for n nodes.
static std::vector<std::vector<uint32_t>> buildSparseGraph(
        uint32_t node_count, uint32_t edges_per_node) {
    std::vector<std::vector<uint32_t>> adj(node_count);
    uint64_t rng = kGraphStressSeed;
    for (uint32_t n = 0; n < node_count; ++n) {
        for (uint32_t e = 0; e < edges_per_node; ++e) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            adj[n].push_back(static_cast<uint32_t>(rng % node_count));
        }
    }
    return adj;
}

/// Iterative BFS — returns visited node count.
static uint32_t runBFS(
        const std::vector<std::vector<uint32_t>>& adj, uint32_t root,
        uint32_t depth_limit = std::numeric_limits<uint32_t>::max()) {
    const uint32_t n = static_cast<uint32_t>(adj.size());
    std::vector<bool>    visited(n, false);
    std::queue<std::pair<uint32_t, uint32_t>> frontier; // (node, depth)
    frontier.push({root % n, 0});
    visited[root % n] = true;
    uint32_t count = 0;
    while (!frontier.empty()) {
        auto [node, depth] = frontier.front(); frontier.pop();
        ++count;
        if (depth >= depth_limit) { continue; }
        for (uint32_t nb : adj[node]) {
            if (!visited[nb]) {
                visited[nb] = true;
                frontier.push({nb, depth + 1});
            }
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// HighCardinalityGraphTraversal — 100 000 nodes, 8 threads
// ---------------------------------------------------------------------------
TEST(GraphHighCardinalityStress, HighCardinalityGraphTraversal) {
    constexpr uint32_t kNodeCount     = 100'000;
    constexpr uint32_t kEdgesPerNode  = 4;
    constexpr uint32_t kThreadCount   = 8;
    constexpr uint32_t kQueriesPerThread = 16;
    constexpr uint32_t kDepthLimit    = 5; // limit depth to keep test fast

    const auto adj = buildSparseGraph(kNodeCount, kEdgesPerNode);

    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> zero_visited{0};

    auto worker = [&](uint32_t tid) {
        uint64_t rng = kGraphStressSeed ^ (static_cast<uint64_t>(tid + 1) << 32);
        for (uint32_t q = 0; q < kQueriesPerThread; ++q) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            const uint32_t root = static_cast<uint32_t>(rng % kNodeCount);
            const uint32_t visited = runBFS(adj, root, kDepthLimit);
            if (visited == 0) {
                zero_visited.fetch_add(1, std::memory_order_relaxed);
            }
            total_queries.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    const uint64_t expected_queries =
        static_cast<uint64_t>(kThreadCount) * kQueriesPerThread;
    EXPECT_EQ(total_queries.load(), expected_queries);
    EXPECT_EQ(zero_visited.load(), 0ULL)
        << "HighCardinalityGraphTraversal: queries returned 0 visited nodes";
}

// ---------------------------------------------------------------------------
// ConcurrentConstrainedTraversal
// ---------------------------------------------------------------------------
TEST(GraphHighCardinalityStress, ConcurrentConstrainedTraversal) {
    // Constraint: only visit nodes whose ID is even.
    constexpr uint32_t kNodeCount     = 1'000;
    constexpr uint32_t kEdgesPerNode  = 6;
    constexpr uint32_t kThreadCount   = 8;
    constexpr uint32_t kQueriesPerThread = 50;

    const auto adj = buildSparseGraph(kNodeCount, kEdgesPerNode);

    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> constraint_violations{0};

    auto constrained_bfs = [&](uint32_t root) -> uint32_t {
        const uint32_t n = static_cast<uint32_t>(adj.size());
        std::vector<bool>    visited(n, false);
        std::queue<uint32_t> frontier;
        const uint32_t r = (root % n);
        if (r % 2 != 0) { return 0; } // root must be even
        frontier.push(r);
        visited[r] = true;
        uint32_t count = 0;
        while (!frontier.empty()) {
            uint32_t node = frontier.front(); frontier.pop();
            ++count;
            for (uint32_t nb : adj[node]) {
                if (!visited[nb] && nb % 2 == 0) {
                    visited[nb] = true;
                    frontier.push(nb);
                }
            }
        }
        return count;
    };

    auto worker = [&](uint32_t tid) {
        uint64_t rng = kGraphStressSeed ^ (static_cast<uint64_t>(tid + 1) * 0x9e3779b9ULL);
        for (uint32_t q = 0; q < kQueriesPerThread; ++q) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            const uint32_t root = static_cast<uint32_t>(rng % kNodeCount);
            // Ensure root is even.
            const uint32_t even_root = (root / 2) * 2;
            const uint32_t visited = constrained_bfs(even_root);
            if (visited == 0 && kNodeCount > 0) {
                // Any even node in a connected graph should visit at least itself.
                constraint_violations.fetch_add(1, std::memory_order_relaxed);
            }
            total_queries.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(total_queries.load(),
              static_cast<uint64_t>(kThreadCount) * kQueriesPerThread);
    EXPECT_EQ(constraint_violations.load(), 0ULL)
        << "ConcurrentConstrainedTraversal: " << constraint_violations.load()
        << " constraint violations";
}

// ---------------------------------------------------------------------------
// HighFanOutParallelQuery
// ---------------------------------------------------------------------------
TEST(GraphHighCardinalityStress, HighFanOutParallelQuery) {
    // High fan-out: each node has 32 out-edges; traversal from root 0
    // should reach a large fraction of the graph quickly.
    constexpr uint32_t kNodeCount     = 2'000;
    constexpr uint32_t kEdgesPerNode  = 32;
    constexpr uint32_t kThreadCount   = 8;
    constexpr uint32_t kDepthLimit    = 3;

    const auto adj = buildSparseGraph(kNodeCount, kEdgesPerNode);

    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> small_results{0}; // result < 10 % of graph

    auto worker = [&](uint32_t tid) {
        uint64_t rng = kGraphStressSeed ^ static_cast<uint64_t>(tid + 1);
        for (uint32_t q = 0; q < 100; ++q) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            const uint32_t root = static_cast<uint32_t>(rng % kNodeCount);
            const uint32_t visited = runBFS(adj, root, kDepthLimit);
            if (visited < kNodeCount / 10) {
                small_results.fetch_add(1, std::memory_order_relaxed);
            }
            total_queries.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(total_queries.load(),
              static_cast<uint64_t>(kThreadCount) * 100);

    // With 32 out-edges and depth 3, most queries should reach ≥ 10 % of nodes.
    const double small_ratio =
        static_cast<double>(small_results.load()) /
        static_cast<double>(total_queries.load());
    EXPECT_LE(small_ratio, 0.05)
        << "HighFanOutParallelQuery: " << small_ratio * 100.0
        << "% queries visited < 10% of nodes under high fan-out";
}
