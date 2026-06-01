/**
 * @file distributed_planner.cc
 * @brief Distributed tensor retrieval planner implementation stub.
 *
 * Skeleton: topology-aware shard fan-out selection.
 * Replace with scoring-based shard selection in sub-issue #5434.
 */

#include "distributed_tensor/include/distributed_planner.h"

namespace themis::distributed_tensor {

namespace {

class DistributedPlannerImpl final : public IDistributedPlanner {
public:
    DistributedPlannerImpl(std::shared_ptr<IManifestStore> manifest,
                            std::vector<PlacementNode> topology)
        : manifest_(std::move(manifest)), topology_(std::move(topology)) {}

    DistributedRetrievalPlan plan(
        const DistributedRetrievalRequest& req) const override {
        DistributedRetrievalPlan result;

        auto entry = manifest_->lookup(req.artifact_id);
        if (!entry) {
            result.feasible       = false;
            result.failure_reason = "Artifact not found in manifest";
            return result;
        }

        // Build fan-out targets from manifest stripes, up to max_shards.
        std::uint32_t selected = 0;
        for (const auto& stripe : entry->stripes) {
            if (selected >= req.max_shards) break;
            // Skip parity stripes for normal reads.
            if (!stripe.shard_key.empty()) {
                result.targets.push_back({
                    .shard_key = stripe.shard_key,
                    .is_primary = true,
                });
                ++selected;
            }
        }

        result.feasible = !result.targets.empty();
        if (!result.feasible)
            result.failure_reason = "No healthy data stripes available";
        return result;
    }

    void refreshTopology() override {
        // TODO(#5434): Re-probe nodes and update topology_.
    }

    std::vector<std::string> knownShards() const override {
        std::vector<std::string> keys;
        for (const auto& n : topology_) keys.push_back(n.shard_key);
        return keys;
    }

private:
    std::shared_ptr<IManifestStore> manifest_;
    std::vector<PlacementNode>      topology_;
};

} // namespace

std::unique_ptr<IDistributedPlanner> makeDistributedPlanner(
    std::shared_ptr<IManifestStore> manifest,
    const std::vector<PlacementNode>& initial_topology) {
    return std::make_unique<DistributedPlannerImpl>(
        std::move(manifest), initial_topology);
}

} // namespace themis::distributed_tensor
