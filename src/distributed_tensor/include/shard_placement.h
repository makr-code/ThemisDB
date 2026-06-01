/**
 * @file shard_placement.h
 * @brief Factorization-aware shard placement strategy for tensor artifacts.
 *
 * Computes where each stripe of a sharded tensor artifact should reside,
 * considering rack topology, network bandwidth, and tensor factorisation
 * structure to minimise reconstruction overhead.
 *
 * Planned in: docs/EPIC3_SHARD_PLACEMENT.md
 * Sub-issue:   #5431
 */

#pragma once

#include "tensor_artifact_classes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::distributed_tensor {

/// Node descriptor in the placement topology.
struct PlacementNode {
    std::string  shard_key;
    std::string  rack_id;     ///< Rack or availability zone
    std::uint64_t dram_bytes   = 0;
    std::uint64_t storage_bytes = 0;
    double       load_factor   = 0.0; ///< Normalized current load [0, 1]
    bool         is_healthy    = true;
};

/// Constraints that guide placement decisions.
struct PlacementConstraints {
    std::uint32_t num_data_stripes   = 4;
    std::uint32_t num_parity_stripes = 2;
    bool          rack_aware         = true; ///< Spread stripes across racks
    float         max_load_factor    = 0.80f; ///< Skip overloaded nodes
    std::uint64_t min_storage_bytes  = 0;    ///< Node minimum free storage
};

/// An assignment of one stripe to one node.
struct PlacementAssignment {
    std::uint32_t stripe_index;
    std::string   shard_key;
    std::string   rack_id;
    bool          is_parity = false;
};

/// The full placement plan for one sharded artifact.
struct PlacementPlan {
    std::string artifact_id;
    std::vector<PlacementAssignment> assignments;
    bool        feasible = false;
    std::string failure_reason;
};

/**
 * @brief Shard placement strategy interface.
 */
class IShardPlacementStrategy {
public:
    virtual ~IShardPlacementStrategy() = default;

    /// Compute a placement plan for a new sharded artifact.
    virtual PlacementPlan compute(const std::string& artifact_id,
                                   const PlacementConstraints& constraints,
                                   const std::vector<PlacementNode>& topology) const = 0;

    /// Re-balance after a topology change (node added/removed).
    virtual std::vector<PlacementAssignment> rebalance(
        const PlacementPlan& existing,
        const std::vector<PlacementNode>& new_topology) const = 0;

    /// Return healthy candidate nodes sorted by suitability.
    virtual std::vector<PlacementNode> rankNodes(
        const std::vector<PlacementNode>& topology,
        const PlacementConstraints& constraints) const = 0;
};

/// Factory: create a rack-aware placement strategy.
std::unique_ptr<IShardPlacementStrategy> makeRackAwarePlacementStrategy();

} // namespace themis::distributed_tensor
