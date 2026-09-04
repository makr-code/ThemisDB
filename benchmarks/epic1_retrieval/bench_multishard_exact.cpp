// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_multishard_exact.cpp
 * @brief Phase C benchmark gate for exact multi-shard retrieval under failure.
 *
 * Measures the exact shortest-path fallback path when one healthy shard must
 * continue serving while peer shards fail closed. The workload is intentionally
 * deterministic so repeated runs can be compared across machines and CI runs.
 *
 * Benchmark hygiene:
 * - canonical RNG seed: 42
 * - warmup before measurement
 * - fixed minimum iteration count for reproducibility
 * - aggregate reporting over repeated runs
 */

#include "graph/distributed_graph.h"
#include "utils/expected.h"

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kWarmupIterations = 200;
constexpr int kBenchmarkRepetitions = 5;
constexpr int kMinimumIterations = 3000;

class HealthyExecutor final : public themis::graph::ShardGraphExecutor {
public:
    explicit HealthyExecutor(std::string shard_id)
        : shard_id_(std::move(shard_id)) {}

    std::string shardId() const override { return shard_id_; }

    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Ok(std::vector<std::string>{"A@" + shard_id_, "B@" + shard_id_});
    }

    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        themis::GraphIndexManager::PathResult p;
        p.path = {"A@" + shard_id_, "B@" + shard_id_, "C@" + shard_id_};
        p.totalCost = 1.0;
        return themis::Ok(std::move(p));
    }

private:
    std::string shard_id_ = {};
};

class FailingExecutor final : public themis::graph::ShardGraphExecutor {
public:
    explicit FailingExecutor(std::string shard_id)
        : shard_id_(std::move(shard_id)) {}

    std::string shardId() const override { return shard_id_; }

    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Err<std::vector<std::string>>(
            themis::errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "injected benchmark bfs failure");
    }

    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Err<themis::GraphIndexManager::PathResult>(
            themis::errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "injected benchmark dijkstra failure");
    }

private:
    std::string shard_id_ = {};
};

themis::graph::DistributedGraphManager makeManager(int total_shards) {
    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy-0", std::make_shared<HealthyExecutor>("healthy-0"));

    for (int i = 1; i < total_shards; ++i) {
        manager.addShard("failing-" + std::to_string(i),
                         std::make_shared<FailingExecutor>("failing-" + std::to_string(i)));
    }
    return manager;
}

void BM_MultishardExact(benchmark::State& state) {
    const int shard_count = static_cast<int>(state.range(0));
    auto manager = makeManager(shard_count);
    const std::array<std::pair<std::string, std::string>, 3> queries{{
        {"A", "C"},
        {"root", "leaf"},
        {"src", "dst"},
    }};
    std::mt19937_64 rng(kCanonicalRngSeed + static_cast<uint64_t>(shard_count));
    std::uniform_int_distribution<std::size_t> query_dist(0, queries.size() - 1);

    for (int i = 0; i < kWarmupIterations; ++i) {
        const auto& [start, end] = queries[query_dist(rng)];
        auto shortest = manager.shortestPath(start, end);
        if (!shortest.has_value()) {
            state.SkipWithError("Warmup failed: no exact path returned");
            return;
        }
        benchmark::DoNotOptimize(shortest->totalCost);
    }

    for (auto _ : state) {
        const auto& [start, end] = queries[query_dist(rng)];
        auto shortest = manager.shortestPath(start, end);
        if (!shortest.has_value()) {
            state.SkipWithError("Exact multi-shard benchmark failed: no path returned");
            break;
        }
        benchmark::DoNotOptimize(shortest->totalCost);
        benchmark::DoNotOptimize(shortest->path);
    }

    state.SetItemsProcessed(state.iterations() * shard_count);
    state.SetLabel("phase-c-exact-fallback");
}

BENCHMARK(BM_MultishardExact)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Iterations(kMinimumIterations)
    ->Repetitions(kBenchmarkRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

} // namespace
