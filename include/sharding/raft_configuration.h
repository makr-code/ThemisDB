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

/*
 * ThemisDB | File: raft_configuration.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <set>
#include <string>
#include <mutex>

namespace themis {
namespace sharding {

struct ConfigurationEntry {
    std::set<std::string> old_members;  // Empty if not joint consensus
    std::set<std::string> new_members;
    bool is_joint_consensus;
};

class RaftConfiguration {
public:
    RaftConfiguration();
    explicit RaftConfiguration(const std::set<std::string>& members);
    
    // Membership changes
    void addNode(const std::string& node_id);
    void removeNode(const std::string& node_id);
    void applyConfiguration(const ConfigurationEntry& entry);
    
    // State queries
    bool isInTransition() const;
    bool isJointConsensus() const;
    bool isMember(const std::string& node_id) const;
    std::set<std::string> getMembers() const;
    std::set<std::string> getOldMembers() const;
    std::set<std::string> getNewMembers() const;
    
    // Quorum calculation
    bool hasQuorum(const std::set<std::string>& votes) const;
    size_t getQuorumSize() const;
    
private:
    std::set<std::string> old_members_;
    std::set<std::string> new_members_;
    bool is_joint_consensus_;
    mutable std::mutex mutex_;
    
    size_t calculateQuorum(size_t size) const;
};

}  // namespace sharding
}  // namespace themis
