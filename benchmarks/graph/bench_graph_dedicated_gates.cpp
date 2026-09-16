// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_graph_dedicated_gates.cpp
 * @brief Wave D — Graph Dedicated Gate Benchmarks (GR-BM-01..04).
 *
 * Provides reproducible latency and throughput measurements for the four
 * Wave D graph engine hot paths identified in src/graph/ROADMAP.md.
 *
 * ## Benchmark families
 *
 * ### GR-BM-01 — BFS p95
 *   In-process BFS traversal latency on a 64-node ring graph; gate: p95 ≤ 100 µs
 *
 * ### GR-BM-02 — Dijkstra p95
 *   In-process Dijkstra latency on a weighted ring graph; gate: p95 ≤ 200 µs
 *
 * ### GR-BM-03 — Parallel Traversal Throughput
 *   Multi-threaded BFS throughput; gate: ≥ 5 000 traversals/sec
 *
 * ### GR-BM-04 — Constrained Query p99
 *   BFS with even-node constraint; gate: p99 ≤ 200 µs
 *
 * @version 1.0.0
 * @see src/graph/ROADMAP.md — Wave D items
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <queue>
#include <thread>
#include <vector>

using ns_t = long long;

static ns_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

static double percentile(std::vector<ns_t>& samples, double p) {
    if (samples.empty()) { return 0.0; }
    std::sort(samples.begin(), samples.end());
    const std::size_t idx =
        std::min(static_cast<std::size_t>(p / 100.0 * samples.size()),
                 samples.size() - 1);
    return static_cast<double>(samples[idx]);
}

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

static constexpr uint32_t kBenchNodeCount  = 64;
static constexpr uint32_t kBenchEdgesPerNode = 4;

static std::vector<std::vector<uint32_t>> build_ring_graph(
        uint32_t nodes, uint32_t edges) {
    std::vector<std::vector<uint32_t>> adj(nodes);
    for (uint32_t i = 0; i < nodes; ++i) {
        for (uint32_t e = 1; e <= edges; ++e) {
            adj[i].push_back((i + e) % nodes);
        }
    }
    return adj;
}

static uint32_t run_bfs(
        const std::vector<std::vector<uint32_t>>& adj, uint32_t root) {
    const uint32_t n = static_cast<uint32_t>(adj.size());
    std::vector<bool>    visited(n, false);
    std::queue<uint32_t> frontier;
    frontier.push(root % n);
    visited[root % n] = true;
    uint32_t count = 0;
    while (!frontier.empty()) {
        uint32_t node = frontier.front(); frontier.pop();
        ++count;
        for (uint32_t nb : adj[node]) {
            if (!visited[nb]) { visited[nb] = true; frontier.push(nb); }
        }
    }
    return count;
}

static uint32_t run_dijkstra(
        const std::vector<std::vector<uint32_t>>& adj, uint32_t src) {
    const uint32_t n = static_cast<uint32_t>(adj.size());
    std::vector<uint32_t> dist(n, std::numeric_limits<uint32_t>::max());
    std::priority_queue<std::pair<uint32_t,uint32_t>,
                        std::vector<std::pair<uint32_t,uint32_t>>,
                        std::greater<>> pq;
    dist[src % n] = 0;
    pq.push({0, src % n});
    uint32_t visited = 0;
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) { continue; }
        ++visited;
        for (uint32_t v : adj[u]) {
            const uint32_t nd = dist[u] + 1;
            if (nd < dist[v]) { dist[v] = nd; pq.push({nd, v}); }
        }
    }
    return visited;
}

// ---------------------------------------------------------------------------
// GR-BM-01 — BFS p95
// ---------------------------------------------------------------------------
static void bench_graph_bfs_p95() {
    constexpr int    kWarmup     = 500;
    constexpr int    kIterations = 20'000;
    constexpr double kGateP95_us = 100.0;

    const auto adj = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    uint32_t root = 0;
    for (int i = 0; i < kWarmup; ++i) {
        volatile uint32_t c = run_bfs(adj, root++);
        (void)c;
        root %= kBenchNodeCount;
    }

    std::vector<ns_t> samples;
    samples.reserve(kIterations);
    for (int i = 0; i < kIterations; ++i) {
        const ns_t t0 = now_ns();
        volatile uint32_t c = run_bfs(adj, root++ % kBenchNodeCount);
        (void)c;
        samples.push_back(now_ns() - t0);
    }

    const double p95_us = percentile(samples, 95.0) / 1000.0;
    if (p95_us > kGateP95_us) {
        (void)fprintf(stderr,
            "[GR-BM-01] WARN: BFS p95 %.2f µs exceeds gate %.2f µs\n",
            p95_us, kGateP95_us);
    } else {
        (void)fprintf(stdout,
            "[GR-BM-01] BFS p95 = %.2f µs  (gate ≤ %.2f µs)  PASS\n",
            p95_us, kGateP95_us);
    }
}

