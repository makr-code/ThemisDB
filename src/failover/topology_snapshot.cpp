/**
 * @file topology_snapshot.cpp
 * @brief Implementation of TopologySnapshot for rebalance-safe topology versioning.
 */

#include "failover/topology_snapshot.h"

#include <algorithm>
#include <unordered_set>

namespace themis {
namespace failover {

std::vector<std::string>
TopologySnapshot::added_nodes(const TopologySnapshot& other) const {
    // Nodes in `other` but not in `this`
    std::unordered_set<std::string> mine(node_ids.begin(), node_ids.end());
    std::vector<std::string> result;
    for (const auto& id : other.node_ids) {
        if (mine.find(id) == mine.end()) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string>
TopologySnapshot::removed_nodes(const TopologySnapshot& other) const {
    // Nodes in `this` but not in `other`
    std::unordered_set<std::string> theirs(other.node_ids.begin(), other.node_ids.end());
    std::vector<std::string> result;
    for (const auto& id : node_ids) {
        if (theirs.find(id) == theirs.end()) {
            result.push_back(id);
        }
    }
    return result;
}

bool TopologySnapshot::has_topology_change(const TopologySnapshot& other) const {
    return !added_nodes(other).empty() || !removed_nodes(other).empty();
}

/*static*/ TopologySnapshot
TopologySnapshot::capture(uint64_t version,
                          const std::unordered_map<std::string, int>& failures) {
    TopologySnapshot snap;
    snap.version = version;
    snap.failures = failures;
    snap.node_ids.reserve(failures.size());
    for (const auto& [node_id, _] : failures) {
        snap.node_ids.push_back(node_id);
    }
    // Sort alphabetically for determinism
    std::sort(snap.node_ids.begin(), snap.node_ids.end());
    return snap;
}

} // namespace failover
} // namespace themis
