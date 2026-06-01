/**
 * @file shard_placement_test.cc
 * @brief Contract tests for IShardPlacementStrategy (sub-issue #5431).
 *
 * Validates factory construction, plan computation with sufficient topology,
 * infeasibility when topology is too small, rebalance, and node ranking.
 * Production factorization-aware scoring is tracked in sub-issue #5431.
 */

#include "distributed_tensor/include/shard_placement.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace themis::distributed_tensor;

namespace {

std::vector<PlacementNode> makeTopology(int n) {
    std::vector<PlacementNode> nodes;
    for (int i = 0; i < n; ++i) {
        PlacementNode node;
        node.shard_key     = "shard-" + std::to_string(i);
        node.rack_id       = "rack-" + std::to_string(i % 3);
        node.dram_bytes    = 64ULL << 30;
        node.storage_bytes = 1ULL << 40;
        node.load_factor   = 0.3;
        node.is_healthy    = true;
        nodes.push_back(node);
    }
    return nodes;
}

PlacementConstraints defaultConstraints() {
    PlacementConstraints c;
    c.num_data_stripes   = 4;
    c.num_parity_stripes = 2;
    c.rack_aware         = true;
    c.max_load_factor    = 0.80f;
    return c;
}

} // namespace

class ShardPlacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy_ = makeRackAwarePlacementStrategy();
        ASSERT_NE(strategy_, nullptr);
    }

    std::unique_ptr<IShardPlacementStrategy> strategy_;
};

TEST_F(ShardPlacementTest, FactoryReturnsNonNull) {
    EXPECT_NE(strategy_, nullptr);
}

TEST_F(ShardPlacementTest, ComputeWithSufficientTopologyIsFeasible) {
    PlacementPlan plan = strategy_->compute(
        "artifact-1", defaultConstraints(), makeTopology(8));
    EXPECT_TRUE(plan.feasible);
    EXPECT_EQ(plan.artifact_id, "artifact-1");
}

TEST_F(ShardPlacementTest, ComputeAssignmentCountMatchesStripes) {
    PlacementPlan plan = strategy_->compute(
        "artifact-2", defaultConstraints(), makeTopology(8));
    ASSERT_TRUE(plan.feasible);
    // total stripes = data + parity
    EXPECT_EQ(plan.assignments.size(),
              defaultConstraints().num_data_stripes +
              defaultConstraints().num_parity_stripes);
}

TEST_F(ShardPlacementTest, ComputeWithInsufficientTopologyIsNotFeasible) {
    PlacementPlan plan = strategy_->compute(
        "artifact-3", defaultConstraints(), makeTopology(2));
    EXPECT_FALSE(plan.feasible);
    EXPECT_FALSE(plan.failure_reason.empty());
}

TEST_F(ShardPlacementTest, ComputeEmptyTopologyIsNotFeasible) {
    PlacementPlan plan = strategy_->compute(
        "artifact-4", defaultConstraints(), {});
    EXPECT_FALSE(plan.feasible);
}

TEST_F(ShardPlacementTest, AssignmentsHaveUniqueShardKeys) {
    PlacementPlan plan = strategy_->compute(
        "artifact-5", defaultConstraints(), makeTopology(8));
    ASSERT_TRUE(plan.feasible);
    std::vector<std::string> keys;
    for (const auto& a : plan.assignments) {
        keys.push_back(a.shard_key);
    }
    // All shard keys in the plan must be unique.
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(std::unique(keys.begin(), keys.end()), keys.end());
}

TEST_F(ShardPlacementTest, RebalanceDoesNotThrow) {
    auto topology = makeTopology(8);
    PlacementPlan plan = strategy_->compute(
        "artifact-6", defaultConstraints(), topology);
    ASSERT_TRUE(plan.feasible);
    // Remove one node and rebalance.
    topology.pop_back();
    EXPECT_NO_THROW(strategy_->rebalance(plan, topology));
}

TEST_F(ShardPlacementTest, RankNodesDoesNotThrow) {
    EXPECT_NO_THROW(strategy_->rankNodes(makeTopology(5), defaultConstraints()));
}

TEST_F(ShardPlacementTest, RankNodesExcludesUnhealthyNodes) {
    auto topology = makeTopology(4);
    topology[0].is_healthy = false;
    auto ranked = strategy_->rankNodes(topology, defaultConstraints());
    for (const auto& node : ranked) {
        EXPECT_NE(node.shard_key, topology[0].shard_key);
    }
}