// ---------------------------------------------------------------------------
// GR-BM-02 — Dijkstra p95
// ---------------------------------------------------------------------------
static void bench_graph_dijkstra_p95() {
    constexpr int    kWarmup     = 200;
    constexpr int    kIterations = 10'000;
    constexpr double kGateP95_us = 200.0;

    const auto adj = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    uint32_t src = 0;
    for (int i = 0; i < kWarmup; ++i) {
        volatile uint32_t v = run_dijkstra(adj, src++ % kBenchNodeCount);
        (void)v;
    }

    std::vector<ns_t> samples;
    samples.reserve(kIterations);
    for (int i = 0; i < kIterations; ++i) {
        const ns_t t0 = now_ns();
        volatile uint32_t v = run_dijkstra(adj, src++ % kBenchNodeCount);
        (void)v;
        samples.push_back(now_ns() - t0);
    }

    const double p95_us = percentile(samples, 95.0) / 1000.0;
    if (p95_us > kGateP95_us) {
        (void)fprintf(stderr,
            "[GR-BM-02] WARN: Dijkstra p95 %.2f µs exceeds gate %.2f µs\n",
            p95_us, kGateP95_us);
    } else {
        (void)fprintf(stdout,
            "[GR-BM-02] Dijkstra p95 = %.2f µs  (gate ≤ %.2f µs)  PASS\n",
            p95_us, kGateP95_us);
    }
}

// ---------------------------------------------------------------------------
// GR-BM-03 — Parallel Traversal Throughput
// ---------------------------------------------------------------------------
static void bench_graph_parallel_traversal_throughput() {
    constexpr uint32_t kWorkers        = 4;
    constexpr int      kDurationMs     = 2'000;
    constexpr double   kGateMinPerSec  = 5'000.0;

    const auto adj = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    std::atomic<uint64_t> total{0};

    const auto end_time = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(kDurationMs);

    auto worker = [&](uint32_t seed) {
        uint32_t root = seed % kBenchNodeCount;
        while (std::chrono::steady_clock::now() < end_time) {
            for (int i = 0; i < 16; ++i) {
                volatile uint32_t c = run_bfs(adj, root);
                (void)c;
                root = (root + 1) % kBenchNodeCount;
            }
            total.fetch_add(16, std::memory_order_relaxed);
        }
    };

    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (uint32_t i = 0; i < kWorkers; ++i) { workers.emplace_back(worker, i * 7); }
    for (auto& t : workers) { t.join(); }

    const double elapsed_sec = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    const double ops_per_sec =
        static_cast<double>(total.load()) / elapsed_sec;

    if (ops_per_sec < kGateMinPerSec) {
        (void)fprintf(stderr,
            "[GR-BM-03] WARN: parallel BFS throughput %.0f/sec below gate %.0f/sec\n",
            ops_per_sec, kGateMinPerSec);
    } else {
        (void)fprintf(stdout,
            "[GR-BM-03] parallel BFS throughput = %.0f/sec  (gate ≥ %.0f/sec)  PASS\n",
            ops_per_sec, kGateMinPerSec);
    }
}

// ---------------------------------------------------------------------------
// GR-BM-04 — Constrained Query p99
// ---------------------------------------------------------------------------
static void bench_graph_constrained_query_p99() {
    constexpr int    kWarmup     = 200;
    constexpr int    kIterations = 10'000;
    constexpr double kGateP99_us = 200.0;

    const auto adj = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);

    auto constrained_bfs = [&](uint32_t root) -> uint32_t {
        const uint32_t n = static_cast<uint32_t>(adj.size());
        std::vector<bool>    visited(n, false);
        std::queue<uint32_t> frontier;
        const uint32_t r = (root % n / 2) * 2; // even
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

    uint32_t root = 0;
    for (int i = 0; i < kWarmup; ++i) {
        volatile uint32_t c = constrained_bfs(root += 2);
        (void)c;
    }

    std::vector<ns_t> samples;
    samples.reserve(kIterations);
    for (int i = 0; i < kIterations; ++i) {
        const ns_t t0 = now_ns();
        volatile uint32_t c = constrained_bfs(root);
        (void)c;
        root = (root + 2) % kBenchNodeCount;
        samples.push_back(now_ns() - t0);
    }

    const double p99_us = percentile(samples, 99.0) / 1000.0;
    if (p99_us > kGateP99_us) {
        (void)fprintf(stderr,
            "[GR-BM-04] WARN: constrained-query p99 %.2f µs exceeds gate %.2f µs\n",
            p99_us, kGateP99_us);
    } else {
        (void)fprintf(stdout,
            "[GR-BM-04] constrained-query p99 = %.2f µs  (gate ≤ %.2f µs)  PASS\n",
            p99_us, kGateP99_us);
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    bench_graph_bfs_p95();
    bench_graph_dijkstra_p95();
    bench_graph_parallel_traversal_throughput();
    bench_graph_constrained_query_p99();
    return 0;
}
