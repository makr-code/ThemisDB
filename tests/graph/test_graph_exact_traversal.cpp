// Phase A gate tests for exact graph traversal error instrumentation.

#include "graph/distributed_graph.h"
#include "observability/metrics_collector.h"
#include "utils/expected.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

namespace {

class HealthyExactExecutor final : public themis::graph::ShardGraphExecutor {
public:
    std::string shardId() const override { return "healthy"; }

    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Ok(std::vector<std::string>{"A@healthy", "B@healthy"});
    }

    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        themis::GraphIndexManager::PathResult path;
        path.path = {"A@healthy", "B@healthy"};
        path.totalCost = 1.0;
        return themis::Ok(std::move(path));
    }
};

class FailingExactExecutor : public themis::graph::ShardGraphExecutor {
public:
    explicit FailingExactExecutor(std::string shard_id) : shard_id_(std::move(shard_id)) {}

    std::string shardId() const override { return shard_id_; }

    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Err<std::vector<std::string>>(
            themis::errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "injected bfs failure");
    }

    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Err<themis::GraphIndexManager::PathResult>(
            themis::errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "injected dijkstra failure");
    }

private:
    std::string shard_id_;
};

class UnhealthyExactExecutor final : public FailingExactExecutor {
public:
    using FailingExactExecutor::FailingExactExecutor;
    bool isHealthy() const override { return false; }
};

class ThrowingExactExecutor final : public themis::graph::ShardGraphExecutor {
public:
    std::string shardId() const override { return "throwing"; }

    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        throw std::runtime_error("injected bfs exception");
    }

    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        throw std::runtime_error("injected dijkstra exception");
    }
};

} // namespace

TEST(GraphExactTraversalPhaseA, EmitsMetricWhenNoHealthyShardsRemain) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("down", std::make_shared<UnhealthyExactExecutor>("down"));

    const auto result = manager.shortestPath("A", "B");
    EXPECT_FALSE(result.has_value());

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"shortest_path\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"no_healthy_shards\""), std::string::npos);
}

TEST(GraphExactTraversalPhaseA, EmitsMetricWhenShardFailsDuringKHopTraversal) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy", std::make_shared<HealthyExactExecutor>());
    manager.addShard("broken", std::make_shared<FailingExactExecutor>("broken"));

    const auto result = manager.kHopNeighbors("A", 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"k_hop_neighbors\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"shard_execution_failed\""), std::string::npos);
}

TEST(GraphExactTraversalPhaseA, RejectsEmptyStartVertexWithMetric) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy", std::make_shared<HealthyExactExecutor>());

    const auto result = manager.shortestPath("", "B");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"shortest_path\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"invalid_input\""), std::string::npos);
}

TEST(GraphExactTraversalPhaseA, RejectsNegativeHopCountWithMetric) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy", std::make_shared<HealthyExactExecutor>());

    const auto result = manager.kHopNeighbors("A", -1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"k_hop_neighbors\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"invalid_input\""), std::string::npos);
}

TEST(GraphExactTraversalPhaseA, EmitsMetricWhenShardFutureThrowsDuringShortestPath) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy", std::make_shared<HealthyExactExecutor>());
    manager.addShard("throwing", std::make_shared<ThrowingExactExecutor>());

    const auto result = manager.shortestPath("A", "B");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->path.empty());

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"shortest_path\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"shard_execution_exception\""), std::string::npos);
}

TEST(GraphExactTraversalPhaseA, EmitsMetricWhenShardFutureThrowsDuringKHopTraversal) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    themis::graph::DistributedGraphManager manager;
    manager.addShard("healthy", std::make_shared<HealthyExactExecutor>());
    manager.addShard("throwing", std::make_shared<ThrowingExactExecutor>());

    const auto result = manager.kHopNeighbors("A", 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("graph_exact_traversal_errors_total"), std::string::npos);
    EXPECT_NE(exported.find("operation=\"k_hop_neighbors\""), std::string::npos);
    EXPECT_NE(exported.find("reason=\"shard_execution_exception\""), std::string::npos);
}
