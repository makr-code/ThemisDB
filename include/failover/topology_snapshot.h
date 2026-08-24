#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace failover {

/// @brief A point-in-time snapshot of the cluster topology for rebalance safety.
struct TopologySnapshot {
    uint64_t version{0};                           ///< Version at snapshot time
    std::unordered_map<std::string, int> failures; ///< node_id → consecutive_failures copy
    std::vector<std::string> node_ids;             ///< Ordered node list at snapshot time

    /// @brief Returns the set of nodes present in `other` but not in this snapshot.
    std::vector<std::string> added_nodes(const TopologySnapshot& other) const;

    /// @brief Returns the set of nodes present in this snapshot but not in `other`.
    std::vector<std::string> removed_nodes(const TopologySnapshot& other) const;

    /// @brief Returns true if the node sets differ between the two snapshots.
    bool has_topology_change(const TopologySnapshot& other) const;

    /// @brief Captures the current state (used by AutoFailoverManager).
    static TopologySnapshot capture(uint64_t version,
                                    const std::unordered_map<std::string, int>& failures);
};

} // namespace failover
} // namespace themis
