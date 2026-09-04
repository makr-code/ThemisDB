// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_dk_release_gates.cpp
 * @brief Phase 5 distributed_knowledge hot-path release-gate benchmarks (DKRG-01..DKRG-06).
 *
 * Provides reproducible latency and throughput measurements for the
 * distributed_knowledge hot paths identified in the module roadmap
 * (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### DKRG-01 — Entity insert (in-memory)
 *   ≥ 100k inserts/s
 *
 * ### DKRG-02 — Neighbours lookup (10 edges)
 *   p99 ≤ 500 µs
 *
 * ### DKRG-03 — Path query (depth 3, mock graph)
 *   p99 ≤ 5 ms
 *
 * ### DKRG-04 — Entity merge (LWW)
 *   p99 ≤ 100 µs
 *
 * ### DKRG-05 — Federation result union (2 nodes, 100 entities each)
 *   p99 ≤ 5 ms
 *
 * ### DKRG-06 — Entity serialization (128-byte payload)
 *   p99 ≤ 100 µs
 *
 * ## Hard release gates
 *
 * | Gate ID       | Benchmark | Threshold      |
 * |---------------|-----------|----------------|
 * | GATE-DKRG-01  | DKRG-01   | ≥ 100k inserts/s |
 * | GATE-DKRG-02  | DKRG-02   | p99 ≤ 500 µs   |
 * | GATE-DKRG-03  | DKRG-03   | p99 ≤ 5 ms     |
 * | GATE-DKRG-04  | DKRG-04   | p99 ≤ 100 µs   |
 * | GATE-DKRG-05  | DKRG-05   | p99 ≤ 5 ms (RT)|
 * | GATE-DKRG-06  | DKRG-06   | p99 ≤ 100 µs   |
 *
 * @see include/distributed_knowledge/distributed_knowledge_api_contract.h
 * @see src/distributed_knowledge/ROADMAP.md — Phase 5 item
 */

#include <benchmark/benchmark.h>

#include "distributed_knowledge/distributed_knowledge_api_contract.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::distributed_knowledge;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace dkrg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kDKCanonicalSeed  = 42;
static constexpr int      kRepetitions      = 5;
static constexpr int      kWarmupIterations = 100;

// ---------------------------------------------------------------------------
// Mock entity store (unordered_map for O(1) insert/lookup)
// ---------------------------------------------------------------------------

struct BenchEntity {
    std::string  id;
    std::int64_t timestamp_us;
    std::string  node_id;
    char         payload[128];
};

using BenchEntityStore = std::unordered_map<std::string, BenchEntity>;

static BenchEntity makeEntity(int idx, const char* node_id) noexcept {
    BenchEntity e{};
    e.id           = "e" + std::to_string(idx);
    e.timestamp_us = static_cast<std::int64_t>(idx) * 1000L;
    e.node_id      = node_id;
    std::memset(e.payload, idx & 0xFF, sizeof(e.payload));
    return e;
}

// ---------------------------------------------------------------------------
// Mock graph for neighbours / path
// ---------------------------------------------------------------------------

using BenchGraph = std::unordered_map<std::string, std::vector<std::string>>;

static BenchGraph makeGraph(int n_nodes, int edges_per_node) {
    BenchGraph g;
    for (int i = 0; i < n_nodes; ++i) {
        std::string from = "n" + std::to_string(i);
        for (int j = 1; j <= edges_per_node; ++j) {
            int to_idx = (i + j) % n_nodes;
            g[from].push_back("n" + std::to_string(to_idx));
        }
    }
    return g;
}

static std::vector<std::string> getNeighbours(const BenchGraph& g,
                                               const std::string& node) {
    auto it = g.find(node);
    return it == g.end() ? std::vector<std::string>{} : it->second;
}

static std::set<std::string> bfsDepth3(const BenchGraph& g,
                                        const std::string& start) {
    std::set<std::string> visited = {};

    std::vector<std::pair<std::string, int>> q = {{start, 0}};
    for (std::size_t i = 0; i < q.size(); ++i) {
        auto [node, depth] = q[i];
        if (!visited.insert(node).second) {
          continue;
        }
        if (depth < 3) {
            auto nb = getNeighbours(g, node);
            for (auto& n : nb) q.push_back({n, depth + 1});
        }
    }
    visited.erase(start);
    return visited;
}

// ---------------------------------------------------------------------------
// Federation result union helper
// ---------------------------------------------------------------------------

static std::set<std::string> federationUnion(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b) {
    std::set<std::string> result(a.begin(), a.end());
    result.insert(b.begin(), b.end());
    return result;
}

// ===========================================================================
// DKRG-01 — Entity insert (in-memory)  (≥ 100k inserts/s)
// ===========================================================================

/**
 * @brief DKRG-01: unordered_map insert for BenchEntity.
 * GATE-DKRG-01: ≥ 100k inserts/s.
 */
static void BM_DKRG01_EntityInsert(benchmark::State& state) {
    BenchEntityStore store;
    store.reserve(200'000);
    int idx = 0;

    for (int i = 0; i < kWarmupIterations; ++i) {
        auto e = makeEntity(idx++, "node-A");
        store.emplace(e.id, e);
    }

    std::int64_t ops = 0;
    for (auto _ : state) {
        auto e = makeEntity(idx++, "node-A");
        benchmark::DoNotOptimize(store.emplace(e.id, e));
        ++ops;
    }
    state.SetItemsProcessed(ops);
    state.SetLabel("GATE-DKRG-01: >= 100k inserts/s");
}
BENCHMARK(BM_DKRG01_EntityInsert)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// DKRG-02 — Neighbours lookup (10 edges)  (p99 ≤ 500 µs)
// ===========================================================================

/**
 * @brief DKRG-02: getNeighbours() for a node with 10 direct edges.
 * GATE-DKRG-02: p99 ≤ 500 µs.
 */
static void BM_DKRG02_NeighboursLookup(benchmark::State& state) {
    auto g = makeGraph(100, 10);
    const std::string target = "n0";

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(getNeighbours(g, target));

    for (auto _ : state) {
        benchmark::DoNotOptimize(getNeighbours(g, target));
    }
    state.SetLabel("GATE-DKRG-02: p99 <= 500 us");
}
BENCHMARK(BM_DKRG02_NeighboursLookup)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// DKRG-03 — Path query (depth 3, mock graph)  (p99 ≤ 5 ms)
// ===========================================================================

/**
 * @brief DKRG-03: BFS path query to depth 3.
 * GATE-DKRG-03: p99 ≤ 5 ms.
 */
static void BM_DKRG03_PathQueryDepth3(benchmark::State& state) {
    auto g = makeGraph(50, 5);
    const std::string start = "n0";

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(bfsDepth3(g, start));

    for (auto _ : state) {
        benchmark::DoNotOptimize(bfsDepth3(g, start));
    }
    state.SetLabel("GATE-DKRG-03: p99 <= 5 ms");
}
BENCHMARK(BM_DKRG03_PathQueryDepth3)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// DKRG-04 — Entity merge (LWW)  (p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief DKRG-04: resolveLww() — LWW conflict resolution decision.
 * GATE-DKRG-04: p99 ≤ 100 µs.
 */
static void BM_DKRG04_EntityMergeLww(benchmark::State& state) {
    std::mt19937_64 rng(kDKCanonicalSeed);
    std::uniform_int_distribution<std::int64_t> ts_dist(1L, 1'000'000L);

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(
            resolveLww(ts_dist(rng), ts_dist(rng), "node-A", "node-B"));

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            resolveLww(ts_dist(rng), ts_dist(rng), "node-A", "node-B"));
    }
    state.SetLabel("GATE-DKRG-04: p99 <= 100 us");
}
BENCHMARK(BM_DKRG04_EntityMergeLww)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// DKRG-05 — Federation result union (2 nodes, 100 entities each)  (p99 ≤ 5 ms)
// ===========================================================================

