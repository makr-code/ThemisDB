#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace failover {

struct TopologySnapshot {
    uint64_t version{0};                           ///< Version at snapshot time
    std::unordered_map<std::string, int> failures; ///< node_id → consecutive_failures copy
    std::vector<std::string> node_ids;             ///< Ordered node list at snapshot time

    /**
     * @brief Added nodes.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    std::vector<std::string> added_nodes(const TopologySnapshot& other) const;

    /**
     * @brief Removed nodes.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    std::vector<std::string> removed_nodes(const TopologySnapshot& other) const;

    /**
     * @brief Has topology change.
     * @param[in] other Input parameter.
     * @return True when the operation succeeds.
     */
    bool has_topology_change(const TopologySnapshot& other) const;

    static TopologySnapshot capture(uint64_t version,
                                    const std::unordered_map<std::string, int>& failures);
};

} // namespace failover
} // namespace themis
