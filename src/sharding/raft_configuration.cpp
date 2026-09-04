/**
 * @file raft_configuration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/raft_configuration.h"
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace sharding {

/** @brief Construct empty non-transitioning Raft membership configuration. */
RaftConfiguration::RaftConfiguration()
    : is_joint_consensus_(false) {}

/** @brief Construct stable Raft configuration from initial member set. */
RaftConfiguration::RaftConfiguration(const std::set<std::string>& members)
    : new_members_(members), is_joint_consensus_(false) {}

/**
 * @brief Start a joint-consensus transition that adds one node.
 * @param node_id Node identifier to add in target configuration.
 * @throws std::runtime_error If a transition is already active.
 */
void RaftConfiguration::addNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        throw std::runtime_error("Membership change already in progress");
    }
    old_members_ = new_members_;
    new_members_.insert(node_id);
    is_joint_consensus_ = true;
}

/**
 * @brief Start a joint-consensus transition that removes one node.
 * @param node_id Node identifier to remove from target configuration.
 * @throws std::runtime_error If a transition is already active.
 */
void RaftConfiguration::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        throw std::runtime_error("Membership change already in progress");
    }
    old_members_ = new_members_;
    new_members_.erase(node_id);
    is_joint_consensus_ = true;
}

/** @brief Atomically install provided configuration entry state. */
void RaftConfiguration::applyConfiguration(const ConfigurationEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    old_members_ = entry.old_members;
    new_members_ = entry.new_members;
    is_joint_consensus_ = entry.is_joint_consensus;
}

/** @brief Return whether membership transition is currently active. */
bool RaftConfiguration::isInTransition() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_joint_consensus_;
}

/** @brief Return whether joint-consensus quorum semantics are active. */
bool RaftConfiguration::isJointConsensus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_joint_consensus_;
}

/** @brief Check if node is effective member for current state. */
bool RaftConfiguration::isMember(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        return old_members_.count(node_id) > 0 || new_members_.count(node_id) > 0;
    }
    return new_members_.count(node_id) > 0;
}

/** @brief Return effective member set, including both sets during transitions. */
std::set<std::string> RaftConfiguration::getMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        std::set<std::string> all;
        all.insert(old_members_.begin(), old_members_.end());
        all.insert(new_members_.begin(), new_members_.end());
        return all;
    }
    return new_members_;
}

/** @brief Return previous membership set for joint-consensus transitions. */
std::set<std::string> RaftConfiguration::getOldMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return old_members_;
}

/** @brief Return new/target membership set. */
std::set<std::string> RaftConfiguration::getNewMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return new_members_;
}

/**
 * @brief Evaluate votes against stable or joint-consensus quorum rules.
 * @param votes Node IDs that have voted.
 * @return True if required majority conditions are satisfied.
 */
bool RaftConfiguration::hasQuorum(const std::set<std::string>& votes) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_joint_consensus_) {
        // Joint consensus: need majority in BOTH old and new
        size_t old_votes = 0;
        for (const auto& vote : votes) {
            if (old_members_.count(vote) > 0) {
                old_votes++;
            }
        }
        
        size_t new_votes = 0;
        for (const auto& vote : votes) {
            if (new_members_.count(vote) > 0) {
                new_votes++;
            }
        }
        
        size_t old_quorum = calculateQuorum(old_members_.size());
        size_t new_quorum = calculateQuorum(new_members_.size());
        
        return old_votes >= old_quorum && new_votes >= new_quorum;
    } else {
        // Simple majority in new configuration
        size_t vote_count = 0;
        for (const auto& vote : votes) {
            if (new_members_.count(vote) > 0) {
                vote_count++;
            }
        }
        return static_cast<bool>(vote_count  < static_cast<int>(= calculateQuorum(new_members_.size())));
    }
}

/** @brief Return quorum size currently required for commit decisions. */
size_t RaftConfiguration::getQuorumSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        // Return the larger of the two quorums
        return std::max(calculateQuorum(old_members_.size()),
                       calculateQuorum(new_members_.size()));
    }
    return calculateQuorum(new_members_.size());
}

/** @brief Compute majority threshold for provided member count. */
size_t RaftConfiguration::calculateQuorum([[maybe_unused]] size_t size) const {
    return (size / 2) + 1;
}

}  // namespace sharding
}  // namespace themis