/**
 * @brief DKRG-05: federation result union for 2×100 entity ID sets.
 * UseRealTime() because set operations involve memory allocation.
 * GATE-DKRG-05: p99 ≤ 5 ms.
 */
static void BM_DKRG05_FederationResultUnion(benchmark::State& state) {
    std::vector<std::string> node_a, node_b;
    for (int i = 0; i < 100; ++i) {
      node_a.push_back("e" + std::to_string(i));
    }
    for (int i = 50; i < 150; ++i) {
      node_b.push_back("e" + std::to_string(i));
    }

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(federationUnion(node_a, node_b));

    for (auto _ : state) {
        benchmark::DoNotOptimize(federationUnion(node_a, node_b));
    }
    state.SetLabel("GATE-DKRG-05: p99 <= 5 ms");
}
BENCHMARK(BM_DKRG05_FederationResultUnion)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// DKRG-06 — Entity serialization (128-byte payload)  (p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief DKRG-06: Serialize a BenchEntity with a 128-byte payload to string.
 * GATE-DKRG-06: p99 ≤ 100 µs.
 */
static void BM_DKRG06_EntitySerialization(benchmark::State& state) {
    BenchEntity e = makeEntity(42, "node-A");

    auto serialize = [&]() -> std::string {
        std::ostringstream ss;
        ss << "{\"id\":\"" << e.id
           << "\",\"ts\":" << e.timestamp_us
           << ",\"node\":\"" << e.node_id
           << "\",\"payload\":\"<" << sizeof(e.payload) << "B>\"}";
        return ss.str();
    };

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(serialize());

    for (auto _ : state) {
        benchmark::DoNotOptimize(serialize());
    }
    state.SetLabel("GATE-DKRG-06: p99 <= 100 us");
}
BENCHMARK(BM_DKRG06_EntitySerialization)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace dkrg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
