// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_graph_dedicated_gates.cpp
 * @brief Google Benchmark cases for dedicated graph traversal gates.
 *
 * The measured algorithms operate on a deterministic in-process fixture;
 * they are microbenchmarks, not measurements of the production graph engine.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

using ns_t = int64_t;

constexpr uint32_t kBenchNodeCount = 64;
constexpr uint32_t kBenchEdgesPerNode = 4;
constexpr uint32_t kParallelWorkers = 4;

std::vector<std::vector<uint32_t>> build_ring_graph(
    uint32_t nodes, uint32_t edges) {
    std::vector<std::vector<uint32_t>> adjacency(nodes);
    for (uint32_t node = 0; node < nodes; ++node) {
        for (uint32_t edge = 1; edge <= edges; ++edge) {
            adjacency[node].push_back((node + edge) % nodes);
        }
    }
    return adjacency;
}

uint32_t run_bfs(
    const std::vector<std::vector<uint32_t>>& adjacency, uint32_t root) {
    const uint32_t node_count = static_cast<uint32_t>(adjacency.size());
    std::vector<bool> visited(node_count, false);
    std::queue<uint32_t> frontier;
    frontier.push(root % node_count);
    visited[root % node_count] = true;

    uint32_t count = 0;
    while (!frontier.empty()) {
        const uint32_t node = frontier.front();
        frontier.pop();
        ++count;
        for (const uint32_t neighbor : adjacency[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                frontier.push(neighbor);
            }
        }
    }
    return count;
}

uint32_t run_dijkstra(
    const std::vector<std::vector<uint32_t>>& adjacency, uint32_t source) {
    const uint32_t node_count = static_cast<uint32_t>(adjacency.size());
    std::vector<uint32_t> distance(
        node_count, std::numeric_limits<uint32_t>::max());
    using QueueEntry = std::pair<uint32_t, uint32_t>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> queue;
    distance[source % node_count] = 0;
    queue.push({0, source % node_count});

    uint32_t visited = 0;
    while (!queue.empty()) {
        const auto [current_distance, node] = queue.top();
        queue.pop();
        if (current_distance > distance[node]) {
            continue;
        }
        ++visited;
        for (const uint32_t neighbor : adjacency[node]) {
            const uint32_t next_distance = distance[node] + 1;
            if (next_distance < distance[neighbor]) {
                distance[neighbor] = next_distance;
                queue.push({next_distance, neighbor});
            }
        }
    }
    return visited;
}

template <typename Operation>
void run_latency_benchmark(
    benchmark::State& state,
    Operation&& operation,
    double percentile_rank,
    const char* percentile_counter) {
    std::vector<ns_t> samples;
    samples.reserve(static_cast<std::size_t>(state.max_iterations));

    for (auto _ : state) {
        const auto start = std::chrono::steady_clock::now();
        const auto result = operation();
        const auto elapsed = std::chrono::steady_clock::now() - start;
        benchmark::DoNotOptimize(result);

        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
        samples.push_back(elapsed_ns);
        state.SetIterationTime(static_cast<double>(elapsed_ns) * 1e-9);
    }

    std::sort(samples.begin(), samples.end());
    const auto index = std::min(
        static_cast<std::size_t>(percentile_rank / 100.0 * samples.size()),
        samples.size() - 1);
    state.counters[percentile_counter] = static_cast<double>(samples[index]);
    state.SetItemsProcessed(state.iterations());
}

void BM_GraphBFS_P95(benchmark::State& state) {
    const auto adjacency = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    uint32_t root = 0;
    run_latency_benchmark(state, [&] {
        const auto current_root = root++ % kBenchNodeCount;
        return run_bfs(adjacency, current_root);
    }, 95.0, "p95_real_time");
}
BENCHMARK(BM_GraphBFS_P95)->Unit(benchmark::kNanosecond)->UseManualTime();

void BM_GraphDijkstra_P95(benchmark::State& state) {
    const auto adjacency = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    uint32_t source = 0;
    run_latency_benchmark(state, [&] {
        const auto current_source = source++ % kBenchNodeCount;
        return run_dijkstra(adjacency, current_source);
    }, 95.0, "p95_real_time");
}
BENCHMARK(BM_GraphDijkstra_P95)->Unit(benchmark::kNanosecond)->UseManualTime();

void BM_GraphConstrainedBFS_P99(benchmark::State& state) {
    const auto adjacency = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    uint32_t root = 0;
    run_latency_benchmark(state, [&] {
        const uint32_t node_count = static_cast<uint32_t>(adjacency.size());
        const uint32_t start_node = (root++ % node_count / 2) * 2;
        std::vector<bool> visited(node_count, false);
        std::queue<uint32_t> frontier;
        frontier.push(start_node);
        visited[start_node] = true;

        uint32_t count = 0;
        while (!frontier.empty()) {
            const uint32_t node = frontier.front();
            frontier.pop();
            ++count;
            for (const uint32_t neighbor : adjacency[node]) {
                if (!visited[neighbor] && neighbor % 2 == 0) {
                    visited[neighbor] = true;
                    frontier.push(neighbor);
                }
            }
        }
        return count;
    }, 99.0, "p99_real_time");
}
BENCHMARK(BM_GraphConstrainedBFS_P99)
    ->Unit(benchmark::kNanosecond)->UseManualTime();

void BM_GraphParallelBFS_Throughput(benchmark::State& state) {
    const auto adjacency = build_ring_graph(kBenchNodeCount, kBenchEdgesPerNode);
    uint64_t total_traversals = 0;

    for (auto _ : state) {
        const auto end_time =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        std::atomic<uint64_t> iteration_traversals{0};
        std::vector<std::thread> workers;
        workers.reserve(kParallelWorkers);

        for (uint32_t worker = 0; worker < kParallelWorkers; ++worker) {
            workers.emplace_back([&, worker] {
                uint32_t root = worker % kBenchNodeCount;
                uint64_t traversals = 0;
                while (std::chrono::steady_clock::now() < end_time) {
                    benchmark::DoNotOptimize(run_bfs(adjacency, root));
                    root = (root + 1) % kBenchNodeCount;
                    ++traversals;
                }
                iteration_traversals.fetch_add(traversals, std::memory_order_relaxed);
            });
        }
        for (auto& worker : workers) {
            worker.join();
        }
        total_traversals += iteration_traversals.load(std::memory_order_relaxed);
    }

    state.SetItemsProcessed(static_cast<int64_t>(total_traversals));
    state.counters["workers"] = kParallelWorkers;
}
BENCHMARK(BM_GraphParallelBFS_Throughput)->UseRealTime();

}  // namespace

BENCHMARK_MAIN();
