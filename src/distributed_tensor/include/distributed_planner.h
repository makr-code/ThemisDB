/**
 * @file distributed_planner.h
 * @brief Integration of distributed tensor retrieval with the hybrid query planner.
 *
 * Extends the query planning layer to account for distributed tensor artifact
 * placement, cross-shard fan-out costs, and availability constraints.
 *
 * Planned in: docs/EPIC3_DISTRIBUTED_RETRIEVAL.md
 * Sub-issue:   #5434
 */

#pragma once

#include "artifact_manifest.h"
#include "shard_placement.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themis::distributed_tensor {

/// A shard target selected by the distributed planner for one query.
struct ShardTarget {
    std::string  shard_key;
    std::string  endpoint;      ///< gRPC or HTTP URI
    float        relevance_score = 0.0f; ///< Estimated relevance of this shard
    bool         is_primary      = true;  ///< False for parity-only nodes
};

/// Request descriptor for distributed tensor retrieval planning.
struct DistributedRetrievalRequest {
    std::string              artifact_id;
    std::vector<float>       query_embedding;
    std::uint32_t            top_k = 10;
    std::uint32_t            max_shards = 8;
    std::chrono::milliseconds timeout{500};
    bool                     require_provenance = false;
};

/// Plan produced by the distributed planner.
struct DistributedRetrievalPlan {
    std::vector<ShardTarget> targets;
    bool                     feasible = false;
    std::string              failure_reason;
    double                   estimated_fan_out_latency_ms = 0.0;
};

/**
 * @brief Distributed tensor retrieval planner.
 *
 * Given a retrieval request and the current manifest state, produces a
 * shard fan-out plan that respects topology constraints and availability.
 */
class IDistributedPlanner {
public:
    virtual ~IDistributedPlanner() = default;

    /// Produce a retrieval plan for the given request.
    virtual DistributedRetrievalPlan plan(
        const DistributedRetrievalRequest& req) const = 0;

    /// Refresh the cached topology from the manifest store.
    virtual void refreshTopology() = 0;

    /// Return all known shard keys.
    virtual std::vector<std::string> knownShards() const = 0;
};

/// Factory: create a distributed planner backed by the given manifest store.
std::unique_ptr<IDistributedPlanner> makeDistributedPlanner(
    std::shared_ptr<IManifestStore> manifest,
    const std::vector<PlacementNode>& initial_topology);

} // namespace themis::distributed_tensor
