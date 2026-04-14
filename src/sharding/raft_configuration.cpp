/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_configuration.cpp                             ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:52:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/raft_configuration.h"
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace sharding {

RaftConfiguration::RaftConfiguration()
    : is_joint_consensus_(false) {}

RaftConfiguration::RaftConfiguration(const std::set<std::string>& members)
    : new_members_(members), is_joint_consensus_(false) {}

void RaftConfiguration::addNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        throw std::runtime_error("Membership change already in progress");
    }
    old_members_ = new_members_;
    new_members_.insert(node_id);
    is_joint_consensus_ = true;
}

void RaftConfiguration::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        throw std::runtime_error("Membership change already in progress");
    }
    old_members_ = new_members_;
    new_members_.erase(node_id);
    is_joint_consensus_ = true;
}

void RaftConfiguration::applyConfiguration(const ConfigurationEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    old_members_ = entry.old_members;
    new_members_ = entry.new_members;
    is_joint_consensus_ = entry.is_joint_consensus;
}

bool RaftConfiguration::isInTransition() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_joint_consensus_;
}

bool RaftConfiguration::isJointConsensus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_joint_consensus_;
}

bool RaftConfiguration::isMember(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        return old_members_.count(node_id) > 0 || new_members_.count(node_id) > 0;
    }
    return new_members_.count(node_id) > 0;
}

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

std::set<std::string> RaftConfiguration::getOldMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return old_members_;
}

std::set<std::string> RaftConfiguration::getNewMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return new_members_;
}

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
        return vote_count >= calculateQuorum(new_members_.size());
    }
}

size_t RaftConfiguration::getQuorumSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_joint_consensus_) {
        // Return the larger of the two quorums
        return std::max(calculateQuorum(old_members_.size()),
                       calculateQuorum(new_members_.size()));
    }
    return calculateQuorum(new_members_.size());
}

size_t RaftConfiguration::calculateQuorum(size_t size) const {
    return (size / 2) + 1;
}

}  // namespace sharding
}  // namespace themis
