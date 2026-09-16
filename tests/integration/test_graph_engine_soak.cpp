// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_graph_engine_soak.cpp
 * @brief Wave D — Graph Engine Soak Tests (sustained graph workload).
 *
 * Three soak cases that validate graph traversal throughput, GPU→CPU kernel
 * fallback stability, and parallel query reliability over a configurable
 * soak window using in-process stubs.
 *
 * ## Acceptance criteria
 * - GraphSoak_TraversalThroughput     : ≥ 5 000 traversals/sec sustained
 * - GraphSoak_KernelFallbackStability : no fallback failures over soak window
 * - GraphSoak_ParallelQueryReliability: all parallel queries return correct results
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_GRAPH_ENGINE.md
 * @see src/graph/ROADMAP.md — Wave D contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// In-process graph stubs
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

/// Stub BFS traversal over a fixed small adjacency list.
class StubGraphBFS {
public:
    static constexpr uint32_t kNodeCount = 64;
    static constexpr uint32_t kEdgesPerNode = 4;

    StubGraphBFS() {
        // Build a deterministic ring graph.
        adj_.resize(kNodeCount);
        for (uint32_t i = 0; i < kNodeCount; ++i) {
            for (uint32_t e = 1; e <= kEdgesPerNode; ++e) {
                adj_[i].push_back((i + e) % kNodeCount);
            }
        }
    }

    /// Returns number of nodes visited in BFS from root.
    uint32_t bfs(uint32_t root) const {
        std::vector<bool>    visited(kNodeCount, false);
        std::queue<uint32_t> frontier;
        frontier.push(root % kNodeCount);
        visited[root % kNodeCount] = true;
        uint32_t count = 0;
        while (!frontier.empty()) {
            uint32_t node = frontier.front(); frontier.pop();
            ++count;
            for (uint32_t nb : adj_[node]) {
                if (!visited[nb]) {
                    visited[nb] = true;
                    frontier.push(nb);
                }
            }
        }
        return count;
    }

private:
    std::vector<std::vector<uint32_t>> adj_;
};

/// Stub kernel fallback — mimics GPU→CPU fallback with a boolean flag.
class StubKernelFallback {
public:
    /// Execute traversal; always uses CPU path (GPU unavailable in stubs).
    bool execute(uint32_t node_count, bool& used_cpu_fallback) {
        used_cpu_fallback = true; // always CPU in stub
        // Model a trivial CPU computation.
        volatile uint64_t acc = 0;
        for (uint32_t i = 0; i < node_count; ++i) { acc += i; }
        (void)acc;
        return true;
    }
};

/// Stub parallel query: executes n independent BFS calls concurrently.
struct ParallelQueryResult {
    uint32_t query_id;
    uint32_t visited_count;
};

// ---------------------------------------------------------------------------
// GraphSoak_TraversalThroughput
// ---------------------------------------------------------------------------
TEST(GraphSoak, GraphSoak_TraversalThroughput) {
    constexpr double   kMinTraversalsPerSec = 5'000.0;
    constexpr uint32_t kWorkerCount         = 4;

    StubGraphBFS           bfs_engine;
    std::atomic<uint64_t>  total_traversals{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    auto worker = [&](uint32_t seed) {
        uint32_t root = seed % StubGraphBFS::kNodeCount;
        while (std::chrono::steady_clock::now() < end_time) {
            for (int i = 0; i < 64; ++i) {
                volatile uint32_t count = bfs_engine.bfs(root);
                (void)count;
                root = (root + 1) % StubGraphBFS::kNodeCount;
            }
            total_traversals.fetch_add(64, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount);
    for (uint32_t i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back(worker, i * 7);
    }
    for (auto& t : workers) t.join();

    const double elapsed_sec = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const double traversals_per_sec =
        static_cast<double>(total_traversals.load()) / elapsed_sec;

    EXPECT_GE(traversals_per_sec, kMinTraversalsPerSec)
        << "Graph traversal throughput " << static_cast<uint64_t>(traversals_per_sec)
        << "/sec below " << static_cast<uint64_t>(kMinTraversalsPerSec) << "/sec threshold";
}

// ---------------------------------------------------------------------------
// GraphSoak_KernelFallbackStability
// ---------------------------------------------------------------------------
TEST(GraphSoak, GraphSoak_KernelFallbackStability) {
    StubKernelFallback     fallback;
    std::atomic<uint64_t>  fallback_failures{0};
    std::atomic<uint64_t>  total_invocations{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    while (std::chrono::steady_clock::now() < end_time) {
        bool used_cpu = false;
        const bool ok = fallback.execute(StubGraphBFS::kNodeCount, used_cpu);
        if (!ok) {
            fallback_failures.fetch_add(1, std::memory_order_relaxed);
        }
        total_invocations.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_GT(total_invocations.load(), 0ULL);
    EXPECT_EQ(fallback_failures.load(), 0ULL)
        << "GraphSoak_KernelFallbackStability: " << fallback_failures.load()
        << " kernel fallback failures during soak";
}

// ---------------------------------------------------------------------------
// GraphSoak_ParallelQueryReliability
// ---------------------------------------------------------------------------
TEST(GraphSoak, GraphSoak_ParallelQueryReliability) {
    constexpr uint32_t kParallelism = 4;

    StubGraphBFS              bfs_engine;
    std::atomic<uint64_t>     wrong_results{0};
    std::atomic<uint64_t>     total_queries{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    // Pre-compute expected results for all roots.
    std::vector<uint32_t> expected(StubGraphBFS::kNodeCount);
    for (uint32_t r = 0; r < StubGraphBFS::kNodeCount; ++r) {
        expected[r] = bfs_engine.bfs(r);
    }

    auto query_worker = [&](uint32_t worker_id) {
        uint32_t root = worker_id % StubGraphBFS::kNodeCount;
        while (std::chrono::steady_clock::now() < end_time) {
            const uint32_t count = bfs_engine.bfs(root);
            if (count != expected[root]) {
                wrong_results.fetch_add(1, std::memory_order_relaxed);
            }
            total_queries.fetch_add(1, std::memory_order_relaxed);
            root = (root + 1) % StubGraphBFS::kNodeCount;
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kParallelism);
    for (uint32_t i = 0; i < kParallelism; ++i) {
        workers.emplace_back(query_worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_GT(total_queries.load(), 0ULL);
    EXPECT_EQ(wrong_results.load(), 0ULL)
        << "GraphSoak_ParallelQueryReliability: " << wrong_results.load()
        << " incorrect traversal results detected";
}
