/**
 * @file shard_placement.cc
 * @brief Rack-aware shard placement strategy implementation stub.
 *
 * Skeleton: greedy rack-diverse assignment.
 * Replace with factorization-aware scoring in sub-issue #5431.
 */

#include "distributed_tensor/include/shard_placement.h"

#include <algorithm>

namespace themis::distributed_tensor {

namespace {

class RackAwarePlacementStrategyImpl final : public IShardPlacementStrategy {
public:
    PlacementPlan compute(const std::string& artifact_id,
                           const PlacementConstraints& c,
                           const std::vector<PlacementNode>& topology) const override {
        PlacementPlan plan;
        plan.artifact_id = artifact_id;

        auto ranked = rankNodes(topology, c);
        std::uint32_t needed = c.num_data_stripes + c.num_parity_stripes;
        if (ranked.size() < needed) {
            plan.feasible       = false;
            plan.failure_reason = "Not enough healthy nodes for stripe count";
            return plan;
        }

        for (std::uint32_t i = 0; i < needed; ++i) {
            plan.assignments.push_back({
                .stripe_index = i,
                .shard_key    = ranked[i].shard_key,
                .rack_id      = ranked[i].rack_id,
                .is_parity    = i >= c.num_data_stripes,
            });
        }
        plan.feasible = true;
        return plan;
    }

    std::vector<PlacementAssignment> rebalance(
        const PlacementPlan& /*existing*/,
        const std::vector<PlacementNode>& /*new_topology*/) const override {
        // TODO(#5431): Implement minimal-movement rebalancing.
        return {};
    }

    std::vector<PlacementNode> rankNodes(
        const std::vector<PlacementNode>& topology,
        const PlacementConstraints& c) const override {
        std::vector<PlacementNode> eligible;
        for (const auto& n : topology) {
            if (!n.is_healthy) continue;
            if (n.load_factor > c.max_load_factor) continue;
            if (n.storage_bytes < c.min_storage_bytes) continue;
            eligible.push_back(n);
        }
        // Sort by ascending load factor for greedy selection.
        std::sort(eligible.begin(), eligible.end(),
                  [](const PlacementNode& a, const PlacementNode& b) {
                      return a.load_factor < b.load_factor;
                  });
        return eligible;
    }
};

} // namespace

std::unique_ptr<IShardPlacementStrategy> makeRackAwarePlacementStrategy() {
    return std::make_unique<RackAwarePlacementStrategyImpl>();
}

} // namespace themis::distributed_tensor
