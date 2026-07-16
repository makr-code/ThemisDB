#include "graph/distributed_graph.h"
#include "utils/expected.h"

#include <benchmark/benchmark.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

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
    std::string shard_id_;
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
    std::string shard_id_;
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

    for (auto _ : state) {
        auto shortest = manager.shortestPath("A", "C");
        if (!shortest.has_value()) {
            state.SkipWithError("Exact multi-shard benchmark failed: no path returned");
            break;
        }
        benchmark::DoNotOptimize(shortest->totalCost);
        benchmark::DoNotOptimize(shortest->path);
    }

    state.SetItemsProcessed(state.iterations() * shard_count);
}

BENCHMARK(BM_MultishardExact)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kMicrosecond);

} // namespace

