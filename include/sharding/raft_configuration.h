/**
 * @file raft_configuration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <set>
#include <string>
#include <mutex>

namespace themis {
namespace sharding {

/**
 * @brief Raft membership configuration entry.
 *
 * Describes either a stable configuration (`is_joint_consensus == false`) or a
 * joint-consensus transition requiring quorums in both old and new sets.
 */
struct ConfigurationEntry {
    std::set<std::string> old_members;  ///< Previous membership; empty outside transitions.
    std::set<std::string> new_members;  ///< Active or target membership set.
    bool is_joint_consensus;            ///< True while joint consensus rules are active.
};

/**
 * @brief Thread-safe manager for Raft cluster membership state.
 *
 * Supports joint-consensus transitions for membership changes and provides
 * quorum calculations for both stable and transitional states.
 */
class RaftConfiguration {
public:
    /** @brief Construct empty configuration without members. */
    RaftConfiguration();

    /**
     * @brief Construct stable configuration from initial member set.
     * @param members Initial active members.
     */
    explicit RaftConfiguration(const std::set<std::string>& members);

    // Membership changes
    /**
     * @brief Start joint-consensus transition that adds one node.
     * @param node_id Node identifier to include in the new configuration.
     * @throws std::runtime_error If another membership transition is already active.
     */
    void addNode(const std::string& node_id);

    /**
     * @brief Start joint-consensus transition that removes one node.
     * @param node_id Node identifier to remove from new configuration.
     * @throws std::runtime_error If another membership transition is already active.
     */
    void removeNode(const std::string& node_id);

    /**
     * @brief Apply externally prepared configuration state.
     * @param entry Configuration entry to install atomically.
     */
    void applyConfiguration(const ConfigurationEntry& entry);

    // State queries
    /** @brief Return true while a membership transition is active. */
    bool isInTransition() const;

    /** @brief Return true when joint-consensus quorum rules are enabled. */
    bool isJointConsensus() const;

    /**
     * @brief Check whether a node belongs to current effective membership.
     * @param node_id Node identifier to test.
     * @return True when node participates in the active state.
     */
    bool isMember(const std::string& node_id) const;

    /**
     * @brief Return effective members for the current state.
     * @return Union of old/new members during transition, otherwise stable set.
     */
    std::set<std::string> getMembers() const;

    /** @brief Return old configuration members used in joint consensus. */
    std::set<std::string> getOldMembers() const;

    /** @brief Return new/target configuration members. */
    std::set<std::string> getNewMembers() const;

    // Quorum calculation
    /**
     * @brief Validate whether provided votes satisfy quorum requirements.
     * @param votes Node IDs that voted/acknowledged.
     * @return True when quorum is met for stable or joint-consensus state.
     */
    bool hasQuorum(const std::set<std::string>& votes) const;

    /**
     * @brief Return required quorum size for current state.
     * @return Majority size in stable mode, max(old,new majority) in transition.
     */
    size_t getQuorumSize() const;

private:
    std::set<std::string> old_members_;
    std::set<std::string> new_members_;
    bool is_joint_consensus_;
    mutable std::mutex mutex_;

    /**
     * @brief Compute majority quorum for membership size.
     * @param size Number of members in configuration.
     * @return Majority threshold (`size/2 + 1`).
     */
    size_t calculateQuorum(size_t size) const;
};

}  // namespace sharding
}  // namespace themis
