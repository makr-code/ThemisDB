// Phase C gate test for multi-shard exact routing under shard failure injection.

#include "graph/distributed_graph.h"
#include "observability/metrics_collector.h"
#include "utils/expected.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class HealthyExactExecutor final : public themis::graph::ShardGraphExecutor {
public:
    explicit HealthyExactExecutor(std::string shard_id)
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
        themis::GraphIndexManager::PathResult path;
        path.path = {"A@" + shard_id_, "B@" + shard_id_, "C@" + shard_id_};
        path.totalCost = 1.5;
        return themis::Ok(std::move(path));
    }

private:
    std::string shard_id_;
};

class FailingExactExecutor final : public themis::graph::ShardGraphExecutor {
public:
    explicit FailingExactExecutor(std::string shard_id)
        : shard_id_(std::move(shard_id)) {}

    std::string shardId() const override { return shard_id_; }

    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Err<std::vector<std::string>>(
            themis::errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "injected shard bfs failure");
    }

    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Err<themis::GraphIndexManager::PathResult>(
            themis::errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "injected shard dijkstra failure");
    }

private:
    std::string shard_id_;
};

} // namespace

TEST(ShardingMultiShardExactPhaseC, KeepsExactResultWhenOneShardFails) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy", std::make_shared<HealthyExactExecutor>("healthy"));
    manager.addShard("failing", std::make_shared<FailingExactExecutor>("failing"));

    const auto shortest = manager.shortestPath("A", "C");
    ASSERT_TRUE(shortest.has_value());
    ASSERT_FALSE(shortest->path.empty());
    EXPECT_EQ(shortest->path.front(), "A@healthy");
    EXPECT_EQ(shortest->path.back(), "C@healthy");

    const auto neighbors = manager.kHopNeighbors("A", 1);
    ASSERT_TRUE(neighbors.has_value());
    ASSERT_FALSE(neighbors->empty());

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("reason=\"shard_execution_failed\""), std::string::npos);
}

